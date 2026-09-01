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
#ifndef RT_MESSAGE_CRYPTO_H
#define RT_MESSAGE_CRYPTO_H

#include <rtError.h>
#include <rtMessage.h>
#include <rtConnection.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RT_CIPHER_MAX_KEY_SIZE        64
#define RT_CIPHER_SPAKE2_PSK          "spake2_psk"
#define RT_CIPHER_SPAKE2_VERIFY_L     "spake2_verify_L"
#define RT_CIPHER_SPAKE2_VERIFY_W0    "spake2_verify_w0"
#define RT_CIPHER_SPAKE2_IS_SERVER    "spake2_is_server"
#define RT_CIPHER_SPAKE2_CLIENT_ID    "spake2_client_id"
#define RT_CIPHER_SPAKE2_SERVER_ID    "spake2_server_id"
#define RT_CIPHER_SPAKE2_AUTH_DATA    "spake2_auth_data"
#define RT_CIPHER_SPAKE2_GROUP_NAME   "spake2_group_name"
#define RT_CIPHER_SPAKE2_EVPMD_NAME   "spake2_evpmd_name"
#define RT_CIPHER_SPAKE2_MACFUNC_NAME "spake2_macfunc_name"
#define RTROUTED_KEY_EXCHANGE         "_key_exchange"

typedef struct _rtCipher rtCipher;

rtError 
rtCipher_CreateCipherSpake2Plus(
  rtCipher** cipher, 
  rtMessage const opts);

rtError 
rtCipher_Destroy(
  rtCipher* cipher);

rtError 
rtCipher_Encrypt(
  rtCipher* cipher, 
  uint8_t const* data, 
  uint32_t data_length, 
  uint8_t* encrypted, 
  uint32_t encrypted_max_length, 
  uint32_t* encrypted_length);

rtError 
rtCipher_EncryptWithKey(
  uint8_t* key, 
  uint8_t const* data, 
  uint32_t data_length, 
  uint8_t* encrypted, 
  uint32_t encrypted_max_length, 
  uint32_t* encrypted_length);

rtError 
rtCipher_Decrypt(
  rtCipher* cipher, 
  uint8_t const* encrypted, 
  uint32_t encrypted_length, 
  uint8_t* decrypted, 
  uint32_t decrypted_max_length, 
  uint32_t* decrypted_length);

rtError 
rtCipher_DecryptWithKey(
  uint8_t* key, 
  uint8_t const* encrypted, 
  uint32_t encrypted_length, 
  uint8_t* decrypted, 
  uint32_t decrypted_max_length, 
  uint32_t* decrypted_length);

rtError 
rtCipher_RunKeyExchangeClient(
  rtCipher* cipher, 
  rtConnection con);

rtError 
rtCipher_RunKeyExchangeServer(
  rtCipher* cipher, 
  rtMessage request, 
  rtMessage* response, 
  uint8_t** key);

#ifdef __cplusplus
}
#endif
#endif
