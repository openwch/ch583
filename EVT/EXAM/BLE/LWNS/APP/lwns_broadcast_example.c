/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_broadcast_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/20
 * Description        : broadcast，广播程序例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_broadcast_example.h"

//每个文件单独debug打印的开关，置0可以禁止本文件内部打印
#define DEBUG_PRINT_IN_THIS_FILE    1
#if DEBUG_PRINT_IN_THIS_FILE
  #define PRINTF(...)    PRINT(__VA_ARGS__)
#else
  #define PRINTF(...) \
    do                \
    {                 \
    } while(0)
#endif

//用户需要发送的数据缓冲区
static uint8_t TX_DATA[10] =
    {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

static uint8_t  RX_DATA[10]; //用户接收数据的缓冲区
static uint16_t lwns_broadcast_ProcessEvent(uint8_t task_id, uint16_t events);
static void     broadcast_recv(lwns_controller_ptr ptr,
                               const lwns_addr_t  *sender); //接收回调函数声明
static void     broadcast_sent(lwns_controller_ptr ptr);   //发送回调函数声明

static lwns_broadcast_controller broadcast; //广播控制结构体

static uint8_t broadcast_taskID;

/*********************************************************************
 * @fn      broadcast_recv
 *
 * @brief   lwns broadcast接收回调函数
 *
 * @param   ptr     -   本次接收到的数据所属的广播控制结构体指针.
 * @param   sender  -   本次接收到的数据的发送者地址指针.
 *
 * @return  None.
 */
static void broadcast_recv(lwns_controller_ptr ptr,
                           const lwns_addr_t  *sender)
{
    uint8_t len;
    len = lwns_buffer_datalen(); //获取当前缓冲区接收到的数据长度
    if(len == 10)
    {
        lwns_buffer_save_data(RX_DATA); //接收数据到用户数据区域
        PRINTF("broadcast %d rec from %02x %02x %02x %02x %02x %02x\n", get_lwns_object_port(ptr),
               sender->v8[0], sender->v8[1], sender->v8[2], sender->v8[3],
               sender->v8[4], sender->v8[5]); //打印出本次消息的发送者地址
        PRINTF("data:");
        for(uint8_t i = 0; i < len; i++)
        {
            PRINTF("%02x ", RX_DATA[i]); //打印数据
        }
        PRINTF("\n");
    }
    else
    {
        PRINTF("data len err\n");
    }
}

/*********************************************************************
 * @fn      broadcast_sent
 *
 * @brief   lwns broadcast发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的广播控制结构体指针.
 *
 * @return  None.
 */
static void broadcast_sent(lwns_controller_ptr ptr)
{
    PRINTF("broadcast %d sent\n", get_lwns_object_port(ptr)); //打印发送完成信息
}

/**
 * lwns广播回调函数结构体，注册回调函数
 */
static const struct lwns_broadcast_callbacks broadcast_call = {
    broadcast_recv, broadcast_sent};

/*********************************************************************
 * @fn      lwns_broadcast_process_init
 *
 * @brief   lwns broadcast例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_broadcast_process_init(void)
{
    broadcast_taskID = TMOS_ProcessEventRegister(lwns_broadcast_ProcessEvent);
    lwns_broadcast_init(&broadcast, 136, &broadcast_call); //打开一个端口号为136的广播端口，注册回调函数
    tmos_start_task(broadcast_taskID, BROADCAST_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000)); //开始周期性广播任务
}

/*********************************************************************
 * @fn      lwns_broadcast_ProcessEvent
 *
 * @brief   lwns broadcast Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_broadcast_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & BROADCAST_EXAMPLE_TX_PERIOD_EVT)
    {
        uint8_t temp;
        temp = TX_DATA[0];
        for(uint8_t i = 0; i < 9; i++)
        {
            TX_DATA[i] = TX_DATA[i + 1]; //移位发送数据，以便观察效果
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //载入需要发送的数据到缓冲区
        lwns_broadcast_send(&broadcast);                 //广播发送数据
        tmos_start_task(broadcast_taskID, BROADCAST_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(1000)); //周期性发送

        return events ^ BROADCAST_EXAMPLE_TX_PERIOD_EVT;
    }
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(task_id)) != NULL)
        {
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    return 0;
}
