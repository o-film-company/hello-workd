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
|     File name: main.c 
| Target System: Freescale MPC55xx/MPC56xx
|      Compiler: CodeWarrior for MPC55xx/MPC56xx V2.9
|
|   Description: main file
|
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
| Initials     Name                      Company
| --------     ---------------------     -------------------------------------
| Jh           Zhang Junhui              CETC-Motor Technology Co. LTD
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date        Ver   Author  Description
| ----------  ----  ------  --------------------------------------------------
| 2012-03-11  1.00    Jh    - first implementation
*****************************************************************************/

/*---- Datatype include ----------------------------------------------------*/
/* Datatypes and Project settings */
#include "Project.h"
#include "Cetc.h"
/* Processor registers and masks (only for HW-driver) */
#include "hw.h"

/*---- Includes ------------------------------------------------------------*/

//#include "BCMType.h"
//#include "CGM.h"
//#include "ME.h"
#include "mc_me.h"
#include "SIUL.h"
#include "ADC.h"
#include "DIO.h"
#include "DIH.h"
//#include "DOH.h"
#include "MTT.h"
#include "mc_intc.h"
#include "timer.h"
#include "mc_stm.h"
#include "mc_swt.h"
#include "DSPI.h"
//#include "CGM.h"
#include "mc_cgm.h"
#include "mc_pcu.h"
#include "mc_rgm.h"

#include "mc_emios.h"
#include "PIT.h"
#include "PIT_cfg.h"   
#include "os_core.h"
#include "os_task.h"
#include "EED.h"
#include "NVM_Public.h"
#include "Sys_Public.h"
#include "modelApp.h"
//#include "sns.h"


//#include "osApi.h"

#if 0
void disableWatchdog(void) {
  SWT.SR.R = 0x0000c520;     /* Write keys to clear soft lock bit */
  SWT.SR.R = 0x0000d928; 
  SWT.CR.R = 0x8000010A;     /* Clear watchdog enable (WEN) */
}        
#endif  //zhoulei 0113
void main (void) 
{  
    /* init and reset the wdt,the chip enable the wdt default */
	mcSwtInit();
	mcSwtFeed();

    SIUL_Init_v();
	DIO_Init();
    DIH_Init();    
	/* interrupt init */
	mcIntcInit();

    DSPI_Init();
	EED_Init();
    NVM_Init();
    mcStmInit();
	
    PIT_Init_u32();
    mcEmiosInit();
    
    sysWakeUpDetectProcess();
    
    /* delay timer init */
    timerDelayInit();    /* software timer timer init */
    swTimerInit(HW_TIMER_0);
    /* timer start */
    hwTimerStart(HW_TIMER_0);

    /* external interrupt enable */
    EXTENAL_INTERRUPT_ENABLE;
    asm(" wrteei 1"); 
        
    Sys_NVMDataUpdate();
	sysGetStoredDrDoorKeyLckUnlckSwSts();

	/*In order to get change of a switch in the condition that the switchwake up the BCM,
	"modelAppProcess()" should be called at least once before input-sample task.*/
	modelAppInit();
	modelAppProcess();
		
    /* os init */
    osInit();
    /* os task init */
    osTaskInit();
    /* os start */
    osStart();
	
    /* Loop forever */
    for (;;) 
    {
        /* os schedule */
        osSched();
    }
    while(1)
    {
    	;
    }
}

