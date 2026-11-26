#define _GNU_SOURCE 1
#include "rtrouter_diag.h"
#include "rtLog.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

static void
rtDiagAuth_Hex(const unsigned char bytes[32], char hex[65])
{
  static const char digits[] = "0123456789abcdef";
  size_t i;
  for (i = 0; i < 32; ++i)
  {
    hex[2 * i] = digits[bytes[i] >> 4];
    hex[2 * i + 1] = digits[bytes[i] & 15];
  }
  hex[64] = '\0';
}

int
rtDiagAuth_LoadKey(const char* path, unsigned char key[32])
{
  struct stat st;
  char hex[67];
  size_t used = 0;
  int fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
  int result = -1;
  size_t i;
  if (fd < 0)
  {
    if (errno == ENOENT)
      return 1;
    rtLog_Error("Cannot open diagnostic key %s: %s", path, strerror(errno));
    return -1;
  }
  if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) ||
      (st.st_uid != 0 && st.st_uid != geteuid()) || (st.st_mode & 0077) != 0)
    goto done;
  while (used < sizeof(hex))
  {
    ssize_t count = read(fd, hex + used, sizeof(hex) - used);
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0)
      goto done;
    if (count == 0)
      break;
    used += (size_t)count;
  }
  if (used != 64 && !(used == 65 && hex[64] == '\n'))
    goto done;
  for (i = 0; i < 32; ++i)
  {
    unsigned int high, low;
    char a = hex[2 * i], b = hex[2 * i + 1];
    if (a >= '0' && a <= '9') high = (unsigned int)(a - '0');
    else if (a >= 'a' && a <= 'f') high = (unsigned int)(a - 'a' + 10);
    else goto done;
    if (b >= '0' && b <= '9') low = (unsigned int)(b - '0');
    else if (b >= 'a' && b <= 'f') low = (unsigned int)(b - 'a' + 10);
    else goto done;
    key[i] = (unsigned char)((high << 4) | low);
  }
  result = 0;
done:
  if (result != 0)
  {
    OPENSSL_cleanse(key, 32);
    rtLog_Error("Diagnostic key %s must be an owner-only regular file containing 64 lowercase hex digits", path);
  }
  OPENSSL_cleanse(hex, sizeof(hex));
  close(fd);
  return result;
}

int
rtDiagAuth_EnsureKey(const char* path)
{
  unsigned char key[32] = {0};
  char hex[65];
  char temporary[PATH_MAX];
  size_t used = 0;
  int fd = -1;
  int created = 0;
  int length;
  int result = rtDiagAuth_LoadKey(path, key);

  if (result != 1)
    goto done;
  result = -1;
  length = snprintf(temporary, sizeof(temporary), "%s.XXXXXX", path);
  if (length < 0 || (size_t)length >= sizeof(temporary) ||
      RAND_bytes(key, sizeof(key)) != 1)
  {
    rtLog_Error("Cannot generate diagnostic key for %s", path);
    goto done;
  }
  rtDiagAuth_Hex(key, hex);
  fd = mkstemp(temporary);
  if (fd < 0)
    goto failed;
  created = 1;
  if (fchmod(fd, 0600) != 0)
    goto failed;
  while (used < 64)
  {
    ssize_t count = write(fd, hex + used, 64 - used);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0)
      goto failed;
    used += (size_t)count;
  }
  if (fsync(fd) != 0)
    goto failed;
  /* Publish a complete key without replacing an existing file or symlink. */
  if (link(temporary, path) == 0)
    result = 0;
  else if (errno == EEXIST)
    result = rtDiagAuth_LoadKey(path, key) == 0 ? 0 : -1;
  else
    goto failed;
  goto done;
failed:
  rtLog_Error("Cannot provision diagnostic key %s: %s", path, strerror(errno));
done:
  if (fd >= 0 && close(fd) != 0)
  {
    rtLog_Error("Cannot close diagnostic key file: %s", strerror(errno));
    result = -1;
  }
  if (created && unlink(temporary) != 0)
  {
    rtLog_Error("Cannot remove diagnostic key temporary file: %s", strerror(errno));
    result = -1;
  }
  OPENSSL_cleanse(key, sizeof(key));
  OPENSSL_cleanse(hex, sizeof(hex));
  return result;
}

int
rtDiagAuth_Challenge(rtDiagChallenge* state)
{
  unsigned char nonce[32];
  memset(state, 0, sizeof(*state));
  if (RAND_bytes(nonce, sizeof(nonce)) != 1 ||
      clock_gettime(CLOCK_MONOTONIC, &state->issued) != 0)
  {
    rtLog_Error("Cannot generate diagnostic authentication challenge");
    return -1;
  }
  rtDiagAuth_Hex(nonce, state->nonce);
  return 0;
}

int
rtDiagAuth_Sign(const unsigned char key[32], const char* nonce,
    const char* command, const char* value, char mac[65])
{
  /* Fixed-size nonce and length-prefixed fields prevent ambiguous concatenation. */
  unsigned char input[64 + 8 + RTROUTER_DIAG_COMMAND_MAX + RTROUTER_DIAG_VALUE_MAX];
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_size = 0;
  const char* fields[2] = {command, value ? value : ""};
  size_t used = 64;
  size_t i;
  if (!nonce || strlen(nonce) != 64 || !command || !*command ||
      strlen(command) > RTROUTER_DIAG_COMMAND_MAX ||
      (value && strlen(value) > RTROUTER_DIAG_VALUE_MAX))
  {
    rtLog_Error("Invalid diagnostic authentication fields");
    return -1;
  }
  memcpy(input, nonce, 64);
  for (i = 0; i < 2; ++i)
  {
    uint32_t length = (uint32_t)strlen(fields[i]);
    input[used++] = (unsigned char)(length >> 24);
    input[used++] = (unsigned char)(length >> 16);
    input[used++] = (unsigned char)(length >> 8);
    input[used++] = (unsigned char)length;
    memcpy(input + used, fields[i], length);
    used += length;
  }
  if (!HMAC(EVP_sha256(), key, 32, input, used, digest, &digest_size) || digest_size != 32)
  {
    rtLog_Error("Cannot calculate diagnostic authentication MAC");
    return -1;
  }
  rtDiagAuth_Hex(digest, mac);
  return 0;
}

int
rtDiagAuth_Verify(rtDiagChallenge* state, const unsigned char key[32],
    const char* command, const char* value, const char* mac)
{
  rtDiagChallenge challenge = *state;
  struct timespec now;
  char expected[65];
  int64_t elapsed;
  /* Every attempt consumes the challenge, including malformed and failed attempts. */
  memset(state, 0, sizeof(*state));
  if (!challenge.nonce[0] || !mac || strlen(mac) != 64 ||
      clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    goto rejected;
  elapsed = ((int64_t)now.tv_sec - challenge.issued.tv_sec) * 1000000000 +
      now.tv_nsec - challenge.issued.tv_nsec;
  if (elapsed < 0 || elapsed > INT64_C(10000000000) ||
      rtDiagAuth_Sign(key, challenge.nonce, command, value, expected) != 0 ||
      CRYPTO_memcmp(expected, mac, 64) != 0)
    goto rejected;
  return 0;
rejected:
  rtLog_Warn("Diagnostic authentication failed: invalid, expired or reused challenge/MAC");
  return -1;
}
