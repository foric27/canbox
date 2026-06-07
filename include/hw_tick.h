#ifndef HW_TICK_H
#define HW_TICK_H

#include <inttypes.h>

/** @brief Частота системного таймера (Гц) */
#define TICK_HZ 1000

/** @brief Преобразование секунд в тики */
#define SEC_TO_TICK(SEC) (TICK_HZ * SEC)

/** @brief Преобразование миллисекунд в тики */
#define MSEC_TO_TICK(MSEC) ((MSEC * TICK_HZ)/1000)

/**
 * @brief Структура флагов системного таймера
 */
typedef struct
{
	volatile uint16_t flag_tick;	/**< Флаг 1 мс */
	volatile uint16_t flag_5ms;		/**< Флаг 5 мс */
	volatile uint16_t flag_100ms;	/**< Флаг 100 мс */
	volatile uint16_t flag_250ms;	/**< Флаг 250 мс */
	volatile uint16_t flag_1000ms;	/**< Флаг 1000 мс */
	volatile uint32_t msec;			/**< Счётчик миллисекунд */
	volatile uint32_t sec;			/**< Счётчик секунд */
} tick_t;

/** @brief Глобальная переменная таймера */
extern volatile tick_t timer;

/**
 * @brief Настройка системного таймера SysTick
 */
void hw_systick_setup(void);

/**
 * @brief Отключение системного таймера
 */
void hw_systick_disable(void);

/**
 * @brief Callback-функция прерывания SysTick
 * @note Вызывается из ISR каждую миллисекунду
 */
void hw_systick_callback(void);

#endif
