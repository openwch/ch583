/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_unicast_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/19
 * Description        : lwns单播传输例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_unicast_example.h"

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

#if 1
static lwns_addr_t dst_addr = {{0xab, 0xdf, 0x38, 0xe4, 0xc2, 0x84}}; //目标节点地址，测试时，请根据电路板芯片MAC地址不同进行修改。修改为接收方的MAC地址，请勿使用自己的MAC地址
#else
static lwns_addr_t dst_addr = {{0xd9, 0x37, 0x3c, 0xe4, 0xc2, 0x84}};
#endif

static uint8_t TX_DATA[10] =
    {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
static uint8_t  RX_DATA[10];
static uint16_t lwns_unicast_ProcessEvent(uint8_t task_id, uint16_t events);
static void     unicast_recv(lwns_controller_ptr c, const lwns_addr_t *from); //单播接收回调函数
static void     unicast_sent(lwns_controller_ptr ptr);                        //单播发送完成回调函数

static lwns_unicast_controller unicast; //声明单播控制结构体

static uint8_t unicast_taskID; //声明单播控制任务id

/*********************************************************************
 * @fn      unicast_recv
 *
 * @brief   lwns unicast接收回调函数
 *
 * @param   ptr     -   本次接收到的数据所属的单播控制结构体指针.
 * @param   sender  -   本次接收到的数据的发送者地址指针.
 *
 * @return  None.
 */
static void unicast_recv(lwns_controller_ptr ptr, const lwns_addr_t *sender)
{
    uint8_t len;
    len = lwns_buffer_datalen(); //获取当前缓冲区接收到的数据长度
    if(len == 10)
    {
        lwns_buffer_save_data(RX_DATA); //接收数据到用户数据区域
        PRINTF("unicast %d rec from %02x %02x %02x %02x %02x %02x\n",
               get_lwns_object_port(ptr),
               sender->v8[0], sender->v8[1], sender->v8[2], sender->v8[3],
               sender->v8[4], sender->v8[5]); //sender为接收到的数据的发送方地址
        PRINTF("data:");
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
 * @fn      unicast_sent
 *
 * @brief   lwns unicast发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的可靠单播控制结构体指针.
 *
 * @return  None.
 */
static void unicast_sent(lwns_controller_ptr ptr)
{
    PRINTF("unicast %d sent\n", get_lwns_object_port(ptr));
}

/**
 * lwns 单播回调函数结构体，注册回调函数
 */
static const struct lwns_unicast_callbacks unicast_callbacks =
    {unicast_recv, unicast_sent};

/*********************************************************************
 * @fn      lwns_unicast_process_init
 *
 * @brief   lwns unicast例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_unicast_process_init(void)
{
    unicast_taskID = TMOS_ProcessEventRegister(lwns_unicast_ProcessEvent);
    lwns_unicast_init(&unicast,
                      136,                 //打开一个端口号为136的单播
                      &unicast_callbacks); //返回0代表打开失败。返回1打开成功。
    tmos_start_task(unicast_taskID, UNICAST_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));
}

/*********************************************************************
 * @fn      lwns_unicast_ProcessEvent
 *
 * @brief   lwns unicast Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_unicast_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & UNICAST_EXAMPLE_TX_PERIOD_EVT)
    {
        uint8_t temp;
        temp = TX_DATA[0];
        for(uint8_t i = 0; i < 9; i++)
        {
            TX_DATA[i] = TX_DATA[i + 1]; //移位发送数据，以便观察效果
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //载入需要发送的数据到缓冲区
        lwns_unicast_send(&unicast, &dst_addr);          //单播发送数据给指定节点
        tmos_start_task(unicast_taskID, UNICAST_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(1000)); //周期性发送
        return events ^ UNICAST_EXAMPLE_TX_PERIOD_EVT;
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
