#include <libopencm3/stm32/rcc.h>

#include "hw_clock.h"

/**
 * @brief Заглушка настройки тактирования для QEMU
 * @note QEMU target: машина stm32vldiscovery уже настроена,
 *      реальная инициализация закомментирована
 */
void hw_clock_setup(void)
{
	//rcc_clock_setup_in_hse_8mhz_out_72mhz();

	/* Включение тактирования AFIO. */
	//rcc_periph_clock_enable(RCC_AFIO);
}
