#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hw.h"
#include "hw_clock.h"
#include "hw_can.h"
#include "hw_tick.h"
#include "hw_usart.h"
#include "hw_conf.h"

/** @brief Макрос разрешения прерываний (CPSIE i) */
#define cm_enable_interrupts() __asm__ __volatile__ ("cpsie i")
/** @brief Макрос запрета прерываний (CPSID i) */
#define cm_disable_interrupts() __asm__ __volatile__ ("cpsid i")

/** @brief Кольцевой буфер передачи USART (TX), 512 байт */
static uint8_t usart_tx_ring_buffer[512];
/** @brief Кольцевой буфер приема USART (RX), 32 байта */
static uint8_t usart_rx_ring_buffer[32];

/**
 * @brief Инициализация всего оборудования
 * @note Последовательность инициализации:
 *       1. Запрет прерываний
 *       2. Настройка тактирования (PLL, системная частота)
 *       3. Настройка GPIO
 *       4. Настройка SysTick (1 мс)
 *       5. Настройка USART (38400 бод) с кольцевыми буферами
 *       6. Настройка CAN (125 кбит/с по умолчанию)
 *       7. Загрузка конфигурации из Flash/EEPROM
 *       8. Разрешение прерываний
 */
void hw_setup(void)
{
	cm_disable_interrupts();

	hw_clock_setup();

	hw_gpio_setup();

	hw_systick_setup();

	hw_usart_setup(hw_usart_get(), 38400, usart_tx_ring_buffer, sizeof(usart_tx_ring_buffer), usart_rx_ring_buffer, sizeof(usart_rx_ring_buffer));

	hw_can_setup(hw_can_get_mscan(), e_speed_125);

	hw_conf_setup();

	cm_enable_interrupts();
}

/**
 * @brief Перевод устройства в режим сна
 * @note Последовательность перехода в сон:
 *       1. Запрет прерываний
 *       2. Отключение GPIO
 *       3. Отключение SysTick
 *       4. Отключение USART
 *       5. Перевод CAN в спящий режим
 *       6. Разрешение прерываний
 *       7. Переход CPU в режим sleep (WFI)
 * @note Пробуждение происходит по прерыванию от CAN или другого источника
 */
void hw_sleep(void)
{
	cm_disable_interrupts();

	hw_gpio_disable();

	hw_systick_disable();

	hw_usart_disable(hw_usart_get());

	hw_can_sleep(hw_can_get_mscan());

	cm_enable_interrupts();

	hw_cpu_sleep();
}
