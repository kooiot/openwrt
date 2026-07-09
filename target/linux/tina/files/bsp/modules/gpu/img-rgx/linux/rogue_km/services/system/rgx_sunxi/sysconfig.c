/*************************************************************************/ /*!
@File
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "pvrsrv_device.h"
#include "syscommon.h"
#include "vz_vmm_pvz.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#if defined(LINUX)
#include <linux/dma-mapping.h>
#endif
#include "rgx_bvnc_defs_km.h"

#include <linux/clk.h>
#include <linux/platform_device.h>

#include "interrupt_support.h"
#include "platform.h"

static PVRSRV_DEVICE_CONFIG     gsDevices[1];

/*
 * In systems that support trusted device address protection, there are three
 * physical heaps from which pages should be allocated:
 * - one heap for normal allocations
 * - one heap for allocations holding META code memory
 * - one heap for allocations holding secured DRM data
 */

#define PHYS_HEAP_IDX_GENERAL     0
#define PHYS_HEAP_IDX_FW          1
#if defined(SUPPORT_TRUSTED_DEVICE)
#define PHYS_HEAP_IDX_TDFWCODE    2
#define PHYS_HEAP_IDX_TDSECUREBUF 3
#elif defined(SUPPORT_DEDICATED_FW_MEMORY)
#define PHYS_HEAP_IDX_FW_MEMORY   2
#endif

/*
	CPU to Device physical address translation
*/
static
void UMAPhysHeapCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
								   IMG_UINT32 ui32NumOfAddr,
								   IMG_DEV_PHYADDR *psDevPAddr,
								   IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
		}
	}
}

/*
	Device to CPU physical address translation
*/
static
void UMAPhysHeapDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
								   IMG_UINT32 ui32NumOfAddr,
								   IMG_CPU_PHYADDR *psCpuPAddr,
								   IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = psDevPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr;
		}
	}
}

static PHYS_HEAP_FUNCTIONS gsPhysHeapFuncs =
{
	/* pfnCpuPAddrToDevPAddr */
	UMAPhysHeapCpuPAddrToDevPAddr,
	/* pfnDevPAddrToCpuPAddr */
	UMAPhysHeapDevPAddrToCpuPAddr,
};

static PVRSRV_ERROR PhysHeapsCreate(PHYS_HEAP_CONFIG **ppasPhysHeapsOut,
									IMG_UINT32 *puiPhysHeapCountOut)
{
	/*
	 * This function is called during device initialisation, which on Linux,
	 * means it won't be called concurrently. As such, there's no need to
	 * protect it with a lock or use an atomic variable.
	 */
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	IMG_UINT32 uiHeapCount = 2;

#if defined(SUPPORT_TRUSTED_DEVICE)
	uiHeapCount += 2;
#elif defined(SUPPORT_DEDICATED_FW_MEMORY)
	uiHeapCount += 1;
#endif

	pasPhysHeaps = OSAllocZMem(sizeof(*pasPhysHeaps) * uiHeapCount);
	if (!pasPhysHeaps)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].pszPDumpMemspaceName = "SYSMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_GENERAL].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;

	pasPhysHeaps[PHYS_HEAP_IDX_FW].pszPDumpMemspaceName = "SYSMEM_FW";
	pasPhysHeaps[PHYS_HEAP_IDX_FW].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_FW].psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_FW].ui32UsageFlags = PHYS_HEAP_USAGE_FW_SHARED;

#if defined(SUPPORT_TRUSTED_DEVICE)
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].pszPDumpMemspaceName = "TDFWMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_TDFWMEM].ui32UsageFlags =
		PHYS_HEAP_USAGE_FW_CODE | PHYS_HEAP_USAGE_FW_PRIV_DATA;

	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].pszPDumpMemspaceName = "TDSECBUFMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_TDSECUREBUF].ui32UsageFlags =
		PHYS_HEAP_USAGE_GPU_SECURE;

#elif defined(SUPPORT_DEDICATED_FW_MEMORY)
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].pszPDumpMemspaceName = "DEDICATEDFWMEM";
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].eType = PHYS_HEAP_TYPE_UMA;
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].psMemFuncs = &gsPhysHeapFuncs;
	pasPhysHeaps[PHYS_HEAP_IDX_FW_MEMORY].ui32UsageFlags =
		PHYS_HEAP_USAGE_FW_CODE | PHYS_HEAP_USAGE_FW_PRIV_DATA;
#endif

	*ppasPhysHeapsOut = pasPhysHeaps;
	*puiPhysHeapCountOut = uiHeapCount;

	return PVRSRV_OK;
}

static void PhysHeapsDestroy(PHYS_HEAP_CONFIG *pasPhysHeaps)
{
	OSFreeMem(pasPhysHeaps);
}

static void SysDevFeatureDepInit(PVRSRV_DEVICE_CONFIG *psDevConfig, IMG_UINT64 ui64Features)
{
#if defined(SUPPORT_AXI_ACE_TEST)
		if( ui64Features & RGX_FEATURE_AXI_ACELITE_BIT_MASK)
		{
			psDevConfig->eCacheSnoopingMode		= PVRSRV_DEVICE_SNOOP_CPU_ONLY;
		}else
#endif
		{
			psDevConfig->eCacheSnoopingMode		= PVRSRV_DEVICE_SNOOP_NONE;
		}
}

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig;
	RGX_DATA *psRGXData;
	RGX_TIMING_INFORMATION *psRGXTimingInfo;
	PHYS_HEAP_CONFIG *pasPhysHeaps;
	IMG_UINT32 uiPhysHeapCount;
	PVRSRV_ERROR eError;
	struct device *dev = (struct device *)pvOSDevice;
	struct sunxi_platform *sunxi_data;

	if (sunxi_platform_init(dev))
		return PVRSRV_ERROR_INIT_FAILURE;

	sunxi_data = (struct sunxi_platform *)dev->platform_data;

#if defined(LINUX)
#if defined(CONFIG_64BIT)
	dma_set_mask(pvOSDevice, DMA_BIT_MASK(48));
#else
	dma_set_mask(pvOSDevice, DMA_BIT_MASK(32));
#endif
#endif

	psDevConfig = OSAllocZMem(sizeof(*psDevConfig) +
							  sizeof(*psRGXData) +
							  sizeof(*psRGXTimingInfo));
	if (!psDevConfig)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psRGXData = (RGX_DATA *)((IMG_CHAR *)psDevConfig + sizeof(*psDevConfig));
	psRGXTimingInfo = (RGX_TIMING_INFORMATION *)((IMG_CHAR *)psRGXData + sizeof(*psRGXData));

	eError = PhysHeapsCreate(&pasPhysHeaps, &uiPhysHeapCount);
	if (eError)
	{
		goto ErrorFreeDevConfig;
	}

	/* Setup RGX specific timing data */
	psRGXTimingInfo->ui32CoreClockSpeed        = clk_get_rate(sunxi_data->clks.core);
	if (sunxi_ic_version_ctrl(dev)) {
		psRGXTimingInfo->bEnableActivePM           = IMG_TRUE;
	} else {
		psRGXTimingInfo->bEnableActivePM           = IMG_FALSE;
	}
	psRGXTimingInfo->bEnableRDPowIsland        = IMG_TRUE;
	psRGXTimingInfo->ui32ActivePMLatencyms     = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/* Set up the RGX data */
	psRGXData->psRGXTimingInfo = psRGXTimingInfo;

#if defined(SUPPORT_TRUSTED_DEVICE)
	psRGXData->bHasTDFWCodePhysHeap = IMG_TRUE;
	psRGXData->uiTDFWCodePhysHeapID = PHYS_HEAP_IDX_TDFWCODE;

	psRGXData->bHasTDSecureBufPhysHeap = IMG_TRUE;
	psRGXData->uiTDSecureBufPhysHeapID = PHYS_HEAP_IDX_TDSECUREBUF;

#elif defined (SUPPORT_DEDICATED_FW_MEMORY)
	psRGXData->bHasFWMemPhysHeap = IMG_TRUE;
	psRGXData->uiFWMemPhysHeapID = PHYS_HEAP_IDX_FW_MEMORY;
#endif

	/* Setup the device config */
	psDevConfig->pvOSDevice				= pvOSDevice;
	psDevConfig->pszName                = "sunxi";
	psDevConfig->pszVersion             = NULL;
	psDevConfig->pfnSysDevFeatureDepInit = SysDevFeatureDepInit;

	/* Device setup information */
	psDevConfig->sRegsCpuPBase.uiAddr   = sunxi_data->reg_base;
	psDevConfig->ui32RegsSize           = sunxi_data->reg_size;
	psDevConfig->ui32IRQ                = sunxi_data->irq_num;

	psDevConfig->pasPhysHeaps			= pasPhysHeaps;
	psDevConfig->ui32PhysHeapCount		= uiPhysHeapCount;
	psDevConfig->eDefaultHeap = PVRSRV_PHYS_HEAP_GPU_LOCAL;


	/* No power management on no HW system */
	psDevConfig->pfnPrePowerState       = sunxiPrePowerState;
	psDevConfig->pfnPostPowerState      = sunxiPostPowerState;

	/* No clock frequency either */
	psDevConfig->pfnClockFreqGet        = NULL;

	psDevConfig->hDevData               = psRGXData;
	psDevConfig->hSysData               = sunxi_data;

#if defined(PVR_DVFS)
	gsDevices[0].sDVFS.sDVFSDeviceCfg.ui32PollMs = 100;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.bIdleReq = IMG_FALSE;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pfnSetFrequency = sunxiSetFrequencyDVFS;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pfnSetVoltage = sunxiSetVoltageDVFS;

	gsDevices[0].sDVFS.sDVFSGovernorCfg.ui32UpThreshold = 90;
	gsDevices[0].sDVFS.sDVFSGovernorCfg.ui32DownDifferential = 10;
#endif
	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	*ppsDevConfig = psDevConfig;
	sunxi_data->config = &gsDevices[0];

	return PVRSRV_OK;

ErrorFreeDevConfig:
	OSFreeMem(psDevConfig);
	return eError;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
#if defined(SUPPORT_ION)
	IonDeinit();
#endif

	sunxi_platform_term();
	PhysHeapsDestroy(psDevConfig->pasPhysHeaps);
	OSFreeMem(psDevConfig);
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
								  IMG_UINT32 ui32IRQ,
								  const IMG_CHAR *pszName,
								  PFN_LISR pfnLISR,
								  void *pvData,
								  IMG_HANDLE *phLISRData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);
	return OSInstallSystemLISR(phLISRData, ui32IRQ, pszName, pfnLISR, pvData,
								SYS_IRQ_FLAG_TRIGGER_DEFAULT);
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return OSUninstallSystemLISR(hLISRData);
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	PVR_UNREFERENCED_PARAMETER(pvDumpDebugFile);
	return PVRSRV_OK;
}

/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/
