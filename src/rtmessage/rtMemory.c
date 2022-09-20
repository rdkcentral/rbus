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

/*  Using simple abort policy to handle memory allocaiton failures.
    The alternative would be to pass memory errors up to the user
    which would add complexity to our library and the users code.
 */

#include "rtMemory.h"
#include "rtLog.h"
#include <stdio.h>

static void handle_failed_alloc(char const* func, size_t size, char const* file, int line, int do_abort)
{
    /*try to log something first, then try to print something, and abort if asked to*/
    rtLog_Fatal("%s %zu bytes at %s:%d", func, size, file, line);

    fprintf(stderr, "%s %zu bytes at %s:%d", func, size, file, line);
    fflush(stderr);

    if(do_abort)
    {
        rtLog_Fatal("%s aborting process", func);

        fprintf(stderr, "%s aborting process\n", func);
        fflush(stderr);

        abort();
    }
}

void* rt_malloc_at(size_t size, char const* file, int line, int do_abort)
{
    void* ptr = malloc(size);
    if(!ptr && size)
        handle_failed_alloc(__FUNCTION__, size, file, line, do_abort);
    return ptr;
}

void* rt_calloc_at(size_t num, size_t size, char const* file, int line, int do_abort)
{
    void* ptr = calloc(num, size);
    if(!ptr && size)
        handle_failed_alloc(__FUNCTION__, size, file, line, do_abort);
    return ptr;
}

void* rt_realloc_at(void* ptr, size_t new_size, char const* file, int line, int do_abort)
{
    void* new_ptr = realloc(ptr, new_size);
    if(!new_ptr && new_size)
        handle_failed_alloc(__FUNCTION__, new_size, file, line, do_abort);
    return new_ptr;
}

void rt_free_at(void* ptr, char const* file, int line, int do_abort)
{
    (void)file;
    (void)line;
    (void)do_abort;
    free(ptr);
}