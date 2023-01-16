/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_mesh_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/28
 * Description        : mesh传输程序例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_mesh_example.h"

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

#define MESH_TEST_ROUTE_AUTO       1  //设置是否自动管理路由表
#define MESH_TEST_SELF_ADDR_IDX    2  //当前测试中节点地址，在device_addr数组中
#define MESH_TEST_ADDR_MAX_IDX     2  //当前测试中，有多少个节点

static uint8_t mesh_test_send_dst = 0;

static lwns_mesh_controller lwns_mesh_test;

static uint8_t           lwns_mesh_test_taskID;
static uint16_t          lwns_mesh_test_ProcessEvent(uint8_t task_id, uint16_t events);
static uint8_t           TX_DATA[10] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                              0x38, 0x39};
static const lwns_addr_t device_addr[] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}, //根节点地址
    {0x00, 0x00, 0x00, 0x00, 0x01, 0x01}, //组1路由节点1
    {0x00, 0x00, 0x00, 0x00, 0x01, 0x02}, //组1节点2
    {0x00, 0x00, 0x00, 0x00, 0x01, 0x03}, //组1节点3
    {0x00, 0x00, 0x00, 0x00, 0x01, 0x04}, //组1节点4
    {0x00, 0x00, 0x00, 0x00, 0x02, 0x01}, //组2路由节点1
    {0x00, 0x00, 0x00, 0x00, 0x02, 0x02}, //组2节点2
    {0x00, 0x00, 0x00, 0x00, 0x02, 0x03}, //组2节点3
    {0x00, 0x00, 0x00, 0x00, 0x02, 0x04}, //组2节点4
    {0x00, 0x00, 0x00, 0x00, 0x03, 0x01}, //组3路由节点1
    {0x00, 0x00, 0x00, 0x00, 0x03, 0x02}, //组3节点2
    {0x00, 0x00, 0x00, 0x00, 0x03, 0x03}, //组3节点3
    {0x00, 0x00, 0x00, 0x00, 0x03, 0x04}, //组3节点4
};

static void lwns_mesh_recv(lwns_controller_ptr ptr,
                           const lwns_addr_t *sender, uint8_t hops);
static void lwns_mesh_sent(lwns_controller_ptr ptr);
static void lwns_mesh_timedout(lwns_controller_ptr ptr);

/*********************************************************************
 * @fn      lwns_mesh_recv
 *
 * @brief   lwns mesh接收回调函数
 *
 * @param   ptr     -   本次接收到的数据所属的mesh控制结构体指针.
 * @param   sender  -   本次接收到的数据的发送者地址指针.
 * @param   hops    -   本次接收到的数据的从发送方到本节点经历的跳数.
 *
 * @return  None.
 */
static void lwns_mesh_recv(lwns_controller_ptr ptr,
                           const lwns_addr_t *sender, uint8_t hops)
{
    PRINTF("mesh %d received from %02x.%02x.%02x.%02x.%02x.%02x: %s (%d)\n",
           get_lwns_object_port(ptr), sender->v8[0], sender->v8[1],
           sender->v8[2], sender->v8[3], sender->v8[4], sender->v8[5],
           (char *)lwns_buffer_dataptr(), lwns_buffer_datalen());
    if(MESH_TEST_SELF_ADDR_IDX != 0)
    {
        //不为0，证明是从机，收到后回复主机
        lwns_buffer_save_data(TX_DATA);
        tmos_set_event(lwns_mesh_test_taskID, MESH_EXAMPLE_TX_NODE_EVT);
    }
}

/*********************************************************************
 * @fn      lwns_mesh_sent
 *
 * @brief   lwns mesh发送完成回调函数
 *
 * @param   ptr     -   本次发送完成的mesh控制结构体指针.
 *
 * @return  None.
 */
static void lwns_mesh_sent(lwns_controller_ptr ptr)
{
    PRINTF("mesh %d packet sent\n", get_lwns_object_port(ptr));
}

/*********************************************************************
 * @fn      lwns_mesh_timedout
 *
 * @brief   lwns mesh发送超时回调函数
 *
 * @param   ptr     -   本次发送完成的mesh控制结构体指针.
 *
 * @return  None.
 */
static void lwns_mesh_timedout(lwns_controller_ptr ptr)
{
    //寻找路由超时才会进入该回调，如果已经有路由，但是下一跳节点掉线，不会进入该回调。由lwns_route_init(TRUE, 60, HTIMER_SECOND_NUM);来管理掉线时间。
    PRINTF("mesh %d packet timedout\n", get_lwns_object_port(ptr));
}

/**
 * lwns mesh回调函数结构体，注册回调函数
 */
static const struct lwns_mesh_callbacks callbacks = {lwns_mesh_recv,
                                                     lwns_mesh_sent, lwns_mesh_timedout};

/*
 * mesh采用netflood模块进行寻找路由，所以初始化参数中包括了netflood所需要的参数
 * mesh为利用路由进行多跳转发的基础结构
 * mesh模块使用前必须初始化路由，包括库初始化时给路由表提供的内存空间。
 */

/*********************************************************************
 * @fn      lwns_mesh_process_init
 *
 * @brief   lwns mesh例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_mesh_process_init(void)
{
    uint8_t route_enable = FALSE;
    lwns_addr_set(
        (lwns_addr_t *)(&device_addr[MESH_TEST_SELF_ADDR_IDX])); //改变lwns内部addr
    if(device_addr[MESH_TEST_SELF_ADDR_IDX].v8[5] == 1)
    {
        //每一组第一个节点打开路由功能，其他节点不打开路由功能
        route_enable = TRUE;
    }
    lwns_mesh_test_taskID = TMOS_ProcessEventRegister(lwns_mesh_test_ProcessEvent);
#if MESH_TEST_ROUTE_AUTO
    lwns_route_init(TRUE, 60, HTIMER_SECOND_NUM);
#else
    lwns_route_init(TRUE, 0, HTIMER_SECOND_NUM); //disable auto clean route entry
#endif                                    /*MESH_TEST_ROUTE_AUTO*/
    lwns_mesh_init(&lwns_mesh_test, 132,  //打开一个端口号132的mesh网络，同时占用后其他的两个端口用作寻找路由，即133，134也同时打开了。
                   HTIMER_SECOND_NUM / 2, //netflood的QUEUETIME功能
                   1,                     //netflood的dups功能
                   2,                     //最大跳跃次数层级为5级，即数据包最多可以经过5次转发，超出就丢弃数据包
                   FALSE,                 //在路由请求的转发过程中，收到了新的需要转发的数据包，旧数据包是立刻发送出去还是丢弃，FALSE为立刻发送，TRUE为丢弃。
                   50,                    //网络恢复参数，该值定义了一个差距，如果包序号比内存内保存的数据包序号小的值大于此值，则会认为网络故障恢复，继续接收该数据包。
                   //同时，该值也决定了判定为新数据包的差值，即来自同一个节点的新数据包的序号不可以比内存中的大过多，即比此值还大。
                   //例如，内存中保存的为10，新数据包序号为60，差值为50，大于等于此时设置的50，所以将不会被认为为新的数据包，被丢弃。
                   //只有序号为59，差值为49，小于该值，才会被接收。
                   route_enable,          //是否使能本机的路由功能，如果为false，不会响应其他节点的路由请求。
                   TRUE,                  //决定是否添加路由回路，比如收到了一个来自a节点的mesh数据包，本机是否需要存储前往a节点的路由表。FALSE不存，TRUE存。
                   HTIMER_SECOND_NUM * 2, //路由超时时间，超过时间，停止寻找路由，进入timeout回调，该值应大于 route_discovery_hoptime * (hops+1) * 2
                   &callbacks);           //没有初始化路由表的话，会返回0，代表打开失败。返回1打开成功。
    if(MESH_TEST_SELF_ADDR_IDX == 0)
    { //如果是主机，则开始周期性轮训其他节点。
        mesh_test_send_dst = 1;
        tmos_start_task(lwns_mesh_test_taskID, MESH_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(200));
#if MESH_TEST_ROUTE_AUTO
        PRINTF("Auto route\n");
#else
        uint8_t i;
        for(i = 1; i < 5; i++)
        {
            lwns_route_add(&device_addr[i], &device_addr[1], 2); //手动管理，添加路由条目
        }
        for(i = 5; i < 9; i++)
        {
            lwns_route_add(&device_addr[i], &device_addr[5], 2); //手动管理，添加路由条目
        }
        for(i = 9; i < 13; i++)
        {
            lwns_route_add(&device_addr[i], &device_addr[9], 2); //手动管理，添加路由条目
        }
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 1)
    {
        lwns_route_add(&device_addr[4], &device_addr[4], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[3], &device_addr[3], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[2], &device_addr[2], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[0], &device_addr[0], 1); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 2)
    {
        lwns_route_add(&device_addr[0], &device_addr[1], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 3)
    {
        lwns_route_add(&device_addr[0], &device_addr[1], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 4)
    {
        lwns_route_add(&device_addr[0], &device_addr[1], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 5)
    {
        lwns_route_add(&device_addr[6], &device_addr[6], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[7], &device_addr[7], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[8], &device_addr[8], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[0], &device_addr[0], 1); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 6)
    {
        lwns_route_add(&device_addr[0], &device_addr[5], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 7)
    {
        lwns_route_add(&device_addr[0], &device_addr[5], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 8)
    {
        lwns_route_add(&device_addr[0], &device_addr[5], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 9)
    {
        lwns_route_add(&device_addr[10], &device_addr[10], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[11], &device_addr[11], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[12], &device_addr[12], 1); //手动管理，添加路由条目
        lwns_route_add(&device_addr[0], &device_addr[0], 1);   //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 10)
    {
        lwns_route_add(&device_addr[0], &device_addr[9], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 11)
    {
        lwns_route_add(&device_addr[0], &device_addr[9], 2); //手动管理，添加路由条目
    }
    else if(MESH_TEST_SELF_ADDR_IDX == 12)
    {
        lwns_route_add(&device_addr[0], &device_addr[9], 2); //手动管理，添加路由条目
#endif /*MESH_TEST_ROUTE_AUTO*/
    }
}

/*********************************************************************
 * @fn      lwns_mesh_test_ProcessEvent
 *
 * @brief   lwns mesh Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
static uint16_t lwns_mesh_test_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & MESH_EXAMPLE_TX_PERIOD_EVT)
    { //主机周期性轮询从机任务
        uint8_t                  temp;
        struct lwns_route_entry *rt;
        temp = TX_DATA[0];
        for(uint8_t i = 0; i < 9; i++)
        {
            TX_DATA[i] = TX_DATA[i + 1]; //移位发送数据，以便观察效果
        }
        TX_DATA[9] = temp;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA));          //载入需要发送的数据到缓冲区
        rt = lwns_route_lookup(&device_addr[mesh_test_send_dst]); //在路由表中寻找下一跳路由
        if(rt != NULL)
        {
            //打印下一跳路由信息
            PRINTF("dst:%d,forwarding to %02x.%02x.%02x.%02x.%02x.%02x\n",
                   mesh_test_send_dst, rt->nexthop.v8[0], rt->nexthop.v8[1],
                   rt->nexthop.v8[2], rt->nexthop.v8[3], rt->nexthop.v8[4],
                   rt->nexthop.v8[5]);
        }
        else
        {
            PRINTF("no route to dst:%d\n", mesh_test_send_dst);
        }
        lwns_mesh_send(&lwns_mesh_test, &device_addr[mesh_test_send_dst]); //发送mesh消息
        mesh_test_send_dst++;                                              //轮询目标节点改为下一个
        if(mesh_test_send_dst > MESH_TEST_ADDR_MAX_IDX)
        { //循环往复轮询
            mesh_test_send_dst = 1;
        }
        tmos_start_task(lwns_mesh_test_taskID, MESH_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(2500)); //周期性轮询
        return (events ^ MESH_EXAMPLE_TX_PERIOD_EVT);
    }
    if(events & MESH_EXAMPLE_TX_NODE_EVT)
    { //节点给主机发送数据的任务
        struct lwns_route_entry *rt;
        lwns_buffer_load_data(TX_DATA, sizeof(TX_DATA)); //载入需要发送的数据到缓冲区
        rt = lwns_route_lookup(&device_addr[0]);         //在路由表中寻找下一跳路由
        if(rt != NULL)
        {
            //打印出下一跳路由信息
            PRINTF("src:%d,forwarding to %02x.%02x.%02x.%02x.%02x.%02x\n",
                   MESH_TEST_SELF_ADDR_IDX, rt->nexthop.v8[0],
                   rt->nexthop.v8[1], rt->nexthop.v8[2], rt->nexthop.v8[3],
                   rt->nexthop.v8[4], rt->nexthop.v8[5]);
        }
        lwns_mesh_send(&lwns_mesh_test, &device_addr[0]); //调用mesh发送函数，进行数据的发送
        return (events ^ MESH_EXAMPLE_TX_NODE_EVT);
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
