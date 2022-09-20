
# Rbus

RDK Bus (RBus) is a lightweight, fast and efficient bus messaging system. 
It allows interprocess communication (IPC) and remote procedure call (RPC)
between multiple process running on a hardware device.  It supports the
creation and use of a data model, which is a hierarchical tree of named 
objects with properties, events, and methods.


## Desktop Build (Linux)

    export RBUS_ROOT=${HOME}/rbus
    export RBUS_INSTALL_DIR=${RBUS_ROOT}/install
    export RBUS_BRANCH=2105_sprint
    mkdir -p $RBUS_INSTALL_DIR
    cd $RBUS_ROOT

#### Build rbus and dependencies 

    git clone https://github.com/rdkcentral/rbus
    cmake -Hrbus -Bbuild/rbus -DCMAKE_INSTALL_PREFIX=${RBUS_INSTALL_DIR}/usr -DBUILD_FOR_DESKTOP=ON -DCMAKE_BUILD_TYPE=Debug
    make -C build/rbus && make -C build/rbus install

## Run Rbus Apps

Setup 3 terminals:

    export RBUS_ROOT=${HOME}/rbus && \
    export RBUS_INSTALL_DIR=${RBUS_ROOT}/install && \
    export PATH=${RBUS_INSTALL_DIR}/usr/bin:${PATH} && \
    export LD_LIBRARY_PATH=${RBUS_INSTALL_DIR}/usr/lib:${LD_LIBRARY_PATH}

#### Start rtrouted

In terminal 1, run rtrouted.  This deamon must be running for rbus apps to communicate.

    rtrouted -f -l DEBUG

Note that if at any point in the future you want to restart rtrouted you can run this.

    killall -9 rtrouted; rm -fr /tmp/rtroute*; rtrouted -f -l DEBUG

#### Run a sample app

Note that all sample apps come with both a provider and a consumer app. 
The provider must be started first and then quickly, before the provider times out, the consumer should be started.

In terminal 2, run the sample provider.

    rbusSampleProvider

In terminal 3, run the sample consumer.

    rbusSampleConsumer

Here is the list of all samples:

1. rbusSampleProvider / rbusSampleConsumer
2. rbusEventProvider / rbusEventConsumer
3. rbusEventProvider / rbusEventConsumer
4. rbusGeneralEventProvider / rbusGeneralEventConsumer
5. rbusValueChangeProvider / rbusValueChangeConsumer
6. rbusMethodProvider / rbusMethodConsumer
7. rbusTableProvider / rbusTableConsumer

#### Playing with rbuscli

The rbuscli utility app allows the user to register a data model and interact with it, 
exercising the rbus api both from a provider and consumer perspective.

In terminal 2, run rbuscli

    rbuscli -i

Register a property as a provider would

        > reg prop A.B

In terminal 3, run rbuscli and set/get the property, registered by the first rbuscli, as a consumer would.

    rbuscli -i
        > set A.B string "hello"
        > get A.B

In terminal 3, enable event logging and subscribe to a value-change event.

        > log events
        > sub A.B

In terminal 2, change the value so that a value-change event is generated.

        > set A.B string "hello again"

In terminal 3, logs should appear showing a value change event was received.

There's a lot more you can do with rbuscli. 
Enter ***help*** to get a full list of commands. 
When your done, enter ***quit*** to exit rbuscli.

#### Run the test harness

In terminal 2, run the test provider

    rbusTestProvider

In terminal 3, run the test consumer

    rbusTestConsumer -a


The test takes about 5-10 minutes. Check for errors in the summary table at the end.
To get more details for any error, rerun the test passing ***-l debug*** to both the provider and consumer.

#### Run with valgrind

Valgrind is an important tool to help find both memory leaks and memory related bugs.

In terminal 2, run a sample provider using valgrind.

    valgrind --leak-check=full --show-leak-kinds=all rbusSampleProvider

In terminal 3, run a sample consumer using valgrind.

    valgrind --leak-check=full --show-leak-kinds=all rbusSampleConsumer
