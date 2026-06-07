#include "hw_tick.h"

/** @brief Глобальная структура таймерных флагов и счетчиков */
volatile tick_t timer = { 0, 0, 0, 0, 0, 0, 0 };

/**
 * @brief Обработчик прерывания SysTick (1 мс)
 * @note Вызывается аппаратно каждую миллисекунду.
 *       Генерирует флаги для различных таймерных доменов:
 *       - flag_tick:    каждый 1 мс
 *       - flag_5ms:     каждые 5 мс
 *       - flag_100ms:   каждые 100 мс
 *       - flag_250ms:   каждые 250 мс
 *       - flag_1000ms:  каждую секунду
 *       Также обновляет счетчики msec и sec.
 */
void hw_systick_callback(void)
{
	static uint16_t div_1000ms = 0;
	static uint16_t div_250ms = 0;
	static uint16_t div_100ms = 0;
	static uint16_t div_5ms = 0;

	timer.flag_tick = 1;
	timer.msec++;

	if (++div_1000ms >= SEC_TO_TICK(1)) {

		div_1000ms = 0;
		timer.flag_1000ms = 1;
		timer.sec++;
		timer.msec = 0;
	}

	if (++div_250ms >= MSEC_TO_TICK(250)) {

		div_250ms = 0;
		timer.flag_250ms = 1;
	}

	if (++div_100ms >= MSEC_TO_TICK(100)) {

		div_100ms = 0;
		timer.flag_100ms = 1;
	}

	if (++div_5ms >= MSEC_TO_TICK(5)) {

		div_5ms = 0;
		timer.flag_5ms = 1;
	}
}
