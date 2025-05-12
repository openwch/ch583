/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2021/11/18
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_NODE_TEST_EVT               (1 << 0)
#define APP_DELETE_LOCAL_NODE_EVT       (1 << 1)
#define APP_DELETE_NODE_INFO_EVT        (1 << 2)

#define CMD_DELETE_NODE                0xA2
#define CMD_DELETE_NODE_ACK            0x82
#define CMD_DELETE_NODE_INFO           0xA3

#define PERIPHERAL_CMD_LEN             1
#define ADDRESS_LEN                    2

// ɾ���ڵ�������� 1�ֽ�������+2�ֽ���Ҫɾ���Ľڵ��ַ
#define DELETE_NODE_DATA_LEN           (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// ɾ���ڵ�����Ӧ�𣬰��� 1�ֽ�������
#define DELETE_NODE_ACK_DATA_LEN       (PERIPHERAL_CMD_LEN)
// ɾ���洢�Ľڵ���Ϣ������� 1�ֽ�������
#define DELETE_NODE_INFO_DATA_LEN      (PERIPHERAL_CMD_LEN)

/******************************************************************************/

typedef union
{
    struct
    {
        uint8_t cmd;                /* ������ CMD_DELETE_NODE */
        uint8_t addr[ADDRESS_LEN];  /* ������ַ */
    } delete_node;                  /* ɾ���ڵ����� */
    struct
    {
        uint8_t cmd;                /* ������ CMD_DELETE_NODE_ACK */
    } delete_node_ack;              /* ɾ���ڵ�����Ӧ�� */
    struct
    {
        uint8_t cmd;                /* ������ CMD_DELETE_NODE_INFO */
    } delete_node_info;             /* ɾ���洢�Ľڵ���Ϣ���� */
    struct
    {
        uint8_t buf[MAX(CONFIG_MESH_UNSEG_LENGTH_DEF, CONFIG_MESH_TX_SEG_DEF *BLE_MESH_APP_SEG_SDU_MAX - 8)]; /* �������ݰ�*/
    } data;
}app_mesh_manage_t;

/**
 * @brief   Ӧ�ò��ʼ��
 */
void App_Init(void);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
