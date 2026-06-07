#ifndef HW_GPIO_H
#define HW_GPIO_H

#include <inttypes.h>

/**
 * @brief Структура пина GPIO (заглушка для QEMU)
 *
 * @note QEMU target: поля совместимы с STM32F100 (libopencm3).
 * Использует RCC_GPIOx, GPIOx, GPIOx в качестве port/pin/rcc.
 */
struct gpio_t
{
	uint32_t rcc;
	uint32_t port;
	uint32_t pin;
};

/** Макрос инициализации пина: GPIO_INIT(A, 13) → { RCC_GPIOA, GPIOA, GPIO13 } */
#define GPIO_INIT(PORT,PIN) { RCC_GPIO##PORT, GPIO##PORT, GPIO##PIN }

/**
 * @brief Заглушка: установить пин в высокий уровень
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_set(struct gpio_t *);

/**
 * @brief Заглушка: сбросить пин в низкий уровень
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_clr(struct gpio_t *);

/**
 * @brief Заглушка: перевести пин в высокоимпедансный вход
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_set_float(const struct gpio_t * gpio);

#endif
