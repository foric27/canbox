#include "NUC131.h"

#include "hw.h"
#include "hw_gpio.h"

/** Пин управления задним ходом (PA13) */
static struct gpio_t rear = GPIO_INIT(A, 13);
/** Пин управления парковкой (PA12) */
static struct gpio_t park = GPIO_INIT(A, 12);
/** Пин управления ACC (PA8) */
static struct gpio_t acc = GPIO_INIT(A, 8);
/** Пин управления подсветкой (PA9) */
static struct gpio_t ill = GPIO_INIT(A, 9);

/**
 * @brief Установить пин в высокий уровень
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_set(struct gpio_t * gpio)
{
	gpio->port->DOUT |= gpio->pin;
}

/**
 * @brief Сбросить пин в низкий уровень
 * @param gpio Указатель на структуру пина
 */
void hw_gpio_clr(struct gpio_t * gpio)
{
	gpio->port->DOUT &= ~gpio->pin;
}

/**
 * @brief Перевести пин в квази-двунаправленный режим
 * @param gpio Указатель на структуру пина
 *
 * Используется при отключении периферии.
 */
void hw_gpio_set_float(const struct gpio_t * gpio)
{
	GPIO_SetMode(gpio->port, gpio->pin, GPIO_PMD_QUASI);
}

/**
 * @brief Инициализация GPIO
 *
 * Настраивает пины rear, park, acc, ill как выходы push-pull.
 */
void hw_gpio_setup(void)
{
	GPIO_SetMode(rear.port, rear.pin, GPIO_PMD_OUTPUT);

	GPIO_SetMode(park.port, park.pin, GPIO_PMD_OUTPUT);

	GPIO_SetMode(acc.port, acc.pin, GPIO_PMD_OUTPUT);

	GPIO_SetMode(ill.port, ill.pin, GPIO_PMD_OUTPUT);
}

/**
 * @brief Отключить все управляющие выходы
 *
 * Сбрасывает rear, park, acc, ill в низкий уровень.
 */
void hw_gpio_disable(void)
{
	hw_gpio_clr(&rear);
	hw_gpio_clr(&park);
	hw_gpio_clr(&acc);
	hw_gpio_clr(&ill);
}

/** Макрос генерации функции включения пина */
#define gpio_on(pin) void hw_gpio_##pin##_on(void) { hw_gpio_set(&pin); }
/** Макрос генерации функции выключения пина */
#define gpio_off(pin) void hw_gpio_##pin##_off(void) { hw_gpio_clr(&pin); }

gpio_on(rear);
gpio_off(rear);
gpio_on(park);
gpio_off(park);
gpio_on(acc);
gpio_off(acc);
gpio_on(ill);
gpio_off(ill);
