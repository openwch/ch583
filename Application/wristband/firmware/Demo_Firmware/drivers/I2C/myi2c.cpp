extern "C" {
#include <stdbool.h>
#include <stdlib.h>
#include "twi.h"
#include "CH58x_common.h"
}

#include "myi2c.h"

TwoWire Wire;
// Initialize Class Variables //////////////////////////////////////////////////

uint8_t TwoWire::rxBuffer[BUFFER_LENGTH];
uint8_t TwoWire::rxBufferIndex = 0;
uint8_t TwoWire::rxBufferLength = 0;

uint8_t TwoWire::txAddress = 0;
uint8_t TwoWire::txBuffer[BUFFER_LENGTH];
uint8_t TwoWire::txBufferIndex = 0;
uint8_t TwoWire::txBufferLength = 0;

uint8_t TwoWire::transmitting = 0;
void (*TwoWire::user_onRequest)(void);
void (*TwoWire::user_onReceive)(int);

bool TwoWire::isInit = false;

//static volatile uint8_t twi_error;

//void My_I2C_Init(void) {
//    GPIOB_ModeCfg( GPIO_Pin_13|GPIO_Pin_12, GPIO_ModeOut_PP_5mA );
//    SCL_H;
//    SDA_H;
//}
//
//
///*******************************************************************************
//* Function Name  : I2C_Start
//* Description    : Master Start Simulation I2C Communication
//* Input          : None
//* Output         : None
//* Return         : None
//****************************************************************************** */
//void I2C_Start(void)
//{
//    SDA_OUT;
//	SCL_H;
//	SDA_H;
//    DelayUs(4);
//    SDA_L;
//    DelayUs(4);
//    SCL_L;
//
//}
//
///*******************************************************************************
//* Function Name  : I2C_Stop
//* Description    : Master Stop Simulation I2C Communication
//* Input          : None
//* Output         : None
//* Return         : None
//****************************************************************************** */
//void I2C_Stop(void)
//{
//    SDA_OUT;
//    SCL_L;
//    SDA_L;
//    DelayUs(4);
//    SCL_H;
//    SDA_H;
//    DelayUs(4);
//}
//
//
///*******************************************************************************
//* Function Name  : I2C_WaitAck
//* Description    : Master Reserive Slave Acknowledge Single
//* Input          : None
//* Output         : None
//* Return         : Wheather  Reserive Slave Acknowledge Single
//****************************************************************************** */
//uint8_t I2C_Wait_Ack(void)
//{
//	uint8_t ucErrTime=0;
//	SDA_IN;      //SDA设置为输入
//	SDA_H;DelayUs(1);
//	SCL_H;DelayUs(1);
//	while(READ_SDA)
//	{
//		ucErrTime++;
//		if(ucErrTime>250)
//		{
//			I2C_Stop();
//			return true;
//		}
//	}
//	SCL_L;
//	return false;
//}
//
///*******************************************************************************
//* Function Name  : I2C_Ack
//* Description    : Master Send Acknowledge Single
//* Input          : None
//* Output         : None
//* Return         : None
//****************************************************************************** */
//void I2C_Ack(void)
//{
//	SCL_L;
//	SDA_OUT;
//	SDA_L;
//	DelayUs(2);
//	SCL_H;
//	DelayUs(2);
//	SCL_L;
//}
//
///*******************************************************************************
//* Function Name  : I2C_NoAck
//* Description    : Master Send No Acknowledge Single
//* Input          : None
//* Output         : None
//* Return         : None
//****************************************************************************** */
//void I2C_NAck(void)
//{
//    SCL_L;
//	SDA_OUT;
//	SDA_H;
//	DelayUs(2);
//	SCL_H;
//	DelayUs(2);
//	SCL_L;
//}
//
///*******************************************************************************
//* Function Name  : I2C_SendByte
//* Description    : Master Send a Byte to Slave
//* Input          : Will Send Date
//* Output         : None
//* Return         : None
//****************************************************************************** */
//void I2C_Send_Byte(uint8_t txd)
//{
//    uint8_t t;
//	SDA_OUT;
//	SCL_L;//拉低时钟开始数据传输
//    for(t=0;t<8;t++)
//    {
//        if(txd & 0x80)  SDA_H;
//        else SDA_L;
//
//        txd <<= 1;
//
//        DelayUs(2);
//        SCL_H;
//        DelayUs(2);
//        SCL_L;
//        DelayUs(2);  //tbc
//
//
////        I2C_SDA=(txd&0x80)>>7;
////        txd<<=1;
////		delay_us(2);   //对TEA5767这三个延时都是必须的
////		I2C_SCL=1;
////		delay_us(2);
////		I2C_SCL=0;
////		delay_us(2);
//    }
//}
//
//
///*******************************************************************************
//* Function Name  : I2C_RadeByte
//* Description    : Master Reserive a Byte From Slave
//* Input          : ack: 1:send ack; 0: send nack
//* Output         : None
//* Return         : Date From Slave
//****************************************************************************** */
//uint8_t I2C_Read_Byte(unsigned char ack)
//{
//	unsigned char i,receive=0;
//	SDA_IN;//SDA设置为输入
//    for(i=0;i<8;i++ )
//	{
//        SCL_L;
//        DelayUs(2);
//		SCL_H;
//        receive<<=1;
//        if(READ_SDA)receive++;
//        DelayUs(1);
//    }
//    if (!ack)
//        I2C_NAck();//发送nACK
//    else
//        I2C_Ack(); //发送ACK
//    return receive;
//}
//
//
//uint8_t I2C_WriteBytes(uint8_t deviceAddr, uint8_t* data,uint8_t dataLength, uint8_t sendstop)
//{
//
//    uint8_t sendaddr;
//
//    // ensure data will fit into buffer
//    if(BUFFER_LENGTH < dataLength){
//      return 1;
//    }
//
//    twi_error = 0xFF;
//
//	uint8_t i;
//    /* 第1步：发起I2C总线启动信号 */
//    I2C_Start();
//
//    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
//    sendaddr = I2C_WR;
//    sendaddr |= deviceAddr << 1;
//
//    I2C_Send_Byte(sendaddr);      //发送写命令
//
//    /* 第3步：发送ACK */
//    if (I2C_Wait_Ack() != 0)
//    {
//        twi_error = TW_MT_SLA_NACK;
//    }
//
////    /* 第4步：发送字节地址 */
////	I2C_Send_Byte(WriteAddr);	    //发送写命令
////    if (I2C_Wait_Ack() != 0)
////    {
////        goto cmd_fail;  /* EEPROM器件无应答 */
////    }
//
//    /* 第5步：开始写入数据 */
//	for(i=0;i<dataLength;i++)
//	{
//		I2C_Send_Byte(data[i]);
//	    if (I2C_Wait_Ack() != 0)
//	    {
//	        twi_error = TW_MT_DATA_NACK;
//	    }
//	}
//    if(sendstop) I2C_Stop();//产生一个停止条件
////	delay_ms(10);
//
//    if (twi_error == 0xFF)
//      return 0;   // success
//    else if (twi_error == TW_MT_SLA_NACK)
//      return 2;   // error: address send, nack received
//    else if (twi_error == TW_MT_DATA_NACK)
//      return 3;   // error: data send, nack received
//    else
//      return 4;   // other twi error
//}
//
//uint8_t I2C_ReadBytes(uint8_t deviceAddr, uint8_t* data,uint8_t dataLength, uint8_t sendstop)
//{
//
//	uint8_t i;
//	uint8_t sendaddr;
//	/* 第1步：发起I2C总线启动信号 */
//    I2C_Start();
//
////    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
////	I2C_Send_Byte(deviceAddr | I2C_WR);	    //发送写命令
////
////    /* 第3步：发送ACK */
////    if (I2C_Wait_Ack() != 0)
////    {
////        goto cmd_fail;  /* EEPROM器件无应答 */
////    }
//
//    /* 第4步：发送字节地址， */
////	I2C_Send_Byte(writeAddr);
////    if (I2C_Wait_Ack() != 0)
////    {
////        goto cmd_fail;  /* EEPROM器件无应答 */
////    }
//
//    /* 第6步：重新启动I2C总线。下面开始读取数据 */
////    I2C_Start();
//
//    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
//    sendaddr = I2C_RD;
//    sendaddr |= deviceAddr << 1;
//
//	I2C_Send_Byte(sendaddr);//进入接收模式
//
//    /* 第8步：发送ACK */
//    if (I2C_Wait_Ack() != 0)
//    {
//        goto cmd_fail;  /* EEPROM器件无应答 */
//    }
//
//    /* 第9步：读取数据 */
//	for(i=0;i<dataLength-1;i++)
//	{
//		data[i] = I2C_Read_Byte(1);
//	}
//	data[dataLength-1] = I2C_Read_Byte(0);
//
//	if(sendstop) I2C_Stop();//产生一个停止条件
//
//
//    return dataLength;
//
//
//cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
//    /* 发送I2C总线停止信号 */
//    I2C_Stop();
//    return 0;
//}
//
//void I2C_Read_One_Byte(uint8_t daddr,uint8_t addr,uint8_t* data)
//{
//    I2C_Start();
//
//	I2C_Send_Byte(daddr);	   //发送写命令
//	I2C_Wait_Ack();
//	I2C_Send_Byte(addr);//发送地址
//	I2C_Wait_Ack();
//	I2C_Start();
//	I2C_Send_Byte(daddr|0x01);//进入接收模式
//	I2C_Wait_Ack();
//    *data = I2C_Read_Byte(0);
//    I2C_Stop();//产生一个停止条件
//}
//
//void I2C_Write_One_Byte(uint8_t daddr,uint8_t addr,uint8_t data)
//{
//    I2C_Start();
//
//	I2C_Send_Byte(daddr);	    //发送写命令
//	I2C_Wait_Ack();
//	I2C_Send_Byte(addr);//发送地址
//	I2C_Wait_Ack();
//	I2C_Send_Byte(data);     //发送字节
//	I2C_Wait_Ack();
//    I2C_Stop();//产生一个停止条件
////	delay_ms(10);
//}

void RequestCB(void) {
    PRINT("RequestCB\n");
    /*slave transmit*/
    uint8_t testbuf[5] = { 0x12, 0x22, 0x33, 0x22, 0x33 };
    twi_transmit(testbuf, sizeof(testbuf));

}

void ReceiveCB(int len) {
    PRINT("read data=[");
    for (int i = 0; i < len; i++) {
        i && PRINT(" ");
        PRINT("%#x", Wire.read());
    }PRINT("]\n");
}

void TwoWire::begin(void) {
    if(isInit)
        return;

    rxBufferIndex = 0;
    rxBufferLength = 0;

    txBufferIndex = 0;
    txBufferLength = 0;

    user_onRequest = RequestCB;
    user_onReceive = ReceiveCB;

    twi_init();
    twi_attachSlaveTxEvent(onRequestService); // default callback must exist
    twi_attachSlaveRxEvent(onReceiveService); // default callback must exist

    isInit = true;
}

void TwoWire::end(void) {
    isInit = false;
    twi_disable();
}

void TwoWire::beginTransmission(uint8_t address) {
    // indicate that we are transmitting
    transmitting = 1;
    // set address of targeted slave
    txAddress = address;
    // reset tx buffer iterator vars
    txBufferIndex = 0;
    txBufferLength = 0;
}

//
//  Originally, 'endTransmission' was an f(void) function.
//  It has been modified to take one parameter indicating
//  whether or not a STOP should be performed on the bus.
//  Calling endTransmission(false) allows a sketch to
//  perform a repeated start.
//
//  WARNING: Nothing in the library keeps track of whether
//  the bus tenure has been properly ended with a STOP. It
//  is very possible to leave the bus in a hung state if
//  no call to endTransmission(true) is made. Some I2C
//  devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(bool sendStop) {
    // transmit buffer (blocking)
    uint8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, 1, sendStop);
    // reset tx buffer iterator vars
    txBufferIndex = 0;
    txBufferLength = 0;
    // indicate that we are done transmitting
    transmitting = 0;
    return ret;
}

//  This provides backwards compatibility with the original
//  definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire::endTransmission(void) {
    return endTransmission(true);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(uint8_t data) {
    if (transmitting) {
        // in master transmitter mode
        // don't bother if buffer is full
        if (txBufferLength >= BUFFER_LENGTH) {
            PRINT("IIC ERROR!\n");
            return 0;
        }
        // put byte in tx buffer
        txBuffer[txBufferIndex] = data;
        ++txBufferIndex;
        // update amount in buffer
        txBufferLength = txBufferIndex;
    } else {
        // in slave send mode
        // reply to master
//    twi_transmit(&data, 1);
        PRINT("Slave mode, should not be here!\n");
    }
    return 1;
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(const uint8_t *data, size_t quantity) {
    if (transmitting) {
        // in master transmitter mode
        for (size_t i = 0; i < quantity; ++i) {
            write(data[i]);
        }
    } else {
        // in slave send mode
        // reply to master
//    twi_transmit(data, quantity);
        PRINT("Slave mode, should not be here!\n");
    }
    return quantity;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity,
        uint32_t iaddress, uint8_t isize, uint8_t sendStop) {
    if (isize > 0) {
        // send internal address; this mode allows sending a repeated start to access
        // some devices' internal registers. This function is executed by the hardware
        // TWI module on other processors (for example Due's TWI_IADR and TWI_MMR registers)

        beginTransmission(address);

        // the maximum size of internal address is 3 bytes
        if (isize > 3) {
            isize = 3;
        }

        // write internal register address - most significant byte first
        while (isize-- > 0)
            write((uint8_t) (iaddress >> (isize * 8)));
        endTransmission(false);
    }

    // clamp to buffer length
    if (quantity > BUFFER_LENGTH) {
        quantity = BUFFER_LENGTH;
    }
    // perform blocking read into buffer
    uint8_t read = twi_readFrom(address, rxBuffer, quantity, sendStop);
    // set rx buffer iterator vars
    rxBufferIndex = 0;
    rxBufferLength = read;

    return read;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, bool sendStop) {
    return requestFrom((uint8_t) address, (uint8_t) quantity, (uint32_t) 0,
            (uint8_t) 0, (uint8_t) sendStop);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity) {
    return requestFrom(address, quantity, true);
}

uint8_t TwoWire::requestFrom(int address, int quantity, int sendStop) {
    return requestFrom((uint8_t) address, (uint8_t) quantity, (bool) sendStop);
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::available(void) {
    return rxBufferLength - rxBufferIndex;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::read(void) {
    int value = -1;

    // get each successive byte on each call
    if (rxBufferIndex < rxBufferLength) {
        value = rxBuffer[rxBufferIndex];
        ++rxBufferIndex;
    }

    return value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::peek(void) {
    int value = -1;

    if (rxBufferIndex < rxBufferLength) {
        value = rxBuffer[rxBufferIndex];
    }

    return value;
}

void TwoWire::flush(void) {
    // XXX: to be implemented.
}

// behind the scenes function that is called when data is received
void TwoWire::onReceiveService(uint8_t* inBytes, int numBytes) {
    // don't bother if user hasn't registered a callback
    if (!user_onReceive) {
        return;
    }
    // don't bother if rx buffer is in use by a master requestFrom() op
    // i know this drops data, but it allows for slight stupidity
    // meaning, they may not have read all the master requestFrom() data yet
    if (rxBufferIndex < rxBufferLength) {
        return;
    }
    // copy twi rx buffer into local read buffer
    // this enables new reads to happen in parallel
    for (uint8_t i = 0; i < numBytes; ++i) {
        rxBuffer[i] = inBytes[i];
    }
    // set rx iterator vars
    rxBufferIndex = 0;
    rxBufferLength = numBytes;
    // alert user program
    user_onReceive(numBytes);
}

// behind the scenes function that is called when data is requested
void TwoWire::onRequestService(void) {
    // don't bother if user hasn't registered a callback
    if (!user_onRequest) {
        return;
    }
    // reset tx buffer iterator vars
    // !!! this will kill any pending pre-master sendTo() activity
    txBufferIndex = 0;
    txBufferLength = 0;
    // alert user program
    user_onRequest();
}

void scanI2Cdevice(void) {
    uint8_t err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        err = Wire.endTransmission();
        if (err == 0) {
            PRINT("I2C device found at address ");
            PRINT("%#x", addr);
            PRINT(" !\n");
            nDevices++;
        } else if (err == 4) {
            PRINT("Unknow error at address ");
            PRINT("%#x\n", addr);
        }
    }
    if (nDevices == 0)
        PRINT("No I2C devices found\n");
    else
        PRINT("Done\n");
}

