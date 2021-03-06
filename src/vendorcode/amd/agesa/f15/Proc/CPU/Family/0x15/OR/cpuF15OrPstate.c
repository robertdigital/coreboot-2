/* $NoKeywords:$ */
/**
 * @file
 *
 * AMD Family_15 Orochi Pstate feature support functions.
 *
 * Provides the functions necessary to initialize the Pstate feature.
 *
 * @xrefitem bom "File Content Label" "Release Content"
 * @e project:      AGESA
 * @e sub-project:  CPU/Family/0x15/OR
 * @e \$Revision: 55600 $   @e \$Date: 2011-06-23 12:39:18 -0600 (Thu, 23 Jun 2011) $
 *
 */
/*
 ******************************************************************************
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Advanced Micro Devices, Inc. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/*----------------------------------------------------------------------------------------
 *                             M O D U L E S    U S E D
 *----------------------------------------------------------------------------------------
 */
#include "AGESA.h"
#include "amdlib.h"
#include "GeneralServices.h"
#include "cpuPstateTables.h"
#include "cpuRegisters.h"
#include "Table.h"
#include "cpuFamilyTranslation.h"
#include "cpuFamRegisters.h"
#include "cpuF15Utilities.h"
#include "cpuF15PowerMgmt.h"
#include "cpuF15OrPowerMgmt.h"
#include "OptionMultiSocket.h"
#include "Filecode.h"
CODE_GROUP (G3_DXE)
RDATA_GROUP (G3_DXE)

#define FILECODE PROC_CPU_FAMILY_0X15_OR_CPUF15ORPSTATE_FILECODE


/*----------------------------------------------------------------------------------------
 *                   D E F I N I T I O N S    A N D    M A C R O S
 *----------------------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------------------
 *                  T Y P E D E F S     A N D     S T R U C T U R E S
 *----------------------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------------------
 *           P R O T O T Y P E S     O F     L O C A L     F U N C T I O N S
 *----------------------------------------------------------------------------------------
 */
VOID
STATIC
F15OrGetPowerStepValueInTime (
  IN OUT   UINT32  *PowerStepPtr
  );

VOID
STATIC
F15OrGetPllValueInTime (
  IN OUT   UINT32  *PllLockTimePtr
  );

AGESA_STATUS
STATIC
F15OrGetFrequencyXlatRegInfo (
  IN       PSTATE_CPU_FAMILY_SERVICES  *PstateCpuServices,
  IN       UINT8                  PStateNumber,
  IN       UINT32                 Frequency,
     OUT   UINT32                 *CpuFidPtr,
     OUT   UINT32                 *CpuDidPtr1,
     OUT   UINT32                 *CpuDidPtr2,
  IN       AMD_CONFIG_PARAMS      *StdHeader
  );

AGESA_STATUS
F15OrGetPstateTransLatency (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       PSTATE_LEVELING        *PStateLevelingBufferStructPtr,
  IN       PCI_ADDR               *PciAddress,
     OUT   UINT32                 *TransitionLatency,
  IN       AMD_CONFIG_PARAMS      *StdHeader
  );

AGESA_STATUS
F15OrGetPstateFrequency (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       UINT8 StateNumber,
     OUT   UINT32 *FrequencyInMHz,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

AGESA_STATUS
F15OrPstateLevelingCoreMsrModify (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       S_CPU_AMD_PSTATE   *CpuAmdPState,
  IN       AMD_CONFIG_PARAMS  *StdHeader
  );

AGESA_STATUS
F15OrGetPstatePower (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       UINT8 StateNumber,
     OUT   UINT32 *PowerInMw,
  IN       AMD_CONFIG_PARAMS *StdHeader
  );

AGESA_STATUS
F15OrGetPstateMaxState (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
     OUT   UINT32              *MaxPStateNumber,
     OUT   UINT8               *NumberOfBoostStates,
  IN       AMD_CONFIG_PARAMS   *StdHeader
  );

AGESA_STATUS
F15OrGetPstateRegisterInfo (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       UINT32              PState,
     OUT   BOOLEAN             *PStateEnabled,
  IN OUT   UINT32              *IddVal,
  IN OUT   UINT32              *IddDiv,
     OUT   UINT32              *SwPstateNumber,
  IN       AMD_CONFIG_PARAMS   *StdHeader
  );



/*----------------------------------------------------------------------------------------
 *                          E X P O R T E D    F U N C T I O N S
 *----------------------------------------------------------------------------------------
 */
extern OPTION_MULTISOCKET_CONFIGURATION OptionMultiSocketConfiguration;

/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to check if PSD need to be generated.
 *
 * @param[in]     PstateCpuServices  Pstate CPU services.
 * @param[in,out] PlatformConfig     Contains the runtime modifiable feature input data.
 * @param[in]     StdHeader          Config Handle for library, services.
 *
 * @retval       TRUE               PSD need to be generated
 * @retval       FALSE              PSD does NOT need to be generated
 *
 */
BOOLEAN
STATIC
F15OrIsPstatePsdNeeded (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN OUT   PLATFORM_CONFIGURATION *PlatformConfig,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32             LocalPciRegister;
  PCI_ADDR           PciAddress;
  PLATFORM_FEATS     Features;

  // Initialize the union
  Features.PlatformValue = 0;
  GetPlatformFeatures (&Features, PlatformConfig, StdHeader);

  //
  // For Single link processor, PSD needs to be generated
  // For other processor, if D18F5x80[DualCore][0]=0, the _PSD object does not need to be generated.
  //
  OptionMultiSocketConfiguration.GetCurrPciAddr (&PciAddress, StdHeader);
  PciAddress.Address.Register = COMPUTE_UNIT_STATUS;
  PciAddress.Address.Function = FUNC_5;
  LibAmdPciRead (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader);
  if ((!Features.PlatformFeatures.PlatformSingleLink) && ((LocalPciRegister & 0x10000) == 0)) {
    return FALSE;
  }
  return TRUE;
}


/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to check if Pstate PSD is dependent.
 *
 * @param[in]     PstateCpuServices  Pstate CPU services.
 * @param[in,out] PlatformConfig     Contains the runtime modifiable feature input data.
 * @param[in]     StdHeader          Config Handle for library, services.
 *
 * @retval       TRUE               PSD is dependent.
 * @retval       FALSE              PSD is independent.
 *
 */
BOOLEAN
STATIC
F15OrIsPstatePsdDependent (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN OUT   PLATFORM_CONFIGURATION *PlatformConfig,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  PLATFORM_FEATS         Features;

  // Initialize the union
  Features.PlatformValue = 0;
  GetPlatformFeatures (&Features, PlatformConfig, StdHeader);

  //
  // For Single link has PSD option, default is dependent.
  // If multi-link, always return independent.
  //
  if (Features.PlatformFeatures.PlatformSingleLink) {
    if (PlatformConfig->ForcePstateIndependent) {
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/**
 *  Family specific call to set core TscFreqSel.
 *
 * @param[in]     PstateCpuServices  Pstate CPU services.
 * @param[in]     StdHeader          Config Handle for library, services.
 *
 */
VOID
STATIC
F15OrSetTscFreqSel (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  //TscFreqSel: TSC frequency select. Read-only. Reset: 1. 1=The TSC increments at the P0 frequency.
  //This field uses software P-state numbering.
  return;
}

/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to get Pstate Transition Latency.
 *
 *  Calculate TransitionLatency by power step value and pll value.
 *
 *  @param[in]     PstateCpuServices                 Pstate CPU services.
 *  @param[in]     PStateLevelingBufferStructPtr     Pstate row data buffer pointer
 *  @param[in]     PciAddress                        Pci address
 *  @param[out]    TransitionLatency                 The transition latency.
 *  @param[in]     StdHeader                         Header for library and services
 *
 *  @retval      AGESA_SUCCESS Always succeeds.
 */
AGESA_STATUS
F15OrGetPstateTransLatency (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       PSTATE_LEVELING        *PStateLevelingBufferStructPtr,
  IN       PCI_ADDR               *PciAddress,
     OUT   UINT32                 *TransitionLatency,
  IN       AMD_CONFIG_PARAMS      *StdHeader
  )
{
  UINT32               TempVar_b;
  UINT32               TempVar_c;
  UINT32               TempVar_d;
  UINT32               TempVar8_a;
  UINT32               TempVar8_b;
  UINT32               Ignored;
  UINT32               k;
  UINT32               CpuFidSameFlag;
  UINT8                PStateMaxValueOnCurrentCore;
  UINT32               TransAndBusMastLatency;

  CpuFidSameFlag = 1;

  F15OrGetFrequencyXlatRegInfo (
    PstateCpuServices,
    0,
    PStateLevelingBufferStructPtr->PStateCoreStruct[0].PStateStruct[0].CoreFreq,
    &TempVar_b,
    &TempVar_c,
    &Ignored,
    StdHeader
    );

  TempVar_d = TempVar_b;
  PStateMaxValueOnCurrentCore = PStateLevelingBufferStructPtr->PStateCoreStruct[0].PStateMaxValue;

  //
  //Check if MSRC001_00[6B:64][CpuFid] is the same value for all P-states where
  //MSRC001_00[6B:64][PstateEn]=1
  //
  for (k = 1; k <= PStateMaxValueOnCurrentCore; k++) {
    if (PStateLevelingBufferStructPtr->PStateCoreStruct[0].PStateStruct[k].PStateEnable != 0) {
      F15OrGetFrequencyXlatRegInfo (
        PstateCpuServices,
        (UINT8) k,
        PStateLevelingBufferStructPtr->PStateCoreStruct[0].PStateStruct[k].CoreFreq,
        &TempVar_b,
        &TempVar_c,
        &Ignored,
        StdHeader
        );
    }

    if (TempVar_d != TempVar_b) {
      CpuFidSameFlag = 0;
      break;
    }
  }

  PciAddress->Address.Register = 0xD4;
  PciAddress->Address.Function = FUNC_3;
  LibAmdPciRead (AccessWidth32, *PciAddress, &TempVar_d, StdHeader);

  // PowerStepDown - Bits 20:23
  TempVar8_a = (TempVar_d & 0x00F00000) >> 20;

  // PowerStepUp   - Bits 24:27
  TempVar8_b = (TempVar_d & 0x0F000000) >> 24;

  // Convert the raw numbers in TempVar8_a and TempVar8_b into time
  F15OrGetPowerStepValueInTime (&TempVar8_a);
  F15OrGetPowerStepValueInTime (&TempVar8_b);

  //
  //(12 * (F3xD4[PowerStepDown] + F3xD4[PowerStepUp]) /1000) us
  //
  TransAndBusMastLatency =
  (12 * (TempVar8_a + TempVar8_b) + 999) / 1000;

  if (CpuFidSameFlag == 0) {
    //
    //+ F3xA0[PllLockTime]
    //
    PciAddress->Address.Register = 0xA0;
    LibAmdPciRead (AccessWidth32, *PciAddress, &TempVar_d, StdHeader);

    TempVar8_a = (0x00003800 & TempVar_d) >> 11;
    F15OrGetPllValueInTime (&TempVar8_a);
    TransAndBusMastLatency += TempVar8_a;
  }

  *TransitionLatency = TransAndBusMastLatency;

  return (AGESA_SUCCESS);
}

/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to calculates the frequency in megahertz of the desired P-state.
 *
 *  @param[in]   PstateCpuServices        Pstate CPU services.
 *  @param[in]   StateNumber              The hardware P-State to analyze.
 *  @param[out]  FrequencyInMHz           The P-State's frequency in MegaHertz
 *  @param[in]   StdHeader                Header for library and services
 *
 *  @retval      AGESA_SUCCESS   Always Succeeds.
 */
AGESA_STATUS
F15OrGetPstateFrequency (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       UINT8 StateNumber,
     OUT   UINT32 *FrequencyInMHz,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT8 TempValue;
  UINT32 CpuDid;
  UINT32 CpuFid;
  UINT64 LocalMsrRegister;

  ASSERT (StateNumber < NM_PS_REG);
  LibAmdMsrRead (PS_REG_BASE + (UINT32) StateNumber, &LocalMsrRegister, StdHeader);
  ASSERT (((PSTATE_MSR *) &LocalMsrRegister)->PsEnable == 1);
  CpuDid = (UINT32) (((PSTATE_MSR *) &LocalMsrRegister)->CpuDid);
  CpuFid = (UINT32) (((PSTATE_MSR *) &LocalMsrRegister)->CpuFid);

  switch (CpuDid) {
  case 0:
    TempValue = 1;
    break;
  case 1:
    TempValue = 2;
    break;
  case 2:
    TempValue = 4;
    break;
  case 3:
    TempValue = 8;
    break;
  case 4:
    TempValue  =  16;
    break;
  default:
    // CpuDid is set to an undefined value.  This is due to either a misfused CPU, or
    // an invalid P-state MSR write.
    ASSERT (FALSE);
    TempValue = 1;
    break;
  }
  *FrequencyInMHz = (100 * (CpuFid + 0x10) / TempValue);
  return (AGESA_SUCCESS);
}

/*---------------------------------------------------------------------------------------*/
/**
 * Family specific call to sets the Pstate MSR to each APs base on Pstate Buffer.
 *
 *  @param[in]      PstateCpuServices        Pstate CPU services.
 * @param[in]       CpuAmdPState             Gathered P-state data structure for whole system.
 * @param[in]       StdHeader                Config for library and services.
 *
 * @retval          AGESA_STATUS    AGESA_SUCCESS Always succeeds.
 *
 */
AGESA_STATUS
F15OrPstateLevelingCoreMsrModify (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       S_CPU_AMD_PSTATE   *CpuAmdPState,
  IN       AMD_CONFIG_PARAMS  *StdHeader
  )
{
  UINT32             i;
  UINT32             Ignored;
  UINT32             k;
  UINT32             TempVar_d;
  UINT32             TempVar_e;
  UINT32             TempVar_f;
  UINT32             LogicalSocketCount;
  UINT32             LocalPciRegister;
  UINT32             Socket;
  UINT32             Module;
  UINT32             Core;
  UINT8              SwP0;
  UINT64             MsrValue;
  AGESA_STATUS       Status;
  PSTATE_LEVELING    *PStateBufferPtr;
  PSTATE_LEVELING    *PStateBufferPtrTmp;
  S_CPU_AMD_PSTATE   *CpuAmdPstatePtr;
  PCI_ADDR           PciAddress;
  CPU_SPECIFIC_SERVICES   *FamilySpecificServices;

  GetCpuServicesOfCurrentCore ((CONST CPU_SPECIFIC_SERVICES **)&FamilySpecificServices, StdHeader);
  ASSERT (FamilySpecificServices != NULL);

  Ignored = 0;
  CpuAmdPstatePtr = (S_CPU_AMD_PSTATE *) CpuAmdPState;
  PStateBufferPtrTmp = CpuAmdPstatePtr->PStateLevelingStruc;
  PStateBufferPtr = CpuAmdPstatePtr->PStateLevelingStruc;
  LogicalSocketCount = CpuAmdPstatePtr->TotalSocketInSystem;
  PciAddress.AddressValue = 0;
  SwP0 = PStateBufferPtrTmp->PStateCoreStruct[0].NumberOfBoostedStates;

  //
  //Try to find the Pstate buffer specific to this core(socket).
  //
  IdentifyCore (StdHeader, &Socket, &Module, &Core, &Status);
  GetPciAddress (StdHeader, Socket, Module, &PciAddress, &Status);
  for (i = 0; i < LogicalSocketCount; i++) {
    CpuGetPStateLevelStructure (&PStateBufferPtrTmp, CpuAmdPstatePtr, i, StdHeader);
    if (PStateBufferPtrTmp->SocketNumber == Socket) {
      break;
    }
  }

  if (PStateBufferPtr[0].OnlyOneEnabledPState) {
    //
    //If all processors have only 1 enabled P-state, the following sequence should be performed on all cores:
    //

    //1. Write the appropriate CpuFid value resulting from the matched CPU COF to 'software P0'.
    LibAmdMsrRead (MSR_PSTATE_0 + (UINT32) SwP0, &MsrValue, StdHeader);
    Status = F15OrGetFrequencyXlatRegInfo (PstateCpuServices, SwP0, PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[0].CoreFreq, &TempVar_d, &TempVar_e, &Ignored, StdHeader);
    // Bits 5:0
    ((PSTATE_MSR *) &MsrValue)->CpuFid = TempVar_d;
    // Bits 8:6
    ((PSTATE_MSR *) &MsrValue)->CpuDid = TempVar_e;
    // Bits 39:32
    ((PSTATE_MSR *) &MsrValue)->IddValue = PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[0].IddValue;
    // Bits 41:40
    ((PSTATE_MSR *) &MsrValue)->IddDiv = PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[0].IddDiv;
    // Enable the P-State
    ((PSTATE_MSR *) &MsrValue)->PsEnable = 1;
    LibAmdMsrWrite (MSR_PSTATE_0 + (UINT32) SwP0, &MsrValue, StdHeader);

    //2. Copy P0 to P1
    LibAmdMsrWrite (MSR_PSTATE_1 + (UINT32) SwP0, &MsrValue, StdHeader);

    //3. Increment F3xDC[PstatemaxVal] by 1.
    PciAddress.Address.Register = CPTC2_REG;
    PciAddress.Address.Function = FUNC_3;
    LibAmdPciRead (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader);
    ((CLK_PWR_TIMING_CTRL2_REGISTER *) &LocalPciRegister)->PstateMaxVal++;
    LibAmdPciWrite (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader);

    //4. Write 001b to MSRC001_0062[PstateCmd].
    FamilySpecificServices->TransitionPstate (FamilySpecificServices, (UINT8) 1, (BOOLEAN) FALSE, StdHeader);

    //5. Wait for MSRC001_0071[CurCpuFid] = P1[CpuFid].
    do {
      LibAmdMsrRead (MSR_COFVID_STS, &MsrValue, StdHeader);
    } while (((COFVID_STS_MSR *) &MsrValue)->CurCpuFid != TempVar_d);

    //6. Write 000b to MSRC001_0062[PstateCmd].
    FamilySpecificServices->TransitionPstate (FamilySpecificServices, (UINT8) 0, (BOOLEAN) FALSE, StdHeader);

    //7. Wait for MSRC001_0071[CurPstate] = MSRC001_0071[CurPstateLimit].
    do {
      LibAmdMsrRead (MSR_COFVID_STS, &MsrValue, StdHeader);
    } while (((COFVID_STS_MSR *) &MsrValue)->CurPstate != ((COFVID_STS_MSR *) &MsrValue)->CurPstateLimit);

    //8. Write 0b to P1[PstateEn].
    LibAmdMsrRead (MSR_PSTATE_1 + (UINT32) SwP0, &MsrValue, StdHeader);
    ((PSTATE_MSR *) &MsrValue)->PsEnable = 0;
    LibAmdMsrWrite (MSR_PSTATE_1 + (UINT32) SwP0, &MsrValue, StdHeader);

    //9. Decrement F3xDC[PstateMaxVal] by 1 and exit the sequence (no further steps are required).
    LibAmdPciRead (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader);
    ((CLK_PWR_TIMING_CTRL2_REGISTER *) &LocalPciRegister)->PstateMaxVal--;
    LibAmdPciWrite (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader);

  } else {

    TempVar_f = MSR_PSTATE_0 + (UINT32) SwP0;

    for (k = SwP0; k <= PStateBufferPtrTmp->PStateCoreStruct[0].PStateMaxValue; k++, TempVar_f++) {
      // If pState is not disabled then do update
      LibAmdMsrRead (TempVar_f, &MsrValue, StdHeader);

      if (PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[k].PStateEnable == 1) {
        Status = F15OrGetFrequencyXlatRegInfo (PstateCpuServices, (UINT8) k, PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[k].CoreFreq, &TempVar_d, &TempVar_e, &Ignored, StdHeader);
        if (Status != AGESA_ERROR) {
          // Bits 5:0
          ((PSTATE_MSR *) &MsrValue)->CpuFid = TempVar_d;
          // Bits 8:6
          ((PSTATE_MSR *) &MsrValue)->CpuDid = TempVar_e;
        }

        // Bits 39:32
        ((PSTATE_MSR *) &MsrValue)->IddValue = PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[k].IddValue;
        // Bits 41:40
        ((PSTATE_MSR *) &MsrValue)->IddDiv = PStateBufferPtrTmp->PStateCoreStruct[0].PStateStruct[k].IddDiv;
        // Enable the P-State
        ((PSTATE_MSR *) &MsrValue)->PsEnable = 1;
        LibAmdMsrWrite (TempVar_f, &MsrValue, StdHeader);
      } else {
        // Disable the P-State
        ((PSTATE_MSR *) &MsrValue)->PsEnable = 0;
        LibAmdMsrWrite (TempVar_f, &MsrValue, StdHeader);
      }
    }
  }
  return AGESA_SUCCESS;
}

/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to calculates the power in milliWatts of the desired P-state.
 *
 *  @param[in]     PstateCpuServices      Pstate CPU services.
 *  @param[in]   StateNumber              Which P-state to analyze
 *  @param[out]  PowerInMw                The Power in milliWatts of that P-State
 *  @param[in]   StdHeader                Header for library and services
 *
 *  @retval      AGESA_SUCCESS Always succeeds.
 */
AGESA_STATUS
F15OrGetPstatePower (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       UINT8 StateNumber,
     OUT   UINT32 *PowerInMw,
  IN       AMD_CONFIG_PARAMS *StdHeader
  )
{
  UINT32  CpuVid;
  UINT32  IddValue;
  UINT32  IddDiv;
  UINT32  V_x10000;
  UINT32  Power;
  UINT64  LocalMsrRegister;

  ASSERT (StateNumber < NM_PS_REG);
  LibAmdMsrRead (PS_REG_BASE + (UINT32) StateNumber, &LocalMsrRegister, StdHeader);
  ASSERT (((PSTATE_MSR *) &LocalMsrRegister)->PsEnable == 1);
  CpuVid = (UINT32) (((PSTATE_MSR *) &LocalMsrRegister)->CpuVid);
  IddValue = (UINT32) (((PSTATE_MSR *) &LocalMsrRegister)->IddValue);
  IddDiv = (UINT32) (((PSTATE_MSR *) &LocalMsrRegister)->IddDiv);

  if (CpuVid >= 0x7C) {
    V_x10000 = 0;
  } else {
    V_x10000 = 15500L - (125L * CpuVid);
  }

  Power = V_x10000 * IddValue;

  switch (IddDiv) {
  case 0:
    *PowerInMw = Power / 10L;
    break;
  case 1:
    *PowerInMw = Power / 100L;
    break;
  case 2:
    *PowerInMw = Power / 1000L;
    break;
  default:
    // IddDiv is set to an undefined value.  This is due to either a misfused CPU, or
    // an invalid P-state MSR write.
    ASSERT (FALSE);
    *PowerInMw = 0;
    break;
  }
  return (AGESA_SUCCESS);
}

/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to get CPU pstate max state.
 *
 *  @param[in]     PstateCpuServices      Pstate CPU services.
 *  @param[out]    MaxPStateNumber        The max hw pstate value on the current socket.
 *  @param[out]    NumberOfBoostStates    The number of boosted P-states on the current socket.
 *  @param[in]     StdHeader              Handle of Header for calling lib functions and services.
 *
 *  @retval      AGESA_SUCCESS Always succeeds.
 */
AGESA_STATUS
F15OrGetPstateMaxState (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
     OUT   UINT32              *MaxPStateNumber,
     OUT   UINT8               *NumberOfBoostStates,
  IN       AMD_CONFIG_PARAMS   *StdHeader
  )
{
  UINT32               NumBoostStates;
  UINT64               MsrValue;
  UINT32               LocalPciRegister;
  PCI_ADDR             PciAddress;

  LocalPciRegister = 0;

  // For F15 Orochi CPU, skip boosted p-state. The boosted p-state number = D[1F:18]F4x15C[NumBoostStates].
  OptionMultiSocketConfiguration.GetCurrPciAddr (&PciAddress, StdHeader);
  PciAddress.Address.Register = CPB_CTRL_REG;
  PciAddress.Address.Function = FUNC_4;
  LibAmdPciRead (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader); // F4x15C

  NumBoostStates = ((CPB_CTRL_REGISTER *) &LocalPciRegister)->NumBoostStates;
  *NumberOfBoostStates = (UINT8) NumBoostStates;

  //
  // Read PstateMaxVal [6:4] from MSR C001_0061
  // So, we will know the max pstate state in this socket.
  //
  LibAmdMsrRead (MSR_PSTATE_CURRENT_LIMIT, &MsrValue, StdHeader);
  *MaxPStateNumber = (UINT32) (((PSTATE_CURLIM_MSR *) &MsrValue)->PstateMaxVal) + NumBoostStates;

  return (AGESA_SUCCESS);
}

/*---------------------------------------------------------------------------------------*/
/**
 *  Family specific call to get CPU pstate register information.
 *
 *  @param[in]     PstateCpuServices         Pstate CPU services.
 *  @param[in]     PState                    Input Pstate number for query.
 *  @param[out]    PStateEnabled             Boolean flag return pstate enable.
 *  @param[in,out] IddVal                    Pstate current value.
 *  @param[in,out] IddDiv                    Pstate current divisor.
 *  @param[out]    SwPstateNumber            Software P-state number.
 *  @param[in]     StdHeader                 Handle of Header for calling lib functions and services.
 *
 *  @retval      AGESA_SUCCESS Always succeeds.
 */
AGESA_STATUS
F15OrGetPstateRegisterInfo (
  IN       PSTATE_CPU_FAMILY_SERVICES *PstateCpuServices,
  IN       UINT32              PState,
     OUT   BOOLEAN             *PStateEnabled,
  IN OUT   UINT32              *IddVal,
  IN OUT   UINT32              *IddDiv,
     OUT   UINT32              *SwPstateNumber,
  IN       AMD_CONFIG_PARAMS   *StdHeader
  )
{
  UINT32    LocalPciRegister;
  UINT64    LocalMsrRegister;
  PCI_ADDR  PciAddress;

  ASSERT (PState < NM_PS_REG);

  // For F15 Orochi CPU, skip boosted p-state. The boosted p-state number = D[1F:18]F4x15C[NumBoostStates].
  OptionMultiSocketConfiguration.GetCurrPciAddr (&PciAddress, StdHeader);
  PciAddress.Address.Register = CPB_CTRL_REG;
  PciAddress.Address.Function = FUNC_4;
  LibAmdPciRead (AccessWidth32, PciAddress, &LocalPciRegister, StdHeader); // F4x15C

  // Read PSTATE MSRs
  LibAmdMsrRead (PS_REG_BASE + (UINT32) PState, &LocalMsrRegister, StdHeader);

  *SwPstateNumber = PState;

  if (((PSTATE_MSR *) &LocalMsrRegister)->PsEnable == 1) {
    // PState enable = bit 63
    *PStateEnabled = TRUE;
    //
    // Check input pstate belongs to Boosted-Pstate, if yes, return *PStateEnabled = FALSE.
    //
    if (PState < ((CPB_CTRL_REGISTER *) &LocalPciRegister)->NumBoostStates) {
      *PStateEnabled = FALSE;
    } else {
      *SwPstateNumber = PState - ((CPB_CTRL_REGISTER *) &LocalPciRegister)->NumBoostStates;
    }
  } else {
    *PStateEnabled = FALSE;
  }

  // Bits 39:32 (high 32 bits [7:0])
  *IddVal = (UINT32) ((PSTATE_MSR *) &LocalMsrRegister)->IddValue;
  // Bits 41:40 (high 32 bits [9:8])
  *IddDiv = (UINT32) ((PSTATE_MSR *) &LocalMsrRegister)->IddDiv;

  return (AGESA_SUCCESS);
}


CONST PSTATE_CPU_FAMILY_SERVICES ROMDATA F15OrPstateServices =
{
  0,
  F15OrIsPstatePsdNeeded,
  F15OrIsPstatePsdDependent,
  F15OrSetTscFreqSel,
  F15OrGetPstateTransLatency,
  F15OrGetPstateFrequency,
  F15OrPstateLevelingCoreMsrModify,
  F15OrGetPstatePower,
  F15OrGetPstateMaxState,
  F15OrGetPstateRegisterInfo
};


/*---------------------------------------------------------------------------------------
 *                          L O C A L    F U N C T I O N S
 *---------------------------------------------------------------------------------------
 */


/**
 *---------------------------------------------------------------------------------------
 *
 *  F15OrGetPowerStepValueInTime
 *
 *  Description:
 *    Convert power step value in time
 *
 *  Parameters:
 *    @param[out]     *PowerStepPtr
 *
 *    @retval       VOID
 *
 *---------------------------------------------------------------------------------------
 **/
VOID
STATIC
F15OrGetPowerStepValueInTime (
  IN OUT   UINT32  *PowerStepPtr
  )
{
  UINT32 TempVar_a;

  TempVar_a = *PowerStepPtr;

  if (TempVar_a < 0x4) {
    *PowerStepPtr = 400 - (TempVar_a * 100);
  } else if (TempVar_a <  0x9) {
    *PowerStepPtr = 130 - (TempVar_a * 10);
  } else {
    *PowerStepPtr = 90 - (TempVar_a * 5);
  }
}

/**
 *---------------------------------------------------------------------------------------
 *
 *  F15OrGetPllValueInTime
 *
 *  Description:
 *    Convert PLL Value in time
 *
 *  Parameters:
 *    @param[out]     *PllLockTimePtr
 *
 *    @retval       VOID
 *
 *---------------------------------------------------------------------------------------
 **/
VOID
STATIC
F15OrGetPllValueInTime (
  IN OUT   UINT32  *PllLockTimePtr
  )
{
  if (*PllLockTimePtr < 4) {
    *PllLockTimePtr = *PllLockTimePtr + 1;
  } else if (*PllLockTimePtr == 4) {
    *PllLockTimePtr = 8;
  } else if (*PllLockTimePtr == 5) {
    *PllLockTimePtr = 16;
  } else
    *PllLockTimePtr = 0;
}

/*---------------------------------------------------------------------------------------*/
/**
 *    This function will return the CpuFid and CpuDid in MHz, using the formula
 *    described in the BKDG MSRC001_00[68:64] P-State [4:0] Registers:bit 8:0
 *
 *    @param[in]       PstateCpuServices        The current Family Specific Services.
 *    @param[in]       PStateNumber             P-state number to check.
 *    @param[in]       Frequency                Leveled target frequency for PStateNumber.
 *    @param[out]      *CpuFidPtr               New leveled FID.
 *    @param[out]      *CpuDidPtr1              New leveled DID info 1.
 *    @param[out]      *CpuDidPtr2              New leveled DID info 2.
 *    @param[in]       *StdHeader               Header for library and services.
 *
 *    @retval       AGESA_WARNING           This P-State does not need to be modified.
 *    @retval       AGESA_SUCCESS           This P-State must be modified to be level.
 */
AGESA_STATUS
STATIC
F15OrGetFrequencyXlatRegInfo (
  IN       PSTATE_CPU_FAMILY_SERVICES  *PstateCpuServices,
  IN       UINT8                  PStateNumber,
  IN       UINT32                 Frequency,
     OUT   UINT32                 *CpuFidPtr,
     OUT   UINT32                 *CpuDidPtr1,
     OUT   UINT32                 *CpuDidPtr2,
  IN       AMD_CONFIG_PARAMS      *StdHeader
  )
{
  UINT32 i;
  UINT32 j;
  AGESA_STATUS Status;
  UINT32 FrequencyInMHz;

  FrequencyInMHz = 0;
  *CpuDidPtr2 = 0xFFFF;

  Status = AGESA_SUCCESS;

  PstateCpuServices->GetPstateFrequency (PstateCpuServices, PStateNumber, &FrequencyInMHz, StdHeader);
  if (FrequencyInMHz == Frequency) {
    Status |= AGESA_WARNING;
  }

  // CPU Frequency = 100 MHz * (CpuFid + 10h) / (2^CpuDid)
  // In this for loop i = 2^CpuDid


  for (i = 1; i < 17; (i += i)) {
    for (j = 0; j < 64; j++) {
      if (Frequency == ((100 * (j + 0x10)) / i )) {
        *CpuFidPtr = j;
        if (i == 1) {
          *CpuDidPtr1 = 0;
        } else if (i == 2) {
          *CpuDidPtr1 = 1;
        } else if (i == 4) {
          *CpuDidPtr1 = 2;
        } else if (i == 8) {
          *CpuDidPtr1 = 3;
        } else if (i == 16) {
          *CpuDidPtr1 = 4;
        } else {
          *CpuFidPtr = 0xFFFF;
          *CpuDidPtr1 = 0xFFFF;
        }
        // Success
        return Status;
      }
    }
  }

  // Error Condition
  *CpuFidPtr = 0x00FF;
  *CpuDidPtr1 = 0x00FF;
  *CpuDidPtr2 = 0x00FF;

  return AGESA_ERROR;
}

