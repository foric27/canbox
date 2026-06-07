/**
 * @file canbox_raise_vw_pq.c
 * @brief Протокол Raise VW (PQ) для Android-головного устройства
 *
 * Полностью самодостаточный файл. Все символы static, кроме публичного API.
 * Компилируется через #include внутри src/canbox.c.
 */

#include <string.h>

/**
 * @brief Линейное масштабирование значения из одного диапазона в другой
 * @param value Входное значение
 * @param in_min Минимум входного диапазона
 * @param in_max Максимум входного диапазона
 * @param out_min Минимум выходного диапазона
 * @param out_max Максимум выходного диапазона
 * @return Масштабированное значение
 */
static float scale(float value, float in_min, float in_max, float out_min, float out_max)
{
	return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}

/**
 * @brief Расчёт контрольной суммы для протокола Raise
 * @param buf Указатель на буфер данных
 * @param len Длина буфера
 * @return Контрольная сумма (сумма всех байтов, инвертированная XOR 0xFF)
 */
static uint8_t canbox_checksum(uint8_t * buf, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++)
		sum += buf[i];
	sum = sum ^ 0xff;
	return sum;
}

/**
 * @brief Отправка сообщения по протоколу Raise
 * @param type Тип сообщения (байт команды)
 * @param msg Указатель на данные
 * @param size Размер данных
 *
 * Формирует пакет: [0x2E][тип][длина][данные...][чексумма]
 * Отправляет через hw_usart_write().
 */
static void snd_canbox_msg(uint8_t type, uint8_t * msg, uint8_t size)
{
	uint8_t buf[4 + size];
	buf[0] = 0x2e;
	buf[1] = type;
	buf[2] = size;
	memcpy(buf + 3, msg, size);
	buf[3 + size] = canbox_checksum(buf + 1, size + 2);
	hw_usart_write(hw_usart_get(), buf, sizeof(buf));
}

extern uint8_t get_rear_delay_state(void);

/**
 * @brief Отправка данных о положении рулевого колеса
 * @param type Тип сообщения (например, 0x26 для PQ)
 * @param min Минимальное значение угла
 * @param max Максимальное значение угла
 *
 * Получает угол руля через car_get_wheel(), масштабирует и отправляет.
 * Работает только при активном заднем ходе (get_rear_delay_state()).
 */
static void canbox_raise_vw_wheel_process(uint8_t type, int16_t min, int16_t max)
{
	if (!get_rear_delay_state())
		return;

	int8_t wheel = 0;
	if (!car_get_wheel(&wheel))
		return;

	int16_t sangle = scale(wheel, -100, 100, min, max);
	uint8_t wbuf[] = { sangle, sangle >> 8 };
	snd_canbox_msg(type, wbuf, sizeof(wbuf));
}

/**
 * @brief Отправка данных парктроника (радар)
 * @param fmax Массив максимальных значений для 4 передних датчиков
 * @param rmax Массив максимальных значений для 4 задних датчиков
 *
 * Формирует и отправляет:
 * - 0x25: состояние парковочного режима
 * - 0x24: состояние заднего хода, стояночного тормоза, ближнего света
 * - 0x23: данные 4 передних датчиков
 * - 0x22: данные 4 задних датчиков
 *
 * Использует car_get_radar(), car_get_selector(), car_get_park_break(), car_get_near_lights().
 */
static void canbox_raise_vw_radar_process(uint8_t fmax[4], uint8_t rmax[4])
{
	struct radar_t radar;
	car_get_radar(&radar);
	if (radar.state == e_radar_undef)
		return;

	uint8_t _park_is_on = (e_radar_on == radar.state) ? 1 : 0;
	static uint8_t park_is_on = 0;

	if (park_is_on != _park_is_on || park_is_on) {
		park_is_on = _park_is_on;
		uint8_t b = park_is_on ? 0x02 : 0x00;
		snd_canbox_msg(0x25, &b, sizeof(b));
	}

	uint8_t reverse_state = (car_get_selector() == e_selector_r) ? 1 : 0;
	uint8_t park_break = car_get_park_break();
	uint8_t near_lights = car_get_near_lights();
	uint8_t tmp = reverse_state | (park_break << 1) | (near_lights << 2);
	snd_canbox_msg(0x24, &tmp, sizeof(tmp));

	if (!park_is_on)
		return;

	uint8_t fbuf[] = { 0x00, 0x00, 0x00, 0x00 };
	fbuf[0] = fmax[0] + 1 - scale(radar.fr, 0, 99, 0, fmax[0]);
	fbuf[1] = fmax[1] + 1 - scale(radar.frm, 0, 99, 0, fmax[1]);
	fbuf[2] = fmax[2] + 1 - scale(radar.flm, 0, 99, 0, fmax[2]);
	fbuf[3] = fmax[3] + 1 - scale(radar.fl, 0, 99, 0, fmax[3]);
	snd_canbox_msg(0x23, fbuf, sizeof(fbuf));

	uint8_t rbuf[] = { 0x00, 0x00, 0x00, 0x00 };
	rbuf[0] = rmax[0] + 1 - scale(radar.rl, 0, 99, 0, rmax[0]);
	rbuf[1] = rmax[1] + 1 - scale(radar.rlm, 0, 99, 0, rmax[1]);
	rbuf[2] = rmax[2] + 1 - scale(radar.rrm, 0, 99, 0, rmax[2]);
	rbuf[3] = rmax[3] + 1 - scale(radar.rr, 0, 99, 0, rmax[3]);
	snd_canbox_msg(0x22, rbuf, sizeof(rbuf));
}

/**
 * @brief Отправка данных о состоянии дверей
 *
 * Формирует пакет 0x41 с подтипом 0x01.
 * Битовая маска состояния:
 * - бит 0: передняя левая дверь
 * - бит 1: передняя правая дверь
 * - бит 2: задняя левая дверь
 * - бит 3: задняя правая дверь
 * - бит 4: багажник
 * - бит 5: капот (или стояночный тормоз для Skoda Fabia/Q3/Toyota Premio)
 * - бит 6: низкий уровень омывающей жидкости
 * - бит 7: ремень водителя не пристёгнут
 *
 * Использует car_get_door_*(), car_get_tailgate(), car_get_bonnet(),
 * car_get_park_break(), car_get_low_washer(), car_get_ds_belt().
 */
static void canbox_raise_vw_door_process(void)
{
	uint8_t fl_door = car_get_door_fl();
	uint8_t fr_door = car_get_door_fr();
	uint8_t rl_door = car_get_door_rl();
	uint8_t rr_door = car_get_door_rr();
	uint8_t tailgate = car_get_tailgate();
	uint8_t bonnet = car_get_bonnet();
	uint8_t park_break = car_get_park_break();
	uint8_t low_washer = car_get_low_washer();
	uint8_t ds_belt = car_get_ds_belt();

	uint8_t state = 0;

#if defined(CONFIG_CAR_SKODA_FABIA) || defined(CONFIG_CAR_Q3_2015) || defined(CONFIG_CAR_TOYOTA_PREMIO_26X)
	if (ds_belt)
		state |= 0x80;
	if (low_washer)
		state |= 0x40;
	if (park_break)
		state |= 0x20;
#else
	if (bonnet)
		state |= 0x20;
#endif

	if (tailgate)
		state |= 0x10;
	if (rr_door)
		state |= 0x08;
	if (rl_door)
		state |= 0x04;
	if (fr_door)
		state |= 0x02;
	if (fl_door)
		state |= 0x01;

	uint8_t buf[] = { 0x01, state };
	snd_canbox_msg(0x41, buf, sizeof(buf));
}

/**
 * @brief Отправка информации о транспортном средстве
 *
 * Формирует пакет 0x41 с подтипом 0x02 (основные данные) и 0x03 (предупреждения).
 * Данные:
 * - taho: обороты двигателя (car_get_taho())
 * - speed: скорость * 100 (car_get_speed())
 * - voltage: напряжение бортовой сети * 100 (car_get_voltage())
 * - temp: температура ОЖ * 10 (car_get_temp())
 * - odo: одометр, 3 байта (car_get_odometer())
 * - low_fuel: низкий уровень топлива (car_get_low_fuel_level())
 *
 * Предупреждения (0x41 0x03):
 * - бит 7: низкий уровень топлива
 * - бит 6: низкое напряжение
 */
static void canbox_raise_vw_vehicle_info(void)
{
	uint16_t taho = car_get_taho();
	uint8_t t1 = (taho >> 8) & 0xff;
	uint8_t t2 = taho & 0xff;
	uint16_t speed = car_get_speed() * 100;
	uint8_t t3 = (speed >> 8) & 0xff;
	uint8_t t4 = speed & 0xff;
	uint16_t voltage = car_get_voltage() * 100;
	uint8_t t5 = (voltage >> 8) & 0xff;
	uint8_t t6 = voltage & 0xff;
	uint16_t temp = car_get_temp() * 10;
	uint8_t t7 = (temp >> 8) & 0xff;
	uint8_t t8 = temp & 0xff;
	uint32_t odo = car_get_odometer();
	uint8_t t9 = (odo >> 16) & 0xff;
	uint8_t t10 = (odo >> 8) & 0xff;
	uint8_t t11 = odo & 0xff;
	uint8_t t12 = car_get_low_fuel_level();

	uint8_t buf[13] = { 0x02, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12 };
	snd_canbox_msg(0x41, buf, sizeof(buf));

	uint8_t state = 0;
	static uint8_t low_state = 0;

	uint8_t low_voltage = car_get_low_voltage();
	uint8_t low_fuel = car_get_low_fuel_level();

	if (low_fuel)
		state |= 0x80;
	if (low_voltage)
		state |= 0x40;

	uint8_t buf_low[] = { 0x03, state };

	if (state != low_state) {
		low_state = state;
		snd_canbox_msg(0x41, buf_low, sizeof(buf_low));
	}
}

/**
 * @brief Отправка данных о состоянии климат-контроля
 *
 * Формирует пакет 0x21, 5 байт:
 * - [0]: биты режима (AC, рециркуляция, dual, rear, powerfull и т.д.)
 * - [1]: направление потока (wind, middle, floor) + скорость вентилятора
 * - [2]: левая температура
 * - [3]: правая температура
 * - [4]: AQS, rear_lock, подогревы сидений
 *
 * Использует car_get_air_*() функции.
 */
static void canbox_raise_vw_ac_process(void)
{
	uint8_t buf[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

	uint8_t ac = car_get_air_ac();
	uint8_t ac_max = car_get_air_ac_max();
	uint8_t recycling = car_get_air_recycling();
	uint8_t recycling_max = car_get_air_recycling_max();
	uint8_t recycling_min = car_get_air_recycling_min();
	uint8_t dual = car_get_air_dual();
	uint8_t rear = car_get_air_rear();
	uint8_t rear_lock = car_get_air_rear_lock();
	uint8_t aqs = car_get_air_aqs();
	uint8_t wind = car_get_air_wind();
	uint8_t middle = car_get_air_middle();
	uint8_t floor = car_get_air_floor();
	uint8_t powerfull = car_get_air_powerfull();
	uint8_t fanspeed = car_get_air_fanspeed();
	uint8_t l_temp = car_get_air_l_temp();
	uint8_t r_temp = car_get_air_r_temp();
	uint8_t l_seat = car_get_air_l_seat();
	uint8_t r_seat = car_get_air_r_seat();

	if (powerfull)
		buf[0] |= 0x80;
	if (ac)
		buf[0] |= 0x40;
	if (recycling)
		buf[0] |= 0x20;
	if (recycling_max)
		buf[0] |= 0x10;
	if (recycling_min)
		buf[0] |= 0x08;
	if (dual)
		buf[0] |= 0x04;
	if (ac_max)
		buf[0] |= 0x02;
	if (rear)
		buf[0] |= 0x01;

	if (wind)
		buf[1] |= 0x80;
	if (middle)
		buf[1] |= 0x40;
	if (floor)
		buf[1] |= 0x20;

	uint8_t speed = scale(fanspeed, 0x00, 0x0F, 0x00, 0x07);
	buf[1] |= speed & 0x07;

	if ((l_temp % 10) == 0x05)
		buf[2] |= 0x01;
	buf[2] |= ((int)l_temp) << 1;

	if ((r_temp % 10) == 0x05)
		buf[3] |= 0x01;
	buf[3] |= ((int)r_temp) << 1;

	if (aqs)
		buf[4] |= 0x80;
	if (rear_lock)
		buf[4] |= 0x08;
	if (ac_max)
		buf[4] |= 0x04;

	if (l_seat)
		buf[4] |= (l_seat << 4) & 0x30;
	if (r_seat)
		buf[4] |= (r_seat) & 0x03;

	snd_canbox_msg(0x21, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "громкость +" на руле
 * @param val Величина изменения громкости (не используется)
 *
 * Отправляет команду 0x20 0x01 0x01 (нажатие) и 0x20 0x01 0x00 (отпускание)
 * в Android-головное устройство.
 */
void canbox_inc_volume(uint8_t val)
{
	(void)val;
	uint8_t buf[] = { 0x01, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "громкость -" на руле
 * @param val Величина изменения громкости (не используется)
 *
 * Отправляет команду 0x20 0x02 0x01 (нажатие) и 0x20 0x02 0x00 (отпускание).
 */
void canbox_dec_volume(uint8_t val)
{
	(void)val;
	uint8_t buf[] = { 0x02, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "предыдущий трек" на руле
 *
 * Отправляет команду 0x20 0x03 0x01 (нажатие) и 0x20 0x03 0x00 (отпускание).
 */
void canbox_prev(void)
{
	uint8_t buf[] = { 0x03, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "следующий трек" на руле
 *
 * Отправляет команду 0x20 0x04 0x01 (нажатие) и 0x20 0x04 0x00 (отпускание).
 */
void canbox_next(void)
{
	uint8_t buf[] = { 0x04, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "режим" (MODE) на руле
 *
 * Отправляет команду 0x20 0x0A 0x01 (нажатие) и 0x20 0x0A 0x00 (отпускание).
 */
void canbox_mode(void)
{
	uint8_t buf[] = { 0x0a, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "продолжить/пауза" на руле
 *
 * Отправляет команду 0x20 0x09 0x01 (нажатие) и 0x20 0x09 0x00 (отпускание).
 */
void canbox_cont(void)
{
	uint8_t buf[] = { 0x09, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Callback: кнопка "голосовой ввод / микрофон" на руле
 *
 * Отправляет команду 0x20 0x0C 0x01 (нажатие) и 0x20 0x0C 0x00 (отпускание).
 */
void canbox_mici(void)
{
	uint8_t buf[] = { 0x0c, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/**
 * @brief Обработка принятой команды от Android-ГУ
 * @param cmdbuf Буфер с принятым пакетом
 * @param len Длина пакета
 *
 * Распознаёт команды: 0x81, 0x90, 0xA0, 0xA6.
 * В текущей реализации обработчики пустые (зарезервировано).
 */
static void canbox_raise_cmd_process(uint8_t * cmdbuf, uint8_t len)
{
	(void)len;
	uint8_t cmd = cmdbuf[1];

	if (cmd == 0x81) {
	}
	else if (cmd == 0x90) {
	}
	else if (cmd == 0xa0) {
	}
	else if (cmd == 0xa6) {
	}
}

/**
 * @brief Перечисление состояний RX state machine
 */
enum rx_state
{
	RX_WAIT_START, /**< Ожидание стартового байта 0x2E */
	RX_CMD,        /**< Приём байта команды */
	RX_LEN,        /**< Приём байта длины */
	RX_DATA,       /**< Приём полезных данных */
	RX_CRC         /**< Приём контрольной суммы */
};

#define RX_BUFFER_SIZE 32
static uint8_t rx_buffer[RX_BUFFER_SIZE]; /**< Буфер приёма */
static uint8_t rx_idx = 0;                /**< Текущий индекс в буфере */
static uint8_t rx_state = RX_WAIT_START;  /**< Текущее состояние автомата */

/**
 * @brief RX state machine: приём байта от Android-ГУ
 * @param ch Полученный байт с USART
 *
 * Состояния: WAIT_START (0x2E) → CMD → LEN → DATA → CRC.
 * После успешного приёма CRC отправляет ACK (0xFF) и вызывает
 * canbox_raise_cmd_process() для обработки команды.
 * При переполнении буфера сбрасывается в WAIT_START.
 */
static void canbox_raise_rx_process(uint8_t ch)
{
	switch (rx_state) {
		case RX_WAIT_START:
			if (ch != 0x2e)
				break;
			rx_idx = 0;
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_CMD;
			break;
		case RX_CMD:
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_LEN;
			break;
		case RX_LEN:
			rx_buffer[rx_idx++] = ch;
			rx_state = ch ? RX_DATA : RX_CRC;
			break;
		case RX_DATA:
			rx_buffer[rx_idx++] = ch;
			{
				uint8_t len = rx_buffer[2];
				rx_state = ((rx_idx - 2) > len) ? RX_CRC : RX_DATA;
			}
			break;
		case RX_CRC:
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_WAIT_START;
			{
				uint8_t ack = 0xff;
				hw_usart_write(hw_usart_get(), (uint8_t *)&ack, 1);
				canbox_raise_cmd_process(rx_buffer, rx_idx);
			}
			break;
	}
	if (rx_idx > RX_BUFFER_SIZE)
		rx_state = RX_WAIT_START;
}

/**
 * @brief Публичный API: обработка входящего байта с USART
 * @param ch Полученный байт
 *
 * Перенаправляет в canbox_raise_rx_process() — RX state machine протокола Raise.
 */
void canbox_rx_process(uint8_t ch)
{
	canbox_raise_rx_process(ch);
}

/**
 * @brief Точка входа основного цикла (250 мс)
 *
 * Вызывает:
 * - canbox_raise_vw_wheel_process(0x26, -540, 540) — руль
 * - canbox_raise_vw_door_process() — двери
 * - canbox_raise_vw_ac_process() — климат
 * - canbox_raise_vw_vehicle_info() — информация о ТС
 */
static void canbox_raise_vw_pq_process(void)
{
	canbox_raise_vw_wheel_process(0x26, -540, 540);
	canbox_raise_vw_door_process();
	canbox_raise_vw_ac_process();
	canbox_raise_vw_vehicle_info();
}

/**
 * @brief Точка входа цикла парктроника (100 мс)
 *
 * Вызывает canbox_raise_vw_radar_process() с диапазонами 10..10 для всех датчиков.
 */
static void canbox_raise_vw_pq_park_process(void)
{
	uint8_t fmax[4] = { 10, 10, 10, 10 };
	uint8_t rmax[4] = { 10, 10, 10, 10 };
	canbox_raise_vw_radar_process(fmax, rmax);
}
