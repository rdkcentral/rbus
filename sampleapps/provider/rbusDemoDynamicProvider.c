/**
 * @file rbusDemoDynamicProvider.c
 * @brief Demo provider for testing dynamic data model registration and NotifyDML integration.
 * 
 * This sample allows registering any arbitrary TR-181 path as a property.
 * It uses standard R-Bus APIs to demonstrate how the NotifyDML manager in the
 * USP Agent detects new elements via the global discovery signal.
 * 
 * Usage: rbusDemoDynamicProvider <path> <initial_value>
 * Examples:
 *   rbusDemoDynamicProvider Device.Service.MyCustomObj.Param1 Hello
 * 
 * Signals:
 *   SIGUSR1: Updates the value of the registered parameter to 'updated'.
 *   SIGUSR2: Unregisters the parameter and exits gracefully.
 */

#include <rbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static int gRunning = 1;
static char* gPath = NULL;
static char* gValue = NULL;
static rbusHandle_t gHandle = NULL;

/**
 * @brief Signal handler to simulate dynamic events.
 */
void signalHandler(int sig)
{
    if(sig == SIGUSR1) // Update value
    {
       printf("Provider: Updating value for %s to 'updated'\n", gPath);
       rbus_setStr(gHandle, gPath, "updated");
    }
    else if(sig == SIGUSR2) // Unregister and exit
    {
       printf("Provider: Unregistering and exiting...\n");
       gRunning = 0;
    }
    else if(sig == SIGINT || sig == SIGTERM)
    {
       gRunning = 0;
    }
}

static rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* options)
{
    (void)handle; (void)options;
    rbusValue_t v;
    rbusValue_Init(&v);
    rbusValue_SetString(v, gValue);
    rbusProperty_SetValue(property, v);
    rbusValue_Release(v);
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char** argv)
{
    if(argc < 3) {
        printf("Usage: %s <path> <initial_value>\n", argv[0]);
        return 1;
    }
    gPath = argv[1];
    gValue = strdup(argv[2]);

    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if(rbus_open(&gHandle, "DemoDynamicProvider") != RBUS_ERROR_SUCCESS) {
        printf("rbus_open failed\n");
        return 1;
    }

    rbusDataElement_t elem = {0};
    elem.name = gPath;
    elem.type = RBUS_ELEMENT_TYPE_PROPERTY;
    elem.cbTable.getHandler = getHandler;

    if(rbus_regDataElements(gHandle, 1, &elem) != RBUS_ERROR_SUCCESS) {
        printf("rbus_regDataElements failed for %s\n", gPath);
        rbus_close(gHandle);
        return 2;
    }

    printf("Provider: Registered %s with initial value '%s'. PID: %d\n", gPath, gValue, getpid());
    printf("  Send SIGUSR1 to update value, SIGUSR2 to unregister.\n");

    while(gRunning) sleep(1);

    rbus_unregDataElements(gHandle, 1, &elem);
    rbus_close(gHandle);
    printf("Provider: Cleaned up.\n");
    return 0;
}
