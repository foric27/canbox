#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hw.h"
#include "hw_tick.h"
#include "hw_usart.h"
#include "hw_can.h"
#include "car.h"
#include "canbox.h"
#include "config.h"

/** @brief Счетчик задержки выключения заднего хода (в тиках) */
static uint32_t rear_off_delay = 0;
/** @brief Счетчик задержки включения заднего хода (в тиках) */
static uint32_t rear_on_delay = 0;
/** @brief Таймаут подтверждения включения заднего хода (мс) */
static uint32_t rear_on_timeout = 200;

/**
 * @brief Обработка задержки заднего хода
 * @param ticks Количество миллисекунд с момента последнего вызова
 * @note Управляет логикой задержки включения/выключения заднего хода
 *       для предотвращения дребезга при переключении селектора
 */
static void rear_delay_process(uint8_t ticks)
{
	uint8_t rear_state = (e_selector_r == car_get_selector()) ? 1 : 0;

	if (rear_state) {
		rear_on_delay += ticks;
		rear_off_delay = 0;
	}
	else {
		if (rear_on_delay && (rear_on_delay < 2 * rear_on_timeout))
			rear_off_delay = CONFIG_REAR_DELAY;

		rear_off_delay += ticks;
		rear_on_delay = 0;
	}
}

/**
 * @brief Получить состояние задержки заднего хода
 * @return 1 — задний ход активен или находится в задержке выключения,
 *         0 — задний ход неактивен
 * @note Учитывает состояние зажигания: при выключенном зажигании всегда 0
 */
uint8_t get_rear_delay_state(void)
{
	uint8_t ign = car_get_ign();
	if (!ign)
		return 0;

	if ((rear_on_delay > rear_on_timeout) || (rear_off_delay < CONFIG_REAR_DELAY))
		return 1;
	else
		return 0;
}

/** @brief Структура обратных вызовов для кнопок рулевого управления */
struct key_cb_t key_cb =
{
	.mode = canbox_mode,
	.inc_volume = canbox_inc_volume,
	.dec_volume = canbox_dec_volume,
	.prev = canbox_prev,
	.next = canbox_next,
	.cont = canbox_cont,
	.navi = canbox_mode,
	.mici = canbox_mici,
};

/**
 * @brief Обработка входящих данных по USART
 * @note Считывает один байт из кольцевого буфера RX и передает
 *       в парсер протокола canbox
 */
static void usart_process(void)
{
	uint8_t ch = 0;
	if (!hw_usart_read_ch(hw_usart_get(), &ch))
		return;

	canbox_rx_process(ch);
}

/**
 * @brief Обработка состояния GPIO
 * @note Управляет выходными линиями в зависимости от состояния автомобиля:
 *       - ACC (доступность аксессуаров)
 *       - Подсветка (illumination)
 *       - Задний ход (с задержкой)
 */
static void gpio_process(void)
{
	uint8_t acc = car_get_acc();
	uint8_t ill = car_get_illum();

	if (acc)
		hw_gpio_acc_on();
	else
		hw_gpio_acc_off();

	if (ill > CONFIG_ILLUM)
		hw_gpio_ill_on();
	else
		hw_gpio_ill_off();

	if (get_rear_delay_state())
		hw_gpio_rear_on();
	else
		hw_gpio_rear_off();
}

/**
 * @brief Точка входа в прошивку
 * @return Никогда не возвращает управление
 * @note Инициализирует оборудование и автомобиль, затем входит
 *       в бесконечный цикл с диспетчеризацией задач по таймерным доменам:
 *       - 1 мс: обработка задержки заднего хода
 *       - 5 мс: обработка CAN-сообщений
 *       - 100 мс: отправка парковочных данных
 *       - 250 мс: отправка состояния автомобиля
 *       - 1000 мс: мониторинг активности CAN-шины и сон
 */
int main(void)
{
	hw_setup();

	car_init(&key_cb);

	uint8_t acc = car_get_acc();
	uint32_t ms_can_nums = 0;
	uint32_t ms_can_stop_counter = 0;

	while(1) {
		gpio_process();
		usart_process();

		if (timer.flag_tick) {
			timer.flag_tick = 0;
			rear_delay_process(1);
		}

		if (timer.flag_5ms) {
			timer.flag_5ms = 0;
			car_process(5);
		}

		if (timer.flag_100ms) {
			timer.flag_100ms = 0;
			canbox_park_process();
		}

		if (timer.flag_250ms) {
			timer.flag_250ms = 0;
			canbox_process();
		}

		if (timer.flag_1000ms) {
			timer.flag_1000ms = 0;

			uint32_t nums = hw_can_get_pack_nums(hw_can_get_mscan());
			if (nums == ms_can_nums)
				ms_can_stop_counter++;
			else
				ms_can_stop_counter = 0;

#if defined(CONFIG_CAR_SKODA_FABIA)
			if (acc)
				ms_can_stop_counter = 1;
#endif

			ms_can_nums = nums;

			if (ms_can_stop_counter > 10) {
				hw_sleep();
				ms_can_stop_counter = 0;
				hw_setup();
			}
		}
	}
}
