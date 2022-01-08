/********************************** (C) COPYRIGHT *******************************
 * File Name          : KEY.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2014/05/12
 * Description        :
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CH58x_common.h"
#include "HAL/HAL.h"
#include "BLE/peripheral.h"

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
bool is_botton_changed = FALSE;

uint8 Hal_KeyIntEnable; /* interrupt enable/disable flag */

// Registered keys task ID, initialized to NOT USED.
static uint8 registeredKeysTaskID = TASK_NO_TASK;
static uint8 halKeySavedKeys; /* 保留按键最后的状态，用于查询是否有键值变化 */
static uint8 KeyConfigFlag; /* 按键是否配置标志位 */

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
static halKeyCBack_t pHalKeyProcessFunction; /* callback function */

/**************************************************************************************************
 * @fn      HAL_KeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HAL_KeyInit(void) {
    /* enable TPP223 */
    ENABLE_TPP223();
    /* Initialize previous key to 0 */
    halKeySavedKeys = 0;
    /* Initialize callback function */
    pHalKeyProcessFunction = NULL;
    /* Start with key is not configured */
    KeyConfigFlag = FALSE;
    KEY1_DIR;
    KEY1_PD;

    HalKeyConfig( HAL_KEY_INTERRUPT_DISABLE, HalKeyCallback);
    GPIOB_ITModeCfg(GPIO_Pin_8, GPIO_ITMode_RiseEdge);
    PFIC_EnableIRQ(GPIO_B_IRQn);
    HAL_KEY_RegisterForKeys(halTaskID);
}

void HAL_KEY_RegisterForKeys(tmosTaskID id) {
    registeredKeysTaskID = id;
}

/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
 *          cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig(uint8 interruptEnable, halKeyCBack_t cback) {
    /* Enable/Disable Interrupt or */
    Hal_KeyIntEnable = interruptEnable;
    /* Register the callback fucntion */
    pHalKeyProcessFunction = cback;
    /* Determine if interrupt is enable or not */
    if (Hal_KeyIntEnable) {
        /* Do this only after the hal_key is configured - to work with sleep stuff */
        if (KeyConfigFlag == TRUE) {
            tmos_stop_task(halTaskID, HAL_KEY_EVENT); /* Cancel polling if active */
        }
    } else { /* Interrupts NOT enabled */
        tmos_start_task(halTaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE); /* Kick off polling */
    }
    /* Key now is configured */
    KeyConfigFlag = TRUE;
}

/*********************************************************************
 * @fn      OnBoard_SendKeys
 *
 * @brief   Send "Key Pressed" message to application.
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  status
 *********************************************************************/
uint8 OnBoard_SendKeys(uint8 keys, uint8 state) {
    keyChange_t *msgPtr;

    if (registeredKeysTaskID != TASK_NO_TASK) {
        // Send the address to the task
        msgPtr = (keyChange_t *) tmos_msg_allocate(sizeof(keyChange_t));
        if (msgPtr) {
            msgPtr->hdr.event = KEY_CHANGE;
            msgPtr->state = state;
            msgPtr->keys = keys;
            tmos_msg_send(registeredKeysTaskID, (uint8 *) msgPtr);
        }
        return ( SUCCESS);
    } else {
        return ( FAILURE);
    }
}

/*********************************************************************
 * @fn      OnBoard_KeyCallback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  void
 *********************************************************************/
void HalKeyCallback(uint8 keys, uint8 state) {
    ARG_UNUSED(state);

    if (OnBoard_SendKeys(keys, state) != SUCCESS) {
        // Process SW1 here start/stop
        if (keys & HAL_KEY_SW_1) {    // Switch 1
            LOG_INFO("KYE_1 SW!");
        }
        // Process SW2 here B
        if (keys & HAL_KEY_SW_2) {    // Switch 2
            LOG_INFO("KYE_2 SW!");
        }
        // Process SW3 here A
        if (keys & HAL_KEY_SW_3) {    // Switch 3
            LOG_INFO("KYE_3 SW!");
        }
        // Process SW4 here power
        if (keys & HAL_KEY_SW_4) {    // Switch 4
            LOG_INFO("KYE_4 SW!");
        }

        // Process SW5 here reset AB
        if (keys & HAL_KEY_SW_5) {    // Switch 5
            LOG_INFO("KYE_5 SW!");
        }
    }

}

/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
uint8 HalKeyRead(void) {
    uint8 keys = 0;

    if ( HAL_PUSH_BUTTON1()) {    //读按键1 start/stop
        keys |= HAL_KEY_SW_1;
    }
    if ( HAL_PUSH_BUTTON2()) {    //读按键2 B
        keys |= HAL_KEY_SW_2;
    }
    if ( HAL_PUSH_BUTTON3()) {    //读按键3 A
        keys |= HAL_KEY_SW_3;
    }
    if ( HAL_PUSH_BUTTON4()) {    //读按键4 power
        keys |= HAL_KEY_SW_4;
    }
    if ( HAL_PUSH_BUTTON5()) {    //读按键5 reset AB
        keys |= HAL_KEY_SW_5;
    }
    return keys;
}

/**************************************************************************************************
 * @fn      HAL_KeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HAL_KeyPoll(void) {
    uint8 keys = 0;
    static uint8_t keys_before = 0;

    keys = HalKeyRead();
//    if (keys != keys_before) {  //消抖
//        keys_before = keys;
//        return;
//    }
    keys_before = keys;

    if (!Hal_KeyIntEnable) { /* 中断未使能 */
        if (keys == halKeySavedKeys) { /* Exit - since no keys have changed */
            return;     //not return here, since we have to get bottons' states.
        }
        halKeySavedKeys = keys; /* Store the current keys for comparation next time */
    }

    /* Invoke Callback if new keys were depressed */
    if (keys && (pHalKeyProcessFunction))
//  if ( ( pHalKeyProcessFunction ) )
            {
        (pHalKeyProcessFunction)(keys, HAL_KEY_STATE_NORMAL);
    }
}

__HIGH_CODE
void GPIOB_IRQHandler(void) {
    GPIOB_ClearITFlagBit(GPIO_Pin_8);
}

/******************************** endfile @ key ******************************/
