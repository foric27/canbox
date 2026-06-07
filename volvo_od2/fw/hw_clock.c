#include <libopencm3/stm32/rcc.h>

#include "hw_clock.h"

/**
 * @brief Настройка тактирования STM32F103
 *
 * Включает HSE (8 МГц), PLL x9 → 72 МГц.
 * Настраивает шины: AHB = 72 МГц, APB1 = 36 МГц, APB2 = 72 МГц.
 * Включает тактирование AFIO для использования альтернативных функций пинов.
 */
void hw_clock_setup(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Включение тактирования AFIO. */
	rcc_periph_clock_enable(RCC_AFIO);
}
