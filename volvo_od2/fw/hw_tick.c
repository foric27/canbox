#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>

#include "hw_tick.h"

/**
 * @brief Отключить SysTick
 *
 * Останавливает счётчик и запрещает прерывание.
 */
void hw_systick_disable(void)
{
	systick_interrupt_disable();
	systick_counter_disable();
}

/**
 * @brief Настройка SysTick
 *
 * Источник: AHB / 8 = 72 МГц / 8 = 9 МГц.
 * Загрузочное значение: 9000000 / TICK_HZ.
 * Сбрасывает текущее значение счётчика (CVR) и запускает таймер.
 */
void hw_systick_setup(void)
{
	/* 72 МГц / 8 => 9 000 000 отсчётов в секунду */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* Сброс счётчика для немедленного старта */
	STK_CVR = 0;

	systick_set_reload(9000000 / TICK_HZ);

	systick_interrupt_enable();

	/* Запуск счётчика. */
	systick_counter_enable();

}

/**
 * @brief Обработчик прерывания SysTick
 *
 * Вызывает общий callback hw_systick_callback().
 */
void sys_tick_handler(void)
{
	hw_systick_callback();
}
