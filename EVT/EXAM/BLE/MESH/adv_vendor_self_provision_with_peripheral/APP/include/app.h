/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/03/31
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_NODE_EVT                       (1 << 0)
#define APP_NODE_PROVISION_EVT             (1 << 1)
#define APP_DELETE_NODE_TIMEOUT_EVT        (1 << 2)
#define APP_DELETE_LOCAL_NODE_EVT          (1 << 3)
#define APP_DELETE_NODE_INFO_EVT           (1 << 4)
#define APP_ASK_STATUS_NODE_TIMEOUT_EVT    (1 << 5)
#define APP_OTA_UPDATE_TIMEOUT_EVT         (1 << 6)
#define APP_SET_SUB_TIMEOUT_EVT            (1 << 7)

#define CMD_PROVISION_INFO                 0xA0
#define CMD_PROVISION_INFO_ACK             0x80
#define CMD_PROVISION                      0xA1
#define CMD_PROVISION_ACK                  0x81
#define CMD_DELETE_NODE                    0xA2
#define CMD_DELETE_NODE_ACK                0x82
#define CMD_DELETE_NODE_INFO               0xA3
#define CMD_DELETE_NODE_INFO_ACK           0x83
#define CMD_ASK_STATUS                     0xA4
#define CMD_ASK_STATUS_ACK                 0x84
#define CMD_TRANSFER                       0xA5
#define CMD_TRANSFER_RECEIVE               0x85
#define CMD_IMAGE_INFO                     0xA6
#define CMD_IMAGE_INFO_ACK                 0x86
#define CMD_UPDATE                         0xA7
#define CMD_UPDATE_ACK                     0x87
#define CMD_VERIFY                         0xA8
#define CMD_VERIFY_ACK                     0x88
#define CMD_END                            0xA9
#define CMD_SET_SUB                        0xAA
#define CMD_SET_SUB_ACK                    0x8A
#define CMD_LOCAL_RESET                    0xAF
#define CMD_LOCAL_RESET_ACK                0x8F

#define PERIPHERAL_CMD_LEN                 1
#define PROVISION_NET_KEY_LEN              16
#define ADDRESS_LEN                        2
#define UPDATE_ADDRESS_LEN                 2

// ÉèÖÃÅäÍøÐÅÏ¢ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+1×Ö½Ú¿ØÖÆ×Ö+4×Ö½Úiv index+1×Ö½Ú¸üÐÂ±êÖ¾flag
#define PROVISION_INFO_DATA_LEN            (PERIPHERAL_CMD_LEN + 1 + 4 + 1)
// ÉèÖÃÅäÍøÐÅÏ¢ÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+1×Ö½Ú×´Ì¬Âë+4×Ö½Úiv index+1×Ö½Ú¸üÐÂ±êÖ¾flag
#define PROVISION_INFO_ACK_DATA_LEN        (PERIPHERAL_CMD_LEN + 1 + 4 + 1)
// ÉèÖÃÅäÍøÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+16×Ö½ÚÍøÂçÃÜÔ¿+2×Ö½ÚÍøÂçµØÖ·
#define PROVISION_DATA_LEN                 (PERIPHERAL_CMD_LEN + PROVISION_NET_KEY_LEN + ADDRESS_LEN)
// ÉèÖÃÅäÍøÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+1×Ö½Ú×´Ì¬Âë
#define PROVISION_ACK_DATA_LEN             (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// É¾³ý½ÚµãÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÐèÒªÉ¾³ýµÄ½ÚµãµØÖ·
#define DELETE_NODE_DATA_LEN               (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// É¾³ý½ÚµãÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÉ¾³ýµÄ½ÚµãµØÖ·+1×Ö½Ú×´Ì¬Âë
#define DELETE_NODE_ACK_DATA_LEN           (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// É¾³ý´æ´¢µÄ½ÚµãÐÅÏ¢ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë
#define DELETE_NODE_INFO_DATA_LEN          (PERIPHERAL_CMD_LEN)
// É¾³ý´æ´¢µÄ½ÚµãÐÅÏ¢ÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÉ¾³ýµÄ½ÚµãµØÖ·
#define DELETE_NODE_INFO_ACK_DATA_LEN      (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// ²éÑ¯½Úµã×´Ì¬ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·
#define ASK_STATUS_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// ²éÑ¯½Úµã×´Ì¬ÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+1×Ö½Ú×´Ì¬Âë
#define ASK_STATUS_ACK_DATA_LEN            (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// Êý¾Ý´«ÊäÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+N×Ö½ÚÄÚÈÝ
#define TRANSFER_DATA_LEN                  (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// Êý¾Ý´«ÊäÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+N×Ö½ÚÄÚÈÝ
#define TRANSFER_RECEIVE_DATA_LEN          (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// OTA²éÑ¯ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·
#define IMAGE_INFO_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// OTA²éÑ¯ÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+4×Ö½Úimage´óÐ¡+2×Ö½Ú¿é´óÐ¡+2×Ö½ÚÐ¾Æ¬ÐÍºÅ+1×Ö½Ú×´Ì¬Âë
#define IMAGE_INFO_ACK_DATA_LEN            (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 4 + 2 + 2 + 1)
// OTAÉý¼¶ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+2×Ö½ÚflashµØÖ·+N×Ö½ÚÄÚÈÝ
#define UPDATE_DATA_LEN                    (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN)
// OTAÉý¼¶ÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+2×Ö½ÚflashµØÖ·+1×Ö½Ú×´Ì¬Âë
#define UPDATE_ACK_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN + 1)
// OTAÐ£ÑéÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+2×Ö½ÚflashµØÖ·+N×Ö½ÚÄÚÈÝ
#define VERIFY_DATA_LEN                    (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN)
// OTAÐ£ÑéÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+2×Ö½ÚflashµØÖ·+1×Ö½Ú×´Ì¬Âë
#define VERIFY_ACK_DATA_LEN                (PERIPHERAL_CMD_LEN + ADDRESS_LEN + UPDATE_ADDRESS_LEN + 1)
// OTA½áÊøÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·
#define END_DATA_LEN                       (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// ÉèÖÃ¶©ÔÄÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+1×Ö½Ú¿ØÖÆ×Ö+2×Ö½ÚÍøÂçµØÖ·
#define SET_SUB_DATA_LEN                   (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1 + ADDRESS_LEN)
// ÉèÖÃ¶©ÔÄÃüÁîÓ¦´ð£¬°üº¬ 1×Ö½ÚÃüÁîÂë+2×Ö½ÚÍøÂçµØÖ·+1×Ö½Ú×´Ì¬Âë
#define SET_SUB_ACK_DATA_LEN               (PERIPHERAL_CMD_LEN + ADDRESS_LEN + 1)
// ±¾µØ¸´Î»ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë
#define LOCAL_RESET_DATA_LEN               (PERIPHERAL_CMD_LEN)
// ±¾µØ¸´Î»ÃüÁî£¬°üº¬ 1×Ö½ÚÃüÁîÂë+1×Ö½Ú×´Ì¬Âë
#define LOCAL_RESET_ACK_DATA_LEN           (PERIPHERAL_CMD_LEN + 1)

// ×´Ì¬Âë¶¨Òå
#define STATUS_SUCCESS                     0x00
#define STATUS_TIMEOUT                     0x01
#define STATUS_NOMEM                       0x02
#define STATUS_INVALID                     0x03

#define APP_MAX_TX_SIZE                    MAX(CONFIG_MESH_UNSEG_LENGTH_DEF, CONFIG_MESH_TX_SEG_DEF *BLE_MESH_APP_SEG_SDU_MAX - 8)

/* Õû¸öÓÃ»§codeÇø·Ö³ÉÎå¿é£¬4K£¬152K£¬152K£¬4K£¬136K£¬ºóËÄ¿éÏÂÃæ·Ö±ð½Ð×öimageA£¨APP£©£¬imageB£¨OTA£©£¬imageIAPºÍLIB */

/* FLASH¶¨Òå */
#define FLASH_BLOCK_SIZE                   EEPROM_BLOCK_SIZE
#define IMAGE_SIZE                         152 * 1024

/* imageA¶¨Òå */
#define IMAGE_A_FLAG                       0x01
#define IMAGE_A_START_ADD                  0x1000
#define IMAGE_A_SIZE                       IMAGE_SIZE

/* imageB¶¨Òå */
#define IMAGE_B_FLAG                       0x02
#define IMAGE_B_START_ADD                  (IMAGE_A_START_ADD + IMAGE_SIZE)
#define IMAGE_B_SIZE                       IMAGE_SIZE

/* imageIAP¶¨Òå */
#define IMAGE_IAP_FLAG                     0x03
#define IMAGE_IAP_START_ADD                (IMAGE_B_START_ADD + IMAGE_SIZE)
#define IMAGE_IAP_SIZE                     4 * 1024

/* ´æ·ÅÔÚDataFlashµØÖ·£¬²»ÄÜÕ¼ÓÃÀ¶ÑÀµÄÎ»ÖÃ */
#define OTA_DATAFLASH_ADD                  0x00077000 - FLASH_ROM_MAX_SIZE

/* ´æ·ÅÔÚDataFlashÀïµÄOTAÐÅÏ¢ */
typedef struct
{
    unsigned char ImageFlag; //¼ÇÂ¼µÄµ±Ç°µÄimage±êÖ¾
    unsigned char Revd[3];
} OTADataFlashInfo_t;

/******************************************************************************/

typedef struct
{
    uint16_t node_addr;
    uint16_t elem_count;
    uint16_t net_idx;
    uint16_t retry_cnt : 12,
        fixed : 1,
        blocked : 1;

} node_t;

typedef union
{
    struct
    {
        uint8_t cmd;         /* ÃüÁîÂë CMD_PROVISION_INFO */
        uint8_t set_flag;    /* ¿ØÖÆ×Ö Îª1±íÊ¾ÉèÖÃ£¬Îª0±íÊ¾²éÑ¯*/
        uint8_t iv_index[4]; /* iv index */
        uint8_t flag;        /* Net key refresh flag */
    } provision_info;        /* ÅäÍøÐÅÏ¢ÃüÁî */
    struct
    {
        uint8_t cmd;         /* ÃüÁîÂë CMD_PROVISION_INFO_ACK */
        uint8_t status;      /* ×´Ì¬Âë*/
        uint8_t iv_index[4]; /* iv index */
        uint8_t flag;        /* Net key refresh flag */
    } provision_info_ack;    /* ÅäÍøÐÅÏ¢ÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;                            /* ÃüÁîÂë CMD_PROVISION */
        uint8_t net_key[PROVISION_NET_KEY_LEN]; /* ºóÐøÊý¾Ý³¤¶È */
        uint8_t addr[ADDRESS_LEN];              /* ÅäÍøµØÖ· */
    } provision;                                /* ÅäÍøÃüÁî */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_PROVISION_ACK */
        uint8_t addr[ADDRESS_LEN]; /* ÅäÍøµØÖ· */
        uint8_t status;            /* ×´Ì¬Âë±¸ÓÃ */
    } provision_ack;               /* ÅäÍøÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_DELETE_NODE */
        uint8_t addr[ADDRESS_LEN]; /* É¾³ýµØÖ· */
    } delete_node;                 /* É¾³ý½ÚµãÃüÁî */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_DELETE_NODE_ACK */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
        uint8_t status;            /* ×´Ì¬Âë */
    } delete_node_ack;             /* É¾³ý½ÚµãÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;    /* ÃüÁîÂë CMD_DELETE_NODE_INFO */
    } delete_node_info; /* É¾³ý´æ´¢µÄ½ÚµãÐÅÏ¢ÃüÁî */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_DELETE_NODE_INFO_ACK */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
    } delete_node_info_ack;        /* É¾³ý´æ´¢µÄ½ÚµãÐÅÏ¢ÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_ASK_STATUS */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
    } ask_status;                  /* ²éÑ¯½Úµã×´Ì¬ÃüÁî */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_ASK_STATUS_ACK */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
        uint8_t status;            /* ×´Ì¬Âë±¸ÓÃ */
    } ask_status_ack;              /* ²éÑ¯½Úµã×´Ì¬ÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;                       /* ÃüÁîÂë CMD_TRANSFER */
        uint8_t addr[ADDRESS_LEN];         /* ·¢ËÍµØÖ· */
        uint8_t data[APP_MAX_TX_SIZE - 3]; /* Êý¾ÝÄÚÈÝ*/
    } transfer;                            /* ·¢ËÍÊý¾ÝÃüÁî */
    struct
    {
        uint8_t cmd;                       /* ÃüÁîÂë CMD_TRANSFER_ACK */
        uint8_t addr[ADDRESS_LEN];         /* ·¢ËÍµØÖ· */
        uint8_t data[APP_MAX_TX_SIZE - 3]; /* Êý¾ÝÄÚÈÝ*/
    } transfer_receive;                    /* ·¢ËÍÊý¾ÝÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_IMAGE_INFO */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
    } image_info;                  /* OTA²éÑ¯ÃüÁî */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_IMAGE_INFO_ACK */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
        uint8_t image_size[4];     /* image´óÐ¡ */
        uint8_t block_size[2];     /* falsh¿é´óÐ¡ */
        uint8_t chip_id[2];        /* Ð¾Æ¬ÐÍºÅ */
        uint8_t status;            /* ×´Ì¬Âë±¸ÓÃ */
    } image_info_ack;              /* OTA²éÑ¯ÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;                             /* ÃüÁîÂë CMD_UPDATE */
        uint8_t addr[ADDRESS_LEN];               /* ·¢ËÍµØÖ· */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* Éý¼¶µØÖ· */
        uint8_t data[APP_MAX_TX_SIZE - 5];       /* Éý¼¶Êý¾ÝÄÚÈÝ*/
    } update;                                    /* OTAÉý¼¶Êý¾ÝÃüÁî */
    struct
    {
        uint8_t cmd;                             /* ÃüÁîÂë CMD_UPDATE_ACK */
        uint8_t addr[ADDRESS_LEN];               /* ·¢ËÍµØÖ· */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* Éý¼¶µØÖ· */
        uint8_t status;                          /* ×´Ì¬Âë±¸ÓÃ */
    } update_ack;                                /* OTAÉý¼¶Êý¾ÝÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;                             /* ÃüÁîÂë CMD_VERIFY */
        uint8_t addr[ADDRESS_LEN];               /* ·¢ËÍµØÖ· */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* Éý¼¶µØÖ· */
        uint8_t data[APP_MAX_TX_SIZE - 5];       /* Éý¼¶Êý¾ÝÄÚÈÝ*/
    } verify;                                    /* OTAÑéÖ¤Êý¾ÝÃüÁî */
    struct
    {
        uint8_t cmd;                             /* ÃüÁîÂë CMD_VERIFY_ACK */
        uint8_t addr[ADDRESS_LEN];               /* ·¢ËÍµØÖ· */
        uint8_t update_addr[UPDATE_ADDRESS_LEN]; /* Éý¼¶µØÖ· */
        uint8_t status;                          /* ×´Ì¬Âë±¸ÓÃ */
    } verify_ack;                                /* OTAÑéÖ¤Êý¾ÝÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_END */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
    } end;                         /* OTAÍê³ÉÃüÁî */
    struct
    {
        uint8_t cmd;                   /* ÃüÁîÂë CMD_SET_SUB */
        uint8_t addr[ADDRESS_LEN];     /* ·¢ËÍµØÖ· */
        uint8_t add_flag;              /* Îª1±íÊ¾Ìí¼Ó£¬Îª0±íÊ¾É¾³ý */
        uint8_t sub_addr[ADDRESS_LEN]; /* ¶©ÔÄµØÖ· */
    } set_sub;                         /* ÉèÖÃ¶©ÔÄÃüÁî */
    struct
    {
        uint8_t cmd;               /* ÃüÁîÂë CMD_SET_SUB_ACK */
        uint8_t addr[ADDRESS_LEN]; /* ·¢ËÍµØÖ· */
        uint8_t status;            /* ×´Ì¬Âë */
    } set_sub_ack;                 /* ÉèÖÃ¶©ÔÄÃüÁîÓ¦´ð */
    struct
    {
        uint8_t cmd; /* ÃüÁîÂë CMD_LOCAL_RESET */
    } local_reset;   /* ±¾µØ»Ö¸´³ö³§ÉèÖÃÃüÁî */
    struct
    {
        uint8_t cmd;    /* ÃüÁîÂë CMD_LOCAL_RESET */
        uint8_t status; /* ×´Ì¬Âë±¸ÓÃ */
    } local_reset_ack;  /* ±¾µØ»Ö¸´³ö³§ÉèÖÃÃüÁîÓ¦´ð */
    struct
    {
        uint8_t buf[APP_MAX_TX_SIZE]; /* Êý¾ÝÄÚÈÝ*/
    } data;
} app_mesh_manage_t;

void App_Init(void);

void App_peripheral_reveived(uint8_t *pValue, uint16_t len);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
