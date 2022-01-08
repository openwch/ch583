/*
 twi.c - TWI/I2C library for Wiring & Arduino
 Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
 */

#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include "CH58x_common.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include "twi.h"

static volatile uint8_t twi_state;
static volatile uint8_t twi_slarw;
static volatile uint8_t twi_sendStop;  // should the transaction end with a stop
static volatile uint8_t twi_inRepStart;     // in the middle of a repeated start

static void (*twi_onSlaveTransmit)(void);
static void (*twi_onSlaveReceive)(uint8_t*, int);

static uint8_t twi_masterBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t twi_masterBufferIndex;
static volatile uint8_t twi_masterBufferLength;

static uint8_t twi_txBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t twi_txBufferIndex;
static volatile uint8_t twi_txBufferLength;

static uint8_t twi_rxBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t twi_rxBufferIndex;

static volatile uint8_t twi_error;

static bool isnack = false;
static bool lastdata_sent = false;
/*
 * Function twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void twi_init(void) {
    // initialize state
    twi_state = TWI_READY;
    twi_sendStop = true;      // default value
    twi_inRepStart = false;

    GPIOB_ModeCfg( GPIO_Pin_13 | GPIO_Pin_12, GPIO_ModeIN_PU);

    I2C_Init(I2C_Mode_I2C, TWI_FREQ, I2C_DutyCycle_16_9, I2C_Ack_Enable,
            I2C_AckAddr_7bit, TWI_ADDR);

    I2C_ITConfig(I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, ENABLE);

    PFIC_EnableIRQ(I2C_IRQn);
}

/*
 * Function twi_disable
 * Desc     disables twi pins
 * Input    none
 * Output   none
 */
void twi_disable(void) {
    // disable twi module, acks, and twi interrupt
    R16_I2C_CTRL1 &= ~(RB_I2C_PE | RB_I2C_ACK);
    R16_I2C_CTRL2 &= ~(I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
}

/*
 * Function twi_setAddress
 * Desc     sets bus address
 * Input    none
 * Output   none
 */
void twi_setAddress(uint8_t address) {
    // set twi slave address (skip over ADD0 bit)
    R16_I2C_OADDR1 |= address << 1;
}

/*
 * Function twi_setClock
 * Desc     sets twi bit rate
 * Input    Clock Frequency
 * Output   none
 */
void twi_setFrequency(uint32_t frequency) {

}

/*
 * Function twi_readFrom
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */

__HIGH_CODE
uint8_t twi_readFrom(uint8_t address, uint8_t* data, uint8_t length,
        uint8_t sendStop) {
    uint8_t i;


    // ensure data will fit into buffer
    if (TWI_BUFFER_LENGTH < length) {
        return 0;
    }

    // wait until twi is ready, become master receiver
    while (TWI_READY != twi_state) {
        continue;
    }
    twi_state = TWI_MRX;
    twi_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi_error = 0xFF;

    if(length == 1)  {
        R16_I2C_CTRL1 &= ~RB_I2C_ACK;  //由于ACK/NACK在下一次传输数据时生效，所以需要提前配置
        isnack = true;
    }

    // initialize buffer iteration vars
    twi_masterBufferIndex = 0;
    twi_masterBufferLength = length - 1;  // This is not intuitive, read on...
    // On receive, the previously configured ACK/NACK setting is transmitted in
    // response to the received byte before the interrupt is signalled.
    // Therefor we must actually set NACK when the _next_ to last byte is
    // received, causing that NACK to be sent in response to receiving the last
    // expected byte of data.

    // build sla+w, slave device address + w bit
    twi_slarw = TW_READ;
    twi_slarw |= address << 1;

    if (true == twi_inRepStart) {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the
        // repeated start that we sent ourselves, and that would really confuse things.
        twi_inRepStart = false;     // remember, we're dealing with an ASYNC ISR
        do {
            R16_I2C_DATAR = twi_slarw;
        } while (R16_I2C_CTRL1 & RB_I2C_BTF); //数据已发送
        R16_I2C_CTRL2 |= (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
    } else{
        // send start condition
        R16_I2C_CTRL2 |= (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
        twi_start();
    }

    // wait for read operation to complete
    while (TWI_MRX == twi_state) {
        continue;
    }

    if (twi_masterBufferIndex < length)
        length = twi_masterBufferIndex;

    // copy twi buffer to data
    for (i = 0; i < length; ++i) {
        data[i] = twi_masterBuffer[i];
    }

    return length;
}

/*
 * Function twi_writeTo
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the end
 * Output   0 .. success
 *          1 .. length to long for buffer
 *          2 .. address or data send, NACK received
 *          3 .. other twi error (lost bus arbitration, bus error, ..)
 */
__HIGH_CODE
uint8_t twi_writeTo(uint8_t address, uint8_t* data, uint8_t length,
        uint8_t wait, uint8_t sendStop) {
    uint8_t i;
    // ensure data will fit into buffer
    if (TWI_BUFFER_LENGTH < length) {
        return 1;
    }
    // wait until twi is ready, become master transmitter
    while (TWI_READY != twi_state) {
        continue;
    }
    twi_state = TWI_MTX;
    twi_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi_error = 0xFF;

    // initialize buffer iteration vars
    twi_masterBufferIndex = 0;
    twi_masterBufferLength = length;

    // copy data to twi buffer
    for (i = 0; i < length; ++i) {
        twi_masterBuffer[i] = data[i];
    }

    // build sla+w, slave device address + w bit
    twi_slarw = TW_WRITE;
    twi_slarw |= address << 1;

    // if we're in a repeated start, then we've already sent the START
    // in the ISR. Don't do it again.
    //
    if (true == twi_inRepStart) {
        do {
            R16_I2C_DATAR = twi_slarw;
        } while (R16_I2C_CTRL1 & RB_I2C_BTF); //数据已发送
        R16_I2C_CTRL2 |= (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
    } else{
        // send start condition
        R16_I2C_CTRL2 |= (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
        twi_start();
    }
    // wait for write operation to complete
    while (wait && (TWI_MTX == twi_state)) {
        continue;
    }
    if (twi_error == 0xFF)
        return 0;   // success
    else if (twi_error == TW_MT_NACK)
        return 2;   // error: address send, nack received
    else
        return 3;   // other twi error
}

/*
 * Function twi_transmit
 * Desc     fills slave tx buffer with data
 *          must be called in slave tx event callback
 * Input    data: pointer to byte array
 *          length: number of bytes in array
 * Output   1 length too long for buffer
 *          2 not slave transmitter
 *          0 ok
 */
uint8_t twi_transmit(const uint8_t* data, uint8_t length) {
    uint8_t i;

    // ensure data will fit into buffer
    if (TWI_BUFFER_LENGTH < (twi_txBufferLength + length)) {
        return 1;
    }

    // ensure we are currently a slave transmitter
    if (TWI_STX != twi_state) {
        return 2;
    }

    // set length and copy data into tx buffer
    for (i = 0; i < length; ++i) {
        twi_txBuffer[twi_txBufferLength + i] = data[i];
    }
    twi_txBufferLength += length;

    return 0;
}

/*
 * Function twi_attachSlaveRxEvent
 * Desc     sets function called before a slave read operation
 * Input    function: callback function to use
 * Output   none
 */
void twi_attachSlaveRxEvent(void (*function)(uint8_t*, int)) {
    twi_onSlaveReceive = function;
}

/*
 * Function twi_attachSlaveTxEvent
 * Desc     sets function called before a slave write operation
 * Input    function: callback function to use
 * Output   none
 */
void twi_attachSlaveTxEvent(void (*function)(void)) {
    twi_onSlaveTransmit = function;
}

/*
 * Function twi_reply
 * Desc     sends byte or readys receive line
 * Input    ack: byte indicating to ack or to nack
 * Output   none
 */
void twi_reply(uint8_t ack) {
//    LOG_INFO("twi_reply %s!", ack ? "ack" : "nack");
    // transmit master read ready signal, with or without ack
    if (ack) {
        R16_I2C_CTRL1 |= RB_I2C_ACK;
    } else {
        R16_I2C_CTRL1 &= ~RB_I2C_ACK;
    }
}

/*
 * Function twi_start
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
void twi_start(void) {
//    LOG_INFO("twi_start!");
    R16_I2C_CTRL1 &= ~RB_I2C_STOP;
    // send start condition
    R16_I2C_CTRL1 |= RB_I2C_START;
}

/*
 * Function twi_stop
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
void twi_stop(void) {
//    LOG_INFO("twi_stop!");
    // send stop condition
    R16_I2C_CTRL1 |= RB_I2C_STOP;

    // update twi state
    twi_state = TWI_READY;
}

/*
 * Function twi_releaseBus
 * Desc     releases bus control
 * Input    none
 * Output   none
 */
void twi_releaseBus(void) {
//    LOG_INFO("twi_releaseBus!");
    // release bus
    R16_I2C_CTRL1 |= RB_I2C_STOP;

    // update twi state
    twi_state = TWI_READY;
}

static uint32_t lasttmp = 0;
static uint8_t time = 0;

__HIGH_CODE
void I2C_IRQHandler(void) {
    uint32_t status = R16_I2C_STAR1 | (R16_I2C_STAR2 << 16);
//    PRINT("%x\n", status);

    status == lasttmp ? time++: (time = 0);
    time>100?PRINT("%x\n", status):0;

    lasttmp = status;
    //master mode
    if (status & (RB_I2C_MSL << 16)) {
        if (status & RB_I2C_SB) {  //sent start condition
            R16_I2C_DATAR = twi_slarw;
            twi_reply(1);
        }

        if(status & (RB_I2C_TRA<<16)){
            //master transmitter
            if (status & (RB_I2C_ADDR | RB_I2C_BTF | RB_I2C_TxE)) { //slave receiver acked address or sent bit
                // if there is data to send, send it, otherwise stop
                if (twi_masterBufferIndex < (twi_masterBufferLength)) {
                    // copy data to output register and ack
                    R16_I2C_DATAR = twi_masterBuffer[twi_masterBufferIndex++];
                    time>100?PRINT("R16_I2C_DATAR\n"):0;
                    twi_reply(1);

//                    if(twi_masterBufferLength - twi_masterBufferIndex <= 1){
//                        if(twi_sendStop){
//                            twi_stop();
//                        }
//                    }
                } else {
                        lastdata_sent = false;
                        if (twi_sendStop) {
//                            LOG_INFO("send stop");
                             time>100?PRINT("stop\n"):0;
                             twi_stop();
                         } else {
                             twi_inRepStart = true;    // we're gonna send the START
                             // don't enable the interrupt. We'll generate the start, but we
                             // avoid handling the interrupt until we're in the next transaction,
                             // at the point where we would normally issue the start.
                             R16_I2C_CTRL2 &= ~(I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
                             twi_start();
                             twi_state = TWI_READY;
                             time>100?PRINT("restart\n"):0;
                         }
                }
            }

            if (status & RB_I2C_AF) {  //address or data sent, nack received
                R16_I2C_STAR1 &= ~RB_I2C_AF; //clear flag
                twi_error = TW_MT_NACK;
                twi_stop();
            }

        }else{
            //maseter receiver

            if(status & RB_I2C_ADDR){ //address sent,  ack received
                // ack if more bytes are expected, otherwise nack
                if(twi_masterBufferIndex < twi_masterBufferLength){
                  twi_reply(1);
                }else{
                  twi_reply(0);
                }
            }

            if(status & RB_I2C_BTF){    //当发送完地址后，会进入字节发送完成 中断，若同时产生接收事件，会导致读数据错误0xa7
                (void)R16_I2C_DATAR;
                status &= ~(RB_I2C_BTF);
            }

            if (status & RB_I2C_RxNE) { // data received, ack sent

                // put byte into buffer
                twi_masterBuffer[twi_masterBufferIndex++] = R16_I2C_DATAR;
                // ack if more bytes are expected, otherwise nack
                if (twi_masterBufferIndex < twi_masterBufferLength) {
                    twi_reply(1);
                } else {
                    twi_reply(0);

                    if(isnack){
                        isnack = false;
                        if (twi_sendStop)   //注意： 多字节传输时  第一次拉stop未产生stop信号
                            twi_stop();
                        else {
                            twi_inRepStart = true;    // we're gonna send the START
                            // don't enable the interrupt. We'll generate the start, but we
                            // avoid handling the interrupt until we're in the next transaction,
                            // at the point where we would normally issue the start.
                            R16_I2C_CTRL2 &= ~(I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
                            twi_start();
                            twi_state = TWI_READY;
                        }
                    }else {
                        isnack = true;
                    }
                }
            }

            if (status & RB_I2C_AF) {  //nack received
                R16_I2C_STAR1 &= ~RB_I2C_AF; //clear flag
                // put final byte into buffer
                twi_masterBuffer[twi_masterBufferIndex++] = R16_I2C_DATAR;

                if (twi_sendStop)
                    twi_stop();
                else {
                    twi_inRepStart = true;    // we're gonna send the START
                    // don't enable the interrupt. We'll generate the start, but we
                    // avoid handling the interrupt until we're in the next transaction,
                    // at the point where we would normally issue the start.
                    R16_I2C_CTRL2 &= ~(I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR);
                    twi_start();
                    twi_state = TWI_READY;
                }
            }
        }

    } else {  //slave mode

        if (status & RB_I2C_ADDR) {  //addressed, returned ack
            twi_reply(1);

            if(status & (RB_I2C_TRA<<16)){
                LOG_INFO("slave transmit");

                // enter slave transmitter mode
                twi_state = TWI_STX;
                // ready the tx buffer index for iteration
                twi_txBufferIndex = 0;
                // set tx buffer length to be zero, to verify if user changes it
                twi_txBufferLength = 0;
                // request for txBuffer to be filled and length to be set
                // note: user must call twi_transmit(bytes, length) to do this
                twi_onSlaveTransmit();
                // if they didn't change buffer & length, initialize it
                if(0 == twi_txBufferLength){
                  twi_txBufferLength = 1;
                  twi_txBuffer[0] = 0x00;
                }
            } else {
                LOG_INFO("slave receiver");
                // enter slave receiver mode
                twi_state = TWI_SRX;
                // indicate that rx buffer can be overwritten and ack
                twi_rxBufferIndex = 0;
            }
        }

        if(status & (RB_I2C_TRA<<16)){  // slave transmitter

            if (status & RB_I2C_AF) {  //nack received
                R16_I2C_STAR1 &= ~RB_I2C_AF; //clear flag
                // ack future responses
                twi_reply(1);
                // leave slave receiver state
                twi_state = TWI_READY;
                //clear status
                status = 0;
            }

          if(status & (RB_I2C_BTF | RB_I2C_TxE)){
              // copy data to output register
              R16_I2C_DATAR = twi_txBuffer[twi_txBufferIndex++];
              // if there is more to send, ack, otherwise nack
              if(twi_txBufferIndex < twi_txBufferLength){
                  twi_reply(1);
              }else{
                  twi_reply(0);
              }
          }
        }else {                         //slave receiver
            if (status & RB_I2C_RxNE) {
                // if there is still room in the rx buffer
                if (twi_rxBufferIndex < TWI_BUFFER_LENGTH) {
                    // put byte in buffer and ack
                    twi_rxBuffer[twi_rxBufferIndex++] = R16_I2C_DATAR;
                    twi_reply(1);
                } else {
                    // otherwise nack
                    twi_reply(0);
                }
            }

            if (status & RB_I2C_STOPF) {
                // ack future responses and leave slave receiver state

                //twi_releaseBus();  //auto relaseBus?

                R16_I2C_CTRL1 |= RB_I2C_PE; //clear flag

                // put a null char after data if there's room
                if (twi_rxBufferIndex < TWI_BUFFER_LENGTH) {
                    twi_rxBuffer[twi_rxBufferIndex] = '\0';
                }
                // callback to user defined callback
                twi_onSlaveReceive(twi_rxBuffer, twi_rxBufferIndex);
                // since we submit rx buffer to "wire" library, we can reset it
                twi_rxBufferIndex = 0;
            }

            if (status & RB_I2C_AF) {  // data received, returned nack
                R16_I2C_STAR1 &= ~RB_I2C_AF; //clear flag
                // ack future responses
                twi_reply(1);
            }

        }
    }

    if(status & RB_I2C_BERR){
        LOG_INFO("RB_I2C_BERR");
        R16_I2C_STAR1 &= ~RB_I2C_BERR; //clear flag
        twi_error = TW_BUS_ERROR;
        twi_stop();
    }

    if(status & RB_I2C_ARLO){
        LOG_INFO("RB_I2C_ARLO");
        R16_I2C_STAR1 &= ~RB_I2C_ARLO; //clear flag
    }

//    PRINT("\n");
}

