#include "NUC131.h"

#include "hw.h"
#include "hw_tick.h"
#include "clock.h"

/**
 * @brief Настройка SysTick для NUC131
 *
 * Источник: HIRC / 2, счётчик = PLL_CLOCK / 4 / TICK_HZ.
 * Разрешает прерывание и запускает таймер.
 */
void hw_systick_setup(void)
{
	/* (ahb_frequency / 8) отсчётов в секунду */
	uint32_t counter = PLL_CLOCK / 4 / TICK_HZ;
	CLK_EnableSysTick(CLK_CLKSEL0_STCLK_S_HIRC_DIV2, counter);
}

/**
 * @brief Отключить SysTick
 *
 * Останавливает счётчик SysTick.
 */
void hw_systick_disable(void)
{
	CLK_DisableSysTick();
}

/**
 * @brief Обработчик прерывания SysTick
 *
 * Вызывает общий callback hw_systick_callback().
 */
void SysTick_Handler(void)
{
	hw_systick_callback();
}
