/********************************** (C) COPYRIGHT *******************************
 * File Name          : hci_test.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/30
 * Description        : 
 * Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "CH58x_common.h"
#include "hci_test.h"
#include "config.h"

static uint16_t _opcode;
static uint8_t hci_pool[100];
static struct simple_buf hci_buffer;

struct simple_buf *bt_hci_cmd_complete_create(uint16_t op, uint8_t plen)
{
	struct simple_buf *buf;
	struct bt_hci_evt_cmd_complete *cc;
    struct bt_hci_evt_hdr *hdr;
    struct bt_hci_pkt_type *pt;

    buf = simple_buf_create(&hci_buffer, hci_pool, sizeof(hci_pool));

    pt = simple_buf_add(buf, sizeof(*pt));
    pt->type = BT_HCI_PKT_TYPE_EVT;

    hdr = simple_buf_add(buf, sizeof(*hdr));
    hdr->evt = BT_HCI_EVT_CMD_COMPLETE;
    hdr->len = sizeof(*cc) + plen;

	cc = simple_buf_add(buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = op;

	return buf;
}

void *hci_cmd_complete(struct simple_buf **buf, uint8_t plen)
{
	*buf = bt_hci_cmd_complete_create(_opcode, plen);

	return simple_buf_add(*buf, plen);
}

struct simple_buf *cmd_complete_status(uint8_t status)
{
    struct simple_buf *buf;
	struct bt_hci_evt_cc_status *ccst;

    buf = bt_hci_cmd_complete_create(_opcode, sizeof(*ccst));
    ccst = simple_buf_add(buf, sizeof(*ccst));
    ccst->status = status;

    return buf;
}


static void le_rx_test(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_cp_le_rx_test *cmd = (void *)buf->data;
	uint8_t status;

	PRINT("%s:\n", __FUNCTION__);
	PRINT("  rx channel: %#x\n", cmd->rx_ch);

    API_LE_ReceiverTestCmd((uint8_t *)cmd, BT_HCI_OP_LE_RX_TEST);
    *evt = cmd_complete_status(status);
}

/* Modulation_Index feature is not currently supported */
static void le_rx_test_v2(struct simple_buf *buf, struct simple_buf **evt)
{
    struct bt_hci_cp_le_rx_test_v2 *cmd = (void *)buf->data;
    uint8_t status;

    PRINT("%s:\n", __FUNCTION__);
    PRINT("  rx channel: %#x\n", cmd->rx_ch);
	PRINT("  phy: ");
    switch(cmd->phy) {
	case BT_HCI_LE_RX_PHY_1M:
		PRINT("1M\n");
		break;
	case BT_HCI_LE_RX_PHY_2M:
		PRINT("2M\n");
		break;;
	case BT_HCI_LE_RX_PHY_CODED:
		PRINT("CODED\n");
		break;
	default:
		PRINT("Invalid\n");
		break;
    }
	PRINT("  mod_index: %#x\n", cmd->mod_index);

    API_LE_ReceiverTestCmd((uint8_t *)cmd, BT_HCI_OP_LE_RX_TEST_V2);
    *evt = cmd_complete_status(status);
}

static void le_tx_test(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_cp_le_tx_test *cmd = (void *)buf->data;
	uint8_t status;

	PRINT("%s:\n", __FUNCTION__);
	PRINT("  tx channel: %#x\n", cmd->tx_ch);
	PRINT("  test data len: %d\n", cmd->test_data_len);
	PRINT("  pkt payload: ");
	switch (cmd->pkt_payload) {
	case BT_HCI_TEST_PKT_PAYLOAD_PRBS9:
		PRINT("PRBS9\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_11110000:
		PRINT("11110000\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_10101010:
		PRINT("10101010\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_PRBS15:
		PRINT("PRBS15\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_11111111:
		PRINT("11111111\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_00000000:
		PRINT("00000000\n");
		break;	
	case BT_HCI_TEST_PKT_PAYLOAD_00001111:
		PRINT("00001111\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_01010101:
		PRINT("01010101\n");
		break;
	default:
		PRINT("Invalid\n");
		break;
	}
	
    API_LE_TransmitterTestCmd((uint8_t *)cmd, BT_HCI_OP_LE_TX_TEST);
    *evt = cmd_complete_status(status);
}

static void le_tx_test_v2(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_cp_le_tx_test_v2 *cmd = (void *)buf->data;
	uint8_t status;

	PRINT("%s:", __FUNCTION__);
	PRINT("  tx channel: %#x\n", cmd->tx_ch);
	PRINT("  test data len: %d\n", cmd->test_data_len);
	PRINT("  pkt payload: ");
	switch (cmd->pkt_payload) {
	case BT_HCI_TEST_PKT_PAYLOAD_PRBS9:
		PRINT("PRBS9\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_11110000:
		PRINT("11110000\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_10101010:
		PRINT("10101010\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_PRBS15:
		PRINT("PRBS15\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_11111111:
		PRINT("11111111\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_00000000:
		PRINT("00000000\n");
		break;	
	case BT_HCI_TEST_PKT_PAYLOAD_00001111:
		PRINT("00001111\n");
		break;
	case BT_HCI_TEST_PKT_PAYLOAD_01010101:
		PRINT("01010101\n");
		break;
	default:
		PRINT("Invalid\n");
		break;
	}

	PRINT("  phy: ");
    switch(cmd->phy) {
	case BT_HCI_LE_TX_PHY_1M:
		PRINT("1M\n");
		break;
	case BT_HCI_LE_TX_PHY_2M:
		PRINT("2M\n");
		break;;
	case BT_HCI_LE_TX_PHY_CODED_S8:
		PRINT("CODED_S8\n");
		break;
	case BT_HCI_LE_TX_PHY_CODED_S2:
		PRINT("CODED_S2\n");
		break;
	default:
		PRINT("Invalid\n");
		break;
    }

    API_LE_TransmitterTestCmd((uint8_t *)cmd, BT_HCI_OP_LE_TX_TEST_V2);
    *evt = cmd_complete_status(status);
}

static void le_test_end(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_rp_le_test_end *rp;
	uint16_t rx_pkt_count;
	uint8_t status;

    PRINT("%s:\n", __FUNCTION__);
	
	LL_TestEnd((uint8_t *)&rx_pkt_count);
	status = API_LE_TestEndCmd();

	PRINT("  status: %#x\n", status);
	PRINT("  pkt count: %ld\n", rx_pkt_count);

    rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->rx_pkt_count = rx_pkt_count;
}

static void le_set_txpower(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_set_txpower *tp = (void *)buf->data;
	uint8_t status;

    PRINT("%s:\n", __FUNCTION__);
	PRINT("  tx power: %#x\n", tp->tx_power);

	status = LL_SetTxPowerLevel(tp->tx_power);
	*evt = cmd_complete_status(status);
}

static void le_set_xt32m_tune(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_xt32m_tune *xt = (void *)buf->data;
	uint8_t status;

    PRINT("%s:\n", __FUNCTION__);
	PRINT("  xt32m tune: %#x\n", xt->xt32m_tune);

	if(xt->xt32m_tune > 7) {
		*evt = cmd_complete_status(0x12); //Invalid hci comd param
	}

	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
	SAFEOPERATE;
	R8_XT32M_TUNE &= ~RB_XT32M_C_LOAD;
	R8_XT32M_TUNE |= (xt->xt32m_tune<<4);
	R8_SAFE_ACCESS_SIG = 0;

	*evt = cmd_complete_status(0x00);
}

static void le_set_single_carrier(struct simple_buf *buf, struct simple_buf **evt)
{
	struct bt_hci_single_carrier *sc = (void *)buf->data;
	uint8_t status;

    PRINT("%s:\n", __FUNCTION__);
	PRINT("  single carrier: %#x\n", sc->single_carrier);

	status = LL_SingleChannel(sc->single_carrier);
	*evt = cmd_complete_status(status);
}

static int controller_cmd_handle(uint16_t ocf, struct simple_buf *cmd,
				 struct simple_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_LE_RX_TEST):
		le_rx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TX_TEST):
		le_tx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_RX_TEST_V2):
		le_rx_test_v2(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TX_TEST_V2):
		le_tx_test_v2(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TEST_END):
		le_test_end(cmd, evt);
		break;
    default:
        return -1;
    }

	return 0;
}

static int hci_vendor_cmd_handle(uint16_t ocf, struct simple_buf *cmd,
				 struct simple_buf **evt)
{
	switch (ocf)
	{
	case BT_OCF(BT_HCI_SET_TXPOWER):
		le_set_txpower(cmd, evt);
		break;
	case BT_OCF(BT_HCI_XT32M_TUNE):
		le_set_xt32m_tune(cmd, evt);
		break;
	case BT_OCF(BT_HCI_SINGLE_CARRIER):
		le_set_single_carrier(cmd, evt);
		break;
	default:
		return -1;
	}

	return 0;	
}

struct simple_buf *hci_cmd_handle(struct simple_buf *cmd)
{
	struct bt_hci_pkt_type *pt;
    struct bt_hci_cmd_hdr *chdr;
	struct simple_buf *evt = NULL;
	uint16_t ocf;
	int err;

	pt = simple_buf_pull(cmd, sizeof(*pt));
	if(pt->type != BT_HCI_PKT_TYPE_CMD) {
		PRINT("Invalid HCI CMD packet type\n");
		return NULL;
	}

	if (cmd->len < sizeof(*chdr)) {
		PRINT("No HCI Command header\n");
		return NULL;
	}

	chdr = simple_buf_pull(cmd, sizeof(*chdr));

	if (cmd->len < chdr->param_len) {
		PRINT("Invalid HCI CMD packet length\n");
		return NULL;
	}

	/* store in a global for later CC/CS event creation */
	_opcode = chdr->opcode;

	ocf = BT_OCF(_opcode);

	switch (BT_OGF(_opcode)) {
	case BT_OGF_LE:
		err = controller_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_VS:
		err = hci_vendor_cmd_handle(ocf, cmd, &evt);
		break;
    default:
		err = -1;
		break;
    }

    return evt;
}
