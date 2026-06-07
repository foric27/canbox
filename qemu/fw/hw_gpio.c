#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "hw.h"
#include "hw_gpio.h"

/** Пин управления задним ходом (PB5) */
static struct gpio_t rear = GPIO_INIT(B, 5);
/** Пин управления парковкой (PB8) */
static struct gpio_t park = GPIO_INIT(B, 8);
/** Пин управления ACC (PB9) */
static struct gpio_t acc = GPIO_INIT(B, 9);
/** Пин управления подсветкой (PC13) */
static struct gpio_t ill = GPIO_INIT(C, 13);

/**
 * @brief Заглушка: установить пин в высокий уровень
 * @param gpio Указатель на структуру пина
 * @note QEMU target: использует реальные регистры GPIO STM32F100
 */
void hw_gpio_set(struct gpio_t * gpio)
{
	gpio_set(gpio->port, gpio->pin);
}

/**
 * @brief Заглушка: сбросить пин в низкий уровень
 * @param gpio Указатель на структуру пина
 * @note QEMU target: использует реальные регистры GPIO STM32F100
 */
void hw_gpio_clr(struct gpio_t * gpio)
{
	gpio_clear(gpio->port, gpio->pin);
}

/**
 * @brief Заглушка: перевести пин в высокоимпедансный вход
 * @param gpio Указатель на структуру пина
 * @note QEMU target: используется при отключении периферии
 */
void hw_gpio_set_float(const struct gpio_t * gpio)
{
	gpio_set_mode(gpio->port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, gpio->pin);
}

/**
 * @brief Заглушка инициализации GPIO
 * @note QEMU target: настраивает пины как выходы push-pull 50 МГц
 */
void hw_gpio_setup(void)
{
	rcc_periph_clock_enable(rear.rcc);
	gpio_clear(rear.port, rear.pin);
	gpio_set_mode(rear.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, rear.pin);

	rcc_periph_clock_enable(park.rcc);
	gpio_clear(park.port, park.pin);
	gpio_set_mode(park.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, park.pin);

	rcc_periph_clock_enable(acc.rcc);
	gpio_clear(acc.port, acc.pin);
	gpio_set_mode(acc.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, acc.pin);

	rcc_periph_clock_enable(ill.rcc);
	gpio_clear(ill.port, ill.pin);
	gpio_set_mode(ill.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, ill.pin);
}

/**
 * @brief Заглушка отключения всех управляющих выходов
 * @note QEMU target: сбрасывает пины в низкий уровень
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

gpio_on(rear)
gpio_off(rear)
gpio_on(park)
gpio_off(park)
gpio_on(acc)
gpio_off(acc)
gpio_on(ill)
gpio_off(ill)
