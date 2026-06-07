#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include "ring.h"
#include "hw_gpio.h"

/**
 * @brief Структура USART-интерфейса (заглушка для QEMU)
 * @note QEMU target: использует реальные регистры STM32F100 USART1
 */
struct usart_t
{
	uint32_t baddr;
	uint32_t rcc;
	uint32_t irq;
	struct gpio_t tx;
	struct gpio_t rx;
	struct ring_t tx_ring;
	struct ring_t rx_ring;
	uint16_t baudrate;
	uint32_t rx_cnt;
	uint32_t tx_cnt;
};

/** Экземпляр USART1 (PA9-TX, PA10-RX) */
static struct usart_t usart1 =
{
	.baddr = USART1,
	.rcc = RCC_USART1,
	.tx = GPIO_INIT(A, 9),
	.rx = GPIO_INIT(A, 10),
	.irq = NVIC_USART1_IRQ,
	.baudrate = 0,
	.rx_cnt = 0,
	.tx_cnt = 0,
};

/**
 * @brief Заглушка: получить указатель на USART1
 * @return Указатель на usart1
 * @note QEMU target: возвращает статический экземпляр
 */
struct usart_t * hw_usart_get(void)
{
	return &usart1;
}

/**
 * @brief Заглушка инициализации USART для QEMU
 * @param usart    Указатель на структуру USART
 * @param speed    Скорость (бод)
 * @param txbuf    Буфер передачи
 * @param txbuflen Размер буфера передачи
 * @param rxbuf    Буфер приёма
 * @param rxbuflen Размер буфера приёма
 * @note QEMU target: настраивает реальный USART1 STM32F100
 */
void hw_usart_setup(struct usart_t * usart, uint32_t speed, uint8_t * txbuf, uint32_t txbuflen, uint8_t * rxbuf, uint32_t rxbuflen)
{
	ring_init(&usart->tx_ring, txbuf, txbuflen);
	ring_init(&usart->rx_ring, rxbuf, rxbuflen);

	usart_disable(usart->baddr);

	/* Включение тактирования USART1. */
	rcc_periph_clock_enable(usart->rcc);

	rcc_periph_clock_enable(RCC_GPIOA);

	/* Разрешение прерывания USART1. */
	nvic_enable_irq(usart->irq);

	gpio_set_mode(usart->tx.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, usart->tx.pin);
	gpio_set_mode(usart->rx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, usart->rx.pin);

	/* Настройка параметров UART. */
	usart_set_baudrate(usart->baddr, speed);
	usart_set_databits(usart->baddr, 8);
	usart_set_stopbits(usart->baddr, USART_STOPBITS_1);
	usart_set_parity(usart->baddr, USART_PARITY_NONE);
	usart_set_flow_control(usart->baddr, USART_FLOWCONTROL_NONE);
	usart_set_mode(usart->baddr, USART_MODE_TX_RX);

	/* Разрешить прерывание по приёму (RXNE). */
	USART_CR1(usart->baddr) |= USART_CR1_RXNEIE;

	/* Включение USART. */
	usart_enable(usart->baddr);
}

/** Счётчик входов в прерывание USART (для отладки) */
uint32_t usart_isr_cnt = 0;

/**
 * @brief Заглушка обработчика прерывания USART для QEMU
 * @param usart Указатель на структуру USART
 * @note QEMU target: использует реальные регистры STM32F100
 */
void usart_isr(struct usart_t * usart)
{
	usart_isr_cnt++;

	/* Проверка флага RXNE. */
	if (((USART_CR1(usart->baddr) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_SR(usart->baddr) & USART_SR_RXNE) != 0)) {

		usart->rx_cnt++;

		/* Чтение данных из периферии. */
		ring_write_ch(&usart->rx_ring, usart_recv(USART1));
	}

	/* Проверка флага TXE. */
	if (((USART_CR1(usart->baddr) & USART_CR1_TXEIE) != 0) &&
	    ((USART_SR(usart->baddr) & USART_SR_TXE) != 0)) {

		uint8_t ch;
		if (!ring_read_ch(&usart->tx_ring, &ch)) {

			/* Отключить прерывание TXE, передача завершена. */
			USART_CR1(usart->baddr) &= ~USART_CR1_TXEIE;
		} else {

			usart->tx_cnt++;

			/* Запись данных в регистр передачи. */
			usart_send(usart->baddr, ch);
		}
	}
}

/**
 * @brief Заглушка обработчика прерывания USART1
 * @note QEMU target: перенаправляет в usart_isr()
 */
void usart1_isr(void)
{
	usart_isr(&usart1);
}

/**
 * @brief Заглушка записи данных в USART для QEMU
 * @param usart Указатель на структуру USART
 * @param ptr   Указатель на данные
 * @param len   Длина данных
 * @return Количество записанных байт
 * @note QEMU target: использует кольцевой буфер
 */
int hw_usart_write(struct usart_t * usart, const uint8_t * ptr, int len)
{
	int ret = ring_write(&usart->tx_ring, (uint8_t *)ptr, len);

	USART_CR1(USART1) |= USART_CR1_TXEIE;

	return ret;
}

/**
 * @brief Заглушка отключения USART для QEMU
 * @param usart Указатель на структуру USART
 * @note QEMU target: отключает USART и переводит пины в floating
 */
void hw_usart_disable(struct usart_t * usart)
{
	usart_disable(usart->baddr);

	hw_gpio_set_float(&usart->rx);
	hw_gpio_set_float(&usart->tx);
}

/**
 * @brief Заглушка чтения байта из RX-буфера
 * @param usart Указатель на структуру USART
 * @param ch    Указатель для записи байта
 * @return 1 при успехе, 0 если буфер пуст
 */
uint8_t hw_usart_read_ch(struct usart_t * usart, uint8_t *ch)
{
	return ring_read_ch(&usart->rx_ring, ch);
}

/**
 * @brief Заглушка: получить счётчик переполнений RX-буфера
 * @param usart Указатель на структуру USART
 * @return Значение счётчика переполнений
 */
uint32_t hw_usart_get_rx_overflow(struct usart_t * usart)
{
	return ring_get_overflow(&usart->rx_ring);
}

/**
 * @brief Заглушка: получить счётчик переполнений TX-буфера
 * @param usart Указатель на структуру USART
 * @return Значение счётчика переполнений
 */
uint32_t hw_usart_get_tx_overflow(struct usart_t * usart)
{
	return ring_get_overflow(&usart->tx_ring);
}

/**
 * @brief Заглушка: получить количество переданных байт
 * @param usart Указатель на структуру USART
 * @return Значение счётчика tx_cnt
 */
uint32_t hw_usart_get_tx(struct usart_t * usart)
{
	return usart->tx_cnt;
}

/**
 * @brief Заглушка: получить количество принятых байт
 * @param usart Указатель на структуру USART
 * @return Значение счётчика rx_cnt
 */
uint32_t hw_usart_get_rx(struct usart_t * usart)
{
	return usart->rx_cnt;
}
