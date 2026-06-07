#include "NUC131.h"

#include "hw.h"
#include "hw_usart.h"
#include "hw_gpio.h"
#include "ring.h"

/**
 * @brief Структура UART-интерфейса для NUC131
 *
 * Содержит указатель на регистры UART_T, тактирование, сброс,
 * пины TX/RX, кольцевые буферы и счётчики.
 */
struct usart_t
{
	UART_T * baddr;
	uint32_t clk;
	uint32_t rst;
	uint32_t irq;
	struct gpio_t tx;
	struct gpio_t rx;
	struct ring_t tx_ring;
	struct ring_t rx_ring;
	uint16_t baudrate;
	uint32_t rx_cnt;
	uint32_t tx_cnt;
};

/** Экземпляр UART0 (пины PB1-TX, PB0-RX) */
static struct usart_t usart0 =
{
	.baddr = UART0,
	.clk = UART0_MODULE,
	.rst = UART0_RST,
	.tx = GPIO_INIT(B, 1),
	.rx = GPIO_INIT(B, 0),
	.baudrate = 0,
	.rx_cnt = 0,
	.tx_cnt = 0,
};

/**
 * @brief Записать данные в UART (через кольцевой буфер)
 * @param usart Указатель на структуру UART
 * @param ptr   Указатель на данные
 * @param len   Длина данных
 * @return Количество записанных байт
 */
int hw_usart_write(struct usart_t * usart, const uint8_t * ptr, int len)
{
	int ret = ring_write(&usart->tx_ring, (uint8_t *)ptr, len);
	UART_EnableInt(usart->baddr, UART_IER_THRE_IEN_Msk | UART_IER_RDA_IEN_Msk);

	return ret;
}

/**
 * @brief Прочитать один байт из RX-кольцевого буфера
 * @param usart Указатель на структуру UART
 * @param ch    Указатель для записи байта
 * @return 1 при успехе, 0 если буфер пуст
 */
uint8_t hw_usart_read_ch(struct usart_t * usart, uint8_t *ch)
{
	return ring_read_ch(&usart->rx_ring, ch);
}

/**
 * @brief Получить указатель на UART0
 * @return Указатель на usart0
 */
struct usart_t * hw_usart_get(void)
{
	return &usart0;
}

/** Счётчик входов в прерывание UART (для отладки) */
uint32_t usart_isr_cnt = 0;

/**
 * @brief Обработчик прерывания UART (общий)
 * @param usart Указатель на структуру UART
 *
 * RDA: читает байт из RX FIFO и пишет в rx_ring.
 * THRE: читает байт из tx_ring и пишет в TX FIFO;
 *       если tx_ring пуст — отключает прерывание THRE.
 */
void usart_isr(struct usart_t * usart)
{
	uint32_t isr = usart->baddr->ISR;

	if (isr & UART_ISR_RDA_INT_Msk)
	{
		usart->rx_cnt++;

		/* Чтение байта из UART */
		uint8_t ch = UART_READ(usart->baddr);
		ring_write_ch(&usart->rx_ring, ch);

		if (!(isr & UART_ISR_THRE_INT_Msk))
			usart_isr_cnt++;
	}

	if (isr & UART_ISR_THRE_INT_Msk)
	{
		uint8_t ch;
		if (!ring_read_ch(&usart->tx_ring, &ch)) {

			/* Отключить прерывание THRE, передача завершена. */
			UART_DISABLE_INT(usart->baddr, UART_IER_THRE_IEN_Msk);
		} else {

			usart->tx_cnt++;

			/* Запись данных в регистр передачи. */
			UART_WRITE(usart->baddr, ch);
		}
	}
}

/**
 * @brief Обработчик прерывания UART02_IRQn
 *
 * Вызывает usart_isr() для usart0.
 */
void UART02_IRQHandler(void)
{
	usart_isr(&usart0);
}

/**
 * @brief Отключить UART
 * @param usart Указатель на структуру UART
 *
 * Отключает прерывания, закрывает UART и переводит пины в квази-режим.
 */
void hw_usart_disable(struct usart_t * usart)
{
	UART_DisableInt(usart->baddr, UART_IER_THRE_IEN_Msk | UART_IER_RDA_IEN_Msk);

	UART_Close(usart->baddr);

	hw_gpio_set_float(&usart->rx);
	hw_gpio_set_float(&usart->tx);
}

/**
 * @brief Инициализация UART
 * @param usart    Указатель на структуру UART
 * @param speed    Скорость (бод)
 * @param txbuf    Буфер передачи
 * @param txbuflen Размер буфера передачи
 * @param rxbuf    Буфер приёма
 * @param rxbuflen Размер буфера приёма
 *
 * Настраивает кольцевые буферы, включает тактирование UART0,
 * устанавливает источник тактирования (HIRC), конфигурирует
 * мультифункциональные пины PB0/PB1, сбрасывает модуль,
 * открывает UART с заданной скоростью и разрешает прерывания.
 */
void hw_usart_setup(struct usart_t * usart, uint32_t speed, uint8_t * txbuf, uint32_t txbuflen, uint8_t * rxbuf, uint32_t rxbuflen)
{
	ring_init(&usart->tx_ring, txbuf, txbuflen);
	ring_init(&usart->rx_ring, rxbuf, rxbuflen);

	/* Включение тактирования модуля UART */
	CLK_EnableModuleClock(usart->clk);

	/* Выбор источника тактирования UART */
	CLK_SetModuleClock(usart->clk, CLK_CLKSEL1_UART_S_HIRC, CLK_CLKDIV_UART(1));

	/* Настройка мультифункциональных пинов PB0 (RXD) и PB1 (TXD) */
	SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
	SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD);

	SYS_ResetModule(usart->rst);

	UART_Open(usart->baddr, speed);

	UART_EnableInt(usart->baddr, UART_IER_THRE_IEN_Msk | UART_IER_RDA_IEN_Msk);
}

/**
 * @brief Получить счётчик переполнений RX-буфера
 * @param usart Указатель на структуру UART
 * @return Значение счётчика переполнений
 */
uint32_t hw_usart_get_rx_overflow(struct usart_t * usart)
{
	return ring_get_overflow(&usart->rx_ring);
}

/**
 * @brief Получить счётчик переполнений TX-буфера
 * @param usart Указатель на структуру UART
 * @return Значение счётчика переполнений
 */
uint32_t hw_usart_get_tx_overflow(struct usart_t * usart)
{
	return ring_get_overflow(&usart->tx_ring);
}

/**
 * @brief Получить общее количество переданных байт
 * @param usart Указатель на структуру UART
 * @return Значение счётчика tx_cnt
 */
uint32_t hw_usart_get_tx(struct usart_t * usart)
{
	return usart->tx_cnt;
}

/**
 * @brief Получить общее количество принятых байт
 * @param usart Указатель на структуру UART
 * @return Значение счётчика rx_cnt
 */
uint32_t hw_usart_get_rx(struct usart_t * usart)
{
	return usart->rx_cnt;
}
