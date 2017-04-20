#include "ccsp_custom.h"
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"

extern void* g_vBus_handle;

#define CCSP_SUBSYS     "eRT."
#define PSM_VALUE_GET_STRING(name, str) PSM_Get_Record_Value2(g_vBus_handle, CCSP_SUBSYS, name, NULL, &(str))
#define PSM_VALUE_GET_INS(name, pIns, ppInsArry) PsmGetNextLevelInstances(g_vBus_handle, CCSP_SUBSYS, name, pIns, ppInsArry)

void bring_lan_up();
void ipv4_status(int, char *);
