/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#include "rtCipher.h"
#include "rtError.h"
#include "rtLog.h"
#include "rtBase64.h"
#include "rtMemory.h"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#define RT_UNUSED(x) UNUSED_ ## x __attribute__((__unused__))

#ifdef WITH_SPAKE2

#include "spake2plus.h"
#include "common.h"

#define APPLY_STRING_OPTION(message, option, variable, default)\
  {\
    const char* sVal;\
    if (rtMessage_GetString(message, option, &sVal) == RT_OK)\
      snprintf(variable, sizeof(variable), "%s", sVal);\
    else\
      snprintf(variable, sizeof(variable), "%s", default);\
  }

struct _rtCipher
{
  SPAKE2PLUS* spake2_ctx;
  uint8_t key[EVP_MAX_MD_SIZE];
  bool is_server;
};

static rtError
CreateSpake2PlusInstance(rtMessage const opts, SPAKE2PLUS** spake2_ctx)
{
  // https://tools.ietf.org/id/draft-irtf-cfrg-spake2-10.html
  #define OPT_STR_MAX_LEN 256

  int             ret;
  char const*     psk = NULL;
  char const*     verify_L_str;
  uint8_t const*  verify_L = NULL;
  size_t          verify_L_len = 0;
  char const*     verify_w0_str;
  uint8_t const*  verify_w0 = NULL;
  size_t          verify_w0_len = 0;
  char            client_id[OPT_STR_MAX_LEN];
  char            server_id[OPT_STR_MAX_LEN];
  char            auth_data[OPT_STR_MAX_LEN];
  char            group_name[OPT_STR_MAX_LEN];
  char            evpmd_name[OPT_STR_MAX_LEN];
  char            macfunc_name[OPT_STR_MAX_LEN];
  int             client_or_server;
  bool            is_server;

  APPLY_STRING_OPTION(opts, RT_CIPHER_SPAKE2_CLIENT_ID,     client_id,    "client");
  APPLY_STRING_OPTION(opts, RT_CIPHER_SPAKE2_SERVER_ID,     server_id,    "server");
  APPLY_STRING_OPTION(opts, RT_CIPHER_SPAKE2_AUTH_DATA,     auth_data,    "Use SPAKE2+ latest version.");
  APPLY_STRING_OPTION(opts, RT_CIPHER_SPAKE2_GROUP_NAME,    group_name,   SPAKE2PLUS_GROUP_P256_SEARCH_NAME);
  APPLY_STRING_OPTION(opts, RT_CIPHER_SPAKE2_EVPMD_NAME,    evpmd_name,   SPAKE2PLUS_HASH_SHA256_SEARCH_NAME);
  APPLY_STRING_OPTION(opts, RT_CIPHER_SPAKE2_MACFUNC_NAME,  macfunc_name, SPAKE2PLUS_HMAC_SEARCH_NAME);

  is_server = false;
  if (rtMessage_GetBool(opts, RT_CIPHER_SPAKE2_IS_SERVER, &is_server) == RT_OK && is_server)
    client_or_server = SPAKE2PLUS_SERVER;
  else
    client_or_server = SPAKE2PLUS_CLIENT;

  // psk is a required parameter
  if (is_server)
  {
    if(rtMessage_GetString(opts, RT_CIPHER_SPAKE2_VERIFY_L, &verify_L_str) != RT_OK)
    {
      rtLog_Error("Failed to intiailize spake2+.  %s parameter is required but not found.", RT_CIPHER_SPAKE2_VERIFY_L);
      return rtErrorFromErrno(EINVAL);
    }
    if(rtBase64_decode((unsigned char const*)verify_L_str, (const unsigned int)strlen(verify_L_str), 
                       (void**)&verify_L, (unsigned int*)&verify_L_len) != RT_OK)
    {
      rtLog_Error("Failed to intiailize spake2+.  %s parameter invalid base64 string.", RT_CIPHER_SPAKE2_VERIFY_L);
      return rtErrorFromErrno(EINVAL);
    }

    if(rtMessage_GetString(opts, RT_CIPHER_SPAKE2_VERIFY_W0, &verify_w0_str) != RT_OK)
    {
      rtLog_Error("Failed to intiailize spake2+.  %s parameter is required but not found.", RT_CIPHER_SPAKE2_VERIFY_W0);
      return rtErrorFromErrno(EINVAL);
    }
    if(rtBase64_decode((unsigned char const*)verify_w0_str, (const unsigned int)strlen(verify_w0_str), 
                       (void**)&verify_w0, (unsigned int*)&verify_w0_len) != RT_OK)
    {
      rtLog_Error("Failed to intiailize spake2+.  %s parameter invalid base64 string.", RT_CIPHER_SPAKE2_VERIFY_L);
      return rtErrorFromErrno(EINVAL);
    }
  }
  else
  {
    if(rtMessage_GetString(opts, RT_CIPHER_SPAKE2_PSK, &psk) != RT_OK)
    {
      rtLog_Error("Failed to intiailize spake2+.  %s parameter is required but not found.", RT_CIPHER_SPAKE2_PSK);
      return rtErrorFromErrno(EINVAL);
    }
  }

  if ((ret = spake2plus_init( spake2_ctx,
                              client_id,
                              strlen(client_id),
                              server_id,
                              strlen(server_id),
                              auth_data,
                              strlen(auth_data),
                              group_name,
                              evpmd_name,
                              macfunc_name,
                              client_or_server)) != SPAKE2PLUS_OK)
  {
    rtLog_Error("Failed to intiailize spake2+. Err = %d", ret);
    return RT_FAIL;
  }

  if(is_server)
  {
    if ((ret = spake2plus_load_L_w0(*spake2_ctx,
                                     (uint8_t*)verify_L,
                                     verify_L_len,
                                     (uint8_t*)verify_w0,
                                     verify_w0_len)) != SPAKE2PLUS_OK)
    {
      rtLog_Error("Failed to initialize spake2+ verify data. Err = %d", ret);
      spake2plus_free(*spake2_ctx);
      return RT_FAIL;
    }
  }
  else
  {
    if ((ret = spake2plus_pwd_init(*spake2_ctx,
                                    (char*)psk,
                                    strlen(psk)
                                    )) != SPAKE2PLUS_OK)
    {
      rtLog_Error("Failed to initialize spake2+ password. Err = %d", ret);
      spake2plus_free(*spake2_ctx);
      return RT_FAIL;
    }
  }

  rtLog_Debug("Spake2+ %s created", (*spake2_ctx)->client_server == SPAKE2PLUS_SERVER ? "verifier" : "prover");
  return RT_OK;
}

rtError
rtCipher_CreateCipherSpake2Plus(rtCipher** cipher, rtMessage const opts)
{
  rtError err;
  SPAKE2PLUS* spake2_ctx = NULL;

  if (cipher == NULL)
  {
    rtLog_Error("Failed to intiailize spake2+.  cipher parameter is NULL.");
    return rtErrorFromErrno(EINVAL);
  }

  *cipher = NULL;

  err = CreateSpake2PlusInstance(opts, &spake2_ctx);
  if(err != RT_OK)
    return err;

  (*cipher) = rt_try_malloc(sizeof(struct _rtCipher));
  if(!(*cipher))
    return rtErrorFromErrno(ENOMEM);
  (*cipher)->spake2_ctx = spake2_ctx;
  (*cipher)->is_server = false;
  rtMessage_GetBool(opts, RT_CIPHER_SPAKE2_IS_SERVER, &(*cipher)->is_server);

  rtLog_Debug("rtCipher created");

  return RT_OK;
}

rtError
rtCipher_Destroy(rtCipher* cipher)
{
  if (!cipher)
    return RT_OK;

  if (cipher->spake2_ctx)
    spake2plus_free(cipher->spake2_ctx);

  free(cipher);

  rtLog_Debug("rtCipher destroyed");
    
  return RT_OK;
}

rtError
rtCipher_Encrypt(rtCipher* cipher, uint8_t const* data, uint32_t data_length, uint8_t* encrypted, uint32_t encrypted_max_length, uint32_t* encrypted_length)
{
  if(!cipher || !cipher->spake2_ctx || !cipher->spake2_ctx->Ke_len)
    return RT_FAIL;

  return rtCipher_EncryptWithKey(cipher->spake2_ctx->Ke, data, (size_t)data_length, encrypted, (size_t)encrypted_max_length, encrypted_length);
}

rtError
rtCipher_EncryptWithKey(uint8_t* key, uint8_t const* data, uint32_t data_length, uint8_t* encrypted, uint32_t encrypted_max_length, uint32_t* encrypted_length)
{
  if(!key)
    return RT_FAIL;

  *encrypted_length = (uint32_t)aes_encrypt(data, (size_t)data_length, encrypted, (size_t)encrypted_max_length, key);

  //assert(*encrypted_length == data_length);
  //if(*encrypted_length == data_length)
    return RT_OK;
  //else
    //return RT_FAIL;
}

rtError
rtCipher_Decrypt(rtCipher* cipher, uint8_t const* encrypted, uint32_t encrypted_length, uint8_t* decrypted, uint32_t decrypted_max_length, uint32_t* decrypted_length)
{
  if(!cipher || !cipher->spake2_ctx || !cipher->spake2_ctx->Ke_len)
    return RT_FAIL;
  return rtCipher_DecryptWithKey(cipher->spake2_ctx->Ke, encrypted, encrypted_length, decrypted, decrypted_max_length, decrypted_length);
}

rtError
rtCipher_DecryptWithKey(uint8_t* key, uint8_t const* encrypted, uint32_t encrypted_length, uint8_t* decrypted, uint32_t decrypted_max_length, uint32_t* decrypted_length)
{
  if(!key)
    return RT_FAIL;

  *decrypted_length = (uint32_t)aes_decrypt(encrypted, (size_t)encrypted_length, decrypted, (size_t)decrypted_max_length, key);

  //assert(*data_length == encrypted_length);
  //if(*data_length == encrypted_length)
    return RT_OK;
  //else
   // return RT_FAIL;
}

/*
  Spake2+ key exchange sequence

  client                    server
  gen pA
  send pA       ->          read pA
                            derive Fb from pA
                            gen pB
  read pB       <-          send pB
  derive Fa from pB         
  read Fb       <-          send Fb
  send Fa       ->          read Fa
  verify Fb                 verify Fa
  get key !                 get key !
*/

rtError 
rtCipher_RunKeyExchangeClient(rtCipher* cipher, rtConnection con)
{
  rtError err;
  int ret = 0;
  rtMessage msg1 = NULL;
  rtMessage msg2 = NULL;
  rtMessage res1 = NULL;
  uint8_t *pA = NULL;
  size_t pA_len = 0;
  uint8_t *pB = NULL;
  size_t pB_len = 0;
  uint8_t *Fb = NULL;
  size_t Fb_len = 0;
  uint8_t Fa[EVP_MAX_MD_SIZE];
  size_t Fa_len = 0;

  rtLog_Info("spake2+ running client key exchange using messages");

  rtMessage_Create(&msg1);
  rtMessage_SetString(msg1, "type", "spake2plus");
  rtMessage_SetInt32(msg1, "step", 1);

  //
  // setup for new key
  //
  rtLog_Info("spake2+ setup protocol");
  if (SPAKE2PLUS_OK != (ret = spake2plus_setup_protocol(
                  cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ setup protocol failed: %d", ret);
    goto on_err;
  }

  //
  // generate pA
  //
  rtLog_Info("spake2+ get pA length");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_own_pA_or_pB(
                  NULL,
                  &pA_len,
                  cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ get pA length failed: %d", ret);
    goto on_err;
  }

  if (NULL == (pA =(uint8_t *)rt_try_malloc(pA_len)))
  {
    rtLog_Error("spake2+ failed to allocate %zu bytes", pA_len);
    goto on_err;
  }

  rtLog_Info("spake2+ get pA value");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_own_pA_or_pB(
                  pA,
                  &pA_len,
                  cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ get pA value failed: %d", ret);
    goto on_err;
  }

  //
  // add our pA to message
  //
  rtLog_Info("spake2+ add pA to message");
  err = rtMessage_AddBinaryData(msg1, "pA", pA, pA_len);
  if (err != RT_OK)
  {
    rtLog_Error("spake2+ add pA to message failed");
    goto on_err;
  }

  //
  // send message to broker
  //
  err = rtConnection_SendRequest(con, msg1, RTROUTED_KEY_EXCHANGE, &res1, 3000);
  if(err != RT_OK)
  {
    rtLog_Error("spake2+ failed to send key exchange request");
    goto on_err;
  }

  //
  //handle response
  //

  //
  //  get the server's pB
  //
  rtLog_Info("spake2+ read pB from response");
  err = rtMessage_GetBinaryData(res1, "pB", (void**)&pB, (uint32_t*)&pB_len);
  if (err != RT_OK)
  {
    rtLog_Error("spake2+ read pB from response failed");
    goto on_err;
  }

  //
  // confirm keys
  //
  rtLog_Info("spake2+ confirm keys");
  ret = spake2plus_derive_confirmation_keys(Fa, &Fa_len, cipher->spake2_ctx, pB, pB_len);
  if (ret != SPAKE2PLUS_OK)
  {
    rtLog_Info("spake2+ confirm keys failed: %d", ret);
    goto on_err;
  }

  //
  //  get the server's Fb
  //
  rtLog_Info("spake2+ read Fb from response");
  err = rtMessage_GetBinaryData(res1, "Fb", (void**)&Fb, (uint32_t*)&Fb_len);
  if (err != RT_OK)
  {
    rtLog_Error("spake2+ read Fb from response failed");
    goto on_err;
  }

  //
  //  add our Fa to a final message
  //
  rtMessage_Create(&msg2);
  rtMessage_SetString(msg2, "type", "spake2plus");
  rtMessage_SetInt32(msg2, "step", 2);

  rtLog_Info("spake2+ add Fa to response");
  err = rtMessage_AddBinaryData(msg2, "Fa", Fa, Fa_len);
  if (err != RT_OK)
  {
    rtLog_Error("spake2+ add Fa to response failed");
    goto on_err;
  }

  //
  // send final message with Fa to broker
  //
  err = rtConnection_SendMessage(con, msg2, RTROUTED_KEY_EXCHANGE);
  if(err != RT_OK)
  {
    rtLog_Error("spake2+ failed to send key exchange final message");
    goto on_err;
  }

  //
  //  verify Fb
  //
  rtLog_Info("spake2+ verify Fb");
  ret = spake2plus_verify(cipher->spake2_ctx, Fb, Fb_len);
  if (ret != SPAKE2PLUS_OK)
  {
    rtLog_Info("spake2+ verify Fb failed: %d", ret);
    goto on_err;
  }

  //
  //  finally get the key
  //
  rtLog_Info("spake2+ get key");
  size_t Ke_len;
  ret = spake2plus_get_key_Ke(cipher->key, &Ke_len, cipher->spake2_ctx);
  if (ret != SPAKE2PLUS_OK)
  {
      rtLog_Error("spake2+ get key failed: %d", ret);
      goto on_err;
  }

  rtLog_Info("spake2+ key exchange succeeded");
  err = RT_OK;
  goto on_cleanup;

on_err:
  rtLog_Info("spake2+ key exchange failed");
  err = RT_FAIL;

on_cleanup:
  if(pA)
    free(pA);
  if(pB)
    free(pB);
  if(Fb)
    free(Fb);
  if(msg1)
    rtMessage_Release(msg1);
  if(msg2)
    rtMessage_Release(msg2);
  if(res1)
    rtMessage_Release(res1);

  return err;
}

rtError
rtCipher_RunKeyExchangeServer(rtCipher* cipher, rtMessage request, rtMessage* response, uint8_t** key)
{
  rtError err;
  int ret = 1;
  uint8_t *pA = NULL;
  uint32_t pA_len = 0;
  uint8_t *pB = NULL;
  size_t pB_len = 0;
  uint8_t *Fa = NULL;
  uint32_t Fa_len = 0;
  uint8_t Fb[EVP_MAX_MD_SIZE];
  size_t Fb_len = 0;
  int32_t step;

  rtLog_Info("spake2+ running server key exchange using messages");

  *response = NULL;

  if(rtMessage_GetInt32(request, "step", &step) != RT_OK)
  {
    rtLog_Error("rtCipher_HandleServerMessage missing step parameter");
    goto on_err;
  }

  rtLog_Debug("spake2+ key exchange step %d", step);

  if(step == 1)
  {
    //
    // setup for new key
    //
    rtLog_Info("spake2+ setup protocol");
    ret = spake2plus_setup_protocol(cipher->spake2_ctx);
    if (ret != SPAKE2PLUS_OK)
    {
      rtLog_Error("spake2+ setup protocol fail: %d", ret);
      goto on_err;
    }

    //
    // generate pB
    //
    rtLog_Info("spake2+ get pB length");
    ret = spake2plus_get_own_pA_or_pB(NULL, &pB_len, cipher->spake2_ctx);
    if (ret != SPAKE2PLUS_OK)
    {
      rtLog_Error("spake2+ get pB length failed: %d", ret);
      goto on_err;
    }

    pB = rt_try_malloc(pB_len);
    if(!pB)
    {
      rtLog_Error("spake2+ failed to allocate %zu bytes", pB_len);
      goto on_err;
    }

    rtLog_Info("spake2+ get pB value");
    ret = spake2plus_get_own_pA_or_pB(pB, &pB_len, cipher->spake2_ctx);
    if (ret != SPAKE2PLUS_OK)
    {
      rtLog_Info("spake2+ get pB value failed: %d", ret);
      goto on_err;
    }

    //
    // get client's pA
    //
    rtLog_Info("spake2+ read pA from request");
    err = rtMessage_GetBinaryData(request, "pA", (void**)&pA, &pA_len);
    if (err != RT_OK)
    {
      rtLog_Error("spake2+ read pA from request failed");
      goto on_err;
    }

    //
    // add our pB to response
    //
    rtLog_Info("spake2+ add pB to response");

    rtMessage_Create(response);

    err = rtMessage_AddBinaryData(*response, "pB", pB, pB_len);
    if (err != RT_OK)
    {
      rtLog_Error("spake2+ add pB to response failed");
      goto on_err;
    }

    //
    // confirm keys
    //
    rtLog_Info("spake2+ confirm keys");
    ret = spake2plus_derive_confirmation_keys(Fb, &Fb_len, cipher->spake2_ctx, pA, pA_len);
    if (ret != SPAKE2PLUS_OK)
    {
      rtLog_Error("spake2+ confirm keys failed: %d", ret);
      goto on_err;
    }

    //
    // add our Fb to response
    //
    rtLog_Info("spake2+ add Fb to response");
    err = rtMessage_AddBinaryData(*response, "Fb", Fb, Fb_len);
    if (err != RT_OK)
    {
      rtLog_Error("spake2+ add Fb to response failed");
      goto on_err;
    }

    rtLog_Info("spake2+ key exchange step 1 succeeded");
    goto on_success;
  }
  else if(step == 2)
  {
    //
    // get the client's Fa
    //
    rtLog_Info("spake2+ read Fa from request");
    err = rtMessage_GetBinaryData(request, "Fa", (void**)&Fa, &Fa_len);
    if (err != RT_OK)
    {
      rtLog_Error("spake2+ read Fa from request failed");
      goto on_err;
    }

    //
    // verify the client's Fa
    //
    rtLog_Info("spake2+ verify the Fa");
    ret = spake2plus_verify(cipher->spake2_ctx, Fa, Fa_len);
    if (ret != SPAKE2PLUS_OK)
    {
      rtLog_Info("spake2+ verify failed: %d", ret);
      goto on_err;
    }

    //
    //  finally get the key
    //
    size_t Ke_len;
    rtLog_Info("spake2+ get key");
    ret = spake2plus_get_key_Ke(cipher->key, &Ke_len, cipher->spake2_ctx);
    if (ret != SPAKE2PLUS_OK)
    {
      rtLog_Error("spake2+ get key failed: %d", ret);
      goto on_err;
    }

    if(key)
    {
      *key = rt_try_malloc(RT_CIPHER_MAX_KEY_SIZE);/* it fails if Ke_len is used for size */
      if(!(*key))
      {
        rtLog_Error("spake2+ failed to allocate %d bytes", RT_CIPHER_MAX_KEY_SIZE);
        goto on_err;
      }      
      memset(*key, 0, RT_CIPHER_MAX_KEY_SIZE);
      memcpy(*key, cipher->key, Ke_len);
    }

    rtLog_Info("spake2+ key exchange succeeded");
    goto on_success;
  }
  else
  {
    rtLog_Error("rtCipher_HandleServerMessage invalid step");
    goto on_err;
  }

on_success:
  err = RT_OK;
  goto on_cleanup;

on_err:
  rtLog_Info("spake2+ key exchange failed");
  err = RT_FAIL;

on_cleanup:
  if(pA)
    free(pA);
  if(pB)
    free(pB);
  if(Fa)
    free(Fa);

  return err;
}

#if 0

/*
  Saving the intial way of doing key exchange by writing directly to a socket.
  This way was seems slightly uglier thus I went with doing a regular message
  request/response(above).  This only works if you run it before the rtConnection 
  reader thread is started and it requires what looks like a hack to 
  tConnectedClient_Read that would sniff for some new header preamble flag.
*/
static rtError 
rtCipher_RunKeyExchangeServer(rtCipher* cipher, int sock, uint8_t** key)
{
  uint8_t *pB = NULL;
  size_t pB_len = 0;
  uint8_t Fb[EVP_MAX_MD_SIZE];
  size_t Fb_len = 0;
  int ret = 1;
  uint8_t buffer[2000] = {0};

  rtLog_Info("spake2+ server key exchange start");

  rtLog_Info("spake2+ setup protocol");
  if (SPAKE2PLUS_OK != (ret = spake2plus_setup_protocol(cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ setup protocol fail: %d", ret);
    return RT_FAIL;
  }

  rtLog_Info("spake2+ get pB length");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_own_pA_or_pB(
                            NULL,
                            &pB_len,
                            cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ get pB length failed: %d", ret);
    return RT_FAIL;
  }

  if (NULL == (pB = rt_try_malloc(pB_len)))
  {
    rtLog_Error("spake2+ failed to allocate %zu bytes", pB_len);
    return RT_FAIL;
  }

  rtLog_Info("spake2+ get pB value");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_own_pA_or_pB(
                            pB,
                            &pB_len,
                            cipher->spake2_ctx)))
  {
    rtLog_Info("spake2+ get pB value failed: %d", ret);
    goto on_err;
  }

  rtLog_Info("spake2+ read pA");
  int data_size = spake2_read_block(sock, buffer, sizeof(buffer));
  if (data_size < 0)
  {
    rtLog_Error("spake2+ read pA failed");
    goto on_err;
  }

  rtLog_Info("spake2+ write pB");
  if (spake2_write_block(sock, pB, pB_len) <= 0)
  {
    rtLog_Error("spake2+ write pB");
    goto on_err;
  }

  rtLog_Info("spake2+ confirm keys");
  if (SPAKE2PLUS_OK != (ret = spake2plus_derive_confirmation_keys(
                            Fb,
                            &Fb_len,
                            cipher->spake2_ctx,
                            buffer,
                            data_size)))
  {
    rtLog_Error("spake2+ confirm keys failed: %d", ret);
    goto on_err;
  }

  rtLog_Info("spake2+ write Fb");
  if (spake2_write_block(sock, Fb, Fb_len) <= 0)
  {
    rtLog_Error("spake2+ write Fb failed");
    goto on_err;
  }

  rtLog_Info("spake2+ read Fa");
  data_size = spake2_read_block(sock, buffer, sizeof(buffer));
  if (data_size < 0)
  {
    rtLog_Error("spake2+ read Fa failed");
    goto on_err;
  }

  rtLog_Info("spake2+ verify");
  if (SPAKE2PLUS_OK != (ret = spake2plus_verify(cipher->spake2_ctx, buffer, data_size)))
  {
    rtLog_Info("spake2+ verify failed: %d", ret);
    goto on_err;
  }

  size_t Ke_len;
  rtLog_Info("spake2+ get key");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_key_Ke(cipher->key, &Ke_len, cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ get key failed: %d", ret);
    goto on_err;
  }

  if(key)
  {
    *key = rt_try_malloc(Ke_len);
    memcpy(*key, cipher->key, Ke_len);
  }

  rtLog_Info("spake2+ key exchange succeeded");
  return RT_OK;

on_err:
  //TODO -- should we delete this is success case too ?
  if(NULL != pB)
  {
    free(pB);
    pB = NULL;
  }
  rtLog_Info("spake2+ key exchange failed");
  return RT_FAIL;
}

static rtError 
rtCipher_RunKeyExchangeClient(rtCipher* cipher, int sock, uint8_t** key)
{
  uint8_t Fa[EVP_MAX_MD_SIZE];
  size_t Fa_len = 0;
  int ret = 0;
  uint8_t *pA = NULL;
  size_t pA_len = 0;
  uint8_t buffer[2000];

  rtLog_Info("spake2+ client key exchange start");

  //
  // Send command to broker to initiate key exchange
  //
  send(sock, RTROUTED_KEY_EXCHANGE , strlen(RTROUTED_KEY_EXCHANGE), 0);
  
  rtLog_Info("spake2+ setup protocol");
  if (SPAKE2PLUS_OK != (ret = spake2plus_setup_protocol(
                  cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ setup protocol failed: %d", ret);
    goto on_err;
  }

  rtLog_Info("spake2+ get pA length");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_own_pA_or_pB(
                  NULL,
                  &pA_len,
                  cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ get pA length failed: %d", ret);
    goto on_err;
  }

  if (NULL == (pA =(uint8_t *)rt_try_malloc(pA_len)))
  {
    rtLog_Error("spake2+ failed to allocate %zu bytes", pA_len);
    goto on_err;
  }

  rtLog_Info("spake2+ get pA value");
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_own_pA_or_pB(
                  pA,
                  &pA_len,
                  cipher->spake2_ctx)))
  {
    rtLog_Error("spake2+ get pA value failed: %d", ret);
    goto on_err;
  }

  rtLog_Info("spake2+ write pA");
  if (spake2_write_block(sock, pA, pA_len) <= 0)
  {
    rtLog_Error("spake2+ write pA failed");
    goto on_err;
  }

  rtLog_Info("spake2+ read pB");
  int data_size = spake2_read_block(sock, buffer, sizeof(buffer));
  if (data_size <= 0)
  {
    rtLog_Error("spake2+ read pB failed");
    goto on_err;
  }

  rtLog_Info("spake2+ confirm keys");
  if (SPAKE2PLUS_OK != (ret = spake2plus_derive_confirmation_keys(
                  Fa,
                  &Fa_len,
                  cipher->spake2_ctx,
                  buffer,
                  data_size)))
  {
    rtLog_Info("spake2+ confirm keys failed: %d", ret);
    goto on_err;
  }

  //Read Fb from server
  rtLog_Info("spake2+ read Fb");
  data_size = spake2_read_block(sock, buffer, sizeof(buffer));
  if (data_size <= 0)
  {
    rtLog_Error("spake2+ read Fb failed");
    goto on_err;
  }

  rtLog_Info("spake2+ write Fa");
  if (spake2_write_block(sock, Fa, Fa_len) <= 0)
  {
    rtLog_Error("spake2+ write Fa failed");
    goto on_err;
  }

  rtLog_Info("spake2+ verify");
  if (SPAKE2PLUS_OK != (ret = spake2plus_verify(cipher->spake2_ctx, buffer, data_size)))
  {
    rtLog_Info("spake2+ verify failed: %d", ret);
    goto on_err;
  }

  rtLog_Info("spake2+ get key");
  size_t Ke_len;
  if (SPAKE2PLUS_OK != (ret = spake2plus_get_key_Ke(cipher->key, &Ke_len, cipher->spake2_ctx)))
  {
      rtLog_Error("spake2+ get key failed: %d", ret);
      goto on_err;
  }

  if(key)
  {
    *key = rt_try_malloc(Ke_len);
    memcpy(*key, cipher->key, Ke_len);
  }

  rtLog_Info("spake2+ key exchange succeeded");
  return RT_OK;

on_err:
  /*TODO : rtsend didn't delete this but maybe we need to
  if(NULL != pA)
  {
      free(pA);
      pA = NULL;
  }*/
  rtLog_Info("spake2+ key exchange failed");
  return RT_FAIL; 
}

rtError rtCipher_RunKeyExchange(rtCipher* cipher, int sock, uint8_t** key)
{
  if(!cipher || !cipher->spake2_ctx)
    return RT_FAIL;

  rtLog_Info("spake2+ running key exchange using socket");

  if(cipher->is_server)
    return rtCipher_RunKeyExchangeServer(cipher, sock, key);
  else
    return rtCipher_RunKeyExchangeClient(cipher, sock, key);
}

#endif


#else //WITH_SPAKE2

rtError 
rtCipher_CreateCipherSpake2Plus(
  rtCipher** RT_UNUSED(cipher), 
  rtMessage const RT_UNUSED(opts))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

rtError 
rtCipher_Destroy(
  rtCipher* RT_UNUSED(cipher))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

rtError 
rtCipher_Encrypt(
  rtCipher* RT_UNUSED(cipher), 
  uint8_t const* RT_UNUSED(data), 
  uint32_t RT_UNUSED(data_length), 
  uint8_t* RT_UNUSED(encrypted), 
  uint32_t RT_UNUSED(encrypted_max_length), 
  uint32_t* RT_UNUSED(encrypted_length))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

rtError 
rtCipher_EncryptWithKey(
  uint8_t* RT_UNUSED(key), 
  uint8_t const* RT_UNUSED(data), 
  uint32_t RT_UNUSED(data_length), 
  uint8_t* RT_UNUSED(encrypted), 
  uint32_t RT_UNUSED(encrypted_max_length), 
  uint32_t* RT_UNUSED(encrypted_length))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

rtError 
rtCipher_Decrypt(
  rtCipher* RT_UNUSED(cipher), 
  uint8_t const* RT_UNUSED(encrypted), 
  uint32_t RT_UNUSED(encrypted_length), 
  uint8_t* RT_UNUSED(decrypted), 
  uint32_t RT_UNUSED(decrypted_max_length), 
  uint32_t* RT_UNUSED(decrypted_length))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

rtError 
rtCipher_DecryptWithKey(
  uint8_t* RT_UNUSED(key), 
  uint8_t const* RT_UNUSED(encrypted), 
  uint32_t RT_UNUSED(encrypted_length), 
  uint8_t* RT_UNUSED(decrypted), 
  uint32_t RT_UNUSED(decrypted_max_length), 
  uint32_t* RT_UNUSED(decrypted_length))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

rtError 
rtCipher_RunKeyExchange(
  rtCipher* RT_UNUSED(cipher), 
  int RT_UNUSED(sock), 
  uint8_t** RT_UNUSED(key))
{
  rtLog_Error("spake2+ support not enabled");
  return RT_ERROR_INVALID_OPERATION;
}

#endif //WITH_SPAKE2
