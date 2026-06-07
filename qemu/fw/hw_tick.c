#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>

#include "hw_tick.h"

/**
 * @brief Заглушка отключения SysTick для QEMU
 * @note QEMU target: останавливает счётчик и прерывание
 */
void hw_systick_disable(void)
{
	systick_interrupt_disable();
	systick_counter_disable();
}

/**
 * @brief Заглушка настройки SysTick для QEMU
 * @note QEMU target: 24 МГц / 8 => 3 000 000 отсчётов/с
 */
void hw_systick_setup(void)
{
	/* 24 МГц / 8 => 3 000 000 отсчётов в секунду */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* Сброс счётчика для немедленного старта */
	STK_CVR = 0;

	systick_set_reload(3000000 / TICK_HZ);

	systick_interrupt_enable();

	/* Запуск счётчика. */
	systick_counter_enable();
}

/**
 * @brief Заглушка обработчика прерывания SysTick
 * @note QEMU target: вызывает hw_systick_callback()
 */
void sys_tick_handler(void)
{
	hw_systick_callback();
}
