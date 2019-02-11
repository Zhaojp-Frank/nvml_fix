#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

/////////////////color: printf(RED""NONE,va1);
#define NONE                 "\e[0m"
#define RED                  "\e[0;31m"
#define GREEN                "\e[0;32m"
#define LGREEN               "\e[1;32m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define LBLUE                "\e[1;34m"
#define MAG										"\e[0;35m"
#define CYAN									"\e[0;36m"
#define WHITE                "\e[1;37m"

#if defined(NVML_PATCH_319) || defined(NVML_PATCH_325) || defined(NVML_PATCH_331)
#include "nvml.h"
#elif defined(NVML_PATCH_390) || defined(NVML_PATCH_396) || defined(NVML_PATCH_410)
#include <nvml.h>
#else
#error "No valid NVML_PATCH_* option specified! Currently supported versions are: 319, 325, 331 (x86_64 only), 390 (x86_64 only), 396 (x86_64 only), 410 (x86_64 only)."
#endif

#define FUNC(f) static typeof(f) * real_##f;
#define FUNC_v2(f) static typeof(f) * real_##f##_v2;

#if defined(NVML_PATCH_319) || defined(NVML_PATCH_325) || defined(NVML_PATCH_331)
FUNC(nvmlInit)
FUNC(nvmlDeviceGetHandleByIndex)
FUNC(nvmlDeviceGetHandleByPciBusId)
#elif defined(NVML_PATCH_390) || defined(NVML_PATCH_396) || defined(NVML_PATCH_410)
FUNC(nvmlInitWithFlags);
#endif
FUNC_v2(nvmlInit)
FUNC_v2(nvmlDeviceGetHandleByIndex)
FUNC(nvmlDeviceGetHandleBySerial)
FUNC(nvmlDeviceGetHandleByUUID)
FUNC_v2(nvmlDeviceGetHandleByPciBusId)

///////////////////frank: step1. declare func ptr here ONLY IF customized needed
FUNC(nvmlDeviceGetName);
FUNC_v2(nvmlDeviceGetCount);

///////////////////////////////////////////////////////////////////////
#define LOAD(f) if (!(real_##f = dlsym(nvml, #f))) return NVML_ERROR_UNKNOWN
#if defined(NVML_PATCH_319) || defined(NVML_PATCH_325) || defined(NVML_PATCH_331)
#define INIT(name) nvmlReturn_t name() \
{ \
	void *nvml = dlopen("/usr/lib/x86_64-linux-gnu/libnvidia-ml.so." NVML_VERSION, RTLD_NOW); \
\
	LOAD(nvmlInit); \
	LOAD(nvmlInit_v2); \
	LOAD(nvmlDeviceGetHandleByIndex); \
	LOAD(nvmlDeviceGetHandleByIndex_v2); \
	LOAD(nvmlDeviceGetHandleBySerial); \
	LOAD(nvmlDeviceGetHandleByUUID); \
	LOAD(nvmlDeviceGetHandleByPciBusId); \
	LOAD(nvmlDeviceGetHandleByPciBusId_v2); \
\
	return real_##name(); \
}
#elif defined(NVML_PATCH_390) || defined(NVML_PATCH_396) || defined(NVML_PATCH_410)
#define INIT(name) nvmlReturn_t name() \
{ \
void *nvml = dlopen("/usr/lib/x86_64-linux-gnu/libnvidia-ml.so." NVML_VERSION, RTLD_NOW); \
\
	LOAD(nvmlInit_v2); \
	LOAD(nvmlInitWithFlags); \
	LOAD(nvmlDeviceGetHandleByIndex_v2); \
	LOAD(nvmlDeviceGetHandleBySerial); \
	LOAD(nvmlDeviceGetHandleByUUID); \
	LOAD(nvmlDeviceGetHandleByPciBusId_v2); \
\
  /*//////////////////frank: 2.1 set customized func ptr here (optional? as nvmlInit_v2 would call nvmlInitWithFlags */ \
	printf("shim-%s....11\n", __FUNCTION__); \
	return real_##name(); \
}
#endif

#if defined(NVML_PATCH_319) || defined(NVML_PATCH_325) || defined(NVML_PATCH_331)
INIT(nvmlInit)
#endif
INIT(nvmlInit_v2)

#if defined(NVML_PATCH_390) || defined(NVML_PATCH_396) || defined(NVML_PATCH_410)
nvmlReturn_t nvmlInitWithFlags(unsigned int flags) {
	void *nvml = dlopen("/usr/lib/x86_64-linux-gnu/libnvidia-ml.so." NVML_VERSION, RTLD_NOW);
	LOAD(nvmlInit_v2);
	LOAD(nvmlInitWithFlags);
	LOAD(nvmlDeviceGetHandleByIndex_v2);
	LOAD(nvmlDeviceGetHandleBySerial);
	LOAD(nvmlDeviceGetHandleByUUID);
	LOAD(nvmlDeviceGetHandleByPciBusId_v2);

  //////////////////frank: Step2.2, set func ptr here for customized func ptr here
	printf(RED"Frank load customized NVML .... done!\n"NONE, __FUNCTION__);fflush(stdout);
	LOAD(nvmlDeviceGetCount_v2);
	LOAD(nvmlDeviceGetName);


	//return real_nvmlInitWithFlags(flags);
	real_nvmlInitWithFlags(flags);

	///////////test
	unsigned int cnt;
	real_nvmlDeviceGetCount_v2(&cnt);
  nvmlDevice_t device;
	char name[64];
  real_nvmlDeviceGetHandleByIndex_v2(0, &device);
	real_nvmlDeviceGetName(device, name, 64);
	printf(BLUE"pGPUCnt:%d name:%s -> customize as vGPU 1060\n\n"NONE, cnt, name);fflush(stdout);
	return NVML_SUCCESS; 
}
#endif

void fix_unsupported_bug(nvmlDevice_t device)
{
	unsigned int *fix = (unsigned int *)device;
#if defined(NVML_PATCH_319) || defined(NVML_PATCH_325)
# ifdef __i386__
	fix[201] = 1;
	fix[202] = 1;
# else
	fix[202] = 1;
	fix[203] = 1;
# endif
#elif defined(NVML_PATCH_331)
# ifdef __i386__
#  error "No i386 support for this version yet!"
# else
	fix[187] = 2;
	fix[188] = 1;
# endif
#elif defined(NVML_PATCH_390) || defined(NVML_PATCH_396)
# ifdef __i386__
#  error "No i386 support for this version yet!"
# else
	fix[352] = 1;
	fix[353] = 1;
# endif
#elif defined(NVML_PATCH_410)
# ifdef __i386__
#  error "No i386 support for this version yet!"
# else
	fix[362] = 1;
	fix[363] = 1;
# endif
#endif
}

#define GET_HANDLE_BY(name, type) \
nvmlReturn_t nvmlDeviceGetHandleBy##name(type x, nvmlDevice_t *device) \
{ \
	nvmlReturn_t ret; \
\
	if (!real_nvmlDeviceGetHandleBy##name) \
		return NVML_ERROR_UNINITIALIZED; \
\
	ret = real_nvmlDeviceGetHandleBy##name(x, device); \
	if (ret != NVML_SUCCESS) \
		return ret; \
\
	fix_unsupported_bug(*device); \
	return NVML_SUCCESS; \
}

#if defined(NVML_PATCH_319) || defined(NVML_PATCH_325) || defined(NVML_PATCH_331)
GET_HANDLE_BY(Index, unsigned int)
GET_HANDLE_BY(PciBusId, const char *)
#endif
GET_HANDLE_BY(Index_v2, unsigned int)
GET_HANDLE_BY(Serial, const char *)
GET_HANDLE_BY(UUID, const char *)
GET_HANDLE_BY(PciBusId_v2, const char *)


//////////////////frank: step3: customize any func ONLY IF needed here 
nvmlReturn_t DECLDIR nvmlDeviceGetCount_v2(unsigned int *deviceCount)
{
	real_nvmlDeviceGetCount_v2(deviceCount);
	//printf("->pGPUCnt:%d \n", *deviceCount);
	//*deviceCount = 4;
	return NVML_SUCCESS; 
}

nvmlReturn_t DECLDIR nvmlDeviceGetName(nvmlDevice_t device, char *name, unsigned int length) 
{
	unsigned int cnt;
	real_nvmlDeviceGetCount_v2(&cnt);
	real_nvmlDeviceGetName(device, name, length);
	//printf(BLUE" pGPUCnt:%d name:%s -> customize as vGPU 1060\n"NONE, cnt, name);
	strcpy(name, "Frank-vGPU 1060 Max-Q");
	return NVML_SUCCESS; 
}
