#include "mySPI.h"
#include "CH58x_common.h"

void MySPIinit(void)
{
    GPIOA_SetBits( GPIO_Pin_12 );
    GPIOA_ModeCfg( GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14, GPIO_ModeOut_PP_5mA );

    R8_SPI0_CTRL_MOD &= ~RB_SPI_MODE_SLAVE;
    R8_SPI0_CLOCK_DIV = 2;
    R8_SPI0_CTRL_MOD = RB_SPI_ALL_CLEAR;
    R8_SPI0_CTRL_MOD = RB_SPI_MOSI_OE | RB_SPI_SCK_OE ;

    R8_SPI0_CTRL_MOD &= ~RB_SPI_FIFO_DIR;

    R16_SPI0_DMA_BEG = (uint32_t)0;
    R16_SPI0_DMA_END = (uint32_t)(4);
    R16_SPI0_TOTAL_CNT = 0;

    R8_SPI0_INT_FLAG &= ~RB_SPI_IF_DMA_END;

    R8_SPI0_INTER_EN = RB_SPI_IE_DMA_END;

    PFIC_EnableIRQ( SPI0_IRQn );
}


void MySPIdisbale(void)
{
    R8_SPI0_CTRL_MOD &= ~(RB_SPI_MISO_OE | RB_SPI_MOSI_OE | RB_SPI_SCK_OE);
    PFIC_DisableIRQ( SPI0_IRQn );
}

__HIGH_CODE
void MySPIsendbuf( uint8_t *pbuf, uint16_t len)
{
    while(!(R8_SPI0_INT_FLAG & RB_SPI_FREE) )
    {
//        LOG_INFO(".");
        ;
    }

    R8_SPI0_CTRL_MOD |= RB_SPI_ALL_CLEAR;
    R8_SPI0_CTRL_MOD &= ~RB_SPI_ALL_CLEAR;
    R16_SPI0_DMA_BEG = (uint32_t)pbuf;
    R16_SPI0_DMA_END = (uint32_t)(pbuf + len);

    R16_SPI0_TOTAL_CNT = len;
    R8_SPI0_CTRL_CFG |= RB_SPI_DMA_ENABLE;
}

__HIGH_CODE
void MySPIsenddata(uint8_t data)
{
    MySPIsendbuf(&data, 1);

}

__HIGH_CODE
void SPI0_IRQHandler(void)
{
    uint8_t status = R8_SPI0_INT_FLAG;

    R8_SPI0_INT_FLAG = 0xff;

    if(status & RB_SPI_IF_DMA_END){
        R16_SPI0_DMA_BEG = (uint32_t)0;
        R16_SPI0_DMA_END = (uint32_t)(4);
        R8_SPI0_CTRL_CFG &= ~RB_SPI_DMA_ENABLE;

    }

}
/*

static struct SPI_t SPI0 = {
        .txBufferIndex = 0,
        .txBufferLength = 0,
        .transmitting = false,
};


void MySPIinit(void){

    SPI0.txBufferIndex = 0;
    SPI0.txBufferLength = 0;

    GPIOA_SetBits( GPIO_Pin_12 );
    GPIOA_ModeCfg( GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14, GPIO_ModeOut_PP_5mA );

    R8_SPI0_CTRL_MOD &= ~RB_SPI_MODE_SLAVE;
    R8_SPI0_CLOCK_DIV = 2;
    R8_SPI0_CTRL_MOD = RB_SPI_ALL_CLEAR;
    R8_SPI0_CTRL_MOD = RB_SPI_MOSI_OE | RB_SPI_SCK_OE ;

    R8_SPI0_CTRL_MOD &= ~RB_SPI_FIFO_DIR;

    R16_SPI0_TOTAL_CNT = 0;

    R8_SPI0_INT_FLAG &= ~(RB_SPI_IF_CNT_END | RB_SPI_IF_BYTE_END);

    R8_SPI0_INTER_EN = RB_SPI_IE_CNT_END | RB_SPI_IE_BYTE_END | RB_SPI_IE_FIFO_OV;

    PFIC_EnableIRQ( SPI0_IRQn );
}


__HIGH_CODE
int MySPIsendbuf( uint8_t *pbuf, uint16_t len)
{



    return 0;
}

__HIGH_CODE
void MySPIsenddata(uint8_t data) {
    MySPIsendbuf(&data, 1);

}

__HIGH_CODE
void SPI0_IRQHandler(void){
    uint8_t status = R8_SPI0_INT_FLAG;

    R8_SPI0_INT_FLAG = 0xff;

    if(status & RB_SPI_IF_CNT_END){

    }

    if(status & RB_SPI_IF_BYTE_END){

    }

    if(status & RB_SPI_IE_FIFO_OV){

    }
}*/
