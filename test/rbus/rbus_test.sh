#############################################################################
# If not stated otherwise in this file or this component's Licenses.txt file
# the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################
echo "starting test"

expected_results=""

logProvider=/tmp/log.provider
logConsumer=/tmp/log.consumer
duration=`./rbusTestConsumer -d -a`

echo "test will complete in about $duration seconds"

./rbusTestProvider > $logProvider &
./rbusTestConsumer -a > $logConsumer &

wait

logmultiRbusOpenMethodProvider=/tmp/log.multiRbusOpenMethodProvider
logmultiRbusOpenMethodConsumer=/tmp/log.multiRbusOpenMethodConsumer

echo "Multi RbusOpen with Method Invoke test will complete in about 90 seconds"

./multiRbusOpenMethodProvider > $logmultiRbusOpenMethodProvider 2>&1 &
./multiRbusOpenMethodConsumer > $logmultiRbusOpenMethodConsumer 2>&1 &

#wait

logRbusOpenProvider=/tmp/log.multiRbusOpenprovider
logRbusOpenConsumer=/tmp/log.multiRbusOpenconsumer

echo "Multi RbusOpen with Register and subscribe test will complete in about 80 seconds"
./multiRbusOpenProvider > $logRbusOpenProvider &
./multiRbusOpenConsumer > $logRbusOpenConsumer &

wait

logRbusOpenRegRbusOpenProvider=/tmp/log.rbusOpenRegRbusOpenProvider
logRbusOpenSubRbusOpenConsumer=/tmp/log.rbusOpenSubRbusOpenConsumer

echo "Multi Rbus Open Register and Subscribe Negative test will complete in about 75 seconds"
./rbusOpenRegRbusOpenProvider > $logRbusOpenRegRbusOpenProvider 2>&1 &
./rbusOpenSubRbusOpenConsumer > $logRbusOpenSubRbusOpenConsumer 2>&1 &

wait

logmultiRbusOpenRbusGetProvider=/tmp/log.multiRbusOpenRbusGetProvider
logmultiRbusOpenRbusGetConsumer=/tmp/log.multiRbusOpenRbusGetConsumer

echo "multi rbus open register and subscribe negative test will complete in about 30 seconds"
./multiRbusOpenRbusGetProvider > $logmultiRbusOpenRbusGetProvider 2>&1 &
./multiRbusOpenRbusGetConsumer > $logmultiRbusOpenRbusGetConsumer 2>&1 &

wait

logmultiRbusOpenRbusSetProvider=/tmp/log.multiRbusOpenRbusSetProvider
logmultiRbusOpenRbusSetConsumer=/tmp/log.multiRbusOpenRbusSetConsumer

echo "multi rbus open register and subscribe negative test will complete in about 30 seconds"
./multiRbusOpenRbusGetProvider > $logmultiRbusOpenRbusSetProvider 2>&1 &
./multiRbusOpenRbusSetConsumer > $logmultiRbusOpenRbusSetConsumer 2>&1 &

#wait

#logMultiProviderThreadsForSingleEvent=/tmp/log.multiProviderThreadsForSingleEvent
#logMultiConsumerThreadsForSingleEvent=/tmp/log.multiConsumerThreadsForSingleEvent

#echo "multiProvider and multiConsumer Threads For a SingleEvent long duration"
#./multiProviderThreadsForSingleEvent > logMultiProviderThreadsForSingleEvent &
#./multiConsumerThreadsForSingleEvent > logMultiConsumerThreadsForSingleEvent &
#result=`diff -rupN <(grep _test_ $logConsumer) <(echo "$expected_results")`
#if [ -n "$result" ]; then
#    echo "The following tests failed:"
#    echo $result
#else
#    echo "All tests passed"
#fi
echo "Done."


