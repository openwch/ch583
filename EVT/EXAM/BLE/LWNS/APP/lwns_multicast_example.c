/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_multicast_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/20
 * Description        : single-hop multicast，组播传输例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_multicast_example.h"

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

static uint8_t subaddrs_index = 0;                 //发送订阅地址序号
#define SUBADDR_NUM    3                           //订阅地址数量
static uint16_t subaddrs[SUBADDR_NUM] = {1, 2, 3}; //订阅地址数组

static uint8_t TX_DATA[10] =
    {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
static uint8_t  RX_DATA[10];
static uint16_t lwns_multicast_ProcessEvent(uint8_t task_id, uint16_t events);
static void     multicast_recv(lwns_controller_ptr c, uint16_t subaddr, const lwns_addr_t *sender); //组播接收回调函数
static void     multicast_sent(lwns_controller_ptr ptr);                                            //组播发送完成回调函数

static lwns_multicast_controller multicast; //声明组播控制结构体

static uint8_t multicast_taskID; //声明组播控制任务id

/*********************************************************************
 * @fn      multicast_recv
 *
 * @brief   lwns multicast接收回调函数
 *
 * @param   ptr     -   本次接收到的数据所属的组播控制结构体指针.
 * @param   subaddr -   本次接收到的数据的订阅地址.
 * @param   sender  -   本次接收到的数据的发送者地址指针.
 *
 * @return  None.
 */
static void multicast_recv(lwns_controller_ptr ptr, uint16_t subaddr, const lwns_addr_t *sender)
{
    uint8_t len;
    len = lwns_buffer_datalen(); //获取当前缓冲区接收到的数据长度
    if(len == 10)
    {
        lwns_buffer_save_data(RX_DATA); //接收数据到用户数据区域
        PRINTF("multicast %d rec from %02x %02x %02x %02x %02x %02x\n",
               get_lwns_object_port(ptr),
               sender->v8[0], sender->v8[1], sender->v8[2], sender->v8[3],
               sender->v8[4], sender->v8[5]); //from为接收到的数据的发送方地址
        PRINTF("subaddr:%d data:", subaddr);
        for(uint8_t i = 0; i < len; i++)
        {
            PRINTF("%02x ", RX_DATA[i]);
        }
        PRINTF("\n");
    }
    else
    {
        PRINTF("data len err\n");
    }
}

/*********************************************************************
 * @fn      multicast_sent
 *
 * @brief   lwns multicast发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的组播控制结构体指针.
 *
 * @return  None.
 */
static void multicast_sent(lwns_controller_ptr ptr)
{
    PRINTF("multicast %d sent\n", get_lwns_object_port(ptr));
}

/**
 * lwns组播回调函数结构体，注册回调函数
 */
static const struct lwns_multicast_callbacks multicast_callbacks =
    {multicast_recv, multicast_sent};

/*********************************************************************
 * @fn      lwns_multicast_process_init
 *
 * @brief   lwns multicast例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_multicast_process_init(void)
{
    multicast_taskID = TMOS_ProcessEventRegister(lwns_multicast_ProcessEvent);
    lwns_multicast_init(&multicast,
                        136,                   //打开一个端口号为136的组播
                        subaddrs,              //订阅地址数组指针
                        SUBADDR_NUM,           //订阅地址数量
                        &multicast_callbacks); //返回0代表打开失败。返回1打开成功。
    tmos_start_task(multicast_taskID, MULTICAST_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));
}

/*********************************************************************
 * @fn      lwns_multicast_ProcessEvent
 *
 * @brief   lwns multicast Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_multicast_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & MULTICAST_EXAMPLE_TX_PERIOD_EVT)
    { //周期性在不同的组播地址上发送组播消息
        uint8_t temp;
        temp = TX_DATA[0];
        for(uint8_t i = 0; i < 9; i++)
        {
            TX_DATA[i] = TX_DATA[i + 1]; //移位发送数据，以便观察效果
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //载入需要发送的数据到缓冲区
        if(subaddrs_index >= SUBADDR_NUM)
        {
            subaddrs_index = 0;
        }
        lwns_multicast_send(&multicast, subaddrs[subaddrs_index]); //组播发送数据给指定节点
        subaddrs_index++;
        tmos_start_task(multicast_taskID, MULTICAST_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(1000)); //周期性发送
        return events ^ MULTICAST_EXAMPLE_TX_PERIOD_EVT;
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
