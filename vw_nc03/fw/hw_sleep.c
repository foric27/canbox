#include "NUC131.h"

#include "hw.h"

/**
 * @brief Заглушка обработчика HardFault
 *
 * Требуется новым BSP (v3.00.006).
 * startup_NUC131.S вызывает ProcessHardFault() из HardFault_Handler.
 * Переопределение слабой функции для кастомной диагностики.
 * @param sp Указатель на стек
 */
void __attribute__((weak)) ProcessHardFault(unsigned long * sp)
{
	(void)sp;
	while (1);
}

/**
 * @brief Перевести CPU в режим Power-Down (сон)
 *
 * Вызывает CLK_PowerDown() для входа в режим пониженного энергопотребления.
 */
void hw_cpu_sleep(void)
{
	CLK_PowerDown();
}
