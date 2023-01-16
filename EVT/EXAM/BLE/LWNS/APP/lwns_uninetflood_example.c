/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_uninetflood_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/10/25
 * Description        : lwns单播网络泛洪传输例子，将数据网络泛洪发送至指定节点
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_uninetflood_example.h"

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
static lwns_addr_t dst_addr = {{0xa3, 0xdf, 0x38, 0xe4, 0xc2, 0x84}}; //目标节点地址，测试时，请根据电路板芯片MAC地址不同进行修改。修改为接收方的MAC地址，请勿使用自己的MAC地址
#else
static lwns_addr_t dst_addr = {{0xd9, 0x37, 0x3c, 0xe4, 0xc2, 0x84}};
#endif

static uint8_t  TX_DATA[LWNS_DATA_SIZE] = {0}; //最大长度数据收发测试
static uint8_t  RX_DATA[LWNS_DATA_SIZE] = {0}; //最大长度数据收发测试
static uint16_t lwns_uninetflood_ProcessEvent(uint8_t task_id, uint16_t events);
static void     uninetflood_recv(lwns_controller_ptr ptr, const lwns_addr_t *sender, uint8_t hops); //单播网络泛洪接收回调函数
static void     uninetflood_sent(lwns_controller_ptr ptr);                                          //单播网络泛洪发送完成回调函数

static lwns_uninetflood_controller uninetflood; //单播网络泛洪控制结构体

static uint8_t uninetflood_taskID; //单播网络泛洪控制任务id

/*********************************************************************
 * @fn      uninetflood_recv
 *
 * @brief   lwns uninetflood接收回调函数
 *
 * @param   ptr         -   本次接收到的数据所属的uninetflood控制结构体指针.
 * @param   sender      -   本次接收到的数据的发送者地址指针.
 * @param   hops        -   本次接收到的数据的从发送方到本节点经历的跳数.
 *
 * @return  None.
 */
static void uninetflood_recv(lwns_controller_ptr ptr, const lwns_addr_t *sender, uint8_t hops)
{
    uint8_t len;
    len = lwns_buffer_datalen();    //获取当前缓冲区接收到的数据长度
    lwns_buffer_save_data(RX_DATA); //接收数据到用户数据区域
    PRINTF("uninetflood %d rec from %02x %02x %02x %02x %02x %02x\n",
           get_lwns_object_port(ptr),
           sender->v8[0], sender->v8[1], sender->v8[2], sender->v8[3],
           sender->v8[4], sender->v8[5]); //from为接收到的数据的发送方地址
    PRINTF("data:");
    for(uint8_t i = 0; i < len; i++)
    {
        PRINTF("%02x ", RX_DATA[i]); //打印出数据
    }
    PRINTF("\n");
}

/*********************************************************************
 * @fn      uninetflood_sent
 *
 * @brief   lwns uninetflood发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的可靠单播控制结构体指针.
 *
 * @return  None.
 */
static void uninetflood_sent(lwns_controller_ptr ptr)
{
    PRINTF("uninetflood %d sent\n", get_lwns_object_port(ptr));
}

/**
 * lwns 单播网络泛洪回调函数结构体，注册回调函数
 */
static const struct lwns_uninetflood_callbacks uninetflood_callbacks =
    {uninetflood_recv, uninetflood_sent};

/*********************************************************************
 * @fn      lwns_uninetflood_process_init
 *
 * @brief   lwns uninetflood例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_uninetflood_process_init(void)
{
    uninetflood_taskID = TMOS_ProcessEventRegister(lwns_uninetflood_ProcessEvent);
    for(uint8_t i = 0; i < LWNS_DATA_SIZE; i++)
    {
        TX_DATA[i] = i;
    }
    lwns_uninetflood_init(&uninetflood,
                          137,                   //打开一个端口号为137的单播网络泛洪结构体
                          HTIMER_SECOND_NUM * 2, //最大等待转发时间
                          1,                     //在等待期间，接收到几次同样的数据包就取消本数据包的发送
                          3,                     //最大转发层级
                          FALSE,                 //在等待转发过程中，收到了新的需要转发的数据包，旧数据包是立刻发送出去还是丢弃，FALSE为立刻发送，TRUE为丢弃。
                          50,                    //网络恢复参数，该值定义了一个差距，如果包序号比内存内保存的数据包序号小的值大于此值，则会认为网络故障恢复，继续接收该数据包。
                          //同时，该值也决定了判定为新数据包的差值，即来自同一个节点的新数据包的序号不可以比内存中的大过多，即比此值还大。
                          //例如，内存中保存的为10，新数据包序号为60，差值为50，大于等于此时设置的50，所以将不会被认为为新的数据包，被丢弃。
                          //只有序号为59，差值为49，小于该值，才会被接收。
                          TRUE,                    //本机是否转发目标非本机的数据包，类似于蓝牙mesh是否启用relay功能。
                          &uninetflood_callbacks); //返回0代表打开失败。返回1打开成功。
    tmos_start_task(uninetflood_taskID, UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));
}

/*********************************************************************
 * @fn      lwns_uninetflood_ProcessEvent
 *
 * @brief   lwns uninetflood Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_uninetflood_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT)
    {
        uint8_t temp;
        temp = TX_DATA[0];
        for(uint8_t i = 0; i < 9; i++)
        {
            TX_DATA[i] = TX_DATA[i + 1]; //移位发送数据，以便观察效果
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //载入需要发送的数据到缓冲区
        lwns_uninetflood_send(&uninetflood, &dst_addr);  //单播网络泛洪发送数据给目标地址
        tmos_start_task(uninetflood_taskID, UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(1000)); //周期性发送
        return events ^ UNINETFLOOD_EXAMPLE_TX_PERIOD_EVT;
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
