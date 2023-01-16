/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_multinetflood_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/11/10
 * Description        : multinetflood，组播网络泛洪传输例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_multinetflood_example.h"

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

/*
 * 订阅的地址为2字节u16类型。
 */
static uint8_t subaddrs_index = 0;                 //发送订阅地址序号
#define SUBADDR_NUM    3                           //订阅地址数量
static uint16_t subaddrs[SUBADDR_NUM] = {1, 2, 3}; //订阅地址数组

static uint8_t  TX_DATA[LWNS_DATA_SIZE] = {0}; //最大长度数据收发测试
static uint8_t  RX_DATA[LWNS_DATA_SIZE] = {0}; //最大长度数据收发测试
static uint16_t lwns_multinetflood_ProcessEvent(uint8_t task_id, uint16_t events);
static void     multinetflood_recv(lwns_controller_ptr ptr, uint16_t subaddr, const lwns_addr_t *sender, uint8_t hops); //组播网络泛洪接收回调函数
static void     multinetflood_sent(lwns_controller_ptr ptr);                                                            //组播网络泛洪发送完成回调函数

static lwns_multinetflood_controller multinetflood; //声明组播网络泛洪控制结构体

static uint8_t multinetflood_taskID; //组播网络泛洪控制任务id

/*********************************************************************
 * @fn      multinetflood_recv
 *
 * @brief   lwns multinetflood接收回调函数
 *
 * @param   ptr     -   本次接收到的数据所属的multinetflood控制结构体指针.
 * @param   subaddr -   本次接收到的数据的订阅地址.
 * @param   sender  -   本次接收到的数据的发送者地址指针.
 * @param   hops    -   本次接收到的数据的从发送方到本节点经历的跳数.
 *
 * @return  None.
 */
static void multinetflood_recv(lwns_controller_ptr ptr, uint16_t subaddr, const lwns_addr_t *sender, uint8_t hops)
{
    uint8_t len;
    len = lwns_buffer_datalen();    //获取当前缓冲区接收到的数据长度
    lwns_buffer_save_data(RX_DATA); //接收数据到用户数据区域
    PRINTF("multinetflood %d rec from %02x %02x %02x %02x %02x %02x\n",
           get_lwns_object_port(ptr),
           sender->v8[0], sender->v8[1], sender->v8[2], sender->v8[3],
           sender->v8[4], sender->v8[5]); //from为接收到的数据的发送方地址
    PRINTF("subaddr:%d,data:", subaddr);
    for(uint8_t i = 0; i < len; i++)
    {
        PRINTF("%02x ", RX_DATA[i]); //打印出数据
    }
    PRINTF("\n");
}

/*********************************************************************
 * @fn      multinetflood_sent
 *
 * @brief   lwns multinetflood发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的组播网络泛洪控制结构体指针.
 *
 * @return  None.
 */
static void multinetflood_sent(lwns_controller_ptr ptr)
{
    PRINTF("multinetflood %d sent\n", get_lwns_object_port(ptr));
}

/**
 * lwns 组播网络泛洪回调函数结构体，注册回调函数
 */
static const struct lwns_multinetflood_callbacks multinetflood_callbacks =
    {multinetflood_recv, multinetflood_sent};

/*********************************************************************
 * @fn      lwns_multinetflood_process_init
 *
 * @brief   lwns multinetflood例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_multinetflood_process_init(void)
{
    multinetflood_taskID = TMOS_ProcessEventRegister(lwns_multinetflood_ProcessEvent);
    for(uint8_t i = 0; i < LWNS_DATA_SIZE; i++)
    {
        TX_DATA[i] = i;
    }
    lwns_multinetflood_init(&multinetflood,
                            137,               //打开一个端口号为137的组播网络泛洪结构体
                            HTIMER_SECOND_NUM, //最大等待转发时间
                            1,                 //在等待期间，接收到几次同样的数据包就取消本数据包的发送
                            3,                 //最大转发层级
                            FALSE,             //在等待转发过程中，收到了新的需要转发的数据包，旧数据包是立刻发送出去还是丢弃，FALSE为立刻发送，TRUE为丢弃。
                            50,                //网络恢复参数，该值定义了一个差距，如果包序号比内存内保存的数据包序号小的值大于此值，则会认为网络故障恢复，继续接收该数据包。
                            //同时，该值也决定了判定为新数据包的差值，即来自同一个节点的新数据包的序号不可以比内存中的大过多，即比此值还大。
                            //例如，内存中保存的为10，新数据包序号为60，差值为50，大于等于此时设置的50，所以将不会被认为为新的数据包，被丢弃。
                            //只有序号为59，差值为49，小于该值，才会被接收。
                            TRUE,                      //本机是否转发目标非本机的数据包，类似于蓝牙mesh是否启用relay功能。
                            subaddrs,                  //订阅的地址数组指针
                            SUBADDR_NUM,               //订阅地址数量
                            &multinetflood_callbacks); //返回0代表打开失败。返回1打开成功。
#if 1
    tmos_start_task(multinetflood_taskID, MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));
#endif
}

/*********************************************************************
 * @fn      lwns_multinetflood_ProcessEvent
 *
 * @brief   lwns multinetflood Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_multinetflood_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT)
    {
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
        lwns_multinetflood_send(&multinetflood, subaddrs[subaddrs_index]); //组播网络泛洪发送数据到订阅地址
        subaddrs_index++;

        tmos_start_task(multinetflood_taskID, MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(1000)); //周期性发送
        return events ^ MULTINETFLOOD_EXAMPLE_TX_PERIOD_EVT;
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
