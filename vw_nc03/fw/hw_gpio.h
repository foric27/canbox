#ifndef HW_GPIO_H
#define HW_GPIO_H

#include <inttypes.h>

/** Прототип структуры GPIO_T из BSP NUC131 */
struct GPIO_T;

/**
 * @brief Структура пина GPIO для NUC131
 *
 * Хранит указатель на порт (GPIO_T) и битовую маску пина.
 */
struct gpio_t
{
	GPIO_T * port;
	uint32_t pin;
};

/** Макрос инициализации пина: GPIO_INIT(A, 13) → { PA, BIT13 } */
#define GPIO_INIT(PORT,PIN) { P##PORT, BIT##PIN }

/**
 * @brief Установить пин в высокий уровень
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_set(struct gpio_t *);

/**
 * @brief Сбросить пин в низкий уровень
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_clr(struct gpio_t *);

/**
 * @brief Перевести пин в квази-двунаправленный режим
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_set_float(const struct gpio_t * gpio);

#endif
