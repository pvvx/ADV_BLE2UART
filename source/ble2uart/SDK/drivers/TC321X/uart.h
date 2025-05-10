/********************************************************************************************************
 * @file    uart.h
 *
 * @brief   This is the header file for TC321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
/**	@page UART
 *
 *	Header File: uart.h
 *
 *	How to use this driver
 *	==============
 *  nodma usage instructions:
 *  -# rx_irq interrupt processing: Use reg_uart_buf_cnt to determine the number of FIFOs and use uart_ndma_read_byte () to read all data in fifo;
 *  -# The depth size of the uart fifo is 8. If the time before and after entering the rx_irq interrupt exceeds the time of receiving 8 bytes, the fifo pointer may be disturbed, resulting in abnormal received data.
 *     You can determine whether reg_uart_buf_cnt is greater than 8 as an exception,If this exception occurs, it is recommended to use dma mode to receive.
 *
 *  dma:
 *  advantage: Automatically received by dma hardware, does not require mcu polling receive,
 *  shortcoming:dma maximum receive length (4079-4) bytes, if this length is reached, excess data will overwrite the previously received data.
 *  note: only UART0 support DMA mode.
 *
 */
#include "register.h"
#include "gpio.h"
#include "compiler.h"
#ifndef __UART_H__
#define __UART_H__

#define uart_rtx_pin_tx_trig(uart_num) uart_clr_tx_done(uart_num)
#define uart_ndmairq_get(uart_num)     uart_ndma_irq_get(uart_num) /* Just for compatibility */

/**
 *  @brief  Define UART chn
 */
typedef enum
{
    UART0 = 0, /**< Supports NODMA, DMA, RX, TX, CTS, RTS and RTX function. */
    UART1,     /**< Supports NODMA, RX, TX, and RTX function. */
    UART2,     /**< Supports NODMA, RX, TX, and RTX function. */
} uart_num_e;

/**
 *  @brief  Define mul bits
 */
enum{
    UART_BW_MUL1  = 0,
    UART_BW_MUL2  = 1,
    UART_BW_MUL3  = 2,
    UART_BW_MUL4  = 3,
};

/**
 *  @brief  Define parity type
 */
typedef enum {
    PARITY_NONE = 0,
    PARITY_EVEN,
    PARITY_ODD,
} UART_ParityTypeDef;

/**
 *  @brief  Define the length of stop bit
 */
typedef enum {
    STOP_BIT_ONE          = 0,
    STOP_BIT_ONE_DOT_FIVE = BIT(4),
    STOP_BIT_TWO          = BIT(5),
} UART_StopBitTypeDef;

/**
 *  @brief  Define UART RTS mode
 */
typedef enum {
    UART_RTS_MODE_AUTO = 0,
    UART_RTS_MODE_MANUAL,
} UART_RTSModeTypeDef;


/**
 * @brief     This function servers to indicate Tx state.
 * @param[in]   uart_num - UART0, UART1 or UART2.
 * @return    the state of Tx 0:Tx done 1:not.
 */
static inline unsigned char uart_tx_is_busy(uart_num_e uart_num)
{
    return ( (reg_uart_status1(uart_num) & FLD_UART_TX_DONE) ? 0 : 1) ;
}

/**
 *  @brief  Define UART1 and UART2 TX pin.
 */
typedef enum
{
    UART1_TX_PA1 = GPIO_PA1,
    UART1_TX_PA2 = GPIO_PA2,
    UART1_TX_PA4 = GPIO_PA4,
    UART1_TX_PA6 = GPIO_PA6,
    UART1_TX_PB0 = GPIO_PB0,
    UART1_TX_PB5 = GPIO_PB5,
    UART1_TX_PB7 = GPIO_PB7,
    UART1_TX_PC1 = GPIO_PC1,
    UART1_TX_PC3 = GPIO_PC3,
    UART1_TX_PC5 = GPIO_PC5,
    UART1_TX_PC7 = GPIO_PC7,
    UART1_TX_PD1 = GPIO_PD1,
    UART1_TX_PD3 = GPIO_PD3,
    UART1_TX_PD6 = GPIO_PD6,
    UART1_TX_PE0 = GPIO_PE0,
    UART2_TX_PA0 = GPIO_PA0,
    UART2_TX_PA2 = GPIO_PA2,
    UART2_TX_PA5 = GPIO_PA5,
    UART2_TX_PA7 = GPIO_PA7,
    UART2_TX_PB2 = GPIO_PB2,
    UART2_TX_PB4 = GPIO_PB4,
    UART2_TX_PB6 = GPIO_PB6,
    UART2_TX_PC0 = GPIO_PC0,
    UART2_TX_PC2 = GPIO_PC2,
    UART2_TX_PC4 = GPIO_PC4,
    UART2_TX_PC6 = GPIO_PC6,
    UART2_TX_PD0 = GPIO_PD0,
    UART2_TX_PD2 = GPIO_PD2,
    UART2_TX_PD5 = GPIO_PD5,
    UART2_TX_PD7 = GPIO_PD7,
    UART_TX_NONE_PIN = 0xfff,
} UART_TxPinDef;

/**
 *  @brief  Define UART1 and UART2 RX pin.
 */
typedef enum
{
    UART1_RX_PA0 = GPIO_PA0,
    UART1_RX_PA1 = GPIO_PA1,
    UART1_RX_PA2 = GPIO_PA2,
    UART1_RX_PA5 = GPIO_PA5,
    UART1_RX_PA7 = GPIO_PA7,
    UART1_RX_PB2 = GPIO_PB2,
    UART1_RX_PB4 = GPIO_PB4,
    UART1_RX_PB6 = GPIO_PB6,
    UART1_RX_PC0 = GPIO_PC0,
    UART1_RX_PC2 = GPIO_PC2,
    UART1_RX_PC4 = GPIO_PC4,
    UART1_RX_PC6 = GPIO_PC6,
    UART1_RX_PD0 = GPIO_PD0,
    UART1_RX_PD2 = GPIO_PD2,
    UART1_RX_PD5 = GPIO_PD5,
    UART1_RX_PD7 = GPIO_PD7,
    UART2_RX_PA1 = GPIO_PA1,
    UART2_RX_PA4 = GPIO_PA4,
    UART2_RX_PA6 = GPIO_PA6,
    UART2_RX_PB0 = GPIO_PB0,
    UART2_RX_PB5 = GPIO_PB5,
    UART2_RX_PB7 = GPIO_PB7,
    UART2_RX_PC1 = GPIO_PC1,
    UART2_RX_PC3 = GPIO_PC3,
    UART2_RX_PC5 = GPIO_PC5,
    UART2_RX_PC7 = GPIO_PC7,
    UART2_RX_PD1 = GPIO_PD1,
    UART2_RX_PD3 = GPIO_PD3,
    UART2_RX_PD6 = GPIO_PD6,
    UART2_RX_PE0 = GPIO_PE0,
    UART_RX_NONE_PIN = 0xfff,
} UART_RxPinDef;

/**
 *  @brief  Define RTX pin.
 */
typedef enum
{
    UART_RTX_PB0 = GPIO_PB0,  /** UART0, UART1 and UART2 common use. */
    UART0_RTX_PD7 = GPIO_PD7, /** only for UART0. */
    UART0_RTX_PE0 = GPIO_PE0, /** only for UART0. */
    UART0_RTX_PE1 = GPIO_PE1, /** only for UART0. */
    UART2_RTX_PB3 = GPIO_PB3, /** only for UART2. */
} UART_RTxPinDef;

/**
 * @brief     This function resets the UART module.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    none
 * @note      After calling the uart_reset interface, uart_ndma_clear_tx_index and uart_ndma_clear_rx_index must be called to clear the read/write pointer,
 *            after the uart_reset interface is invoked, the hardware read and write Pointers are cleared to zero,therefore the software read and write Pointers are cleared to ensure logical correctness.
 */
static inline void uart_reset(uart_num_e uart_num)
{
    if (uart_num == UART0)
    {
        reg_rst0 &= (~FLD_RST0_UART0);
        reg_rst0 |= FLD_RST0_UART0;
    }
    else if (uart_num == UART1)
    {
        reg_rst3 &= (~FLD_RST3_UART1);
        reg_rst3 |= FLD_RST3_UART1;
    }
    else if (uart_num == UART2)
    {
        reg_rst2 &= (~FLD_RST2_UART2);
        reg_rst2 |= FLD_RST2_UART2;
    }
}

/**
 * @brief     This function serves to clear tx done.
 * @param[in] uart_num - UART0.
 * @return    none
 */
static inline void uart_clr_tx_done(uart_num_e uart_num)
{
    reg_uart_state(uart_num) = BIT(7);
}

/**
 * @brief          This function is used to enable the rtx function.
 * @param[in]      uart_num - UART0, UART1 or UART2.
 * @return         none.
 */
static inline void uart_rtx_en(uart_num_e uart_num)
{
    reg_uart_rx_timeout1(uart_num) |=FLD_UART_P7816_EN;
}

/**
 * @brief     This function serves to set the mask of uart_txdone
 * @param[in] uart_num - UART0.
 * @return    none
 */
static inline void uart_mask_tx_done_irq_enable(uart_num_e uart_num){
    reg_uart_rx_timeout1(uart_num) |= FLD_UART_MASK_TXDONE_IRQ;

}

/**
 * @brief      This function initializes the UART module.
 * @param[in]  uart_num    - UART0, UART1 or UART2.
 * @param[in]  g_uart_div  -  uart clock divider.
 * @param[in]  g_bwpc      -  bitwidth, should be set to larger than.
 *
 *              sys_clk      baud rate   g_uart_div         g_bwpc
 *
 *                      16Mhz        9600          118                13
 *                                   19200         118                6
 *                                   115200         9                 13
 *
 *                      24Mhz        9600          249                9
 *                                   19200         124                9
 *                                   115200        12                 15
 *
 *                      32Mhz        9600          235                13
 *                                   19200         235                6
 *                                   115200        17                 13
 *
 *                      48Mhz        9600          499                9
 *                                   19200         249                9
 *                                   115200        25                 15
 *
 * @param[in]  Parity      - selected parity type for UART interface.
 * @param[in]  StopBit     - selected length of stop bit for UART interface.
 * @return     none
 */
void uart_init(uart_num_e uart_num,unsigned short g_uart_div, unsigned char g_bwpc, UART_ParityTypeDef Parity, UART_StopBitTypeDef StopBit);

/**
 * @brief      This function initializes the UART module.
 * @param[in]  uart_num     - UART0, UART1 or UART2.
 * @param[in]  Baudrate      - uart baud rate
 * @param[in]  System_clock - clock of system
 * @param[in]  Parity          - selected parity type for UART interface
 * @param[in]  StopBit         - selected length of stop bit for UART interface
 * @return     none
 */
void uart_init_baudrate(uart_num_e uart_num,unsigned int Baudrate,unsigned int System_clock , UART_ParityTypeDef Parity, UART_StopBitTypeDef StopBit);

/**
 * @brief      enable uart DMA mode, in dma mode, rx_dma_en and tx_dma_en must set 1 \n
 *             in no dma mode, rx_dma_en and tx_dma_en must set 0
 * @param[in]  uart_num - UART0.
 * @param[in]  rx_dma_en
 * @param[in]  tx_dma_en
 * @return     none
 * @note
 *             - Only UART 0 support DMA mode.
 */
void uart_dma_enable(uart_num_e uart_num,unsigned char rx_dma_en, unsigned char tx_dma_en);

/**
 * @brief     config the irq of uart tx and rx
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @param[in] rx_irq_en - 1:enable rx irq. 0:disable rx irq
 * @param[in] tx_irq_en - 1:enable tx irq. 0:disable tx irq
 *                        (In general, nodma does not use this interrupt,is sent in polling mode, uart_tx_is_busy() is used to determine whether the sending is complete)
 * @return    none
 */


void uart_irq_enable(uart_num_e uart_num,unsigned char rx_irq_en, unsigned char tx_irq_en);

/**
 * @brief use this index to cycle the four register of uart. this index should be reset to 0,when send data after system wakeup.
 * 
 */
extern unsigned char uart_TxIndex[3];

/**
 * @brief     uart send data function with not DMA method.
 *            variable uart_TxIndex,it must cycle the four registers 0x90 0x91 0x92 0x93 for the design of SOC.
 *            so we need variable to remember the index.
 *              Before entering power-saving mode,you need to call this function in order to ensure that the data sent is completed
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @param[in] uartData - the data to be send.
 * @return    none
 */
void uart_ndma_send_byte(uart_num_e uart_num,unsigned char uartData);

/**
 * @brief     This function is used to set the 'uart_TxIndex' to 0.
 *              After wakeup from power-saving mode or uart_reset(), you must call this function before sending the data.
 * @param[in] none.
 * @return    none.
 */
static inline void uart_ndma_clear_tx_index(uart_num_e uart_num)
{
    uart_TxIndex[uart_num]=0;
}

/**
 * @brief use this index to cycle the four register of uart. this index should be reset to 0,when send data after system wakeup.
 * 
 */
extern unsigned char uart_RxIndex[3];

/**
 * @brief     uart read data function with not DMA method.
 *            variable uart_RxIndex,it must cycle the four registers 0x90 0x91 0x92 0x93 for the design of SOC.
 *            so we need variable to remember the index.
 * @param[in]   uart_num - UART0, UART1 or UART2.
 * @return    data received.
 */
volatile unsigned char uart_ndma_read_byte(uart_num_e uart_num);

/**
 * @brief     This function is used to set the 'uart_RxIndex' to 0.
 *              After wakeup from power-saving mode or uart_reset, you must call this function before read data.
 * @param[in] none.
 * @return    none.
 */
static inline void uart_ndma_clear_rx_index(uart_num_e uart_num)
{
    uart_RxIndex[uart_num]=0;
}

/**
 * @brief     configure the trigger level setting the rx_irq and tx_irq interrupt
 * @param[in]   uart_num - UART0, UART1 or UART2.
 * @param[in] rx_level - rx_irq trigger level value.When the number of rxfifo is greater than or equal to the rx_level, an interrupt is generated, and the interrupt flag is automatically cleared.
 * @param[in] tx_level - tx_irq trigger level value.When the number of txfifo is less than or equal to the tx_level, an interrupt is generated and the interrupt flag is automatically cleared.
 * @return    none
 */
void uart_ndma_irq_triglevel(uart_num_e uart_num,unsigned char rx_level, unsigned char tx_level);

/**
 * @brief     get the status of uart irq.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    0: not uart irq ;
 *            not 0: indicate tx or rx irq
 */
unsigned char uart_ndma_irq_get(uart_num_e uart_num);

/**
 * @brief     Send an amount of data in DMA mode, only UART0 support dma mode.
 * @param[in] uart_num - UART0.
 * @param[in] Addr   - Pointer to data buffer. It must be 4-bytes aligned address,
 *                     The first four bytes of addr store the send length,the send length can only send (4079-4) bytes one time at most.
 * @return    none
 * @note      If you want to use uart DMA mode to send data, it is recommended to use this function.
 *            This function just triggers the sending action, you can use interrupt or polling with the FLD_UART_TX_DONE flag to judge whether the sending is complete.
 *            After the current packet has been sent, this FLD_UART_TX_DONE will be set to 1, and FLD_UART_TX_DONE interrupt can be generated.
 *              If you use interrupt mode, you need to call uart_clr_tx_done() in the interrupt processing function, uart_clr_tx_done() will set FLD_UART_TX_DONE to 0.
 */
void uart_send_dma(uart_num_e uart_num,unsigned char* Addr);

/**
 * @brief     This function is saved for compatibility with other SDK and isn't be used in driver sdk.Because it has the following problems:
 *              You can't use this function if you open FLD_UART_TX_DONE irq,This function can only be used in polling method.
 *              There may be a risk of data loss under certain usage conditions.
 *              It will first check whether the last packet has been sent, if it is checked that the last packet has been sent,
 *              it will trigger the sending, otherwise it will not send.
 *
 * @param[in] uart_num - UART0.
 * @param[in] Addr - pointer to the buffer containing data need to send
 * @return    1: DMA triggered successfully
 *            0: UART busy : last packet not send over,you can't start to send the current packet data
 *
 * @note      -# DMA can only send (4079-4) bytes one time at most.
 *            -# Only UART0 support dma mode
 *
 */
volatile unsigned char uart_dma_send(uart_num_e uart_num,unsigned char* Addr);

/**
 * @brief     uart send data function, this  function tell the DMA to get data from the RAM and start
 *            the DMA transmission
 * @param[in] uart_num - UART0.
 * @param[in] byte - single byte data need to send
 * @return    1: send success ;
 *            0: DMA busy
 * @note
 *             - Only UART 0 support DMA mode.
 */
volatile unsigned char uart_send_byte(uart_num_e uart_num,unsigned char byte);

/**
 * @brief     Receive an amount of data in DMA mode.
 * @param[in] uart_num - UART0.
 * @param[in] RecvAddr - Pointer to data buffer, it must be 4-bytes aligned.
 * @param[in] RecvBufLen - Length of DMA in bytes, it must be multiple of 16,the maximum value can be up to 4079,
 *                         RecvBufLen contains the first four bytes to indicate the received length,so uart packet length needs to be no larger than (recBuffLen - 4).
 * @return    none
 * @note      -# If the dma receive length reaches the set length, the uart is still receiving data, no rxtimeout is generated,
 *               the dma will continue to receive, but no buff overflow occurs, and the loopback receive overwrites the received data.
 *            -# Only UART 0 support DMA mode.
 */

void uart_recbuff_init(uart_num_e uart_num,unsigned char *RecvAddr, unsigned short RecvBufLen);

/**
 * @brief     This function determines whether parity error occurs once a packet arrives.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    1: parity error ;
 *            0: no parity error
 */
unsigned char uart_is_parity_error(uart_num_e uart_num);

/**
 * @brief     This function clears parity error status once when it occurs.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    none
 *
 * Note:
 *(1)DMA mode
 * RX FIFO will also be cleared when parity error flag is cleared .
 *(2)NON-DMA mode
 * When parity error occurs, clear parity error flag after UART receives the data.
 * Cycle the four registers (0x90 0x91 0x92 0x93) from register "0x90" to get data when UART receives the data next time.
 */
void uart_clear_parity_error(uart_num_e uart_num);

/**
 * @brief     UART hardware flow control configuration. Configure RTS pin.
 * @param[in] uart_num - UART0.
 * @param[in] Enable - enable or disable RTS function.
 * @param[in] Mode - set the mode of RTS(auto or manual).
 * @param[in] Thresh - threshold of trig RTS pin's level toggle(only for auto mode),
 *                     it means the number of bytes that has arrived in Rx buf.
 * @param[in] Invert - whether invert the output of RTS pin(only for auto mode)
 * @param[in] pin   - RTS pin select.
 * @return    none
 */
void uart_set_rts(uart_num_e uart_num,unsigned char Enable, UART_RTSModeTypeDef Mode, unsigned char Thresh, unsigned char Invert,  GPIO_PinTypeDef pin);

/**
 * @brief     This function sets the RTS pin's level manually
 * @param[in] uart_num - UART0.
 * @param[in] Polarity - set the output of RTS pin(only for manual mode)
 * @return    none
 */
void uart_set_rts_level(uart_num_e uart_num,unsigned char Polarity);

/**
 * @brief      UART hardware flow control configuration. Configure CTS pin.
 * @param[in]  uart_num - UART0.
 * @param[in]  Enable - enable or disable CTS function.
 * @param[in]  Select - when CTS's input equals to select, tx will be stopped
 * @param[in]  pin   - CTS pin select.
 * @return     none
 */
void uart_set_cts(uart_num_e uart_num,unsigned char Enable, unsigned char Select,GPIO_PinTypeDef pin);

/**
 * @brief      This function serves to select pin for UART module.
 * @param[in]  uart_num - UART0, UART1 or UART2.
 * @param[in]  tx_pin   - the pin to send data.
 * @param[in]  rx_pin   - the pin to receive data.
 * @return     none
 * @note
 *             - UART0 does not support: PA3, PB1, PB3, PD4, PE0, PE1, PF0~3;
 *             - UART1 and UART2 Refer to UART_RxPinDef and UART_TxPinDef.
 */
void uart_gpio_set(uart_num_e uart_num, GPIO_PinTypeDef tx_pin, GPIO_PinTypeDef rx_pin);

/**
 * @brief   This function enables the irq when UART module receives error data.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    none
 */
void uart_mask_error_irq_enable(uart_num_e uart_num);

/**
* @brief      This function serves to set rtx pin for UART module.
* @param[in]  uart_num - UART0, UART1 or UART2.
* @param[in]  rtx_pin  - the rtx pin need to set.
* @return     none
*/
void uart_set_rtx_pin(uart_num_e uart_num,UART_RTxPinDef rtx_pin);

/**
 * @brief     This function enables the rx_done irq.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    none
 */
void uart_rxdone_irq_en(uart_num_e uart_num);

/**
 * @brief     This function disable the rx_done irq.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    none
 */
void uart_rxdone_irq_dis(uart_num_e uart_num);

/**
 * @brief     This function disables the irq when UART module receives error data.
 * @param[in] uart_num - UART0, UART1 or UART2.
 * @return    none
 */
void uart_mask_error_irq_dis(uart_num_e uart_num);

#endif
