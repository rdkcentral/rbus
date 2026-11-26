#include "rtrouter_diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHECK(expression) do { if (!(expression)) { \
  fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #expression); return 1; \
} } while (0)

int main(void)
{
  unsigned char key[32] = {1};
  unsigned char wrong_key[32] = {2};
  unsigned char loaded[32];
  rtDiagChallenge state;
  rtDiagChallenge other;
  char mac[65], other_mac[65];
  char path[] = "/tmp/rtrouted-diag-key-XXXXXX";
  char link_path[128];
  FILE* file;
  struct stat st;
  int fd;

  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "heartbeat", NULL, mac) == 0);
  CHECK(rtDiagAuth_Verify(&state, key, "heartbeat", NULL, mac) == 0);
  CHECK(rtDiagAuth_Verify(&state, key, "heartbeat", NULL, mac) != 0);

  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "shutdown", NULL, mac) == 0);
  CHECK(rtDiagAuth_Verify(&state, wrong_key, "shutdown", NULL, mac) != 0);
  CHECK(rtDiagAuth_Verify(&state, key, "shutdown", NULL, mac) != 0);

  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "heartbeat", NULL, mac) == 0);
  CHECK(rtDiagAuth_Verify(&state, key, "shutdown", NULL, mac) != 0);

  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Challenge(&other) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "addNewListener", "tcp://127.0.0.1:1234", mac) == 0);
  CHECK(rtDiagAuth_Verify(&other, key, "addNewListener", "tcp://127.0.0.1:1234", mac) != 0);
  CHECK(rtDiagAuth_Verify(&state, key, "addNewListener", "tcp://0.0.0.0:1234", mac) != 0);

  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "heartbeat", NULL, mac) == 0);
  state.issued.tv_sec -= 11;
  CHECK(rtDiagAuth_Verify(&state, key, "heartbeat", NULL, mac) != 0);

  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Verify(&state, key, "heartbeat", NULL, NULL) != 0);
  CHECK(state.nonce[0] == '\0');
  CHECK(rtDiagAuth_Sign(key, "short", "heartbeat", NULL, mac) != 0);
  CHECK(rtDiagAuth_Challenge(&state) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "ab", "c", mac) == 0);
  CHECK(rtDiagAuth_Sign(key, state.nonce, "a", "bc", other_mac) == 0);
  CHECK(strcmp(mac, other_mac) != 0);

  fd = mkstemp(path);
  CHECK(fd >= 0);
  file = fdopen(fd, "w");
  CHECK(file != NULL);
  CHECK(fputs("0100000000000000000000000000000000000000000000000000000000000000\n", file) >= 0);
  CHECK(fclose(file) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) == 0);
  CHECK(memcmp(loaded, key, sizeof(key)) == 0);
  CHECK(chmod(path, 0644) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) != 0);
  CHECK(chmod(path, 0600) == 0);
  snprintf(link_path, sizeof(link_path), "%s-link", path);
  CHECK(symlink(path, link_path) == 0);
  CHECK(rtDiagAuth_LoadKey(link_path, loaded) != 0);
  CHECK(unlink(link_path) == 0);
  file = fopen(path, "w");
  CHECK(file != NULL);
  CHECK(fputs("short-key", file) >= 0);
  CHECK(fclose(file) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) != 0);
  CHECK(rtDiagAuth_EnsureKey(path) != 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) != 0);
  CHECK(unlink(path) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) == 1);
  CHECK(rtDiagAuth_EnsureKey(path) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) == 0);
  memcpy(key, loaded, sizeof(key));
  CHECK(stat(path, &st) == 0);
  CHECK((st.st_mode & 0777) == 0600 && st.st_uid == geteuid());
  CHECK(rtDiagAuth_EnsureKey(path) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) == 0);
  CHECK(memcmp(loaded, key, sizeof(key)) == 0);

  CHECK(symlink(path, link_path) == 0);
  CHECK(rtDiagAuth_EnsureKey(link_path) != 0);
  CHECK(lstat(link_path, &st) == 0 && S_ISLNK(st.st_mode));
  CHECK(unlink(link_path) == 0);
  CHECK(chmod(path, 0644) == 0);
  CHECK(rtDiagAuth_EnsureKey(path) != 0);
  CHECK(stat(path, &st) == 0 && (st.st_mode & 0777) == 0644);
  CHECK(chmod(path, 0600) == 0);
  CHECK(rtDiagAuth_LoadKey(path, loaded) == 0);
  CHECK(memcmp(loaded, key, sizeof(key)) == 0);

  CHECK(rtDiagAuth_EnsureKey(link_path) == 0);
  CHECK(rtDiagAuth_LoadKey(link_path, loaded) == 0);
  CHECK(memcmp(loaded, key, sizeof(key)) != 0);
  CHECK(unlink(link_path) == 0);
  CHECK(unlink(path) == 0);
  snprintf(link_path, sizeof(link_path), "%s/missing/key", path);
  CHECK(rtDiagAuth_EnsureKey(link_path) != 0);
  puts("Diagnostic challenge-response and key-file tests passed");
  return 0;
}
