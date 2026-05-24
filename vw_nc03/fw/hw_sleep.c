#include "NUC131.h"

#include "hw.h"

/* HardFault handler stub — required by new BSP (v3.00.006).
 * The startup_NUC131.S calls ProcessHardFault() from HardFault_Handler.
 * Override this weak function for custom fault diagnostics. */
void __attribute__((weak)) ProcessHardFault(unsigned long * sp)
{
	(void)sp;
	while (1);
}

void hw_cpu_sleep(void)
{
	CLK_PowerDown();
	//CLK_Idle();
}

