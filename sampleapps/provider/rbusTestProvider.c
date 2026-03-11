#include <stdio.h>
#include <stdlib.h>
#include <rbus.h>
#include <unistd.h>
#include <string.h>

static char g_val[256] = "N/A";

static rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    rbusValue_t v;
    rbusValue_Init(&v);
    rbusValue_SetString(v, g_val);
    rbusProperty_SetValue(property, v);
    rbusValue_Release(v);
    return RBUS_ERROR_SUCCESS;
}

#include <signal.h>

static int g_keep_running = 1;
void signal_handler(int sig)
{
    g_keep_running = 0;
}

int main(int argc, char** argv)
{
    rbusHandle_t handle;
    rbusError_t err;
    if(argc < 3) return 1;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    char component_name[64];
    sprintf(component_name, "TestProvider_%d", getpid());
    
    err = rbus_open(&handle, component_name);
    if(err != RBUS_ERROR_SUCCESS) return 1;

    strncpy(g_val, argv[2], 255);
    g_val[255] = '\0';

    rbusDataElement_t el[1];
    memset(el, 0, sizeof(el));
    
    el[0].name = argv[1];
    el[0].type = RBUS_ELEMENT_TYPE_PROPERTY;
    el[0].cbTable.getHandler = getHandler;
    
    printf("Registering property %s\n", el[0].name);

    err = rbus_regDataElements(handle, 1, el);
    if(err != RBUS_ERROR_SUCCESS) return 1;

    while(g_keep_running) sleep(1);
    
    printf("rbusTestProvider: Cleaning up...\n");
    rbus_unregDataElements(handle, 1, el);
    rbus_close(handle);
    return 0;
}
