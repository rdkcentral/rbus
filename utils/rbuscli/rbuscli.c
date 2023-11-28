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
#define _GNU_SOURCE 1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <rbus.h>
#include <rtList.h>
#include <rtHashMap.h>
#include <rtTime.h>
#include <rtMemory.h>
#include <linenoise.h>
#include <stdarg.h>
#include <rbuscore.h>
#include <signal.h>
#include <fcntl.h>
#if (__linux__ && __GLIBC__ && !__UCLIBC__) || __APPLE__
  #include <execinfo.h>
#endif

#define RBUS_CLI_COMPONENT_NAME "rbuscli"
#define RBUS_CLI_MAX_PARAM      25
#define RBUS_CLI_MAX_CMD_ARG    (RBUS_CLI_MAX_PARAM * 3)
#define BT_BUF_SIZE 512


static int runSteps = __LINE__;

bool g_isDebug = true;
#define RBUSCLI_LOG(...) if (g_isDebug == true) {printf(__VA_ARGS__);}
typedef struct _tlv_t {
    char *p_name;
    char *p_type;
    char *p_val;
} rbus_cli_tlv_t;

rbus_cli_tlv_t g_tlvParams[RBUS_CLI_MAX_PARAM];
rbusHandle_t   g_busHandle = 0;
rtList g_registeredProps = NULL;
rtList g_subsribeUserData = NULL;
rtList g_messageUserData = NULL;
rtHashMap gDirectHandlesHash = NULL;

bool g_isInteractive = false;
bool g_logEvents = true;
unsigned int g_curr_sessionId = 0;
rbusLogLevel_t g_logLevel = RBUS_LOG_WARN;

bool matchCmd(const char* sub, size_t lenmin,  const char* full)
{
    if(lenmin <= strlen(sub) && strlen(sub) <= strlen(full) && strncmp(sub, full, strlen(sub)) == 0)
        return true;
    else
        return false;
}

void show_menu(const char* command)
{
    runSteps = __LINE__;
    if(command)
    {
        if(matchCmd(command, 3, "getvalues"))
        {
            printf ("\e[1mget\e[0mvalues \e[4mpath\e[0m [\e[4mpath\e[0m \e[4m...\e[0m]\n\r");
            printf ("Gets the value(s) of one or more parameters.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sEither the name of a parameter or a partial path(ending with '.')\n\r", "path");
            printf ("Returns:\n\r");
            printf ("\tValue(s) for all parameter\n\r");
            printf ("Examples:\n\r");
            printf ("\tget Example.Prop1\n\r");
            printf ("\tget Example.Prop1 Example.Prop2\n\r");
            printf ("\tget Example.\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "setvalues"))
        {
            printf ("\e[1mset\e[0mvalues \e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m [[\e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m] \e[4m...\e[0m] [commit]\n\r");
            printf ("Sets the value(s) of one or more parameters.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of a parameter\n\r", "parameter");
            printf ("\t%-20sData type of the parameter. Supported data types: string, int, uint, boolean, datetime, \n\r", "type");
            printf ("\t%-20ssingle, double, bytes, char, byte, int8, uint8, int16, uint16, int32, uint32, int64, uint64\n\r", " ");
            printf ("\t%-20sThe new value.\n\r", "value");
            printf ("\t%-20sOptional commit flag that if set to true will commit all parameters as a session (default true)\n\r", "commit");
            printf ("Examples:\n\r");
            printf ("\tset Example.Prop1 string \"Hello World\"\n\r");
            printf ("\tset Example.Prop2 int 10\n\r");
            printf ("\tset Example.Prop1 string \"Hello World\" Example.Prop2 int 10\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "addrow"))
        {
            printf ("\e[1madd\e[0mrow \e[4mtable\e[0m [alias]\n\r");
            printf ("Add a new row to a table.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the table\n\r", "table");
            printf ("\t%-20sOptional alias name for new row\n\r", "alias");
            printf ("Examples:\n\r");
            printf ("\tadd Example.SomeTable.\n\r");
            printf ("\tadd Example.SomeTable. MyAlias\n\r");
            printf ("\n\r");

        }
        else if(matchCmd(command, 3, "delrow"))
        {
            printf ("\e[1mdel\e[0mrow \e[4mrow\e[0m\n\r");
            printf ("Remove a row from a table.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the row to remove\n\r", "row");
            printf ("Examples:\n\r");
            printf ("\tdel Example.SomeTable.1\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 4, "getrows"))
        {
            printf ("\e[1mgetr\e[0mows \e[4mpath\e[0m\n\r");
            printf ("Get the names of the rows in a table\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of a table\n\r", "path");
            printf ("Returns:\n\r");
            printf ("\tNames of all rows\n\r");
            printf ("Examples:\n\r");
            printf ("\tgetr Example.Table1.\n\r");
            printf ("\n\r");
        }        
        else if(matchCmd(command, 4, "getnames"))
        {
            printf ("\e[1mgetn\e[0mames \e[4mpath\e[0m  [nextLevel]\n\r");
            printf ("Gets the names of the elements inside an object or table.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of a table or object\n\r", "path");
            printf ("\t%-20sOptional flag to get only the next level if true or get all names if false (default true)\n\r", "nextLevel");
            printf ("Returns:\n\r");
            printf ("\tNames of all children\n\r");
            printf ("Examples:\n\r");
            printf ("\tgetn Example.Object1.\n\r");
            printf ("\tgetn Example.Table1.\n\r");
            printf ("\n\r");
        }        
        else if(matchCmd(command, 5, "discallcomponents"))
        {
            printf ("\e[1mdisca\e[0mllcomponents\n\r");
            printf ("Get a list of all components registered with the bus.\n\r");
            printf ("Returns:\n\r");
            printf ("\tList of components\n\r");
            printf ("Examples:\n\r");
            printf ("\tdisca\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "disccomponents"))
        {
            printf ("\e[1mdiscc\e[0momponents \e[4melement\e[0m [\e[4melement\e[0m \e[4m...\e[0m]\n\r");
            printf ("Get a list of components that provide a set of data elements name(s).\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sPath of a single element (an element can be a parameter, table, event, or method)\n\r", "element");
            printf ("Returns:\n\r");
            printf ("\tList of component, one per element\n\r");
            printf ("Examples:\n\r");
            printf ("\tdiscc Example.Prop1\n\r");
            printf ("\tdiscc Example.Prop1 Another.Prop2\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "discelements"))
        {
            printf ("\e[1mdisce\e[0mlements \e[4mcomponent\e[0m\n\r");
            printf ("Get a list of all data elements provided by a component.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sPath of a single element (an element can be a parameter, table, event, or method)\n\r", "element");
            printf ("Returns:\n\r");
            printf ("\tValue for each parameter\n\r");
            printf ("Examples:\n\r");
            printf ("\tdisce ComponentA\n\r");
            printf ("\tdisce ComponentA ComponentB\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "discwildcarddests"))
        {
            printf ("\e[1mdiscw\e[0mildcarddests\n\r");
            printf ("Get the list of components handling a wildcard/partial path query.\n\r");
            printf ("Returns:\n\r");
            printf ("\tList of components\n\r");
            printf ("Examples:\n\r");
            printf ("\tdiscw Device.DeviceInfo.\n\r");
            printf ("\n\r");
        }        
        else if(matchCmd(command, 3, "register"))
        {
            printf ("\e[1mreg\e[0mister \e[4mtype\e[0m \e[4mname\e[0m\n\r"); //(property,event,table)
            printf ("Register a new element with the bus.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe type of the new element. Supported types: prop(e.g. property), table, event, method\n\r", "type");
            printf ("\t%-20sA system-wide unique name for the new element\n\r", "name");
            printf ("\t%-20sIf type is table, the name must end in \".{i}.\"\n\r", " ");
            printf ("\t%-20sIf type is event, the name must end in \"!\"\n\r", " ");
            printf ("\t%-20sIf type is method, the name must end in \"()\"\n\r", " ");
            printf ("Examples:\n\r");
            printf ("\treg prop Example.NewProp\n\r");
            printf ("\treg table Example.NewTable.{i}.\n\r");
            printf ("\treg event Example.NewEvent!\n\r");
            printf ("\treg method Example.NewMethod()\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "unregister"))
        {
            printf ("\e[1munreg\e[0mister \e[4mname\e[0m\n\r");
            printf ("Unregister a previously registered element from the bus.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the element to unregister\n\r", "name");
            printf ("Examples:\n\r");
            printf ("\tunreg Example.NewProp\n\r");
            printf ("\tunreg Example.NewTable.{i}.\n\r");
            printf ("\tunreg Example.NewEvent!\n\r");
            printf ("\tunreg Example.NewMethod()\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "subscribe"))
        {
            printf ("\e[1msub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m \e[4minitialValue\e[0m]\n\r");
            printf ("Subscribe to a single event.\n\r");
            printf ("Rbus supports general events, value-change events, and table events.\n\r");
            printf ("And the type depends on that type of element \e[4mevent\e[0m refers to.\n\r");
            printf ("If the type is a parameter then it is value-change event.\n\r");
            printf ("If the type is a table then it is table events.\n\r");
            printf ("If the type is a event then it is a general event.\n\r");
            printf ("For value-change, an optional filter can be applied using the \e[4moperator\e[0m \e[4mvalue\e[0m parameters.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to subscribe to\n\r", "event");
            printf ("\t%-20sOptional filter relational operator. Supported operators (>, >=, <, <=, =, !=)\n\r", "operator");
            printf ("\t%-20sOptional filter trigger value\n\r", "value");
            printf ("\t%-20sTo get initial value of the event being subscribed\n\r", "initialValue");
            printf ("Examples:\n\r");
            printf ("\tsub Example.SomeEvent!\n\r");
            printf ("\tsub Example.SomeTable.\n\r");
            printf ("\tsub Example.SomeIntProp > 10\n\r");
            printf ("\tsub Example.SomeStrProp = \"Hello\"\n\r");
            printf ("\tsub Example.SomeEvent! true\n\r");
            printf ("\tsub Example.SomeEvent! = \"data\" true\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 4, "subinterval"))
        {
            printf ("\e[1msubi\e[0mnterval \e[4mevent\e[0m \e[4minterval\e[0m [\e[4mduration\e[0m] \e[4minitialValue\e[0m]\n\r");
            printf ("Subscribe to an event with interval\n\r");
            printf ("For interval, can be applied using the \e[4minterval\e[0m parameter.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to subscribe to\n\r", "event");
            printf ("\t%-20sThe interval trigger value\n\r", "interval");
            printf ("\t%-20sOptional duration trigger value\n\r", "duration");
            printf ("\t%-20sOptional to get initial value of the event being subscribed\n\r", "initialValue");
            printf ("Example:\n\r");
            printf ("\tsubint Example.SomeIntProp 10\n\r");
            printf ("\tsubint Example.SomeIntProp 10 60\n\r");
            printf ("\tsubint Example.SomeIntProp 5 true\n\r");
            printf ("\tsubint Example.SomeIntProp 5 20 true\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "unsubscribe"))
        {
            printf ("\e[1munsub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m]\n\r");
            printf ("Unsubscribe from a single event.\n\r");
            printf ("If a value-change filter was used to subscribe then the same filter must be passed to unsubscribe.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to unsubscribe from\n\r", "event");
            printf ("\t%-20sOptional operator that was used when subscribing to this event. Supported operators (>, >=, <, <=, =, !=)\n\r", "operator");
            printf ("\t%-20sOptional value that was used when subscribing to this event.\n\r", "value");
            printf ("Examples:\n\r");
            printf ("\tunsub Example.SomeEvent!\n\r");
            printf ("\tunsub Example.SomeTable.\n\r");
            printf ("\tunsub Example.SomeIntProp > 10\n\r");
            printf ("\tunsub Example.SomeStrProp = \"Hello\"\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 11, "rawdataunsubscribe"))
        {
            printf ("\e[1mrawdataunsub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m]\n\r");
            printf ("Unsubscribe from a single event.\n\r");
            printf ("If a value-change filter was used to subscribe then the same filter must be passed to rawdataunsubscribe.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to rawdataunsubscribe from\n\r", "event");
            printf ("\t%-20sOptional operator that was used when subscribing to this event. Supported operators (>, >=, <, <=, =, !=)\n\r", "operator");
            printf ("\t%-20sOptional value that was used when subscribing to this event.\n\r", "value");
            printf ("Examples:\n\r");
            printf ("\trawdataunsub Example.SomeEvent!\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 6, "unsubinterval"))
        {
            printf ("\e[1munsubi\e[0mnterval \e[4mevent\e[0m \e[4minterval\e[0m [\e[4mduration\e[0m]\n\r");
            printf ("Unsubscribe from a single event\n\r");
            printf ("If interval, duration are used to subscribe then the same interval, duration must be passed to unsubscribe.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to unsubscribe from\n\r", "event");
            printf ("\t%-20sThe interval that was used when subscribing to this event.\n\r", "interval");
            printf ("\t%-20sOptional duration that was used when subscribing to this event.\n\r", "duration");
            printf ("Example:\n\r");
            printf ("\tunsubint Example.SomeIntProp 10\n\r");
            printf ("\tunsubint Example.SomeIntProp 10 60\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 4, "asubscribe"))
        {
            printf ("\e[1masub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m]\n\r");
            printf ("Asynchronously subscribe to a single event.\n\r");
            printf ("Rbus supports general events, value-change events, and table events.\n\r");
            printf ("And the type depends on that type of element \e[4mevent\e[0m refers to.\n\r");
            printf ("If the type is a parameter then it is value-change event.\n\r");
            printf ("If the type is a table then it is table events.\n\r");
            printf ("If the type is a event then it is a general event.\n\r");
            printf ("For value-change, an optional filter can be applied using the \e[4moperator\e[0m \e[4mvalue\e[0m parameters.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to subscribe to\n\r", "event");
            printf ("\t%-20sOptional filter relational operator. Supported operators (>, >=, <, <=, =, !=)\n\r", "operator");
            printf ("\t%-20sOptional filter trigger value\n\r", "value");
            printf ("Examples:\n\r");
            printf ("\tasub Example.SomeEvent!\n\r");
            printf ("\tasub Example.SomeTable.\n\r");
            printf ("\tasub Example.SomeIntProp > 10\n\r");
            printf ("\tasub Example.SomeStrProp = \"Hello\"\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "rawdatasubscribe"))
        {
            printf ("\e[1mrawdatasub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m \e[4minitialValue\e[0m]\n\r");
            printf ("Subscribe to a single event.\n\r");
            printf ("Rbus supports general events, value-change events, and table events.\n\r");
            printf ("And the type depends on that type of element \e[4mevent\e[0m refers to.\n\r");
            printf ("If the type is a parameter then it is value-change event.\n\r");
            printf ("If the type is a table then it is table events.\n\r");
            printf ("If the type is a event then it is a general event.\n\r");
            printf ("For value-change, an optional filter can be applied using the \e[4moperator\e[0m \e[4mvalue\e[0m parameters.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to subscribe to\n\r", "event");
            printf ("\t%-20sOptional filter relational operator. Supported operators (>, >=, <, <=, =, !=)\n\r", "operator");
            printf ("\t%-20sOptional filter trigger value\n\r", "value");
            printf ("\t%-20sTo get initial value of the event being subscribed\n\r", "initialValue");
            printf ("Examples:\n\r");
            printf ("\tsub Example.SomeEvent!\n\r");
            printf ("\tsub Example.SomeTable.\n\r");
            printf ("\tsub Example.SomeIntProp > 10\n\r");
            printf ("\tsub Example.SomeStrProp = \"Hello\"\n\r");
            printf ("\tsub Example.SomeEvent! true\n\r");
            printf ("\tsub Example.SomeEvent! = \"data\" true\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "publish"))
        {
            printf ("\e[1mpub\e[0mlish \e[4mevent\e[0m [\e[4mdata\e[0m]\n\r");
            printf ("Publishes an event which will be sent to all subscribers of this event.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to publish\n\r", "event");
            printf ("\t%-20sThe data to publish with the event (as a string)\n\r", "data");
            printf ("Examples:\n\r");
            printf ("\tpub Example.MyEvent! \"Hello World\"\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "rawdatapublish"))
        {
            printf ("\e[1mpub\e[0mlish \e[4mevent\e[0m [\e[4mdata\e[0m]\n\r");
            printf ("Publishes an event which will be sent to all subscribers of this event.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of the event to publish\n\r", "event");
            printf ("\t%-20sThe data to publish with the event (as a string)\n\r", "data");
            printf ("Examples:\n\r");
            printf ("\tpub Example.MyEvent! \"Hello World\"\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 9, "method_noargs"))
        {
            printf ("\e[1mmethod_no\e[0margs \e[4mmethodname\e[0m\n\r");
            printf ("Used when a method does not require any input arguement.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sName of a method\n\r", "methodname");
            printf ("Examples:\n\r");
            printf ("\tIsLanEnabled()\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 9, "method_names"))
        {
            printf ("\e[1mmethod_na\e[0mmes \e[4mmethodname\e[0m \e[4mparameter\e[0m [[\e[4mparameter\e[0m] \e[4m...\e[0m]\n\r");
            printf ("Uses the parameter name as an input to the specified method.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sName of a method\n\r", "methodname");
            printf ("\t%-20sName of a parameter\n\r", "parameter");
            printf ("Examples:\n\r");
            printf ("\tGetPSMRecordValue() Deveice.Test.Psm\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 9, "method_values"))
        {
            printf ("\e[1mmethod_va\e[0mlues \e[4mmethodname\e[0m \e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m [[\e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m] \e[4m...\e[0m] [commit]\n\r");
            printf ("Uses the type and value of the parameter name as an input to the specified method.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sName of a method\n\r", "methodname");
            printf ("\t%-20sName of a parameter\n\r", "parameter");
            printf ("\t%-20sType of the parameter\n\r", "type");
            printf ("\t%-20sValue to be stored in the parameter\n\r", "value");
            printf ("Examples:\n\r");
            printf ("\tSetPSMRecordValue() Deveice.Test.Psm string test_value\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 10, "create_session"))
        {
            printf ("\e[1mcreate_ses\e[0sion\n\r");
            printf ("Create a session.\n\r");
            printf ("Examples:\n\r");
            printf ("\tcreate_session\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 7, "get_session"))
        {
            printf ("\e[1mget_ses\e[0sion\n\r");
            printf ("Get the current session Id value.\n\r");
            printf ("\tget_session\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 9, "close_session"))
        {
            printf ("\e[1mclose_ses\e[0sion\n\r");
            printf ("Close the current session.\n\r");
            printf ("\tclose_session\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 3, "log"))
        {
            printf ("\t\e[1mlog\e[0m \e[4mlevel\e[0m\n\r");
            printf ("Sets the logging level.\n\r");
            printf ("You both adjust the rbus log level and enable additional client logging of event data for any subscription made by the program.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe level of logging to enable. Supported levels: debug, info, warn, error, fatal, event\n\r", "level");
            printf ("Examples:\n\r");
            printf ("\tToggle event logging:\n\r");
            printf ("\t\tlog events\n\r");
            printf ("\tEnable full rbus logging (minus events):\n\r");
            printf ("\t\tlog debug\n\r");
            printf ("\tReturn logging to default/quiet:\n\r");
            printf ("\t\tlog fatal\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "opendirect"))
        {
            printf ("\e[1mopend\e[0mirect \e[4mpath\e[0m\n\r");
            printf ("Opens direct connection to the DML.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of a parameter \n\r", "path");
            printf ("Examples:\n\r");
            printf ("\topendirect Example.Prop1\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 5, "closedirect"))
        {
            printf ("\e[1mclosed\e[0mirect \e[4mpath\e[0m\n\r");
            printf ("closes direct connection to the DML.\n\r");
            printf ("Args:\n\r");
            printf ("\t%-20sThe name of a parameter \n\r", "path");
            printf ("Examples:\n\r");
            printf ("\tclosedirect Example.Prop1\n\r");
            printf ("\n\r");
        }
        else if(matchCmd(command, 4, "quit"))
        {
            printf ("\t\e[1mquit\e[0m\n\r");
            printf ("exist the program\n\r");
        }
        else
        {
            printf ("Invalid command: %s\n\r", command);
        }
    }
    else
    {
        if(!g_isInteractive)
        {
            printf ("\nrbuscli\n\r");
            printf ("\nThis utility allows you to interact with the rbus system on this device.\n\r");
            printf ("\nUsage:\n\r");
            printf ("\trbuscli [command]\n\r");
            printf ("\trbuscli [-i]\n\r");
            printf ("\trbuscli [-g \e[0m \e[4mparameter\e\e[0m \e[0m]\n\r");
            printf ("\trbuscli [-s \e[0m \e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m \e[0m]\n\r");
            printf ("\n\r");
            printf ("Options:\n\r");
            printf ("\t\e[1m-i\e[0m Run as interactive shell.\n\r");
            printf ("\t\e[1m-g\e[0m Get the value of parameter\n\r");
            printf ("\t\e[1m-s\e[0m Set the value of parameter\n\r");
            printf ("\n\r");
            printf ("Commands:\n\r");
        }
        printf ("\t\e[1mopen\e[0mdirect \e[4mpath\e[0m\n\r");
        printf ("\t\e[1mclose\e[0mdirect \e[4mpath\e[0m\n\r");
        printf ("\t\e[1mget\e[0mvalues \e[4mpath\e[0m [\e[4mpath\e[0m \e[4m...\e[0m]\n\r");
        printf ("\t\e[1mset\e[0mvalues \e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m [[\e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m] \e[4m...\e[0m] [commit]\n\r");
        printf ("\t\e[1madd\e[0mrow \e[4mtable\e[0m [alias]\n\r");
        printf ("\t\e[1mdel\e[0mrow \e[4mrow\e[0m\n\r");
        printf ("\t\e[1mgetr\e[0mows \e[4mpath\e[0m\n\r");
        printf ("\t\e[1mgetn\e[0mames \e[4mpath\e[0m [nextLevel]\n\r");
        printf ("\t\e[1mdiscc\e[0momponents \e[4melement\e[0m [\e[4melement\e[0m \e[4m...\e[0m]\n\r");
        printf ("\t\e[1mdisca\e[0mllcomponents\n\r");
        printf ("\t\e[1mmethod_no\e[0margs \e[4mmethodname\e[0m\n\r");
        printf ("\t\e[1mmethod_na\e[0mmes \e[4mmethodname\e[0m \e[4mparameter\e[0m [[\e[4mparameter\e[0m] \e[4m...\e[0m]\n\r");
        printf ("\t\e[1mmethod_va\e[0mlues \e[4mmethodname\e[0m \e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m [[\e[4mparameter\e[0m \e[4mtype\e[0m \e[4mvalue\e[0m] \e[4m...\e[0m] [commit]\n\r");
        printf ("\t\e[1mdisce\e[0mlements \e[4mcomponent\e[0m\n\r");
        printf ("\t\e[1mdisce\e[0mlements \e[4mpartial-path\e[0m immediate/all\n\r");
        printf ("\t\e[1mdiscw\e[0mildcarddests\n\r");
        if(!g_isInteractive)    
        {
            printf ("\t\e[1mhelp\e[0m [\e[4mcommand\e[0m]\n\r");
            printf ("Additional interactive shell commands:\n\r");
        }
        printf ("\t\e[1mreg\e[0mister \e[4mtype\e[0m \e[4mname\e[0m\n\r");
        printf ("\t\e[1munreg\e[0mister \e[4mname\e[0m\n\r");
        printf ("\t\e[1msub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m] [\e[4minitialValue\e[0m]\n\r");
        printf ("\t\e[1msubi\e[0mnterval \e[4mevent\e[0m \e[4minterval\e[0m [\e[4mduration\e[0m] [\e[4minitialValue\e[0m]\n\r");
        printf ("\t\e[1mrawdatasub\e[0mscribe \e[4mevent\e[0m]\n\r");
        printf ("\t\e[1munsub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m]\n\r");
        printf ("\t\e[1mrawdataunsub\e[0mscribe \e[4mevent\e[0m]\n\r");
        printf ("\t\e[1munsubi\e[0mnterval \e[4mevent\e[0m \e[4minterval\e[0m [\e[4mduration\e[0m] [\e[4minitialValue\e[0m]\n\r");
        printf ("\t\e[1masub\e[0mscribe \e[4mevent\e[0m [\e[4moperator\e[0m \e[4mvalue\e[0m]\n\r");
        printf ("\t\e[1mpub\e[0mlish \e[4mevent\e[0m [\e[4mdata\e[0m]\n\r");
        printf ("\t\e[1mrawdatapub\e[0mlish \e[4mevent\e[0m [\e[4mdata\e[0m]\n\r");
        printf ("\t\e[1mcreate_ses\e[0msion\n\r");
        printf ("\t\e[1mget_ses\e[0msion\n\r");
        printf ("\t\e[1mclose_ses\e[0msion\n\r");
        printf ("\t\e[1mquit\e[0m\n\r");
        if(g_isInteractive)    
        {
            printf ("\t\e[1mhelp\e[0m [\e[4mcommand\e[0m]\n\r");
        }
        printf ("\n\r");
    }
}

void rbus_log_handler(
    rbusLogLevel level,
    const char* file,
    int line,
    int threadId,
    char* message)
{
    rtTime_t tm;
    char tbuff[50];
    rtTime_Now(&tm);
    const char* slevel = "";
    runSteps = __LINE__;

    if(level < g_logLevel)
        return;

    switch(level)
    {
    case RBUS_LOG_DEBUG:    slevel = "DEBUG";   break;
    case RBUS_LOG_INFO:     slevel = "INFO";    break;
    case RBUS_LOG_WARN:     slevel = "WARN";    break;
    case RBUS_LOG_ERROR:    slevel = "ERROR";   break;
    case RBUS_LOG_FATAL:    slevel = "FATAL";   break;
    }
    printf("%s %5s %s:%d -- Thread-%d: %s \n\r", rtTime_ToString(&tm, tbuff), slevel, file, line, threadId, message);
}

rbusValueType_t getDataType_fromString(const char* pType)
{
    runSteps = __LINE__;
    rbusValueType_t rc = RBUS_NONE;

    if (strncasecmp ("boolean",    pType, 4) == 0)
        rc = RBUS_BOOLEAN;
    else if (strncasecmp("char",   pType, 4) == 0)
        rc = RBUS_CHAR;
    else if (strncasecmp("bytes",    pType, 5) == 0)
        rc = RBUS_BYTES;
    else if (strncasecmp("byte",   pType, 4) == 0)
        rc = RBUS_BYTE;
    else if (strncasecmp("int8",   pType, 4) == 0)
        rc = RBUS_INT8;
    else if (strncasecmp("uint8",   pType, 5) == 0)
        rc = RBUS_UINT8;
    else if (strncasecmp("int16",  pType, 5) == 0)
        rc = RBUS_INT16;
    else if (strncasecmp("uint16", pType, 6) == 0)
        rc = RBUS_UINT16;
    else if (strncasecmp("int32",  pType, 5) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint32", pType, 6) == 0)
        rc = RBUS_UINT32;
    else if (strncasecmp("int64",  pType, 5) == 0)
        rc = RBUS_INT64;
    else if (strncasecmp("uint64", pType, 6) == 0)
        rc = RBUS_UINT64;
    else if (strncasecmp("single",  pType, 5) == 0)
        rc = RBUS_SINGLE;
    else if (strncasecmp("double", pType, 6) == 0)
        rc = RBUS_DOUBLE;
    else if (strncasecmp("datetime", pType, 4) == 0)
        rc = RBUS_DATETIME;
    else if (strncasecmp("string", pType, 6) == 0)
        rc = RBUS_STRING;

    /* Risk handling, if the user types just int, lets consider int32; same for unsigned too  */
    else if (strncasecmp("int",  pType, 3) == 0)
        rc = RBUS_INT32;
    else if (strncasecmp("uint",  pType, 4) == 0)
        rc = RBUS_UINT32;

    return rc;
}

char *getDataType_toString(rbusValueType_t type)
{
    runSteps = __LINE__;
    char *pTextData = "None";
    switch(type)
    {
    case RBUS_BOOLEAN:
        pTextData = "boolean";
        break;
    case RBUS_CHAR :
        pTextData = "char";
        break;
    case RBUS_BYTE:
        pTextData = "byte";
        break;
    case RBUS_INT8:
        pTextData = "int8";
        break;
    case RBUS_UINT8:
        pTextData = "uint8";
        break;
    case RBUS_INT16:
        pTextData = "int16";
        break;
    case RBUS_UINT16:
        pTextData = "uint16";
        break;
    case RBUS_INT32:
        pTextData = "int32";
        break;
    case RBUS_UINT32:
        pTextData = "uint32";
        break;
    case RBUS_INT64:
        pTextData = "int64";
        break;
    case RBUS_UINT64:
        pTextData = "uint64";
        break;
    case RBUS_STRING:
        pTextData = "string";
        break;
    case RBUS_DATETIME:
        pTextData = "datetime";
        break;
    case RBUS_BYTES:
        pTextData = "bytes";
        break;
    case RBUS_SINGLE:
        pTextData = "single";
        break;
    case RBUS_DOUBLE:
        pTextData = "double";
        break;
    case RBUS_PROPERTY:
    case RBUS_OBJECT:
    case RBUS_NONE:
        pTextData = "unknown";
        break;
    }
    return pTextData ;
}

void free_registered_property(void* p)
{
    runSteps = __LINE__;
    rbusProperty_t prop = (rbusProperty_t)p;
    rbusProperty_Release(prop);
    runSteps = __LINE__;
}

void free_userdata(void* p)
{
    runSteps = __LINE__;
    rt_free(p);
    runSteps = __LINE__;
}

rbusProperty_t get_registered_property(const char* name)
{
    rtListItem li;
    rtList_GetFront(g_registeredProps, &li);
    runSteps = __LINE__;
    while(li)
    {
        rbusProperty_t prop;
        rtListItem_GetData(li, (void**)&prop);
        if(strcmp(rbusProperty_GetName(prop), name) == 0)
            return prop;
        rtListItem_GetNext(li, &li);
    }
    return NULL;
}

rbusProperty_t remove_registered_property(const char* name)
{
    rtListItem li;
    rtList_GetFront(g_registeredProps, &li);
    runSteps = __LINE__;
    while(li)
    {
        rbusProperty_t prop;
        rtListItem_GetData(li, (void**)&prop);
        if(strcmp(rbusProperty_GetName(prop), name) == 0)
            rtList_RemoveItem(g_registeredProps, li, free_registered_property);
        rtListItem_GetNext(li, &li);
    }
    return NULL;
}

rbusError_t property_get_handler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    const char* name  = rbusProperty_GetName(property);
    rbusProperty_t registeredProp = get_registered_property(name);
    runSteps = __LINE__;
    if(!registeredProp)
    {
        if(g_logEvents)
        {
            printf("Get handler called for invalid property %s\n\r", name);
        }
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusProperty_SetValue(property, rbusProperty_GetValue(registeredProp));
#if 0
    if(g_logEvents)
    {
        char* buff;
        printf("Get handler called for %s, returning value: %s\n\r", 
            name, buff = rbusValue_ToString(rbusProperty_GetValue(property), NULL, 0));
        free(buff);
    }
#endif
    return RBUS_ERROR_SUCCESS;
}

rbusError_t property_set_handler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    char* buff;

    const char* name  = rbusProperty_GetName(prop);

    runSteps = __LINE__;
    rbusProperty_t registeredProp = get_registered_property(name);
    if(!registeredProp)
    {
        if(g_logEvents)
        {
            printf("Get handler called for invalid property %s\n\r", name);
        }
        return RBUS_ERROR_INVALID_INPUT;
    }

    rbusProperty_SetValue(registeredProp, rbusProperty_GetValue(prop));

    if(g_logEvents)
    {
        printf("Set handler called for %s, new value: %s\n\r", 
            name, buff = rbusValue_ToString(rbusProperty_GetValue(registeredProp), NULL, 0));
        free(buff);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t table_add_row_handler(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    (void)handle;
    (void)aliasName;
    (void)instNum;
    static int instanceCount = 1;
    *instNum = instanceCount++;
    runSteps = __LINE__;
    if(g_logEvents)
    {
        printf("Table add row handler called for %s\n\r", tableName);
    }
    return RBUS_ERROR_SUCCESS;
}

rbusError_t table_remove_row_handler(rbusHandle_t handle, char const* rowName)
{
    (void)handle;
    if(g_logEvents)
    {
        printf("Table remove row handler called for %s\n\r", rowName);
    }
    return RBUS_ERROR_SUCCESS;
}

static rbusError_t method_invoke_handler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)inParams;
    (void)outParams;
    (void)asyncHandle;
    runSteps = __LINE__;
    if(g_logEvents)
    {
        printf("Method handler called for %s\n\r", methodName);
    }
    rbusObject_fwrite(inParams, 1, stdout);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t event_subscribe_handler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)autoPublish;
    (void)interval;
    runSteps = __LINE__;
    if(g_logEvents)
    {
        printf("Subscribe handler called for %s, action: %s\n\r", eventName, 
            action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe");
    }
    return RBUS_ERROR_SUCCESS;
}

static void event_receive_handler1(rbusHandle_t handle, rbusEventRawData_t const* event, rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;
    runSteps = __LINE__;
    printf("\nevent_receive_handler1 called\n\r");
    printf("Event received %s\n\r", event->name);
    printf("Event data: %s\n\r", (char*)event->rawData);
    printf("Event data len: %d\n\r", event->rawDataLen);
    printf("\n\r");
}

void event_receive_handler2(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;
    runSteps = __LINE__;
    if(g_logEvents)
    {
        const char* stype = "";
        switch(event->type)
        {
            case RBUS_EVENT_OBJECT_CREATED: stype = "RBUS_EVENT_OBJECT_CREATED";    break;
            case RBUS_EVENT_OBJECT_DELETED: stype = "RBUS_EVENT_OBJECT_DELETED";    break;
            case RBUS_EVENT_VALUE_CHANGED:  stype = "RBUS_EVENT_VALUE_CHANGED";     break;
            case RBUS_EVENT_GENERAL:        stype = "RBUS_EVENT_GENERAL";           break;
            case RBUS_EVENT_INITIAL_VALUE:  stype = "RBUS_EVENT_INITIAL_VALUE";     break;
            case RBUS_EVENT_INTERVAL:       stype = "RBUS_EVENT_INTERVAL";          break;
            case RBUS_EVENT_DURATION_COMPLETE: stype = "RBUS_EVENT_DURATION_COMPLETE"; break;
        }

        printf("Event received %s of type %s\n\r", event->name, stype);
        printf("Event data:\n\r");
        rbusObject_fwrite(event->data, 2, stdout); 
        printf("\n\r");
        if (subscription->userData)
            printf("User data: %s\n\r", (const char*)subscription->userData);
    }
}

void event_receive_subscription_handler(rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error)
{
  (void)handle;
  if (subscription)
      printf ("event name (%s) subscribe %s\n", subscription->eventName, error == RBUS_ERROR_SUCCESS ? "success" : "failed");
}

static bool verify_rbus_open()
{
    if(!g_busHandle)
    {
        rbusError_t rc;
        char compName[50] = "";
        snprintf(compName, 50, "%s-%d", RBUS_CLI_COMPONENT_NAME, getpid());
        runSteps = __LINE__;
        rc = rbus_open(&g_busHandle, compName);
        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf("rbus_open failed err: %d\n\r", rc);
            return false;
        }
    }
    return true;
}
void execute_discover_registered_components_cmd(int argc, char* argv[])
{
    rbusCoreError_t rc = RBUSCORE_SUCCESS;
    int componentCnt = 0;
    char **pComponentNames;

    (void)argc;
    (void)argv;

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    rc = rbus_discoverRegisteredComponents(&componentCnt, &pComponentNames);
    if(RBUSCORE_SUCCESS == rc)
    {
        int i;
        printf ("Discovered registered components..\n\r");
        for (i = 0; i < componentCnt; i++)
        {
            printf ("\tComponent %d: %s\n\r", (i + 1), pComponentNames[i]);
            free(pComponentNames[i]);
        }
        free(pComponentNames);
    }
    else
    {
        printf ("Failed to discover components. Error Code = %d\n\r", rc);
    }
}

void execute_discover_component_cmd(int argc, char* argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int elementCnt = argc - 2;
    int index = 0;
    int i;
    int componentCnt = 0;
    char **pComponentNames;
    char const* pElementNames[RBUS_CLI_MAX_PARAM] = {0, 0};

    if (!verify_rbus_open())
        return;

    for (index = 0, i = 2; index < elementCnt; index++, i++)
    {
        pElementNames[index] = argv[i];
    }

    runSteps = __LINE__;
    rc = rbus_discoverComponentName (g_busHandle, elementCnt, pElementNames, &componentCnt, &pComponentNames);
    if(RBUS_ERROR_SUCCESS == rc)
    {
        printf ("Discovered components for the given elements.\n\r");
        if(componentCnt)
        {
            for (i = 0; i < componentCnt; i++)
            {
                printf ("\tComponent %d: %s\n\r", (i + 1), pComponentNames[i]);
                free(pComponentNames[i]);
            }
            free(pComponentNames);
        }
        else
        {
            printf ("\tNone\n\r");
        }
    }
    else
    {
        printf ("Failed to discover components. Error Code = %d\n\r", rc);
    }
}

void execute_discover_elements_cmd(int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    bool nextLevel = true;
    int numElements = 0;
    char** pElementNames; // FIXME: every component will have more than RBUS_CLI_MAX_PARAM elements right?

    if (!verify_rbus_open())
        return;

    if (2 == numOfInputParams)
    {
        if (0 == strncmp (argv[3], "all", 3))
        {
            nextLevel = false;
        }
    }

    runSteps = __LINE__;
    rc = rbus_discoverComponentDataElements (g_busHandle, argv[2], nextLevel, &numElements, &pElementNames);
    if(RBUS_ERROR_SUCCESS == rc)
    {
        if(numElements)
        {
            int i;
            printf ("Discovered elements are:\n\r");
            for (i = 0; i < numElements; i++)
            {
                printf ("\tElement %d: %s\n\r", (i + 1), pElementNames[i]);
                free(pElementNames[i]);
            }
            free(pElementNames);
        }
        else
        {
            printf("No elements discovered!\n\r");
        }
    }
    else
    {
        printf ("Failed to discover elements. Error Code = %d\n\r", rc);
    }
}

void execute_discover_wildcard_dests_cmd(int argc, char* argv[])
{
    rbusCoreError_t rc = RBUSCORE_SUCCESS;
    int numDestinations = 0;
    char** destinations;

    (void)argc;
    (void)argv;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    rc = rbus_discoverWildcardDestinations(argv[2], &numDestinations, &destinations);
    
    if(RBUSCORE_SUCCESS == rc)
    {
        if (numDestinations)
        {
            int i;
            for(i = 0; i < numDestinations; i++)
            {
                printf ("\tComponent %d: %s\n\r", (i + 1), destinations[i]);
                free(destinations[i]);
            }
            free(destinations);
        }
        else
        {
            printf("No destinations discovered.\n\r");
        }
    }
    else
    {
        printf ("Failed to discover components. Error Code = %d\n\r", rc);
    }
}

void validate_and_execute_open_n_close_direct_cmd(int argc, char *argv[], bool isOpen)
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    const char *pInputParam[RBUS_CLI_MAX_PARAM] = {0, 0};

    if (numOfInputParams != 1)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    pInputParam[0] = argv[2];

    if(pInputParam[0][strlen(pInputParam[0])-1] == '.' || strchr(pInputParam[0], '*'))
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    rbusHandle_t directHandle = NULL;
    if (isOpen)
    {
        if(!gDirectHandlesHash)
            rtHashMap_Create(&gDirectHandlesHash);

        rc = rbus_openDirect(g_busHandle, &directHandle, pInputParam[0]);
        if (RBUS_ERROR_SUCCESS != rc)
        {
            printf ("Failed to open direct connection to %s\n\r", pInputParam[0]);
        }
        else
        {
            rtHashMap_Set(gDirectHandlesHash, pInputParam[0], directHandle);
        }
    }
    else
    {
        if(gDirectHandlesHash)
            directHandle = rtHashMap_Get(gDirectHandlesHash, pInputParam[0]);

        if (directHandle)
        {
            rc = rbus_closeDirect(directHandle);
            if (RBUS_ERROR_SUCCESS != rc)
            {
                printf ("Failed to close direct connection to %s\n\r", pInputParam[0]);
            }
            else
            {
                rtHashMap_Remove(gDirectHandlesHash, pInputParam[0]);
            }
        }
        else
        {
            printf ("Failed to find direct connection to %s\n\r", pInputParam[0]);
        }
    }
    return;
}

void validate_and_execute_get_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int numOfInputParams = argc - 2;
    int i = 0;
    int index = 0;
    int numOfOutVals = 0;
    rbusProperty_t outputVals = NULL;
    const char *pInputParam[RBUS_CLI_MAX_PARAM] = {0, 0};
    bool isWildCard = false;

    if ((numOfInputParams < 1) || ((strcmp("-g", argv[1]) == 0) && (numOfInputParams > 1)))
    {
        RBUSCLI_LOG ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    for (index = 0, i = 2; index < numOfInputParams; index++, i++)
        pInputParam[index] = argv[i];

    if(numOfInputParams == 1)
    {
        if(pInputParam[0][strlen(pInputParam[0])-1] == '.' || strchr(pInputParam[0], '*'))
	{
	    isWildCard = true;
	    if ((strcmp("-g", argv[1]) == 0))
	    {
		return;
	    }
	}
    }

    if ((!isWildCard) && (1 == numOfInputParams))
    {
        rbusValue_t getVal;
        rc = rbus_get(g_busHandle, pInputParam[0], &getVal);
        if(RBUS_ERROR_SUCCESS == rc)
        {
            numOfOutVals = 1;
            rbusProperty_Init(&outputVals, pInputParam[0], getVal);
            rbusValue_Release(getVal);
        }
    }
    else
    {
        rc = rbus_getExt(g_busHandle, numOfInputParams, pInputParam, &numOfOutVals, &outputVals);
    }

    if(RBUS_ERROR_SUCCESS == rc)
    {
        rbusProperty_t next = outputVals;
        for (i = 0; i < numOfOutVals; i++)
        {
            rbusValue_t val = rbusProperty_GetValue(next);
            rbusValueType_t type = rbusValue_GetType(val);
            char *pStrVal = rbusValue_ToString(val,NULL,0);

	    if ((strcmp("-g", argv[1]) == 0))
	    {
		printf ("%s\n", pStrVal);
	    }
	    else
	    {
		printf ("Parameter %2d:\n\r", i+1);
		printf ("              Name  : %s\n\r", rbusProperty_GetName(next));
		printf ("              Type  : %s\n\r", getDataType_toString(type));
		printf ("              Value : %s\n\r", pStrVal);
	    }
	    if(pStrVal)
	    {
		free(pStrVal);
	    }
	    next = rbusProperty_GetNext(next);
	}
        /* Free the memory */
        rbusProperty_Release(outputVals);
    }
    else
    {
        RBUSCLI_LOG ("Failed to get the data. Error : %d\n\r",rc);
    }
}

void validate_and_execute_addrow_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    char *pTablePathName = NULL;
    char *pTableAliasName = NULL;
    uint32_t instanceNum = 0;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    pTablePathName = argv[2];

    if (argc == 4)
        pTableAliasName = argv[3];

    rc = rbusTable_addRow(g_busHandle, pTablePathName, pTableAliasName, &instanceNum);

    if(RBUS_ERROR_SUCCESS == rc)
    {
        printf ("\n\n%s%d. added\n\n\r", pTablePathName, instanceNum);
    }
    else
    {
        printf ("Add row to a table failed with error code:%d..\n\r", rc);
    }
}

void validate_and_execute_delrow_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    char *pTablePathName = NULL;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    pTablePathName = argv[2];

    rc = rbusTable_removeRow(g_busHandle, pTablePathName);

    if(RBUS_ERROR_SUCCESS == rc)
    {
        printf ("\n\n%s deleted successfully\n\n\r", pTablePathName);
    }
    else
    {
        printf ("\n\nDeletion of a row from table failed with error code:%d..\n\r", rc);
    }
}

void validate_and_execute_set_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int i = argc - 2;
    bool isCommit = true;
    unsigned int sessionId = 0;

    runSteps = __LINE__;
    /* must have 3 or multiples of 3 parameters.. it could possibliy have 1 extra param which
     * could be having commit and set to true/false */
    if (((strcmp("-s", argv[1]) == 0 ) && (i > 3)) || (!((i >= 3) && ((i % 3 == 0) || (i % 3 == 1)))))
    {
        RBUSCLI_LOG ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    if ((i > 4) && !(strcmp("-s", argv[1]) == 0)) /* Multiple set commands; Lets use rbusValue_t */
    {
        int isInvalid = 0;
        int index = 0;
        int loopCnt = 0;
        int paramCnt = i/3;
        rbusProperty_t properties = NULL;
        rbusValue_t setVal[RBUS_CLI_MAX_PARAM];
        char const* setNames[RBUS_CLI_MAX_PARAM];

        for (index = 0, loopCnt = 0; index < paramCnt; loopCnt+=3, index++)
        {
            /* Create Param w/ Name */
            rbusValue_Init(&setVal[index]);
            setNames[index] = argv[loopCnt+2];

            printf ("Name = %s \n\r", argv[loopCnt+2]);

            /* Get Param Type */
            rbusValueType_t type = getDataType_fromString(argv[loopCnt+3]);

            if (type == RBUS_NONE)
            {
                printf ("Invalid data type. Please see the help\n\r");
                isInvalid = 1;
                break;
            }

            rbusValue_SetFromString(setVal[index], type, argv[loopCnt+4]);

            rbusProperty_t next;
            rbusProperty_Init(&next, setNames[index], setVal[index]);
            if(properties == NULL)
            {
                properties = next;
            }
            else
            {
                rbusProperty_Append(properties, next);
                rbusProperty_Release(next);
            }
        }

        if (0 == isInvalid)
        {
            /* Set Session ID & Commit value */
            if (g_isInteractive)
            {
                /* Do we have commit param? (n*3) + 1 */
                if (i % 3 == 1)
                {
                    /* Is commit */
                    if (strncasecmp ("true", argv[argc - 1], 4) == 0)
                        isCommit = true;
                    else if (strncasecmp ("false", argv[argc - 1], 5) == 0)
                    {
                        isCommit = false;
                        if(g_curr_sessionId == 0)
                        {
                            rc = rbus_createSession(g_busHandle, &g_curr_sessionId);
                            if(rc != RBUS_ERROR_SUCCESS)
                            {
                                printf("Session creation failed with err = %d\n", rc);
                            }
                        }
                        if(g_curr_sessionId == 0)
                            printf("Can't set the value temporarily with sessionId 0 and commit false\n");
                    }
                    else
                        isCommit = true;

                }
                else
                {
                    isCommit = true;
                }
                if(isCommit && g_curr_sessionId != 0)
                {
                    printf("closing the session\n");
                    fflush(stdout);
                    rc = rbus_closeSession(g_busHandle, g_curr_sessionId);
                    if(rc != RBUS_ERROR_SUCCESS)
                    {
                        printf("Session close failed with err = %d\n", rc);
                    }
                    g_curr_sessionId = 0;
                }
                sessionId = g_curr_sessionId;
            }
            else
            {
                /* For non-interactive mode, regardless of COMMIT value tha is passed, we assume the isCommit as TRUE */
                isCommit = true;
                sessionId = 0;
            }

            rbusSetOptions_t opts = {isCommit,sessionId};
            rc = rbus_setMulti(g_busHandle, paramCnt, properties/*setNames, setVal*/, &opts);
        }
        else
        {
            /* Since we allocated memory for `loopCnt` times, we must free them.. lets update the `paramCnt` to `loopCnt`; so that the below free function will take care */
            paramCnt = index;
            rc = RBUS_ERROR_INVALID_INPUT;
        }

        /* free the memory that was allocated */
        for (loopCnt = 0; loopCnt < paramCnt; loopCnt++)
        {
            rbusValue_Release(setVal[loopCnt]);
        }
        rbusProperty_Release(properties);
    }
    else /* Single Set Command */
    {
        rbusValue_t setVal;
        bool value = false;
        /* Get Param Type */
        rbusValueType_t type = getDataType_fromString(argv[3]);
        rbusValue_Init(&setVal);
        value = rbusValue_SetFromString(setVal, type, argv[4]);
        if(value == false)
        {
            rc = RBUS_ERROR_INVALID_INPUT;
            RBUSCLI_LOG ("Invalid data value passed to set. Please pass proper value with respect to the data type\nsetvalues failed with return value: %d\n", rc);
            return;
        }
        if (type != RBUS_NONE)
        {

            runSteps = __LINE__;
            /* Set Session ID & Commit value */
            if (g_isInteractive)
            {
                if (4 == i)
                {
                    /* Is commit */
                    if (strncasecmp ("true", argv[argc - 1], 4) == 0)
                        isCommit = true;
                    else if (strncasecmp ("false", argv[argc - 1], 5) == 0)
                    {
                        isCommit = false;
                        if(g_curr_sessionId == 0)
                        {
                            rc = rbus_createSession(g_busHandle, &g_curr_sessionId);
                            if(rc != RBUS_ERROR_SUCCESS)
                            {
                                printf("Session creation failed with err = %d\n", rc);
                            }
                        }
                        if(g_curr_sessionId == 0)
                            printf("Can't set the value temporarily with sessionId 0 and commit false\n");
                    }
                    else
                        isCommit = true;
                    sessionId = g_curr_sessionId;
                }
                else
                {
                    isCommit = true;
                    sessionId = 0;
                }
                if(isCommit && g_curr_sessionId != 0)
                {
                    printf("closing the session\n");
                    fflush(stdout);
                    rc = rbus_closeSession(g_busHandle, g_curr_sessionId);
                    if(rc != RBUS_ERROR_SUCCESS)
                    {
                        printf("Session close failed with err = %d\n", rc);
                    }
                    g_curr_sessionId = 0;
                }
            }
            else
            {
                /* For non-interactive mode, regardless of COMMIT value tha is passed, we assume the isCommit as TRUE */
                isCommit = true;
                sessionId = 0;
            }
            (void)sessionId;

            /* Assume a sessionId as it is going to be single entry thro this cli app; */
            rbusSetOptions_t opts = {isCommit,sessionId};
            rc = rbus_set(g_busHandle, argv[2], setVal, &opts);

            /* Free the data pointer that was allocated */
            rbusValue_Release(setVal);
        }
        else
        {
            rc = RBUS_ERROR_INVALID_INPUT;
            RBUSCLI_LOG ("Invalid data type. Please see the help\n\r");
        }
    }

    if(RBUS_ERROR_SUCCESS == rc)
    {
        RBUSCLI_LOG ("setvalues succeeded..\n\r");
    }
    else
    {
        RBUSCLI_LOG ("setvalues failed with return value: %d\n\r", rc);
    }
}

void validate_and_execute_getnames_cmd (int argc, char *argv[])
{
    bool nextLevel = true;
    rbusElementInfo_t* elems = NULL;
    rbusError_t rc;
    int index = 1;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;

    if(argc > 3)
    {
        if(strcmp(argv[3], "true") == 0)
            nextLevel = true;
        else if(strcmp(argv[3], "false") == 0)
            nextLevel = false;
        else
        {
            printf ("Invalid nextLevel option.  This should be 'true' or 'false'.\n\r");
            return;
        }
    }

    rc = rbusElementInfo_get(g_busHandle, argv[2], nextLevel ? -1 : RBUS_MAX_NAME_DEPTH, &elems);

    if(RBUS_ERROR_SUCCESS == rc)
    {
        if(elems)
        {
#ifdef PRINT_BASH_STYLE
            rbusElementInfo_t* elem;
            char const* component;
            size_t maxIndexLen = 0;
            size_t maxComponentLen = 0;
            size_t maxNameLen = 0;
            size_t len;
            char buff[20];

            elem = elems;
            component = NULL;
            index = 1;

            while(elem)
            {
                if(component == NULL || strcmp(component, elem->component) != 0)
                {
                    len = strlen(elem->component);
                    if(len > maxComponentLen)
                        maxComponentLen = len;
                    component = elem->component;
                }

                len = strlen(elem->name);
                if(len > maxNameLen)
                    maxNameLen = len;

                snprintf(buff, 20, "%d", index++);
                len = strlen(buff);
                if(len > maxIndexLen)
                    maxIndexLen = len;

                elem = elem->next;
            }

            len = printf("%*s  %-8s  %6s  %-*s  %-*s\n\r", (int)maxIndexLen, " ", "type", "access", (int)maxComponentLen, "component", (int)maxNameLen, "name");
            for(index = 0; index < (int)len; ++index)
                printf("%c", '-');
            printf("\n\r");

            elem = elems;
            component = NULL;
            index = 1;


            while(elem)
            {
                printf("%*d  %-8s  %d%d%d%d%d%d  %-*s  %s\n\r",
                    (int)maxIndexLen,
                    index++,
                    elem->type == RBUS_ELEMENT_TYPE_PROPERTY ? "property" :
                    (elem->type == RBUS_ELEMENT_TYPE_TABLE ? "table" :
                    (elem->type == RBUS_ELEMENT_TYPE_EVENT ? "event" :
                    (elem->type == RBUS_ELEMENT_TYPE_METHOD ? "method" : "object"))),
                    elem->access & RBUS_ACCESS_GET ? 1 : 0,
                    elem->access & RBUS_ACCESS_SET ? 1 : 0,
                    elem->access & RBUS_ACCESS_ADDROW ? 1 : 0,
                    elem->access & RBUS_ACCESS_REMOVEROW ? 1 : 0,
                    elem->access & RBUS_ACCESS_SUBSCRIBE ? 1 : 0,
                    elem->access & RBUS_ACCESS_INVOKE ? 1 : 0,
                    (int)maxComponentLen,
                    elem->component,
                    elem->name);
                elem = elem->next;
            }
#else

            rbusElementInfo_t* elem;
            char const* component;

            elem = elems;
            component = NULL;
            index = 1;

            while(elem)
            {
                if(component == NULL || strcmp(component, elem->component) != 0)
                {
                    printf("\n\rComponent %s:\n\r", elem->component);
                    component = elem->component;
                    index = 1;
                }
                printf ("Element   %2d:\n\r", index++);
                printf ("              Name  : %s\n\r", elem->name);
                printf ("              Type  : %s\n\r",
                    elem->type == RBUS_ELEMENT_TYPE_PROPERTY ? "Property" :
                    (elem->type == RBUS_ELEMENT_TYPE_TABLE ?   "Table" :
                    (elem->type == RBUS_ELEMENT_TYPE_EVENT ?   "Event" :
                    (elem->type == RBUS_ELEMENT_TYPE_METHOD ?  "Method" :
                                                               "Object"))));
                printf ("              Writable: ");
                if(elem->type == RBUS_ELEMENT_TYPE_TABLE && elem->access & RBUS_ACCESS_ADDROW)
                    printf( "Writable");
                else if(elem->access & RBUS_ACCESS_SET)
                    printf( "Writable");
                else if(elem->access & RBUS_ACCESS_GET)
                    printf( "ReadOnly");
                printf( "\n\r");

                printf ("              Access Flags: %d%d%d%d%d%d\n\r",
                    elem->access & RBUS_ACCESS_GET ? 1 : 0,
                    elem->access & RBUS_ACCESS_SET ? 1 : 0,
                    elem->access & RBUS_ACCESS_ADDROW ? 1 : 0,
                    elem->access & RBUS_ACCESS_REMOVEROW ? 1 : 0,
                    elem->access & RBUS_ACCESS_SUBSCRIBE ? 1 : 0,
                    elem->access & RBUS_ACCESS_INVOKE ? 1 : 0);

                elem = elem->next;
            }

#endif

            rbusElementInfo_free(g_busHandle, elems);
        }
        else
        {
            printf ("No results returned for %s\n\r",argv[2]);
        }
    }
    else
    {
        printf ("Failed to get the data. Error : %d\n\r",rc);
    }
}

void validate_and_execute_getrownames_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusRowName_t* rows;
    int i = 1;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;

    rc = rbusTable_getRowNames(g_busHandle, argv[2], &rows);
    if(RBUS_ERROR_SUCCESS == rc)
    {
        rbusRowName_t* row = rows;
        while(row)
        {
            printf("Row %2u: instNum=%u, alias=%s, name=%s\n\r", i++, row->instNum, row->alias ? row->alias : "None", row->name);
            row = row->next;
        }
        rbusTable_freeRowNames(g_busHandle, rows);
    }
    else
    {
        printf ("Failed to get the data. Error : %d\n\r",rc);
    }
}

void validate_and_execute_register_command (int argc, char *argv[], bool add)
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    int i;

    if( !(argc >= 4 && (argc % 2) == 0))
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return; 

    if(NULL == g_registeredProps)
    {
        rtList_Create(&g_registeredProps);
    }

    runSteps = __LINE__;
    for(i = 2; i < argc; i += 2)
    {
        const char* stype = argv[i];
        const char* name = argv[i+1];

        rbusDataElement_t elem = {(char*)name, 0, {NULL}};

        if(strncmp(stype, "property", 4) == 0)
        {
            elem.type = RBUS_ELEMENT_TYPE_PROPERTY;
            elem.cbTable.getHandler = property_get_handler;
            elem.cbTable.setHandler = property_set_handler;
        }
        else if(strncmp(stype, "table", 4) == 0)
        {
            elem.type = RBUS_ELEMENT_TYPE_TABLE;
            elem.cbTable.tableAddRowHandler = table_add_row_handler;
            elem.cbTable.tableRemoveRowHandler = table_remove_row_handler;
        }
        else if(strncmp(stype, "event", 4) == 0)
        {
            elem.type = RBUS_ELEMENT_TYPE_EVENT;
            elem.cbTable.eventSubHandler = event_subscribe_handler;
        }
        else if(strncmp(stype, "method", 4) == 0)
        {
            elem.type = RBUS_ELEMENT_TYPE_METHOD;
            elem.cbTable.methodHandler = method_invoke_handler;
        }
        else 
        {
            printf("Invalid element type: %s.  Must be prop, table, event, or method.\n\r", stype);
            continue;
        }

        if(add)
        {
            rc = rbus_regDataElements(g_busHandle, 1, &elem);
        }
        else
        {
            rc = rbus_unregDataElements(g_busHandle, 1, &elem);
        }

        if(rc == RBUS_ERROR_SUCCESS)
        {
            printf("%s %s\n\r", add ? "Registered" : "Unregistered", name);

            if(add)
            {
                rbusProperty_t prop;
                rbusValue_t value;

                rbusValue_Init(&value);
                rbusValue_SetString(value, "default value");

                rbusProperty_Init(&prop, name, value);
                rbusValue_Release(value);

                rtList_PushBack(g_registeredProps, prop, NULL);
            
            }
            else
            {
                rtListItem li;
                rtList_GetFront(g_registeredProps, &li);
                while(li)
                {
                    rbusProperty_t prop;
                    rtListItem_GetData(li, (void**)&prop);
                    if(strcmp(rbusProperty_GetName(prop), name) == 0)
                    {
                        rtList_RemoveItem(g_registeredProps, li, free_registered_property);
                        break;
                    }                            
                    rtListItem_GetNext(li, &li);
                }
            }
        }
        else
        {
            printf("Failed to %s %s, error: %d\n\r", add ? "register" : "unregister", name, rc);
        }
    }
}

void set_filter_value(const char* arg, rbusValue_t value)
{
    int len;
    int ival;
    float fval;
    char sval[10];
    int ret;

    runSteps = __LINE__;
    /*try to guess the type represented by arg*/
    ret = sscanf(arg, "%d %n", &ival, &len);
    if(ret==1 && !arg[len])
    {
        rbusValue_SetInt32(value, ival);
        return;
    }

    ret = sscanf(arg, "%f %n", &fval, &len);
    if(ret==1 && !arg[len])
    {
        rbusValue_SetSingle(value, fval);
        return;
    }

    ret = sscanf(arg, "%6s %n", sval, &len);
    if(ret==1)
    {
        if(strcasecmp(sval, "true") == 0)
        {
            rbusValue_SetBoolean(value, true);
            return;
        }
        else if(strcasecmp(sval, "false") == 0)
        {
            rbusValue_SetBoolean(value, false);
            return;
        }
    }
    /*everything else is treated as string*/
    rbusValue_SetString(value, arg);
}

int find_filter(char *argv[])
{
    if(strcmp(argv[3], ">") == 0)
       return RBUS_FILTER_OPERATOR_GREATER_THAN;
    else if(strcmp(argv[3], ">=") == 0)
       return RBUS_FILTER_OPERATOR_GREATER_THAN_OR_EQUAL;
    else if(strcmp(argv[3], "<") == 0)
       return  RBUS_FILTER_OPERATOR_LESS_THAN;
    else if(strcmp(argv[3], "<=") == 0)
       return  RBUS_FILTER_OPERATOR_LESS_THAN_OR_EQUAL;
    else if(strcmp(argv[3], "=") == 0)
       return RBUS_FILTER_OPERATOR_EQUAL;
    else if(strcmp(argv[3], "!=") == 0)
       return  RBUS_FILTER_OPERATOR_NOT_EQUAL;
    else
       return -1;
}

int set_publishOnSubscribe(int argc, char *argv[])
{
    int publishOnSubscribe = 0;

    if (strncasecmp ("true", argv[argc - 1], 4) == 0)
        publishOnSubscribe = 1;
    else if(strncasecmp ("false", argv[argc - 1], 5) == 0)
        publishOnSubscribe = 0;
    else
        publishOnSubscribe = -1;
    return publishOnSubscribe;
}

void validate_and_execute_subscribe_cmd (int argc, char *argv[], bool add, bool isAsync, bool rawDataSub)
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusFilter_t filter = NULL;
    rbusValue_t filterValue = NULL;
    int relOp;
    char* userData = NULL;
    int interval = 0;
    int duration = 0;
    bool subinterval = false;
    int publishOnSubscribe = 0;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    runSteps = __LINE__;
    if(strlen(argv[2]) + (argc>3 ? strlen(argv[3]):0) + (argc>4 ? strlen(argv[4]):0)
            + (argc>5 ? strlen(argv[5]):0) > 255)
    {
        printf("Query too long.");
        return;
    }

    if (!verify_rbus_open())
        return;    

    if(!g_subsribeUserData)
       rtList_Create(&g_subsribeUserData);

    if(1)
    {
        userData = rt_calloc(1, 256);
        if (matchCmd(argv[1], 4, "subinterval") || matchCmd(argv[1], 6, "unsubinterval"))
        {
            subinterval = true;
            strcat(userData, "subint ");
        }
        else
        {
            strcat(userData, "sub ");
        }
        strcat(userData, argv[2]);
    }
    if(argc > 3) /*filter*/
    {
        /*interval*/
        if (subinterval && (argc < 7))
        {
            if (!atoi(argv[3])) {
                goto exit_error;
            }

            interval = atoi(argv[3]);
            strcat(userData, " ");
            strcat(userData, argv[3]);
            if(argv[4] != NULL)
            {
                if( argc > 5 ) {
                    /*duration*/
                    duration = atoi(argv[4]);
                    strcat(userData, " ");
                    strcat(userData, argv[4]);

                    publishOnSubscribe = set_publishOnSubscribe(argc, argv);
                    if(publishOnSubscribe == -1)
                        goto exit_error;
                }
                else if(argc == 5)
                {
                    if (atoi(argv[4])) {
                        duration = atoi(argv[4]);
                        strcat(userData, " ");
                        strcat(userData, argv[4]);
                    }
                    else
                    {
                        publishOnSubscribe = set_publishOnSubscribe(argc, argv);
                        if(publishOnSubscribe == -1)
                            goto exit_error;
                    }
                }
            }
        }
        else if (((relOp = find_filter(argv)) >= 0) && (argc < 7))
        {
            strcat(userData, " ");
            strcat(userData, argv[3]);
            strcat(userData, " ");
            strcat(userData, argv[4]);

            rbusValue_Init(&filterValue);

            set_filter_value(argv[4], filterValue);

            rbusFilter_InitRelation(&filter, relOp, filterValue);
            if(argv[5] != NULL)
            {
                publishOnSubscribe = set_publishOnSubscribe(argc, argv);
                if(publishOnSubscribe == -1)
                    goto exit_error;
            }
        }
        else
        {
            if(argc == 4)
            {
                publishOnSubscribe = set_publishOnSubscribe(argc, argv);
                if(publishOnSubscribe == -1)
                    goto exit_error;
            }
            else
            {
                goto exit_error;
            }
        }
    }
    else if(subinterval || argc > 7)
    {
exit_error:
        runSteps = __LINE__;
        printf ("Invalid arguments. Please see the help\n\r");
        rt_free(userData);
        return;
    }

    rbusEventSubscription_t subscription_rawdata = {argv[2], filter, interval, duration, event_receive_handler1, userData, NULL, NULL, publishOnSubscribe};
    rbusEventSubscription_t subscription = {argv[2], filter, interval, duration, event_receive_handler2, userData, NULL, NULL, publishOnSubscribe};

    /* Async will be TRUE only when add is TRUE */
    if (isAsync && add)
    {
        rc = rbusEvent_SubscribeExAsync(g_busHandle, &subscription, 1, event_receive_subscription_handler, 0);
    }
    else if(add && rawDataSub)
    {
        rc = rbusEvent_SubscribeExRawData(g_busHandle, &subscription_rawdata, 1, 0);
    }
    else if(add)
    {
        rc = rbusEvent_SubscribeEx(g_busHandle, &subscription, 1, 0);
    }
    else if(rawDataSub)
    {
        rc = rbusEvent_UnsubscribeExRawData(g_busHandle, &subscription, 1);
    }
    else
    {
        rc = rbusEvent_UnsubscribeEx(g_busHandle, &subscription, 1);
    }

    if(filterValue)
        rbusValue_Release(filterValue);
    if(filter)
        rbusFilter_Release(filter);

    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("Invalid Subscription err:%d\n\r", rc);
        rt_free (userData);
    }
    else
    {
        /* Retain the userData for the interactive mode of rbuscli */
        if(add)
        {
            rtList_PushBack(g_subsribeUserData, userData, NULL);
        }
        else
        {
            /* Remove the userData from the list that are maintained for */
            rtListItem li;
            rtList_GetFront(g_subsribeUserData, &li);
            runSteps = __LINE__;
            while(li)
            {
                char* prtUserData = NULL;
                rtListItem_GetData(li, (void**)&prtUserData);
                if(strcmp(prtUserData, userData) == 0)
                {
                    rtList_RemoveItem(g_subsribeUserData, li, free_userdata);
                    break;
                }
                rtListItem_GetNext(li, &li);
            }

            rt_free(userData);
        }
    }
}

void validate_and_execute_publish_command(int argc, char *argv[], bool rawDataPub)
{
    rbusError_t rc;
    rbusObject_t data;
    rbusValue_t value;

    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;    

    runSteps = __LINE__;
    if(rawDataPub)
    {
        rbusEventRawData_t event = {0};
        event.name = argv[2];
        event.rawData = argc < 4 ? "default event data" : argv[3];
        event.rawDataLen = strlen(event.rawData);

        rc = rbusEvent_PublishRawData(g_busHandle, &event);
        if(rc != RBUS_ERROR_SUCCESS)
            printf("provider: rbusEvent_Publish Event1 failed: %d\n", rc);
    }
    else
    {
        rbusEvent_t event = {0};
        rbusValue_Init(&value);
        rbusValue_SetString(value, argc < 4 ? "default event data" : argv[3]);
        rbusObject_Init(&data, NULL);
        rbusObject_SetValue(data, "value", value);

        event.name = argv[2];
        event.data = data;
        event.type = RBUS_EVENT_GENERAL;

        rc = rbusEvent_Publish(g_busHandle, &event);

        rbusValue_Release(value);

        if(rc != RBUS_ERROR_SUCCESS)
        {
            printf("Publish failed err: %d\n\r", rc);
        }
        rbusObject_Release(data);
    }
}

static void execute_method_cmd(char *cmd, char *method, rbusObject_t inParams)
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    rbusValue_t value = NULL;
    rbusObject_t outParams = NULL;
    rbusProperty_t prop = NULL;
    rbusValueType_t type = RBUS_NONE;
    char *str_value = NULL;
    int i = 0;

    runSteps = __LINE__;
    rc = rbusMethod_Invoke(g_busHandle, method, inParams, &outParams);
    if(inParams)
        rbusObject_Release(inParams);
    if(RBUS_ERROR_SUCCESS != rc)
    {
        if(outParams)
        {
            printf("%s failed for %s with err: '%s'\n\r",cmd, method,rbusError_ToString(rc));
            rbusObject_fwrite(outParams, 1, stdout);
            rbusObject_Release(outParams);
        }
        else
        {
            printf("Unexpected error in handling outparams\n");
        }
        return;
    }

    prop = rbusObject_GetProperties(outParams);
    while(prop)
    {
        value = rbusProperty_GetValue(prop);
        if(value)
        {
            type = rbusValue_GetType(value);
            str_value = rbusValue_ToString(value,NULL,0);

            if(str_value)
            {
                printf ("Parameter %2d:\n\r", ++i);
                printf ("              Name  : %s\n\r", rbusProperty_GetName(prop));
                printf ("              Type  : %s\n\r", getDataType_toString(type));
                printf ("              Value : %s\n\r", str_value);
                free(str_value);
            }
        }
        prop = rbusProperty_GetNext(prop);
    }

    if(outParams)
        rbusObject_Release(outParams);
}

void validate_and_execute_method_values_cmd (int argc, char *argv[])
{
    if(!((argc >= 6) && (0 == (argc % 3))))
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    rbusValue_t value = NULL;
    rbusProperty_t prop = NULL;
    rbusValueType_t type = RBUS_NONE;
    rbusObject_t inParams = NULL;
    int i = 3;

    runSteps = __LINE__;
    rbusObject_Init(&inParams, NULL);
    while( i < argc )
    {
        type = getDataType_fromString(argv[i+1]);
        if (type == RBUS_NONE)
        {
            printf ("Invalid data type '%s' for the parameter %s\n\r",argv[i+1],argv[i]);
            if(inParams)
                rbusObject_Release(inParams);
            return;
        }

        rbusValue_Init(&value);
        if(false == rbusValue_SetFromString(value, type, argv[i+2]))
        {
            printf ("Invalid value '%s' for the parameter %s\n\r",argv[i+2],argv[i]);
            if(inParams)
                rbusObject_Release(inParams);
            return;
        }
        rbusProperty_Init(&prop, argv[i], value);
        rbusObject_SetProperty(inParams,prop);
        rbusValue_Release(value);
        rbusProperty_Release(prop);

        i = i+3;
    }
    execute_method_cmd(argv[1], argv[2], inParams);
}

void validate_and_execute_method_noargs_cmd (int argc, char *argv[])
{
    if (argc < 3)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    execute_method_cmd(argv[1], argv[2], NULL);
}

void validate_and_execute_method_names_cmd (int argc, char *argv[])
{
    if (argc < 4)
    {
        printf ("Invalid arguments. Please see the help\n\r");
        return;
    }

    if (!verify_rbus_open())
        return;

    runSteps = __LINE__;
    rbusProperty_t prop = NULL;
    rbusObject_t inParams = NULL;
    int i = 3;

    rbusObject_Init(&inParams, NULL);
    for( ; i<argc; i++ )
    {
        rbusProperty_Init(&prop, argv[i], NULL) ;
        rbusObject_SetProperty(inParams,prop);
        rbusProperty_Release(prop);
    }
    execute_method_cmd(argv[1], argv[2], inParams);
}

void validate_and_execute_create_session_cmd ( )
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    if (!verify_rbus_open())
        return;
    rc = rbus_createSession(g_busHandle, &g_curr_sessionId);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("Session creation failed with err = %d\n", rc);
    }
}

void validate_and_execute_get_session_cmd ( )
{
    if (!verify_rbus_open())
        return;
    rbus_getCurrentSession(g_busHandle, &g_curr_sessionId);
    printf ("current sessionID %d\n\r", g_curr_sessionId);
}

void validate_and_execute_close_session_cmd (int argc, char *argv[])
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    (void)argc;
    if (!verify_rbus_open())
        return;
    if (strncasecmp ("false", argv[argc - 1], 5) == 0)
    {
        /* Todo : Provider need to rollback the changes to actual value that is present before commit field is passed as false*/
    }
    rbus_getCurrentSession(g_busHandle, &g_curr_sessionId);
    if(g_curr_sessionId != 0)
    {
        rc = rbus_closeSession(g_busHandle, g_curr_sessionId);
    }
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("Session close failed with err = %d\n", rc);
    }
    g_curr_sessionId = 0;
}

int handle_cmds (int argc, char *argv[])
{
    /* Interactive shell; handle the enter key */
    if (1 == argc)
        return 0;

    char* command = argv[1];

    runSteps = __LINE__;
    if(matchCmd(command, 3, "getvalues") || (matchCmd(command, 2, "-g") && !g_isInteractive))
    {
        validate_and_execute_get_cmd (argc, argv);
    }
    else if(matchCmd(command, 3, "setvalues") || (matchCmd(command, 2, "-s") && !g_isInteractive))
    {
        validate_and_execute_set_cmd (argc, argv);
    }
    else if(matchCmd(command, 4, "getnames"))
    {
        validate_and_execute_getnames_cmd (argc, argv);
    } 
    else if(matchCmd(command, 4, "getrows") || matchCmd(command, 4, "getrownames"))
    {
        validate_and_execute_getrownames_cmd (argc, argv);
    }       
    else if(matchCmd(command, 5, "disccomponents"))
    {
        int i = argc - 2;
        if (i != 0)
        {
            execute_discover_component_cmd(argc, argv);
        }
        else
        {
            printf ("Invalid arguments. Please see the help\n\r");
        }
    }
    else if(matchCmd(command, 5, "discelements"))
    {
        int i = argc - 2;
        if ((i >= 1) && (i <= 2))
        {
            execute_discover_elements_cmd(argc, argv);
        }
        else
        {
            printf ("Invalid arguments. Please see the help\n\r");
        }
    }
    else if(matchCmd(command, 5, "discallcomponents"))
    {
        execute_discover_registered_components_cmd(argc, argv);
    }
    else if(matchCmd(command, 5, "discwildcarddests"))
    {
        execute_discover_wildcard_dests_cmd(argc, argv);
    }
    else if(matchCmd(command, 3, "addrow"))
    {
        validate_and_execute_addrow_cmd (argc, argv);
    }
    else if(matchCmd(command, 3, "delrow"))
    {
        validate_and_execute_delrow_cmd (argc, argv);
    }
    else if(matchCmd(command, 3, "register"))
    {
        validate_and_execute_register_command (argc, argv, true);
    }
    else if(matchCmd(command, 5, "unregister"))
    {
        validate_and_execute_register_command (argc, argv, false);
    }
    else if(matchCmd(command, 3, "subscribe") || matchCmd(command, 4, "subinterval"))
    {
        validate_and_execute_subscribe_cmd (argc, argv, true, false, false);
    }
    else if(matchCmd(command, 5, "unsubscribe") || matchCmd(command, 6, "unsubinterval"))
    {
        validate_and_execute_subscribe_cmd (argc, argv, false, false, false);
    }
    else if(matchCmd(command, 4, "asubscribe"))
    {
        validate_and_execute_subscribe_cmd (argc, argv, true, true, false);
    }
    else if(matchCmd(command, 9, "rawdatasubscribe"))
    {
        validate_and_execute_subscribe_cmd (argc, argv, true, false, true);
    }
    else if(matchCmd(command, 11, "rawdataunsubscribe"))
    {
        validate_and_execute_subscribe_cmd (argc, argv, false, false, true);
    }
    else if(matchCmd(command, 3, "publish"))
    {
        validate_and_execute_publish_command (argc, argv, false);
    }
    else if(matchCmd(command, 3, "rawdatapublish"))
    {
        validate_and_execute_publish_command (argc, argv, true);
    }
    else if(matchCmd(command, 9, "method_values"))
    {
        validate_and_execute_method_values_cmd (argc, argv);
    }
    else if(matchCmd(command, 9, "method_names"))
    {
        validate_and_execute_method_names_cmd (argc, argv);
    }
    else if(matchCmd(command, 9, "method_noargs"))
    {
        validate_and_execute_method_noargs_cmd (argc, argv);
    }
    else if(matchCmd(command, 10, "create_session"))
    {
        validate_and_execute_create_session_cmd (argc, argv);
    }
    else if(matchCmd(command, 7, "get_session"))
    {
        validate_and_execute_get_session_cmd (argc, argv);
    }
    else if(matchCmd(command, 9, "close_session"))
    {
        validate_and_execute_close_session_cmd (argc, argv);
    }
    else if(matchCmd(command, 4, "help"))
    {
        if(argc == 2)
            show_menu(NULL);
        else
            show_menu(argv[2]);
    }
    else if(matchCmd(command, 3, "log"))
    {
        if(argc < 3)
        {
            printf("Missing log level\n\r");
            return 0;
        }
        if(matchCmd(argv[2], 5, "events"))
        {
            g_logEvents = !g_logEvents;
            printf("Event logs %s\n\r", g_logEvents ? "enabled" : "disabled");
        }
        else
        {
            rbusLogLevel level;
            int valid = 1;

            if (strcasecmp(argv[2], "debug") == 0)
            {
                level = RBUS_LOG_DEBUG;
            }
            else if (strcasecmp(argv[2], "info") == 0)
            {
                level = RBUS_LOG_INFO;
            }
            else if (strcasecmp(argv[2], "warn") == 0)
            {
                level = RBUS_LOG_WARN;
            }
            else if (strcasecmp(argv[2], "error") == 0)
            {
                level = RBUS_LOG_ERROR;
            }
            else if (strcasecmp(argv[2], "fatal") == 0)
            {
                level = RBUS_LOG_FATAL;
            }
            else
            {
                valid = 0;
            }

            if(valid)
            {
                rbus_setLogLevel(level);
                g_logLevel = level;

                printf("Log level set to %s\n\r", argv[2]);
            }
            else
            {
                printf("Invalid log level\n\r");
            }
        }
    }
    else if (matchCmd(command, 5, "opendirect"))
    {
        validate_and_execute_open_n_close_direct_cmd(argc, argv, true);
    }
    else if (matchCmd(command, 5, "closedirect"))
    {
        validate_and_execute_open_n_close_direct_cmd(argc, argv, false);
    }
    else if (matchCmd(command, 4, "quit"))
    {
        return 1;
    }
    else
    {
        printf ("Invalid arguments. Please see the help\n\r");
    }

    return 0;
}

static int construct_input_into_cmds(char* buff, int* pargc, char** argv)
{
    int len = (int)strlen(buff);
    int i, j, quote;
    int argc = 0;
    argv[argc++] = "rbuscli";
    runSteps = __LINE__;
    for(i = 0; i < len; ++i)
    {
        quote = 0;    
        while(i < len && (buff[i] == ' ' || buff[i] == '\t'))
            ++i;
        if(i == len)
            break;
        if(buff[i] == '\'')
            quote = 1;
        else if(buff[i] == '"')
            quote = 2;
        if(quote)
            i++;
        j = i;
        while(i < len)
        {
            if((quote == 0 && buff[i] == ' ' )
            || (quote == 1 && buff[i] == '\'')
            || (quote == 2 && buff[i] == '"' ))
                break;
            ++i;
        }
        if(i == len &&
              ( (quote == 1 && buff[i-1] != '\'') ||
                (quote == 2 && buff[i-1] != '"' ) ) )
        {
            argc = 0;
            return 1;
        }
        if(argv)
        {
            buff[i] = 0;
            argv[argc] = strdup(buff+j);
        }
        argc++;
    }
    *pargc = argc;
    return 0;
}

static char* find_completion(char* token, int count, ...)
{
    int i;
    size_t len = 0;
    va_list va;

    runSteps = __LINE__;
    va_start(va, count);
    len = strlen(token);
    for(i=0; i<count; ++i)
    {
        const char* command = va_arg(va, char*);
        if(len < strlen(command) && strncmp(token, command, len) == 0)
        {
            va_end(va);
            return strdup(command + len);
        }
    }
    va_end(va);
    return NULL;
}


void completion(const char *buf, linenoiseCompletions *lc) {
    int len = 0;
    char* cpy = strdup(buf);
    char* line = NULL;
    char* tok = NULL;
    char* saveptr = NULL;
    int num = 0;
    char* tokens[3];/*3 or the number of tokens we scan for below*/
    char* completion = NULL;

    len = strlen(buf);
    line = rt_malloc(len + 32);/*32 or just enough room to append a word from below*/
    strcpy(line, buf);
  
    tok = strtok_r(cpy, " ", &saveptr);
    while(tok && num < 3)
    {
        tokens[num++] = tok;
        tok = strtok_r(NULL, " ", &saveptr);
    }

    if(num == 1)
    {
        runSteps = __LINE__;
        completion = find_completion(tokens[0], 14, "get", "set", "add", "del", "getr", "getn", "disca", "discc", "disce",
                "discw", "sub", "subint", "rawdatasub", "rawdataunsub", "unsub", "unsubint", "asub", "method_no", "method_na", "method_va", "reg",                 "unreg", "pub", "rawdatapub", "log", "quit", "opend", "closed", "help");
    }
    else if(num == 2)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "reg") == 0)
        {
            completion = find_completion(tokens[1], 4, "prop", "event", "table", "method");
        }
        else if(strcmp(tokens[0], "log") == 0)
        {
            completion = find_completion(tokens[1], 6, "events", "debug", "info", "warn", "error", "fatal");
        }
    }
    else if(num == 3)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "set") == 0)
        {
            completion = find_completion(tokens[2], 18, 
                    "string", "int", "uint", "boolean", "datetime", "single", "double",
                    "bytes", "char", "byte", "int8", "uint8", "int16", "uint16", "int32", 
                    "uint32", "int64", "uint64");
        }
    }
    else if(num == 4)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "method_va") == 0)
        {
            completion = find_completion(tokens[2], 18,
                    "string", "int", "uint", "boolean", "datetime", "single", "double",
                    "bytes", "char", "byte", "int8", "uint8", "int16", "uint16", "int32",
                    "uint32", "int64", "uint64");
        }
    }

    if(completion)
    {
        strcat(line, completion);
        linenoiseAddCompletion(lc, line);
    }

    free(cpy);
    free(line);
    free(completion);
    runSteps = __LINE__;
}

char *hints(const char *buf, int *color, int *bold) {

    int len = 0;
    char* cpy = strdup(buf);
    char* line = NULL;
    char* tok = NULL;
    char* saveptr = NULL;
    int num = 0;
    char* tokens[4];/*3 or the number of tokens we scan for below*/
    char* hint = NULL;

    len = strlen(buf);
    line = rt_malloc(len + 32);/*32 or just enough room to append a word from below*/
    strcpy(line, buf);
  
    tok = strtok_r(cpy, " ", &saveptr);
    while(tok && num < 4)
    {
        tokens[num++] = tok;
        tok = strtok_r(NULL, " ", &saveptr);
    }

    if(num == 1)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "get") == 0)
        {
            hint = " path";
        }
        else if(strcmp(tokens[0], "set") == 0)
        {
            hint = " parameter type value";
        }
        else if(strcmp(tokens[0], "add") == 0)
        {
            hint = " table";
        }
        else if(strcmp(tokens[0], "del") == 0)
        {
            hint = " row";
        }
        else if(strcmp(tokens[0], "getr") == 0)
        {
            hint = " path";
        }
        else if(strcmp(tokens[0], "getn") == 0)
        {
            hint = " path";
        }
        else if(strcmp(tokens[0], "discc") == 0)
        {
            hint = " element";
        }
        else if(strcmp(tokens[0], "disce") == 0)
        {
            hint = " component";
        }
        else if(strcmp(tokens[0], "discw") == 0)
        {
            hint = " expression";
        }
        else if(strcmp(tokens[0], "reg") == 0)
        {
            hint = " type(prop,table,event,method) name";
        }
        else if(strcmp(tokens[0], "unreg") == 0)
        {
            hint = " type(prop,table,event,method) name";
        }
        else if(strcmp(tokens[0], "sub") == 0)
        {
            hint = " event [operator value initialValue]";
        }
        else if(strcmp(tokens[0], "subint") == 0)
        {
            hint = " event interval [duration] [initialValue]";
        }
        else if(strcmp(tokens[0], "rawdatasub") == 0)
        {
            hint = " event";
        }
        else if(strcmp(tokens[0], "rawdataunsub") == 0)
        {
            hint = " event";
        }
        else if(strcmp(tokens[0], "unsub") == 0)
        {
            hint = " event [operator value]";
        }
        else if(strcmp(tokens[0], "unsubint") == 0)
        {
            hint = " event interval [duration]";
        }
        else if(strcmp(tokens[0], "asub") == 0)
        {
            hint = " event [operator value]";
        }
        else if(strcmp(tokens[0], "pub") == 0)
        {
            hint = " event [data]";
        }
        else if(strcmp(tokens[0], "rawdatapub") == 0)
        {
            hint = " event [data]";
        }
        else if(strcmp(tokens[0], "method_va") == 0)
        {
            hint = " methodname parameter type value";
        }
        else if(strcmp(tokens[0], "method_na") == 0)
        {
            hint = " methodname parameter";
        }
        else if(strcmp(tokens[0], "method_no") == 0)
        {
            hint = " methodname";
        }
        else if(strcmp(tokens[0], "log") == 0)
        {
            hint = " level(debug|info|warn|error|fatal|event)";
        }
        else if(strcmp(tokens[0], "opend") == 0)
        {
            hint = " path";
        }
        else if(strcmp(tokens[0], "closed") == 0)
        {
            hint = " path";
        }
        else if(strcmp(tokens[0], "help") == 0)
        {
            hint = " [command]";
        }
    }
    else if(num == 2)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "set") == 0)
        {
            hint = " type(string,int,uint,boolean,...) value";
        }
        else if(strcmp(tokens[0], "getn") == 0)
        {
            hint = " nextLevel(true|false)";
        }
        else if(strcmp(tokens[0], "reg") == 0)
        {
            hint = " name";
        }
        else if(strcmp(tokens[0], "method_va") == 0)
        {
            hint = " parameter type value";
        }
        else if(strcmp(tokens[0], "method_na") == 0)
        {
            hint = " parameter";
        }
    }
    else if(num == 3)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "set") == 0)
        {
            hint = " value";
        }
        else if(strcmp(tokens[0], "method_va") == 0)
        {
            hint = " type(string,int,uint,boolean,...) value";
        }
    }
    else if(num == 3)
    {
        runSteps = __LINE__;
        if(strcmp(tokens[0], "method_va") == 0)
        {
            hint = " value";
        }
    }
    else
    {
        runSteps = __LINE__;
        hint = NULL;
    }

    free(cpy);
    free(line);
    runSteps = __LINE__;

    if(hint)
    {
        *color = 35;
        *bold = 0;
    }

    return hint;
}

static void exception_handler(int sig, siginfo_t *info)
{
    int fd1;
    char cmdFile[32]      = {0};
    char cmdName[32]      = {0};
    time_t rawtime;
    struct tm * timeinfo;
    static char cmdLine[1024]  = {0};

    snprintf( cmdFile,32,  "/proc/self/cmdline" );

    /* Get current time */
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    /* Get command name */
    fd1 = open( cmdFile, O_RDONLY );
    if( fd1 > 0 )
    {
        if( read( fd1, cmdName, sizeof(cmdName)-1 ) <= 0)
            fprintf( stderr, "Error in read function:%s\n",__FUNCTION__ );

        close(fd1);

    }

    /* dump general information */
    printf("\n\r!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    printf("\n\r!!!!!!!!!!!!!!!!!!!!!!!!!!!! Exception Caught !!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    printf("\n\r!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );

    printf("\n\n\rSignal info:\n\r"
                        "\n\rTime: %s "
                        "\n\rProcess name: <%s>"
                        "\n\rPID: %d"
                        "\n\rFault Address: %p"
                        "\n\rSignal: %d"
                        "\n\rSignal Code: %d",
                        asctime (timeinfo),
                        cmdName,
                        getpid(),
                        info->si_addr,
                        sig,
                        info->si_code);
    printf("\n\r\n\rThe cmd line is :%s.", cmdLine );
    printf("\n\rThe latest Line number is:%d.\n\r", runSteps);

#if (__linux__ && __GLIBC__ && !__UCLIBC__) || __APPLE__
    int size, i;
    void *addresses[BT_BUF_SIZE];
    size = backtrace(addresses, BT_BUF_SIZE);
    backtrace_symbols_fd(addresses,size,STDOUT_FILENO);
    if(size >= 3) {
        printf("backtrace returned: %d\n\r", size);
        for(i = 0; i < size; i++) {
            printf("\n\r%d: %p \n\r", i, addresses[i]);
        }
    }
#endif

    printf("\n\r!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    printf("\n\r!!!!!!!!!!!!!!!!!!!!!!!!!! Dump Ending!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" );
    printf("\n\r!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\r");
    exit(0);
}

static void enable_exception_handlers (void)
{
    struct sigaction sigact;

    memset( &sigact, 0, sizeof( struct sigaction ) );
    sigact.sa_sigaction = (void*)exception_handler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGSEGV, &sigact, 0L);
    sigaction(SIGILL,  &sigact, 0L);
    sigaction(SIGBUS,  &sigact, 0L);
    sigaction(SIGQUIT, &sigact, 0L);

    return;
}

int main( int argc, char *argv[] )
{
    if( argc < 2 )
    {
        show_menu(NULL);
        return 0;
    }
    runSteps = __LINE__;
    enable_exception_handlers();
    /* Is interactive */
    if (strcmp (argv[1], "-i") == 0)
    {
        char *line = NULL;
        char *interArgv[RBUS_CLI_MAX_CMD_ARG] = {NULL};
        int interArgc = 0;
        int isExit = 0;

        g_isInteractive = true;
        runSteps = __LINE__;

        rbus_registerLogHandler(rbus_log_handler);

        linenoiseSetCompletionCallback(completion);
        linenoiseSetHintsCallback(hints);
        linenoiseHistoryLoad("/tmp/rbuscli_history");

        while(!isExit && (line = linenoise("rbuscli> ")) != NULL)
        {
            if (line[0] != '\0')
            {
                linenoiseHistoryAdd(line);

                memset(interArgv, 0, sizeof(interArgv));
                if(construct_input_into_cmds(line, &interArgc, interArgv) == 0)
                {
                    int i;
                    isExit = handle_cmds (interArgc, interArgv);
                    for(i = 1; i < interArgc; ++i)
                        if(interArgv[i])
                            free(interArgv[i]);
                }
                else
                    printf("Command missing quotes\n\r");
            }
            runSteps = __LINE__;
            linenoiseFree(line);
        }
        linenoiseHistorySave("/tmp/rbuscli_history");
    }
    else
    {
        if ((strcmp (argv[1], "-g") == 0) || (strcmp (argv[1], "-s") == 0))
        {
            g_isDebug = false;
        }
        handle_cmds (argc, argv);
    }

    /* Close the handle if it is not interative */
    if (gDirectHandlesHash)
    {
        runSteps = __LINE__;
        rtHashMap_Destroy(gDirectHandlesHash);
    }

    if (g_busHandle)
    {
        runSteps = __LINE__;
        rbus_close(g_busHandle);
        g_busHandle = 0;
    }

    if (g_registeredProps)
    {
        runSteps = __LINE__;
        rtList_Destroy(g_registeredProps, free_registered_property);
    }

    if (g_subsribeUserData)
    {
        runSteps = __LINE__;
        rtList_Destroy(g_subsribeUserData, free_userdata);
    }

    if (g_messageUserData)
    {
        runSteps = __LINE__;
        rtList_Destroy(g_messageUserData, free_userdata);
    }

    return 0;
}
