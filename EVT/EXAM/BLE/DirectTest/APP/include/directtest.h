/********************************** (C) COPYRIGHT *******************************
* File Name          : directtest..h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/11
* Description        : 
*******************************************************************************/

#ifndef TEST_H
#define TEST_H

#ifdef __cplusplus
extern "C"
{
#endif



/*********************************************************************
 * FUNCTIONS
 */
 #define  TEST_EVENT    1
  
extern void TEST_Init( void );
 
extern tmosEvents TEST_ProcessEvent( tmosTaskID task_id, tmosEvents events );
  


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif 
