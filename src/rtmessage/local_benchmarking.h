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
#include <stdio.h>
#include <sys/time.h>
#define MAX_REPS 100000
#ifdef ENABLE_ROUTER_BENCHMARKING
#define CHECKPOINT() printf("checkpoint %s:%d\n", __FUNCTION__, __LINE__)
#define INIT_TRACKING() do{list_index = 0;}while(0)
#define TRACKING_BOILERPLATE() struct timeval start_time, end_time
/*
	printf("%s:%d start timestamp: %llu\n", __FUNCTION__, __LINE__, now); \
	printf("%s:%d end timestamp: %llu\n", __FUNCTION__, __LINE__, now); \
	printf("%s[%d]: diff: %lluus\n", __FUNCTION__, __LINE__, now); \
*/
#define STOP_TRACKING_v2() do{gettimeofday(&end_time, NULL); \
	long long now = start_time.tv_sec * 1000000ll + start_time.tv_usec; \
	now = end_time.tv_sec * 1000000ll + end_time.tv_usec; \
	struct timeval diff; \
	timersub(&end_time, &start_time, &diff); \
	now = (diff.tv_sec * 1000000ll + diff.tv_usec); \
	diff_list[list_index] = now; \
	if(list_index < (MAX_REPS - 1)) list_index++; \
	} while(0)

#define STOP_TRACKING() do{gettimeofday(&end_time, NULL); \
	long long now = start_time.tv_sec * 1000000ll + start_time.tv_usec; \
	printf("%s:%d start timestamp: %llu\n", __FUNCTION__, __LINE__, now); \
	now = end_time.tv_sec * 1000000ll + end_time.tv_usec; \
	printf("%s:%d end timestamp: %llu\n", __FUNCTION__, __LINE__, now); \
	struct timeval diff; \
	timersub(&end_time, &start_time, &diff); \
	now = (diff.tv_sec * 1000000ll + diff.tv_usec)/1000ll; \
	printf("diff: %llums\n", now); \
	} while(0)

#define START_TRACKING() do{gettimeofday(&start_time, NULL); \
	} while(0);

#else
#define CHECKPOINT() 
#define INIT_TRACKING()
#define TRACKING_BOILERPLATE()
#define STOP_TRACKING_v2()
#define STOP_TRACKING()
#define START_TRACKING()

#endif
extern long long diff_list[];
extern unsigned int list_index;
void benchmark_print_stats(const char * tag);
void benchmark_reset();
