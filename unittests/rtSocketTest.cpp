#include "gtest/gtest.h"
#include "rtSocket.h"
#include <string>
#include <cstring>
#include <sys/un.h>
#include "rtrouteBase.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <pthread.h>
#include <dirent.h>

TEST(rtSocketTest, rejects_oversized_tcp_host_without_modifying_endpoint)
{
    size_t lengths[] = {128, 129, 512, 4096};
    for(size_t length : lengths)
    {
        struct sockaddr_storage endpoint = {};
        endpoint.ss_family = AF_UNIX;
        struct sockaddr_storage original = endpoint;
        std::string address = "tcp://" + std::string(length, 'a') + ":6667";
        EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, address.c_str()));
        EXPECT_EQ(0, std::memcmp(&original, &endpoint, sizeof(endpoint)));
    }
}

TEST(rtSocketTest, rejects_empty_host_and_null_arguments)
{
    struct sockaddr_storage endpoint = {};
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, "tcp://:6667"));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, NULL));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(NULL, "tcp://127.0.0.1:6667"));
}

TEST(rtSocketTest, validates_unix_path_capacity_without_truncation)
{
    struct sockaddr_storage endpoint = {};
    size_t capacity = sizeof(((struct sockaddr_un*)0)->sun_path);
    std::string path(capacity - 1, 'a');
    ASSERT_EQ(RT_OK, rtSocketStorage_FromString(&endpoint, ("unix://" + path).c_str()));
    struct sockaddr_un* unixEndpoint = reinterpret_cast<struct sockaddr_un*>(&endpoint);
    EXPECT_STREQ(path.c_str(), unixEndpoint->sun_path);
    struct sockaddr_storage original = endpoint;
    path += 'b';
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtSocketStorage_FromString(&endpoint, ("unix://" + path).c_str()));
    EXPECT_EQ(0, std::memcmp(&original, &endpoint, sizeof(endpoint)));
}

TEST(rtSocketTest, preserves_ipv4_and_unix_endpoints)
{
    struct sockaddr_storage endpoint = {};
    ASSERT_EQ(RT_OK, rtSocketStorage_FromString(&endpoint, "tcp://127.0.0.1:6667"));
    EXPECT_EQ(AF_INET, endpoint.ss_family);
    struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(&endpoint);
    EXPECT_EQ(6667, ntohs(ipv4->sin_port));
    EXPECT_EQ(INADDR_LOOPBACK, ntohl(ipv4->sin_addr.s_addr));

    ASSERT_EQ(RT_OK, rtSocketStorage_FromString(&endpoint, "unix:///tmp/rtrouted"));
    EXPECT_EQ(AF_UNIX, endpoint.ss_family);
    struct sockaddr_un* unixEndpoint = reinterpret_cast<struct sockaddr_un*>(&endpoint);
    EXPECT_STREQ("/tmp/rtrouted", unixEndpoint->sun_path);
}

class rtDirectSocketTest : public ::testing::Test
{
protected:
    std::string directory;
    std::string basename;
    std::string address;
    pid_t child = -1;

    void SetUp() override
    {
        char path[64];
        std::snprintf(path, sizeof(path), RT_DIRECT_SOCKET_DIRECTORY_FORMAT,
            static_cast<unsigned long>(geteuid()));
        directory = path;
        basename = ".direct_gtest_" + std::to_string(getpid());
        address = "unix://" + directory + "/" + basename;
    }

    void stopProvider()
    {
        if(child > 0)
        {
            EXPECT_EQ(0, kill(child, SIGKILL));
            int status;
            EXPECT_EQ(child, waitpid(child, &status, 0));
            child = -1;
        }
    }

    void TearDown() override
    {
        stopProvider();
        int fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if(fd >= 0)
        {
            EXPECT_TRUE(unlinkat(fd, basename.c_str(), 0) == 0 || errno == ENOENT);
            std::string lockname = "lock" + basename;
            EXPECT_TRUE(unlinkat(fd, lockname.c_str(), 0) == 0 || errno == ENOENT);
            std::string staleLock = lockname + "_stale";
            EXPECT_TRUE(unlinkat(fd, staleLock.c_str(), 0) == 0 || errno == ENOENT);
            close(fd);
        }
    }

    bool startProvider()
    {
        child = fork();
        if(child == 0)
            _exit(rtRouteDirect_StartInstance(address.c_str(), NULL) == RT_OK ? 0 : 1);
        if(child < 0)
            return false;
        for(int i = 0; i < 300; ++i)
        {
            int status;
            if(waitpid(child, &status, WNOHANG) == child)
            {
                child = -1;
                return false;
            }
            struct stat st;
            if(lstat((directory + "/" + basename).c_str(), &st) == 0 && S_ISSOCK(st.st_mode))
                return true;
            usleep(10000);
        }
        return false;
    }

    int connectConsumer()
    {
        struct sockaddr_storage endpoint = {};
        if(rtSocketStorage_FromString(&endpoint, address.c_str()) != RT_OK)
            return -1;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(fd < 0)
            return -1;
        for(int i = 0; i < 300; ++i)
        {
            if(connect(fd, reinterpret_cast<struct sockaddr*>(&endpoint),
                sizeof(struct sockaddr_un)) == 0)
                return fd;
            if(errno != ECONNREFUSED && errno != ENOENT)
                break;
            usleep(10000);
        }
        close(fd);
        return -1;
    }

    bool setLockSweepDue(bool due)
    {
        int directoryFD = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if(directoryFD < 0)
            return false;
        if(flock(directoryFD, LOCK_EX) != 0)
        {
            close(directoryFD);
            return false;
        }
        int fd = openat(directoryFD, "lock.maintenance", O_RDWR | O_NOFOLLOW | O_CLOEXEC);
        bool updated = false;
        if(fd >= 0)
        {
            struct timespec times[2] = {};
            times[0].tv_nsec = UTIME_OMIT;
            times[1].tv_nsec = due ? 0 : UTIME_NOW;
            updated = futimens(fd, times) == 0;
            if(close(fd) != 0)
                updated = false;
        }
        if(flock(directoryFD, LOCK_UN) != 0)
            updated = false;
        if(close(directoryFD) != 0)
            updated = false;
        return updated;
    }

    bool waitForProviderExit()
    {
        for(int i = 0; i < 300; ++i)
        {
            int status;
            if(waitpid(child, &status, WNOHANG) == child)
            {
                child = -1;
                return WIFEXITED(status) && WEXITSTATUS(status) == 0;
            }
            usleep(10000);
        }
        return false;
    }
};

TEST_F(rtDirectSocketTest, duplicate_start_does_not_replace_active_listener)
{
    ASSERT_TRUE(startProvider());
    ASSERT_TRUE(setLockSweepDue(true));
    int consumer = connectConsumer();
    if(consumer < 0)
    {
        ADD_FAILURE() << "Failed to connect consumer";
        return;
    }
    EXPECT_NE(RT_OK, rtRouteDirect_StartInstance(address.c_str(), NULL));
    int status;
    EXPECT_EQ(0, waitpid(child, &status, WNOHANG));
    close(consumer);
    EXPECT_TRUE(waitForProviderExit());
    EXPECT_EQ(-1, access((directory + "/" + basename).c_str(), F_OK));
    EXPECT_EQ(ENOENT, errno);
    EXPECT_EQ(-1, access((directory + "/lock" + basename).c_str(), F_OK));
    EXPECT_EQ(ENOENT, errno);
}

TEST_F(rtDirectSocketTest, provider_crash_allows_restart_at_same_address)
{
    ASSERT_TRUE(startProvider());
    int consumer = connectConsumer();
    if(consumer < 0)
    {
        ADD_FAILURE() << "Failed to connect consumer";
        return;
    }
    stopProvider();
    close(consumer);
    std::string staleLock = directory + "/lock" + basename + "_stale";
    int staleFD = open(staleLock.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if(staleFD < 0)
    {
        ADD_FAILURE() << "Failed to create unused lock for reclamation test";
        return;
    }
    close(staleFD);
    ASSERT_TRUE(setLockSweepDue(false));
    ASSERT_TRUE(startProvider());
    consumer = connectConsumer();
    if(consumer < 0)
    {
        ADD_FAILURE() << "Failed to reconnect consumer after provider crash";
        return;
    }
    /* The crashed provider's socket path alone does not establish restart readiness. */
    EXPECT_EQ(0, access(staleLock.c_str(), F_OK));
    close(consumer);
    ASSERT_TRUE(waitForProviderExit());
    EXPECT_EQ(0, access(staleLock.c_str(), F_OK));
    EXPECT_EQ(-1, access((directory + "/lock" + basename).c_str(), F_OK));
    EXPECT_EQ(ENOENT, errno);

    ASSERT_TRUE(setLockSweepDue(true));
    ASSERT_TRUE(startProvider());
    consumer = connectConsumer();
    if(consumer < 0)
    {
        ADD_FAILURE() << "Failed to connect consumer after periodic lock sweep";
        return;
    }
    EXPECT_EQ(-1, access(staleLock.c_str(), F_OK));
    EXPECT_EQ(ENOENT, errno);
    close(consumer);
    EXPECT_TRUE(waitForProviderExit());
}

static void* runCancelableDirectListener(void* address)
{
    rtRouteDirect_StartInstance(static_cast<const char*>(address), NULL);
    return NULL;
}

static int countOpenDescriptors()
{
    DIR* directory = opendir("/proc/self/fd");
    if(!directory)
        return -1;
    int count = 0;
    errno = 0;
    while(struct dirent* entry = readdir(directory))
    {
        if(std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
            ++count;
    }
    int readError = errno;
    if(closedir(directory) != 0 || readError != 0)
        return -1;
    return count;
}

TEST_F(rtDirectSocketTest, cancellation_interrupts_occupied_tcp_bind_retry)
{
    int occupied = socket(AF_INET, SOCK_STREAM, 0);
    if(occupied < 0)
    {
        FAIL() << "Failed to create occupied TCP listener";
    }
    struct sockaddr_in endpoint = {};
    endpoint.sin_family = AF_INET;
    endpoint.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(bind(occupied, reinterpret_cast<struct sockaddr*>(&endpoint), sizeof(endpoint)) != 0 ||
        listen(occupied, 1) != 0)
    {
        close(occupied);
        FAIL() << "Failed to bind occupied TCP listener";
    }
    socklen_t length = sizeof(endpoint);
    if(getsockname(occupied, reinterpret_cast<struct sockaddr*>(&endpoint), &length) != 0)
    {
        close(occupied);
        FAIL() << "Failed to read occupied TCP port";
    }
    std::string tcpAddress = "tcp://127.0.0.1:" + std::to_string(ntohs(endpoint.sin_port));
    child = fork();
    if(child != 0)
        close(occupied);
    ASSERT_GE(child, 0);
    if(child == 0)
    {
        /* The child retains its copy of the occupied listening socket. */
        int before = countOpenDescriptors();
        if(before < 0)
            _exit(1);
        for(int attempt = 0; attempt < 2; ++attempt)
        {
            pthread_t thread;
            if(pthread_create(&thread, NULL, runCancelableDirectListener,
                const_cast<char*>(tcpAddress.c_str())) != 0)
                _exit(2);
            bool binding = false;
            for(int i = 0; i < 200; ++i)
            {
                if(countOpenDescriptors() == before + 1)
                {
                    binding = true;
                    break;
                }
                usleep(1000);
            }
            if(!binding)
                _exit(6);
            if(pthread_cancel(thread) != 0)
                _exit(3);
            void* status = NULL;
            if(pthread_join(thread, &status) != 0 || status != PTHREAD_CANCELED)
                _exit(4);
            if(countOpenDescriptors() != before)
                _exit(5);
        }
        _exit(0);
    }
    EXPECT_TRUE(waitForProviderExit());
}

TEST_F(rtDirectSocketTest, cancellation_releases_listener_and_allows_same_process_restart)
{
    child = fork();
    ASSERT_GE(child, 0);
    if(child == 0)
    {
        for(int attempt = 0; attempt < 2; ++attempt)
        {
            pthread_t thread;
            if(pthread_create(&thread, NULL, runCancelableDirectListener,
                const_cast<char*>(address.c_str())) != 0)
                _exit(1);
            bool ready = false;
            for(int i = 0; i < 200; ++i)
            {
                if(access((directory + "/" + basename).c_str(), F_OK) == 0)
                {
                    ready = true;
                    break;
                }
                usleep(1000);
            }
            if(!ready || pthread_cancel(thread) != 0)
                _exit(2);
            void* status = NULL;
            if(pthread_join(thread, &status) != 0 || status != PTHREAD_CANCELED)
                _exit(3);
            if(access((directory + "/" + basename).c_str(), F_OK) == 0 ||
                access((directory + "/lock" + basename).c_str(), F_OK) == 0)
                _exit(4);
        }
        _exit(0);
    }
    EXPECT_TRUE(waitForProviderExit());
}

TEST(rtSocketTest, direct_listener_rejects_legacy_and_traversal_paths)
{
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRouteDirect_StartInstance("unix:///tmp/.direct_legacy", NULL));
    char directory[64];
    std::snprintf(directory, sizeof(directory), RT_DIRECT_SOCKET_DIRECTORY_FORMAT,
        static_cast<unsigned long>(geteuid()));
    std::string prefix = "unix://" + std::string(directory);
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRouteDirect_StartInstance((prefix + "/.direct_../other").c_str(), NULL));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRouteDirect_StartInstance((prefix + "/.direct_").c_str(), NULL));
    EXPECT_EQ(RT_ERROR_INVALID_ARG, rtRouteDirect_StartInstance((prefix + "-other/.direct_name").c_str(), NULL));
}
