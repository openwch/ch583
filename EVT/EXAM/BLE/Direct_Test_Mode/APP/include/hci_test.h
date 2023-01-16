/********************************** (C) COPYRIGHT *******************************
 * File Name          : hci_test.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/29
 * Description        : 
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef BLE_DIRECTTEST_APP_INCLUDE_HCI_TEST_H
#define BLE_DIRECTTEST_APP_INCLUDE_HCI_TEST_H

#include <stdint.h>
#include "buf.h"

#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif

#define BT_HCI_LE_RX_PHY_1M                     0x01
#define BT_HCI_LE_RX_PHY_2M                     0x02
#define BT_HCI_LE_RX_PHY_CODED                  0x03

#define BT_HCI_LE_TX_PHY_1M                     0x01
#define BT_HCI_LE_TX_PHY_2M                     0x02
#define BT_HCI_LE_TX_PHY_CODED_S8               0x03
#define BT_HCI_LE_TX_PHY_CODED_S2               0x04

#define BT_HCI_TEST_PKT_PAYLOAD_PRBS9           0x00
#define BT_HCI_TEST_PKT_PAYLOAD_11110000        0x01
#define BT_HCI_TEST_PKT_PAYLOAD_10101010        0x02
#define BT_HCI_TEST_PKT_PAYLOAD_PRBS15          0x03
#define BT_HCI_TEST_PKT_PAYLOAD_11111111        0x04
#define BT_HCI_TEST_PKT_PAYLOAD_00000000        0x05
#define BT_HCI_TEST_PKT_PAYLOAD_00001111        0x06
#define BT_HCI_TEST_PKT_PAYLOAD_01010101        0x07

struct bt_hci_pkt_type {
	uint8_t type;
}__packed;

struct bt_hci_evt_hdr {
	uint8_t  evt;
	uint8_t  len;
} __packed;

struct bt_hci_cmd_hdr {
	uint16_t opcode;
	uint8_t  param_len;
} __packed;

/* HCI packet type */
#define BT_HCI_PKT_TYPE_CMD						0x01
#define BT_HCI_PKT_TYPE_ACL						0x02
#define BT_HCI_PKT_TYPE_SYNC					0x03
#define BT_HCI_PKT_TYPE_EVT						0x04
#define BT_HCI_PKT_TYPE_ISO						0x05

/* HCI BR/EDR link types */
#define BT_HCI_SCO                              0x00
#define BT_HCI_ACL                              0x01
#define BT_HCI_ESCO                             0x02

/* OpCode Group Fields */
#define BT_OGF_LINK_CTRL                        0x01
#define BT_OGF_BASEBAND                         0x03
#define BT_OGF_INFO                             0x04
#define BT_OGF_STATUS                           0x05
#define BT_OGF_LE                               0x08
#define BT_OGF_VS                               0x3f

/* Construct OpCode from OGF and OCF */
#define BT_OP(ogf, ocf)                         ((ocf) | ((ogf) << 10))

/* Invalid opcode */
#define BT_OP_NOP				0x0000

/* Obtain OGF from OpCode */
#define BT_OGF(opcode)                          (((opcode) >> 10) & ((1<<6)- 1U))
/* Obtain OCF from OpCode */
#define BT_OCF(opcode)                          ((opcode) & ((1<<10) - 1U))

#define BT_HCI_OP_RESET                         BT_OP(BT_OGF_BASEBAND, 0x0003)

#define BT_HCI_OP_LE_TEST_END                   BT_OP(BT_OGF_LE, 0x001f)
struct bt_hci_rp_le_test_end {
	uint8_t  status;
	uint16_t rx_pkt_count;
} __packed;

#define BT_HCI_OP_LE_RX_TEST                    BT_OP(BT_OGF_LE, 0x001d)
struct bt_hci_cp_le_rx_test {
	uint8_t  rx_ch;
} __packed;

#define BT_HCI_OP_LE_RX_TEST_V2         		BT_OP(BT_OGF_LE, 0x0033)
struct bt_hci_cp_le_rx_test_v2 {
	uint8_t  rx_ch;
	uint8_t  phy;
    /* ATTENTION: Modulation_Index feature is not currently supported */
	uint8_t  mod_index;
} __packed;

#define BT_HCI_OP_LE_TX_TEST            		BT_OP(BT_OGF_LE, 0x001e)
struct bt_hci_cp_le_tx_test {
	uint8_t  tx_ch;
	uint8_t  test_data_len;
	uint8_t  pkt_payload;
} __packed;

#define BT_HCI_OP_LE_TX_TEST_V2         		BT_OP(BT_OGF_LE, 0x0034)
struct bt_hci_cp_le_tx_test_v2 {
	uint8_t  tx_ch;
	uint8_t  test_data_len;
	uint8_t  pkt_payload;
	uint8_t  phy;
} __packed;

#define BT_HCI_EVT_CMD_COMPLETE                 0x0e
struct bt_hci_evt_cmd_complete {
	uint8_t  ncmd;
	uint16_t opcode;
} __packed;

struct bt_hci_evt_cc_status {
	uint8_t  status;
} __packed;

/*vender cmds*/
#define BT_HCI_SET_TXPOWER            			BT_OP(BT_OGF_VS, 0x0301)
struct bt_hci_set_txpower {
	uint8_t tx_power;
} __packed;

#define BT_HCI_XT32M_TUNE             			BT_OP(BT_OGF_VS, 0x0302)
struct bt_hci_xt32m_tune {
	uint8_t xt32m_tune;
} __packed;

#define BT_HCI_SINGLE_CARRIER         			BT_OP(BT_OGF_VS, 0x0303)
struct bt_hci_single_carrier {
	uint8_t single_carrier;
} __packed;

struct simple_buf *hci_cmd_handle(struct simple_buf *cmd);

#endif /* BLE_DIRECTTEST_APP_INCLUDE_HCI_TEST_H */
