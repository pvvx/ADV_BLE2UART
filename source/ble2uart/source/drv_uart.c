#include "app_config.h"
#include "tl_common.h"
#include "drivers.h"

#include "drv_uart.h"

typedef struct {
	volatile u32 len;
	u8 data[UART_DMA_BUFF_SIZE];
} __attribute__((aligned(4))) dma_uart_buf_t;

static dma_uart_buf_t uart_rx_buffer;
static dma_uart_buf_t uart_tx_buffer;

#ifndef UART_CH340_TX_PIN
#define UART_CH340_TX_PIN UART_TX_PB1
#endif

#ifndef UART_CH340_RX_PIN
#define UART_CH340_RX_PIN UART_RX_PA0
#endif

void init_uart(int uart_baud)
{
	uart_gpio_set(UART_CH340_TX_PIN, UART_CH340_RX_PIN);
	uart_reset();
	uart_init_baudrate(uart_baud, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);
	uart_recbuff_init((u8 *)&uart_rx_buffer, DATA_BUFF_SIZE);
	uart_dma_enable(1, 1);
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX, 1);
	uart_irq_enable(0, 0);
	dma_chn_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);
	dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX);
	reg_uart_status0 |= FLD_UART_CLEAR_RX_FLAG | FLD_UART_RX_ERR_FLAG;
}

int uart_send(u8 *src, u32 len)
{
	if (!len || len > UART_DMA_BUFF_SIZE) {
		return -1;
	}

	if (!(reg_uart_status1 & FLD_UART_TX_DONE)) {
		return -1;
	}

	memcpy(uart_tx_buffer.data, src, len);
	uart_tx_buffer.len = len;
	uart_send_dma((u8 *)&uart_tx_buffer);

	return (int)len;
}

int uart_read(u8 *des, u32 maxlen)
{
	u32 rxlen;

	if (!(dma_chn_irq_status_get() & FLD_DMA_CHN_UART_RX)) {
		return 0;
	}

	dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
	reg_uart_status0 |= FLD_UART_CLEAR_RX_FLAG | FLD_UART_RX_ERR_FLAG;

	rxlen = uart_rx_buffer.len;
	if (!rxlen) {
		return 0;
	}

	if (rxlen > maxlen) {
		rxlen = maxlen;
	}

	memcpy(des, uart_rx_buffer.data, rxlen);
	return (int)rxlen;
}

