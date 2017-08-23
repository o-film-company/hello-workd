/*****************************************************************************
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) by CETC-Motor Technology Co. LTD.    All rights reserved.
|
|    This software is furnished under a license and may be used and copied 
|    only in accordance with the terms of such license and with the inclusion
|    of the above copyright notice. This software or any other copies thereof 
|    may not be provided or otherwise made available to any other person.
|    No title to and ownership of the software is hereby transferred.
|
|    The information in this software is subject to change without notice 
|    and should not be construed as a commitment by 
|    CETC-Motor Technology Co. LTD.
|
|    CETC-Motor Technology Co. LTD assumes no responsibility for the use 
|    or reliability of its Software on equipment which is not supported by 
|    CETC-Motor Technology Co. LTD.
|-----------------------------------------------------------------------------
|  Project Name: SAIC AP13 SGE Gateway
|     File name: ADH.c 
| Target System: Freescale MPC55xx/MPC56xx
|      Compiler: CodeWarrior for MPC55xx/MPC56xx V2.9
|
|   Description: source file for ADC
|
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
| Initials     Name                      Company
| --------     ---------------------     -------------------------------------
| Jjt          Ji jiantao                CETC-Motor Technology Co. LTD
| CX         Chen Xing              CETC-Motor Technology Co. LTD
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date       Ver   Author  Description
| ---------- ----  ------  ---------------------------------------------------
| 2013-07-01 1.02    CX   - modify debounce function and add diagnostic monitor function
| 2012-08-08 1.01    Jh    - Review and update
| 2012-03-02 1.00    Jjt    - first implementation
*****************************************************************************/


/*---- Module switch -------------------------------------------------------*/
#define _ADH_C_

/*---- Datatype include ----------------------------------------------------*/
/* Datatypes and Project settings */
#include "Project.h"
#include "Cetc.h"

/* Processor registers and masks (only for HW-driver) */
#include "hw.h"
/*---- Includes ------------------------------------------------------------*/
#include "SIUL.h"
#include "DIO.h"
#include "ADC.h"
#include "MUX.h"
#include "ADH.h"

/*---- Internal Defines ----------------------------------------------------*/


/*---- Internal Macros -----------------------------------------------------*/

/*---- Internal Type definitions -------------------------------------------*/


/*---- Configuration switch ------------------------------------------------*/
#define _ADH_CFG_
#include "ADH_cfg.h"

/*---- Internal Forward Declaration ----------------------------------------*/
static U8   ADH_HandleIndexMatchChk(U8 ChnHandle);
static void ADH_CyclicSampleProcess(void);
static void ADH_SmpRefreshProcess(U8 u8Index,U8 u8HwAbChnIdx,U16 u16AdcResult);

/*---- Internal Module Global Variables ------------------------------------*/
U8  u8ADHwAbStatus       = ADH_STATUS_UNINIT;
U8  u8ADHwAbSamplePupSts = ADH_SAMP_PUP_DISABLE;
U16 u16ADHwAbSmpBuff[ADH_HDL_MAX];
U8  u8ADHwAbRefresh[ADH_HDL_MAX];

U16 u16ADHwAbBuffQueue[ADH_HDL_MAX][4];
U8  u8ADHwAbBuffQueueCnt[ADH_HDL_MAX]={0};

U16 u16ADHDebouncedLevel[ADH_HDL_MAX] = {0};
U8  u8ADHwAbDebounceCnt[ADH_HDL_MAX] ={0};
U16 u16ADHwAbVoltBuff[ADH_HDL_MAX];

/*============================ Implementation =============================*/
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
!   Function:       void ADH_Init(void)
!   Author:         Jjt
!   Creation Date:  2012-03-05 
!-----------------------------------------------------------------------------
!   Function Description: The function is used to initialize the ADH 
!
!   
!-----------------------------------------------------------------------------
!   Parameter:   None       Signal handle: None
!   Returnvalue: None
!   Global:      ;;
!+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void ADH_Init(void)
{
    U32 u32index;
    U8  u8ADHwAbChnIdx;
    
    if(u8ADHwAbStatus == ADH_STATUS_UNINIT) 
    {
        u8ADHwAbStatus = ADH_STATUS_INIT;
        
        for(u32index = 0; u32index < ADH_HDL_MAX; u32index++)
        {
            u8ADHwAbChnIdx = ADHwAbSmpChnCfgTab[u32index].u8HwAbChnIndex;
            u16ADHwAbSmpBuff[u8ADHwAbChnIdx] = ADH_RESULT_INVALID;
            u8ADHwAbDebounceCnt[u8ADHwAbChnIdx] = 0;
            u8ADHwAbRefresh[u8ADHwAbChnIdx] = 0;
        }
        
    }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
!   Function:       void ADH_CyclicProcess(void)
!   Author:         Jjt
!   Creation Date:  2012-03-05 
!-----------------------------------------------------------------------------
!   Function Description: The function is used to process adc handle in period 
!
!   
!-----------------------------------------------------------------------------
!   Parameter:   None       Signal handle: None
!   Returnvalue: None
!   Global:      ;;
!+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void ADH_CyclicProcess(void)
{
    if(u8ADHwAbStatus == ADH_STATUS_INIT)
    {
        u8ADHwAbStatus = ADH_STATUS_NORM;
    }
    else
    {
        ;
    }

    if(u8ADHwAbStatus != ADH_STATUS_UNINIT)
    {
        ADH_CyclicSampleProcess();
    }
    else
    {
        ;
    }
    
}

void ADH_CyclicSampleProcess(void)
{
    U8  u8Index;
    U16 u16AdcResultTemp;
    U8  u8ADHwAbChnIdx;
    U8  u8SampleRefreshFlgTemp;

    for(u8Index = 0;u8Index < ADH_HDL_MAX;u8Index++)
    {
        u8SampleRefreshFlgTemp = 0xffu;
        u8ADHwAbChnIdx = ADHwAbSmpChnCfgTab[u8Index].u8HwAbChnIndex ;
        if( (ADHwAbSmpChnCfgTab[u8Index].u8HwAbEnableCfg != ADH_INVALID)  
            &&((ADHwAbSmpChnCfgTab[u8Index].u8ADHwSampPupUsed == ADH_SAMP_PUP_NOT_USED)
               ||((ADHwAbSmpChnCfgTab[u8Index].u8ADHwSampPupUsed == ADH_SAMP_PUP_USED)
                  &&(u8ADHwAbSamplePupSts == ADH_SAMP_PUP_ENABLE))))
        {    
            if(ADHwAbSmpChnCfgTab[u8Index].u8ADHwAbSelectMode == ADH_SELECT_MUX)
            {
                if(ADHwAbSmpChnCfgTab[u8Index].u8ADHwAbChnOption == g_u8MuxChannelOptCur )
                {
                    u8SampleRefreshFlgTemp = 1u;
                }
                else
                {
                    u8SampleRefreshFlgTemp = 0u;
                }
            }
            else if(ADHwAbSmpChnCfgTab[u8Index].u8ADHwAbSelectMode == ADH_SELECT_PHY)
            {
                u8SampleRefreshFlgTemp = 1u;
                u8ADHwAbChnIdx = ADHwAbSmpChnCfgTab[u8Index].u8HwAbChnIndex;
            }
            else
            {   // do not support for the other mode in this vision
                u8SampleRefreshFlgTemp = 0u;
            }
        }
        else 
        {
            if(ADHwAbSmpChnCfgTab[u8Index].u8HwAbEnableCfg == ADH_INVALID)
            {
                u8SampleRefreshFlgTemp = 0u;
                u16ADHwAbSmpBuff[u8ADHwAbChnIdx] = ADC_INV_VAL; //for debug 
            }
        }

        /* */
        if(u8SampleRefreshFlgTemp == 1u)
        {
            u16AdcResultTemp = 0;
            ADC_GetResult(ADHwAbSmpChnCfgTab[u8Index].u8AdcChnHander, &u16AdcResultTemp);
            ADH_SmpRefreshProcess(u8Index,u8ADHwAbChnIdx,u16AdcResultTemp);
        }        
    }
}

void ADH_SmpRefreshProcess(U8 u8Index,U8 u8HwAbChnIdx,U16 u16AdcResult)
{
    U32 temp1;
    U32 temp2;
    U16 tempResult;
    U8 u8HwAbFilterRefreshFlg = 0xffu;
    U8 u8HwAbFilterCnt;

    if(ADHwAbSmpChnCfgTab[u8Index].u8FilterType == ADH_FILTER_RAW)
    {
        u8HwAbFilterRefreshFlg = 1;
        u16ADHwAbSmpBuff[u8HwAbChnIdx] = u16AdcResult;
    }
    else if(ADHwAbSmpChnCfgTab[u8Index].u8FilterType == ADH_FILTER_DEBOUNCE)
    {
        u8HwAbFilterCnt = u8ADHwAbBuffQueueCnt[u8Index];
        u16ADHwAbBuffQueue[u8Index][u8HwAbFilterCnt] = u16AdcResult;
        u8HwAbFilterCnt ++;
        if(u8HwAbFilterCnt >= ADHwAbSmpChnCfgTab[u8Index].u8FilterTimes)
        {
            u8HwAbFilterCnt = 0;
            u16ADHwAbSmpBuff[u8HwAbChnIdx] = u16AdcResult;
            u8HwAbFilterRefreshFlg = 1;
        }
        else
        {
            u8HwAbFilterRefreshFlg = 0;
        }
        u8ADHwAbBuffQueueCnt[u8Index] = u8HwAbFilterCnt;
    }
    else if(ADHwAbSmpChnCfgTab[u8Index].u8FilterType == ADH_FILTER_INERTIA)
    {
        if( u8ADHwAbBuffQueueCnt[u8Index] < ADHwAbSmpChnCfgTab[u8Index].u8FilterTimes)
        {
            u8ADHwAbBuffQueueCnt[u8Index] ++;
            u8HwAbFilterRefreshFlg = 0;
        }
        else
        {
            u8HwAbFilterRefreshFlg = 1;
        }
        if(u16ADHwAbSmpBuff[u8HwAbChnIdx] > u16AdcResult)
        {
            tempResult = u16ADHwAbSmpBuff[u8HwAbChnIdx] - u16AdcResult;
            tempResult = tempResult * 80;
            tempResult = tempResult / 100;
            u16ADHwAbSmpBuff[u8HwAbChnIdx] -= tempResult;    
        }
        else
        {
            tempResult = u16AdcResult - u16ADHwAbSmpBuff[u8HwAbChnIdx];
            tempResult = tempResult * 80;
            tempResult = tempResult / 100;
            u16ADHwAbSmpBuff[u8HwAbChnIdx] += tempResult;
        }   
    }
    else
    {
        u8HwAbFilterRefreshFlg = 0;
    }

    if(u8HwAbFilterRefreshFlg == 1)
    {
        temp1 = (U32)u16ADHwAbSmpBuff[u8HwAbChnIdx];

        temp1 = temp1 * 5000 / 1023;
        temp1 = temp1 * (U32)(ADHwAbSmpChnCfgTab[u8Index].adHwPara.ResistSeries
        + ADHwAbSmpChnCfgTab[u8Index].adHwPara.ResistDivider);
        temp2 = (U32)ADHwAbSmpChnCfgTab[u8Index].adHwPara.ResistDivider;

        tempResult = ADHwAbSmpChnCfgTab[u8Index].adHwPara.offset + (U16)(temp1/temp2);

        u16ADHwAbVoltBuff[u8HwAbChnIdx] = tempResult;

        /* if the result as the parameter */
        if(ADHwAbSmpChnCfgTab[u8Index].handleFunc != NULL_PTR)
        {
            if (ADHwAbSmpChnCfgTab[u8Index].u8ValueType == ADH_VALUE_VOLTAGE)
            {
                ADHwAbSmpChnCfgTab[u8Index].handleFunc(u16ADHwAbVoltBuff[u8HwAbChnIdx]);
            }
            else if (ADHwAbSmpChnCfgTab[u8Index].u8ValueType == ADH_VALUE_SMP)
            {
                ADHwAbSmpChnCfgTab[u8Index].handleFunc(u16ADHwAbSmpBuff[u8HwAbChnIdx]);
            }
        }
    }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
!   Function:       void ADH_Get_Result_u32(U32 adhHandle,U16 *result)
!   Author:         Jjt
!   Creation Date:  2012-03-05 
!-----------------------------------------------------------------------------
!   Function Description: The function is used to get adc result by adh handle 
!
!   
!-----------------------------------------------------------------------------
!   Parameter:   None       Signal handle: None
!   Returnvalue: None
!   Global:      ;;
!+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
U32 ADH_GetSampleAdcVal(U8 u8AdhHandle,U16 *result)
{
    U8 u8ADHwAbChnIdx;
    U32 u32RetVal = ADH_OK;

    /* check ADH state */
    if(u8ADHwAbStatus == ADH_STATUS_UNINIT) 
    {
        u32RetVal = ADH_ERR_UNINIT;
        *result = ADH_RESULT_INVALID;
    }
    else
    {
        /* check argument */
        if(u8AdhHandle > ADH_HDL_MAX) 
        {
            u32RetVal = ADH_ERR_RANGOUT;
            *result = ADH_RESULT_INVALID;
        }
        else if(ADHwAbSmpChnCfgTab[u8AdhHandle].u8HwAbEnableCfg == ADH_INVALID)
        {
            u32RetVal = ADH_ERR_UNVALID;
            *result = ADH_RESULT_INVALID;
        }
        else
        {
            u8ADHwAbChnIdx = ADHwAbSmpChnCfgTab[u8AdhHandle].u8HwAbChnIndex ;
            *result = u16ADHwAbSmpBuff[u8ADHwAbChnIdx];
        }
    }

    return u32RetVal;    
}

U32 ADH_GetCvtVolVal(U8 u8AdhHandle,U16 *result)
{
    U8 u8ADHwAbChnIdx;
    U32 u32RetVal = ADH_OK;

    /* check ADH state */
    if(u8ADHwAbStatus == ADH_STATUS_UNINIT) 
    {
        u32RetVal = ADH_ERR_UNINIT;
        *result = ADH_RESULT_INVALID;
    }
    else
    {
        /* check argument */
        if(u8AdhHandle > ADH_HDL_MAX) 
        {
            u32RetVal = ADH_ERR_RANGOUT;
            *result = ADH_RESULT_INVALID;
        }
        else if(ADHwAbSmpChnCfgTab[u8AdhHandle].u8HwAbEnableCfg == ADH_INVALID)
        {
            u32RetVal = ADH_ERR_UNVALID;
            *result = ADH_RESULT_INVALID;
        }
        else
        {
            u8ADHwAbChnIdx = ADHwAbSmpChnCfgTab[u8AdhHandle].u8HwAbChnIndex;
            *result = u16ADHwAbVoltBuff[u8ADHwAbChnIdx] ; /* resolution 0.001v */
        }
    }

    return u32RetVal; 
}

U32 ADH_SetSampleAdcVal(U8 u8AdhHandle,U16 u16ADHwAbVal)
{
    U8  u8ADHwAbChnIdx;
    U32 u32RetVal = ADH_OK;

    /* check ADH state */
    if(u8ADHwAbStatus == ADH_STATUS_UNINIT) 
    {
        u32RetVal = ADH_ERR_UNINIT;
    }
    else
    {
        /* check argument */
        if(u8AdhHandle >= ADH_HDL_MAX) 
        {
            u32RetVal = ADH_ERR_RANGOUT;
        }
        else if(ADHwAbSmpChnCfgTab[u8AdhHandle].u8HwAbEnableCfg == ADH_INVALID)
        {
            u32RetVal = ADH_ERR_UNVALID;
        }
        else
        {
            u8ADHwAbChnIdx = ADHwAbSmpChnCfgTab[u8AdhHandle].u8HwAbChnIndex;
            u16ADHwAbSmpBuff[u8ADHwAbChnIdx] = u16ADHwAbVal;
        }
    }

    return u32RetVal;    
}
/*
*************************************************

*************************************************
*/
void ADH_SetSamplePupSts(U8 SmpPupSts)
{
    u8ADHwAbSamplePupSts = SmpPupSts;
}

U8 ADH_GetSampleRate(void)
{
    return u8ADHwAbSamplePupSts;
}
U8 ADH_GetState(void)
{
    return u8ADHwAbStatus;
}

U8 ADH_HandleIndexRangeChk(U8 u8ADHandle)
{
    U8 u8RtnCodeTemp = ADH_OK;

    if(u8ADHandle >= ADH_HDL_MAX) 
    {
        u8RtnCodeTemp = ADH_ERR_RANGOUT;
    }
    else
    {
        u8RtnCodeTemp = ADH_OK;
    }

    return u8RtnCodeTemp;
}

U8 ADH_HandleIndexValidChk(U8 u8ADHandle)
{
    U8 u8RtnCodeTemp = ADH_OK;

    if(u8ADHandle >= ADH_HDL_MAX) 
    {
        u8RtnCodeTemp = ADH_ERR_RANGOUT;
    }
    else if(ADHwAbSmpChnCfgTab[u8ADHandle].u8HwAbEnableCfg == ADH_INVALID)
    {
        u8RtnCodeTemp = ADH_ERR_UNVALID;
    }
    else
    {
        u8RtnCodeTemp = ADH_OK;
    }

    return u8RtnCodeTemp;
}


/*
*************************************************

*************************************************
*/

U8 ADH_HandleIndexMatchChk(U8 ChnHandle)
{
    U8 u8ADHandle;
    U8 u8RetunCodeOk = ADH_ERR_NOK;

    if(ChnHandle != ADHwAbSmpChnCfgTab[ChnHandle].u8HwAbChnIndex)
    {
        for(u8ADHandle = 0; u8ADHandle < ADH_HDL_MAX; u8ADHandle ++ )
        {
            if(ChnHandle == ADHwAbSmpChnCfgTab[u8ADHandle].u8HwAbChnIndex)
            {
                u8RetunCodeOk = ADH_OK;
                break;
            }
        }
    }
    else
    {
        u8RetunCodeOk = ADH_OK;
    }

    return u8RetunCodeOk;
}

/*---- End of Module -------------------------------------------------------*/
