/*
  * If not stated otherwise in this file or this component's Licenses.txt file
  * the following copyright and licenses apply:
  *
  * Copyright 2016 RDK Management
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
/*****************************************
Test server for unit test client testing
******************************************/

#define METHOD_GET_BINARY_RPC "METHOD_GET_BINARY_RPC"
#define METHOD_SET_BINARY_DATA_SIZE_RPC "METHOD_SET_BINARY_DATA_SIZE_RPC"
#define METHOD_GET_LARGE_BINARY_RPC "METHOD_GET_LARGE_BINARY_RPC"
#define METHOD_SET_BINARY_RPC "METHOD_SET_BINARY_RPC"
#define METHOD_SET_TIMEOUT_RPC "METHOD_SET_TIMEOUT_RPC"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   char name[100];
   int age;
   bool status;
}test_struct_t;

typedef struct
{
   char object_name[100];
   char student_name[100];
}student_details_t;

typedef struct
{
   int numerals[100];
   int count;
}test_array_data_t;

void reset_stored_data();
int handle_get1(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_set1(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_get2(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_recursive_get(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_set2(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_getStudentInfo(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_setStudentInfo(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_getBinaryData(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_setBinaryDataSize(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_getLargeBinaryData(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_setBinaryData(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_getAttributes1(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_setAttributes1(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_getAttributes2(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_setAttributes2(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int handle_timeout(const char * destination, const char * method, rtMessage request, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
void handle_unknown(const char * destination, const char * method, rtMessage request, rtMessage *response, const rtMessageHeader* hdr);
int callback(const char * destination, const char * method, rtMessage message, void * user_data, rtMessage *response, const rtMessageHeader* hdr);
int sub1_callback(const char * object,  const char * event, const char * listener, int added, const rtMessage filter, void* data);

#ifdef __cplusplus
}
#endif
