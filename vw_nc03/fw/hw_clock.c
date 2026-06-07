#include "NUC131.h"

#include "clock.h"
#include "hw_clock.h"

/**
 * @brief Инициализация системного тактирования NUC131
 *
 * Включает внутренний RC-генератор 22.1184 МГц,
 * дожидается его стабилизации, переключает HCLK на HIRC,
 * затем устанавливает системную частоту через PLL (PLL_CLOCK).
 */
void SYS_Init(void)
{
	/* Включение внутреннего RC 22.1184 МГц */
	CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);

	/* Ожидание готовности внутреннего RC */
	CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

	/* Переключение HCLK на HIRC, делитель = 1 */
	CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

	/* Установка системной частоты через PLL */
	CLK_SetCoreClock(PLL_CLOCK);
}

/**
 * @brief Настройка тактирования NUC131
 *
 * Разблокирует регистры, вызывает SYS_Init(),
 * затем блокирует регистры обратно.
 */
void hw_clock_setup()
{
	SYS_UnlockReg();

	SYS_Init();

	SYS_LockReg();
}
