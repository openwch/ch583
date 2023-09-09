/********************************** (C) COPYRIGHT *******************************
 * File Name          : RingMem.C
 * Author             : WCH
 * Version            : V1.4
 * Date               : 2019/12/20
 * Description        : 
 * NOTE				 : If use interrupts, you need to pay attention to the switch interrupt    
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "RingMem.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
RingMemProtection_t RingMemProtection;
/*********************************************************************
 * @fn      RingMemInit
 *
 * @brief   初始化
 *
 * @param   none
 *
 * @return  none
 */
void RingMemInit( RingMemParm_t *Parm, uint8_t *StartAddr, uint32_t MaxLen, RingMemProtection_t Protection )
{
	Parm->pData		= StartAddr;
	Parm->pWrite 	= StartAddr;
	Parm->pRead  	= StartAddr;
	Parm->RemanentLen = MaxLen;
	Parm->CurrentLen 	= 0;
	Parm->pEnd	  = StartAddr + MaxLen;
	Parm->MaxLen  = MaxLen;
	RingMemProtection = Protection;
}

/*********************************************************************
 * @fn      RingMemWrite
 *
 * @brief   往里写
 *
 * @param   none
 *
 * @return  none
 */
uint8_t RingMemWrite( RingMemParm_t *Parm, uint8_t *pData, uint32_t len )
{
	uint32_t i,edgelen;
	
	if(RingMemProtection)
	{
	    RingMemProtection(ENABLE);
	}

	if( len > Parm->RemanentLen )
		return !SUCCESS;
	
	edgelen = Parm->pEnd - Parm->pWrite; //Calculate the length of the remaining to the boundary
	
	if( len > edgelen )
	{
		for(i=0; i<edgelen; i++)
		{
			*Parm->pWrite++ = *pData++;		
		}
		Parm->pWrite = Parm->pData;	
		for(i=0; i<(len-edgelen); i++)
		{
			*Parm->pWrite++ = *pData++;		
		}		
	}
	else
	{
		for(i=0; i<len; i++)
		{
			*Parm->pWrite++ = *pData++;
		}
	}
	
	Parm->RemanentLen -= len;	
	Parm->CurrentLen 	+= len;	

    if(RingMemProtection)
    {
        RingMemProtection(DISABLE);
    }

	return SUCCESS;
}

/*********************************************************************
 * @fn      RingMemRead
 *
 * @brief   往外读
 *
 * @param   none
 *
 * @return  none
 */
uint8_t RingMemRead( RingMemParm_t *Parm, uint8_t *pData, uint32_t len )
{
	uint32_t i,edgelen;
	
    if(RingMemProtection)
    {
        RingMemProtection(ENABLE);
    }

    if( len > Parm->CurrentLen )	//Not enough reading length
		return !SUCCESS;
	
	edgelen = Parm->pEnd - Parm->pRead;			//Calculate the length of the remaining to the boundarythe length of the remaining to the boundary
	
	if( len > edgelen )
	{
		for(i=0; i<edgelen; i++)
		{
			*pData++ = *Parm->pRead++;		
		}
		Parm->pRead = Parm->pData;	
		for(i=0; i<(len-edgelen); i++)
		{
			*pData++ = *Parm->pRead++;		
		}
	}
	else
	{
		for(i=0; i<len; i++)
		{
			*pData++ = *Parm->pRead++;		
		}
	}
	
	Parm->RemanentLen += len;
	Parm->CurrentLen 	-= len;	

	if(RingMemProtection)
    {
        RingMemProtection(DISABLE);
    }

	return SUCCESS;
}

/*********************************************************************
 * @fn      RingMemCopy
 *
 * @brief   Don't delete it after reading
 *
 * @param   none
 *
 * @return  none
 */
uint8_t RingMemCopy( RingMemParm_t *Parm, uint8_t *pData, uint32_t len )
{
	uint32_t i,edgelen;

    if(RingMemProtection)
    {
        RingMemProtection(ENABLE);
    }

	uint8_t volatile *pRead = Parm->pRead;
	
	if( len > Parm->CurrentLen )						//The length that can be copied is not enough
		return !SUCCESS;
	
	edgelen = Parm->pEnd - Parm->pRead;			//Calculate the length of the remaining to the boundary
	
	if( len > edgelen )
	{
		for(i=0; i<edgelen; i++)
		{
			*pData++ = *pRead++;		
		}
		pRead = Parm->pData;	
		for(i=0; i<(len-edgelen); i++)
		{
			*pData++ = *pRead++;		
		}
	}
	else
	{
		for(i=0; i<len; i++)
		{
			*pData++ = *pRead++;		
		}
	}
	
    if(RingMemProtection)
    {
        RingMemProtection(DISABLE);
    }

	return SUCCESS;
}

/*********************************************************************
 * @fn      RingMemDelete
 *
 * @brief   Delete directly
 *
 * @param   none
 *
 * @return  none
 */
uint8_t RingMemDelete( RingMemParm_t *Parm, uint32_t len )
{
	uint32_t edgelen;
	
    if(RingMemProtection)
    {
        RingMemProtection(ENABLE);
    }

	if( len > Parm->CurrentLen )						//Can delete not enough length
		return !SUCCESS;
	
	edgelen = Parm->pEnd - Parm->pRead;			//Calculate the length of the remaining to the boundary
	
	if( len > edgelen )
	{
		Parm->pRead+=edgelen;		
		Parm->pRead = Parm->pData;	
		Parm->pRead+=(len-edgelen);		
	}
	else
	{
		Parm->pRead+=len;		
	}
	
	Parm->RemanentLen += len;
	Parm->CurrentLen 	-= len;	

    if(RingMemProtection)
    {
        RingMemProtection(DISABLE);
    }

    return SUCCESS;
}

/*********************************************************************
 * @fn      RingAddInStart
 *
 * @brief   向缓冲区开始处添加数据
 *
 * @param   none
 *
 * @return  none
 */
uint8_t RingAddInStart( RingMemParm_t *Parm, uint8_t *pData, uint32_t len )
{
	uint32_t i,edgelen;
	
    if(RingMemProtection)
    {
        RingMemProtection(ENABLE);
    }

	if( len > Parm->RemanentLen )
		return !SUCCESS;
	
	edgelen = Parm->pRead - Parm->pData;			//Calculate the length of the remaining to the boundary
	
	if( len > edgelen )
	{
		for(i=0; i<edgelen; i++)
		{
			*Parm->pRead-- = *pData++;		
		}
		Parm->pRead = Parm->pData + Parm->MaxLen;	
		for(i=0; i<(len-edgelen); i++)
		{
			*Parm->pRead-- = *pData++;		
		}
	}
	else
	{
		for(i=0; i<len; i++)
		{
			*Parm->pRead-- = *pData++;
		}
	}
	
	Parm->RemanentLen -= len;
	Parm->CurrentLen 	+= len;	

	if(RingMemProtection)
    {
        RingMemProtection(DISABLE);
    }

	return SUCCESS;
}

/*********************************************************************
 * @fn      RingCopyOneData
 *
 * @brief   Back the first data on the basis of the first address
 *
 * @param   none
 *
 * @return  none
 */
uint8_t RingReturnSingleData( RingMemParm_t *Parm, uint32_t Num )
{
  uint32_t edgelen;
  
  if( Parm->RemanentLen < Parm->MaxLen )
  {    
    edgelen = Parm->pEnd - Parm->pRead;        
    if( Num >= edgelen )
    {
      return Parm->pData[Num - edgelen];
    }
    else
    {
      return Parm->pRead[Num];
    }
  }
  else
    return 0;
}
/*********************************************************************
*********************************************************************/
