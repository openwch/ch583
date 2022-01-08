#ifndef DFU_PORT_H_
#define DFU_PORT_H_

#include "CH58x_common.h"


#ifdef __cplusplus
extern "C" {
#endif


#define  SUCCESS                                  0x00

#define FAILURE                                   0x01


/* FLASH定义 */
#define FLASH_BLOCK_SIZE                EEPROM_BLOCK_SIZE
#define IMAGE_SIZE                      208*1024

/* imageA定义 */
#define IMAGE_A_FLAG                    0x55555555
#define IMAGE_A_START_ADD               32*1024
#define IMAGE_A_SIZE                    IMAGE_SIZE

/* imageB定义 */
#define IMAGE_B_FLAG                    0xAAAAAAAA
#define IMAGE_B_START_ADD               (IMAGE_A_START_ADD + IMAGE_SIZE)
#define IMAGE_B_SIZE                    IMAGE_SIZE


#define DFU_CMD_LEN                     4

typedef enum
{
  DFU_NO_ERR = 0,
  DFU_READ_ERR,
  DFU_WRITE_ERR,
  DFU_WRITE_END_ERR,
  DFU_ERASE_ERR,
  DFU_VERIFY_ERR,
  DFU_VERIFY_END_ERR,
  DFU_CMD_ERR,
  DFU_UNEXPECT_ERR
}DFUErrCode;


typedef enum
{
  DFU_READ = 0,
  DFU_WRITE,
  DFU_WRITE_END,
  DFU_ERASE,
  DFU_VERIFY,
  DFU_VERIFY_END,
  DFU_CFG
}DFU_CMD_tpye_t;

typedef struct
{
    bool isReadimage;
    bool isErase;
    bool isWrite;
    bool isWriteend;
    bool isVerify;
    bool isVerifyend;
}DFU_state_t;

#define ISREADIMAGE             1<<0
#define ISERASE                 1<<1
#define ISWRITE                 1<<2
#define ISWRITEEND              1<<3
#define ISVERIFY                1<<4
#define ISVERIFYEND             1<<5

typedef struct
{                                       //read        wrtie     write_end       erase       verify    verify_end
    uint8_t         cmd;                //  0           1           2             3           4           5
    uint8_t         len;                //  4      datelen+4        4             4        datelen+4      4
    uint16_t        idex;               //                                2 bytes
    uint8_t         data[128];          //  0        datelen        0             0         datelen       0
}DFU_DATA_t;


DFUErrCode dfu_data_deal( DFU_DATA_t *data ,uint32_t *value );

#ifdef __cplusplus
}
#endif

#endif /* DFU_PORT_H_ */
