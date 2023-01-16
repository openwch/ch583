/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_netflood_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/07/12
 * Description        : netflood，网络泛洪传输例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_netflood_example.h"

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

static uint8_t  TX_DATA[LWNS_DATA_SIZE] = {0}; //最大长度数据收发测试
static uint8_t  RX_DATA[LWNS_DATA_SIZE] = {0}; //最大长度数据收发测试
static uint16_t lwns_netflood_ProcessEvent(uint8_t task_id, uint16_t events);
static int      netflood_recv(lwns_controller_ptr ptr,
                              const lwns_addr_t  *from,
                              const lwns_addr_t *originator, uint8_t hops);
static void     netflood_sent(lwns_controller_ptr ptr);
static void     netflood_dropped(lwns_controller_ptr ptr);

/**
 * lwns 网络泛洪回调函数结构体，注册回调函数
 */
static const struct lwns_netflood_callbacks callbacks = {netflood_recv,
                                                         netflood_sent, netflood_dropped};

static uint8_t netflood_taskID;

void lwns_netflood_process_init(void);

static lwns_netflood_controller netflood; //网络泛洪控制结构体

/*********************************************************************
 * @fn      netflood_recv
 *
 * @brief   lwns netflood接收回调函数
 *
 * @param   ptr         -   本次接收到的数据所属的netflood控制结构体指针.
 * @param   from        -   本次接收到的数据的上一跳转发者的地址指针.
 * @param   originator  -   本次接收到的数据的发送者地址指针.
 * @param   hops        -   本次接收到的数据的从发送方到本节点经历的跳数.
 *
 * @return  0/1         -   0表示本节点不再转发该数据包，1表示本节点继续转发该数据包.
 */
static int netflood_recv(lwns_controller_ptr ptr,
                         const lwns_addr_t  *from,
                         const lwns_addr_t *originator, uint8_t hops)
{
    uint8_t len;
    len = lwns_buffer_datalen(); //获取当前缓冲区接收到的数据长度
    PRINTF("netflood %d rec %02x %02x %02x %02x %02x %02x,hops=%d\r\n", get_lwns_object_port(ptr),
           from->v8[0], from->v8[1], from->v8[2], from->v8[3], from->v8[4],
           from->v8[5], hops); //打印转发者，即为收到的本次转发数据是谁转发的。
    PRINTF("netflood orec %02x %02x %02x %02x %02x %02x,hops=%d\r\n",
           originator->v8[0], originator->v8[1], originator->v8[2],
           originator->v8[3], originator->v8[4], originator->v8[5],
           hops);                   //打印出信息发起者，即为发起本次网络泛洪的节点地址。
    lwns_buffer_save_data(RX_DATA); //接收数据到用户数据区域
    PRINTF("data:");
    for(uint8_t i = 0; i < len; i++)
    {
        PRINTF("%02x ", RX_DATA[i]);
    }
    PRINTF("\n");
    return 1; //返回1，则本节点将继续发送netflood，转发数据
    //return 0;//返回0，则本节点不再继续netflood，直接终止
}

/*********************************************************************
 * @fn      netflood_sent
 *
 * @brief   lwns netflood发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的网络泛洪控制结构体指针.
 *
 * @return  None.
 */
static void netflood_sent(lwns_controller_ptr ptr)
{
    PRINTF("netflood %d sent\n", get_lwns_object_port(ptr));
}

/*********************************************************************
 * @fn      netflood_dropped
 *
 * @brief   lwns netflood数据包丢弃回调函数
 *
 * @param   ptr     -   本次发送完成的网络泛洪控制结构体指针.
 *
 * @return  None.
 */
static void netflood_dropped(lwns_controller_ptr ptr)
{
    PRINTF("netflood %d dropped\n", get_lwns_object_port(ptr));
}

/*********************************************************************
 * @fn      lwns_netflood_process_init
 *
 * @brief   lwns netflood例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_netflood_process_init(void)
{
    netflood_taskID = TMOS_ProcessEventRegister(lwns_netflood_ProcessEvent);
    for(uint8_t i = 0; i < LWNS_DATA_SIZE; i++)
    {
        TX_DATA[i] = i;
    }
    lwns_netflood_init(&netflood,
                       137,                   //打开一个端口号为137的泛洪结构体
                       HTIMER_SECOND_NUM * 1, //等待转发时间
                       1,                     //在等待期间，接收到几次同样的数据包就取消本数据包的发送
                       3,                     //最大转发层级
                       FALSE,                 //在等待转发过程中，收到了新的需要转发的数据包，旧数据包是立刻发送出去还是丢弃，FALSE为立刻发送，TRUE为丢弃。
                       50,                    //网络恢复参数，该值定义了一个差距，如果包序号比内存内保存的数据包序号小的值大于此值，则会认为网络故障恢复，继续接收该数据包。
                       //同时，该值也决定了判定为新数据包的差值，即来自同一个节点的新数据包的序号不可以比内存中的大过多，即比此值还大。
                       //例如，内存中保存的为10，新数据包序号为60，差值为50，大于等于此时设置的50，所以将不会被认为为新的数据包，被丢弃。
                       //只有序号为59，差值为49，小于该值，才会被接收。
                       &callbacks); //返回0代表打开失败。返回1打开成功。
#if 1
    tmos_start_task(netflood_taskID, NETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                    MS1_TO_SYSTEM_TIME(1000));
#endif
}

/*********************************************************************
 * @fn      lwns_netflood_ProcessEvent
 *
 * @brief   lwns netflood Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
static uint16_t lwns_netflood_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & NETFLOOD_EXAMPLE_TX_PERIOD_EVT)
    {
        PRINTF("send\n");
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //载入需要发送的数据到缓冲区
        lwns_netflood_send(&netflood);                   //发送网络泛洪数据包
        tmos_start_task(netflood_taskID, NETFLOOD_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(3000)); //10s发送一次
        return (events ^ NETFLOOD_EXAMPLE_TX_PERIOD_EVT);
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
