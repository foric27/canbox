/**
 * @file canbox_od_bmw_nbt_evo.c
 * @brief Протокол Raise Oudi BMW (NBT/EVO) для Android-головного устройства
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
 * @param type Тип сообщения (например, 0x29 для BMW)
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
 * @brief Отправка данных о состоянии дверей (BMW NBT/EVO-формат)
 *
 * Формирует пакет 0x24, 1 байт:
 * - бит 2: капот
 * - бит 3: багажник
 * - бит 4: задняя левая дверь
 * - бит 5: задняя правая дверь
 * - бит 6: передняя левая дверь
 * - бит 7: передняя правая дверь
 *
 * Использует car_get_door_*(), car_get_tailgate(), car_get_bonnet().
 */
static void canbox_raise_vw_mqb_door_process(void)
{
	uint8_t fl_door = car_get_door_fl();
	uint8_t fr_door = car_get_door_fr();
	uint8_t rl_door = car_get_door_rl();
	uint8_t rr_door = car_get_door_rr();
	uint8_t tailgate = car_get_tailgate();
	uint8_t bonnet = car_get_bonnet();

	uint8_t state = 0;

	if (bonnet)
		state |= 0x4;
	if (tailgate)
		state |= 0x8;
	if (rl_door)
		state |= 0x10;
	if (rr_door)
		state |= 0x20;
	if (fl_door)
		state |= 0x40;
	if (fr_door)
		state |= 0x80;

	snd_canbox_msg(0x24, &state, 1);
}

/**
 * @brief Отправка данных парктроника (радар)
 * @param fmax Массив максимальных значений для 4 передних датчиков
 * @param rmax Массив максимальных значений для 4 задних датчиков
 *
 * Формирует и отправляет:
 * - 0x24 с подтипом 0x00: состояние парковочного режима
 * - 0x23: данные 4 передних датчиков
 * - 0x22: данные 4 задних датчиков
 *
 * Использует car_get_radar().
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
		uint8_t b[] = { 0x0, park_is_on ? 0x08 : 0x00 };
		snd_canbox_msg(0x24, b, sizeof(b));
	}

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
 * @brief Callback: кнопка "громкость +" на руле (заглушка)
 * @param val Величина изменения громкости (игнорируется)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_inc_volume(uint8_t val) { (void)val; }

/**
 * @brief Callback: кнопка "громкость -" на руле (заглушка)
 * @param val Величина изменения громкости (игнорируется)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_dec_volume(uint8_t val) { (void)val; }

/**
 * @brief Callback: кнопка "предыдущий трек" на руле (заглушка)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_prev(void) { }

/**
 * @brief Callback: кнопка "следующий трек" на руле (заглушка)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_next(void) { }

/**
 * @brief Callback: кнопка "режим" (MODE) на руле (заглушка)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_mode(void) { }

/**
 * @brief Callback: кнопка "продолжить/пауза" на руле (заглушка)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_cont(void) { }

/**
 * @brief Callback: кнопка "голосовой ввод / микрофон" на руле (заглушка)
 *
 * Для протокола Oudi BMW SWC не используется через canbox.
 */
void canbox_mici(void) { }

/**
 * @brief Публичный API: обработка входящего байта с USART (заглушка)
 * @param ch Полученный байт (игнорируется)
 *
 * Для протокола Oudi BMW RX state machine не используется.
 */
void canbox_rx_process(uint8_t ch) { (void)ch; }

/**
 * @brief Точка входа основного цикла (250 мс)
 *
 * Вызывает:
 * - canbox_raise_vw_wheel_process(0x29, -5400, 5400) — руль
 * - canbox_raise_vw_mqb_door_process() — двери
 */
static void canbox_od_bmw_nbt_evo_process(void)
{
	canbox_raise_vw_wheel_process(0x29, -5400, 5400);
	canbox_raise_vw_mqb_door_process();
}

/**
 * @brief Точка входа цикла парктроника (100 мс)
 *
 * Вызывает canbox_raise_vw_radar_process() с диапазонами 10..10 для всех датчиков.
 */
static void canbox_od_bmw_nbt_evo_park_process(void)
{
	uint8_t fmax[4] = { 10, 10, 10, 10 };
	uint8_t rmax[4] = { 10, 10, 10, 10 };
	canbox_raise_vw_radar_process(fmax, rmax);
}
