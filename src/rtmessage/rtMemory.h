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

#include <stdlib.h>

#define rt_malloc(size) rt_malloc_at((size), __FILE__, __LINE__, 1)
#define rt_calloc(num, size) rt_calloc_at((num), (size), __FILE__, __LINE__, 1)
#define rt_realloc(ptr, new_size) rt_realloc_at((ptr), (new_size), __FILE__, __LINE__, 1)
#define rt_try_malloc(size) rt_malloc_at((size), __FILE__, __LINE__, 0)
#define rt_try_calloc(num, size) rt_calloc_at((num), (size), __FILE__, __LINE__, 0)
#define rt_try_realloc(ptr, new_size) rt_realloc_at((ptr), (new_size), __FILE__, __LINE__, 0)
#define rt_free(ptr) rt_free_at((ptr), __FILE__, __LINE__, 0)

void* rt_malloc_at(size_t size, char const* file, int line, int do_abort);
void* rt_calloc_at(size_t num, size_t size, char const* file, int line, int do_abort);
void* rt_realloc_at(void* ptr, size_t new_size, char const* file, int line, int do_abort);
void rt_free_at(void* ptr, char const* file, int line, int do_abort);
