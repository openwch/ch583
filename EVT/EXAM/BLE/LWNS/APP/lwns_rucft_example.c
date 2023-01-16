/********************************** (C) COPYRIGHT *******************************
 * File Name          : lwns_rucft_example.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/06/30
 * Description        : reliable unicast file transfer，可靠单播文件传输例子
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "lwns_rucft_example.h"

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
static lwns_addr_t dst_addr = {{0x66, 0xdf, 0x38, 0xe4, 0xc2, 0x84}}; //目标节点地址，测试时，请根据电路板芯片MAC地址不同进行修改。修改为接收方的MAC地址，请勿使用自己的MAC地址
#else
static lwns_addr_t dst_addr = {{0xd9, 0x37, 0x3c, 0xe4, 0xc2, 0x84}};
#endif

static uint8_t rucft_taskID;

static lwns_rucft_controller rucft; //声明rucft控制结构体

#define FILESIZE    4000
static char  strsend[FILESIZE]; //发送缓冲区
static char *strp;
static void  write_file(lwns_controller_ptr ptr, const lwns_addr_t *sender,
                        int offset, int flag, char *data, int datalen);
static int   read_file(lwns_controller_ptr ptr, int offset, char *to);
static void  timedout_rucft(lwns_controller_ptr ptr);

/**
 * lwns 可靠单播文件传输回调函数结构体，注册回调函数
 */
const static struct lwns_rucft_callbacks rucft_callbacks = {write_file,
                                                            read_file, timedout_rucft};

uint16_t lwns_rucft_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
 * @fn      write_file
 *
 * @brief   lwns rucft接收回调函数，作为写入文件回调函数
 *
 * @param   ptr         -   本次接收到的数据所属的netflood控制结构体指针.
 * @param   sender      -   本次接收到的数据的发送者地址指针.
 * @param   offset      -   本次接收到的数据的偏移量，也是本次文件传输接收已经收到的数据量.
 * @param   flag        -   本次接收到数据的标志，LWNS_RUCFT_FLAG_NONE/LWNS_RUCFT_FLAG_NEWFILE/LWNS_RUCFT_FLAG_END.
 * @param   data        -   本次接收到的数据的头指针.
 * @param   datalen     -   本次接收到的数据的长度.
 *
 * @return  None.
 */
static void write_file(lwns_controller_ptr ptr, const lwns_addr_t *sender,
                       int offset, int flag, char *data, int datalen)
{
    //sender为发送方的地址
    //如果需要接收不同的文件，需要在此函数中做好接口
    if(datalen > 0)
    { //声明个缓冲从data里取数据打印
        PRINTF("r:%c\n", *data);
    }
    if(flag == LWNS_RUCFT_FLAG_END)
    {
        PRINTF("re\n");
        //本次文件传输的最后一个包
    }
    else if(flag == LWNS_RUCFT_FLAG_NONE)
    {
        PRINTF("ru\n");
        //本次文件传输正常的包
    }
    else if(flag == LWNS_RUCFT_FLAG_NEWFILE)
    {
        PRINTF("rn\n");
        //本次文件传输的第一个包
    }
}

/*********************************************************************
 * @fn      read_file
 *
 * @brief   lwns rucft发送完成回调函数，作为读取文件回调函数
 *
 * @param   ptr         -   本次接收到的数据所属的netflood控制结构体指针.
 * @param   offset      -   本次接收到的数据的偏移量，也是本次文件传输发送已经发送的数据量.
 * @param   to          -   本次需要发送的数据缓冲区的头指针，用户将数据拷贝到此指针指向的内存空间。.
 *
 * @return  size        -   返回的size即为本次需要发送的数据长度，不可大于LWNS_RUCFT_DATASIZE.
 */
static int read_file(lwns_controller_ptr ptr, int offset, char *to)
{
    //to为需要保存数据过去的指针
    //如果需要发送不同的文件，需要在此函数中做好接口
    int size = LWNS_RUCFT_DATASIZE;
    if(offset >= FILESIZE)
    {
        //上次已经发完,本次是最后确认
        PRINTF("Send done\n");
        tmos_start_task(rucft_taskID, RUCFT_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(5000)); //5秒钟后继续发送测试
        return 0;
    }
    else if(offset + LWNS_RUCFT_DATASIZE >= FILESIZE)
    {
        size = FILESIZE - offset;
    }
    //把本次需要发送的内容压进包缓冲
    tmos_memcpy(to, strp + offset, size);
    return size;
}

/*********************************************************************
 * @fn      timedout_rucft
 *
 * @brief   lwns rucft发送超时回调函数
 *
 * @param   ptr     -   本次发送完成的ruc控制结构体指针.
 *
 * @return  None.
 */
static void timedout_rucft(lwns_controller_ptr ptr)
{
    //rucft中，发送方再重发次数超过最大重发次数后，会调用该回调。
    //接收方超时没接收到下一个包也会调用
    PRINTF("rucft %d timedout\r\n", get_lwns_object_port(ptr));
}

/*********************************************************************
 * @fn      lwns_rucft_process_init
 *
 * @brief   lwns rucft例程初始化.
 *
 * @param   None.
 *
 * @return  None.
 */
void lwns_rucft_process_init(void)
{
    lwns_addr_t MacAddr;
    rucft_taskID = TMOS_ProcessEventRegister(lwns_rucft_ProcessEvent);
    lwns_rucft_init(&rucft, 137,            //端口号
                    HTIMER_SECOND_NUM / 10, //等待目标节点ack时间
                    5,                      //最大重发次数，与ruc中的ruc_send的重发次数功能一样
                    &rucft_callbacks        //回调函数
    );                                      //返回0代表打开失败。返回1打开成功。
    int i;
    for(i = 0; i < FILESIZE; i++)
    { //LWNS_RUCFT_DATASIZE个LWNSNK_RUCFT_DATASIZE个b，等等，初始化需要发送的数据
        strsend[i] = 'a' + i / LWNS_RUCFT_DATASIZE;
    }
    strp = strsend;
    GetMACAddress(MacAddr.v8);
    if(lwns_addr_cmp(&MacAddr, &dst_addr))
    {
    }
    else
    {
        tmos_start_task(rucft_taskID, RUCFT_EXAMPLE_TX_PERIOD_EVT,
                        MS1_TO_SYSTEM_TIME(1000));
    }
}

/*********************************************************************
 * @fn      lwns_rucft_ProcessEvent
 *
 * @brief   lwns rucft Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed.
 */
uint16_t lwns_rucft_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & RUCFT_EXAMPLE_TX_PERIOD_EVT)
    {
        PRINTF("send\n");
        lwns_rucft_send(&rucft, &dst_addr); //开始发送至目标节点，用户启用发送时要配置好回调函数中的数据包读取
        return events ^ RUCFT_EXAMPLE_TX_PERIOD_EVT;
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
