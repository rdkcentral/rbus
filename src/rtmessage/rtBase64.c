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

#include "rtBase64.h"
#include "rtLog.h"
#include "rtMemory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static const char base64_lookup_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char decode_base64_char(unsigned char in, bool * error)
{
    if(('A' <= in) && ('Z' >= in))
    {
        return (in - 'A');
    }

    if(('a' <= in) && ('z' >= in))
    {
        return (26 + in - 'a');
    }

    if(('0' <= in) && ('9' >= in))
    {
        return (52 + in - '0');
    }

    if('+' == in)
        return 62;

    if('/' == in)
        return 63;
    else
    {
        rtLog_Error("Illegal base64 character %x. Cannot decode.", in);
        *error = true; 
        return 0;
    }
}

rtError rtBase64_encode(const void * in, const unsigned int in_size, unsigned char ** out, unsigned int *out_size)
{
    unsigned char * read_buff = (unsigned char * )in;
    unsigned int last_group_len = in_size % 3;

    /*Allocate memory for the output*/
    *out_size = in_size * 4 / 3 + 1 /*Extra byte for string terminator.*/;
    if(0 != last_group_len)
        *out_size += 4;

    unsigned char *write_buff = (unsigned char *) rt_try_malloc(*out_size);
    if(NULL == write_buff)
    {
        rtLog_Error("Couldn't allocate memory for output.");
        return RT_FAIL;
    }

    /*Process all full-length groups first.*/
    unsigned int temp_size = in_size - last_group_len;
    unsigned int read_index  = 0;
    unsigned int write_index  = 0;
    while(read_index < temp_size)
    {
        write_buff[write_index++] = base64_lookup_table[(read_buff[read_index] >> 2)]; 
        write_buff[write_index++] = base64_lookup_table[(((read_buff[read_index] & 0x03) << 4) | (read_buff[read_index + 1] >> 4))];
        read_index++;

        write_buff[write_index++] = base64_lookup_table[(((read_buff[read_index] & 0xF) << 2) | (read_buff[read_index + 1] >> 6))]; 
        read_index++;

        write_buff[write_index++] = base64_lookup_table[(read_buff[read_index] & 0x3F)];
        read_index++;
    }

    switch(last_group_len)
    {
        case 0:
            break;//do nothing.

        case 1:
            /*Only one byte present in the last group.*/
            write_buff[write_index++] = base64_lookup_table[(read_buff[read_index] >> 2)];
            write_buff[write_index++] = base64_lookup_table[((read_buff[read_index] & 0x03) << 4)];
            write_buff[write_index++] = '='; 
            write_buff[write_index++] = '='; 
            break;

        case 2:
            /*Only two bytes present in the last group.*/
            write_buff[write_index++] = base64_lookup_table[(read_buff[read_index] >> 2)];
            write_buff[write_index++] = base64_lookup_table[(((read_buff[read_index] & 0x03) << 4) | (read_buff[read_index + 1] >> 4))];
            read_index++;
            write_buff[write_index++] = base64_lookup_table[((read_buff[read_index] & 0xF) << 2)];
            write_buff[write_index++] = '=';
            break;

        default:
            rtLog_Error("Unexpected condition.");
    }
    write_buff[write_index] = '\0';
    *out = write_buff;
    return RT_OK;
}

rtError rtBase64_decode(const unsigned char * in, const unsigned int in_size,  void ** out, unsigned int *out_size)
{
    if(in_size == 0){
	 *out = NULL;
	 return RT_OK;
    }
    int num_padding_bytes = 0;
    if(0 != (in_size % 4))
    {
        rtLog_Error("Illegal base64 encoding. Length is %d. It has to be a multiple of 4.", in_size);
        return RT_ERROR;
    }
    /*Allocate memory for the output*/
    *out_size = in_size * 3 / 4;
    if('=' == in[in_size -2])
    {
        /*Two character padding.*/
        num_padding_bytes = 2;
    }
    else if('=' == in[in_size -1])
    {
        /*One character padding.*/
        num_padding_bytes = 1;
    }
    *out_size -= num_padding_bytes;

    unsigned char *write_buff = (unsigned char *) rt_try_malloc(*out_size);
    if(NULL == write_buff)
    {
        rtLog_Fatal("Couldn't allocate memory for output.");
        return RT_FAIL;
    }


    /*Process all full-length groups first.*/
    unsigned int read_index  = 0;
    unsigned int write_index  = 0;
    unsigned int temp_size = in_size - (0 == num_padding_bytes ? 0 : 4);
    bool decode_error = false;
    while(read_index < temp_size)
    {
        write_buff[write_index++] = (decode_base64_char(in[read_index], &decode_error) << 2) | (decode_base64_char(in[read_index + 1], &decode_error) >> 4);
        write_buff[write_index++] = (decode_base64_char(in[read_index + 1], &decode_error) << 4) | (decode_base64_char(in[read_index + 2], &decode_error) >> 2);
        write_buff[write_index++] = (decode_base64_char(in[read_index + 2], &decode_error) << 6) | (decode_base64_char(in[read_index + 3], &decode_error) & 0x3F);
        read_index += 4;
    }

    if(2 == num_padding_bytes)
    {
        write_buff[write_index++] = (decode_base64_char(in[read_index], &decode_error) << 2) | (decode_base64_char(in[read_index + 1], &decode_error) >> 4);
    }
    else if(1 == num_padding_bytes)
    {
        write_buff[write_index++] = (decode_base64_char(in[read_index], &decode_error) << 2) | (decode_base64_char(in[read_index + 1], &decode_error) >> 4);
        write_buff[write_index++] = (decode_base64_char(in[read_index + 1], &decode_error) << 4) | (decode_base64_char(in[read_index + 2], &decode_error) >> 2);
    }

    if(true == decode_error)
    {
        rtLog_Error("Decode error.");
        free(write_buff);
        *out = NULL;
        out_size = 0;
        return RT_ERROR;
    }
    *out = write_buff;
    return RT_OK;
}
