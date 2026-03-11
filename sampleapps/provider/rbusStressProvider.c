#include <stdio.h>
#include <stdlib.h>
#include <rbus.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

static int g_keep_running = 1;
void signal_handler(int sig)
{
    g_keep_running = 0;
}

static rbusError_t getHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    rbusValue_t v;
    rbusValue_Init(&v);
    rbusValue_SetString(v, "stress_val");
    rbusProperty_SetValue(property, v);
    rbusValue_Release(v);
    return RBUS_ERROR_SUCCESS;
}

int main(int argc, char** argv)
{
    rbusHandle_t handle;
    rbusError_t err;
    char compName[64];
    
    srand(time(NULL) ^ getpid());
    sprintf(compName, "StressProvider_%d", getpid());

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    err = rbus_open(&handle, compName);
    if(err != RBUS_ERROR_SUCCESS) return 1;

    if (argc > 1 && strcmp(argv[1], "dynamic") == 0)
    {
        // Dynamic mode: Register, wait, unregister in a loop
        int iterations = (argc > 2) ? atoi(argv[2]) : 10;
        char path[256];
        
        for (int i = 0; i < iterations && g_keep_running; i++)
        {
            sprintf(path, "Device.Stress.Table.%d.Param", getpid() * 100 + i);
            
            rbusDataElement_t el[2];
            memset(el, 0, sizeof(el));
            
            char obj_path[256];
            strcpy(obj_path, path);
            char* last_dot = strrchr(obj_path, '.');
            if(last_dot) { *last_dot = '\0'; strcat(obj_path, "."); }
            
            el[0].name = obj_path;
            el[0].type = RBUS_ELEMENT_TYPE_TABLE;
            el[1].name = path;
            el[1].type = RBUS_ELEMENT_TYPE_PROPERTY;
            el[1].cbTable.getHandler = getHandler;

            rbus_regDataElements(handle, 2, el);
            usleep(100000); // 100ms
            rbus_unregDataElements(handle, 2, el);
            usleep(50000);  // 50ms
        }
    }
    else if (argc > 1 && strcmp(argv[1], "multi") == 0)
    {
        // Multi-op mode: Add, Delete, ValueChange randomly
        int iterations = (argc > 2) ? atoi(argv[2]) : 50;
        char path[256];
        int active_indices[100];
        int num_active = 0;
        memset(active_indices, -1, sizeof(active_indices));

        for (int i = 0; i < iterations && g_keep_running; i++)
        {
            int op = rand() % 3; // 0=Add, 1=ValueChange, 2=Delete
            if (num_active == 0) op = 0; // Force add if none
            if (num_active > 30) op = 2; // Force delete if too many

            if (op == 0) // ADD
            {
                int index = rand() % 1000 + (getpid() % 100) * 1000;
                sprintf(path, "Device.Stress.Multi.%d.Param", index);
                
                rbusDataElement_t el[2];
                memset(el, 0, sizeof(el));
                char obj_path[256]; strcpy(obj_path, path);
                char* last_dot = strrchr(obj_path, '.');
                if(last_dot) { *last_dot = '\0'; strcat(obj_path, "."); }
                el[0].name = obj_path; el[0].type = RBUS_ELEMENT_TYPE_TABLE;
                el[1].name = path; el[1].type = RBUS_ELEMENT_TYPE_PROPERTY;
                el[1].cbTable.getHandler = getHandler;

                rbus_regDataElements(handle, 2, el);
                active_indices[num_active++] = index;
            }
            else if (op == 2) // DELETE
            {
                int idx_to_del = rand() % num_active;
                int index = active_indices[idx_to_del];
                sprintf(path, "Device.Stress.Multi.%d.Param", index);

                rbusDataElement_t el[2];
                memset(el, 0, sizeof(el));
                char obj_path[256]; strcpy(obj_path, path);
                char* last_dot = strrchr(obj_path, '.');
                if(last_dot) { *last_dot = '\0'; strcat(obj_path, "."); }
                el[0].name = obj_path; el[0].type = RBUS_ELEMENT_TYPE_TABLE;
                el[1].name = path; el[1].type = RBUS_ELEMENT_TYPE_PROPERTY;

                rbus_unregDataElements(handle, 2, el);
                active_indices[idx_to_del] = active_indices[num_active - 1];
                num_active--;
            }
            usleep(rand() % 100000 + 50000); // 50-150ms
        }
        
        // FINAL CLEANUP
        printf("MULTI: Final cleanup of %d indices\n", num_active);
        for(int i=0; i<num_active; i++) {
             sprintf(path, "Device.Stress.Multi.%d.Param", active_indices[i]);
             rbusDataElement_t el[2];
             memset(el, 0, sizeof(el));
             char obj_path[256]; strcpy(obj_path, path);
             char* last_dot = strrchr(obj_path, '.');
             if(last_dot) { *last_dot = '\0'; strcat(obj_path, "."); }
             el[0].name = obj_path; el[0].type = RBUS_ELEMENT_TYPE_TABLE;
             el[1].name = path; el[1].type = RBUS_ELEMENT_TYPE_PROPERTY;
             rbus_unregDataElements(handle, 2, el);
        }
    }

    rbus_close(handle);
    return 0;
}
