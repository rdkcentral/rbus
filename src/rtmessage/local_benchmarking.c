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
#include "local_benchmarking.h"
long long diff_list[MAX_REPS];
unsigned int list_index = 0;
void benchmark_reset()
{
	printf("Resetting benchmarking data.\n");
	list_index = 0;
}
void benchmark_print_stats(const char * tag)
{
	long long sum = 0;
	long long max = 0;
	long long min= diff_list[0];
	unsigned int i = 0;
	printf("%s - %s\n", __FUNCTION__, tag);
	for(i = 0; i < list_index; i++)
	{
		sum += diff_list[i];
		if(diff_list[i] < min)
			min = diff_list[i];

		if(max < diff_list[i])
			max = diff_list[i];
	}
	printf("--- Start raw data (microseconds) ---\n");
	for(i = 0; i < list_index; i++)
	{
		printf("%llu\n", diff_list[i]);
	}
	printf("--- End raw data (microseconds) (%d entries) ---\n", i);
    if(0 != list_index)
        printf("mean microseconds: %llu, max: %llu, min: %llu\n", (sum/list_index), max, min);

}
