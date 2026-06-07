/**
 * @file canbox_hiworld_vw_mqb.c
 * @brief Протокол HiWorld VW (MQB) для Android-головного устройства
 *
 * Полностью самодостаточный файл. Все символы static, кроме публичного API.
 * Компилируется через #include внутри src/canbox.c.
 * Использует собственный формат пакетов с префиксом 0x5A 0xA5.
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
 * @brief Расчёт контрольной суммы для протокола HiWorld
 * @param buf Указатель на буфер данных
 * @param len Длина буфера
 * @return Контрольная сумма (сумма всех байтов минус 1)
 */
static uint8_t canbox_hiworld_checksum(uint8_t * buf, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++)
		sum += buf[i];
	sum = sum - 1;
	return sum;
}

/**
 * @brief Отправка сообщения по протоколу HiWorld
 * @param type Тип сообщения (байт команды)
 * @param msg Указатель на данные
 * @param size Размер данных
 *
 * Формирует пакет: [0x5A][0xA5][длина][тип][данные...][чексумма]
 * Отправляет через hw_usart_write().
 */
static void snd_canbox_hiworld_msg(uint8_t type, uint8_t * msg, uint8_t size)
{
	uint8_t buf[5 + size];
	buf[0] = 0x5a;
	buf[1] = 0xa5;
	buf[2] = size;
	buf[3] = type;
	memcpy(buf + 4, msg, size);
	buf[4 + size] = canbox_hiworld_checksum(buf + 2, size + 2);
	hw_usart_write(hw_usart_get(), buf, sizeof(buf));
}

/**
 * @brief Отправка данных о положении рулевого колеса и состоянии парковки
 *
 * Формирует пакет 0x11, 10 байт:
 * - [0]: бит 5 — активен ли парковочный режим
 * - [4]: 0x03 при парковке, иначе 0x00
 * - [6..7]: угол поворота руля (big-endian, масштаб -540..540)
 *
 * Получает угол руля через car_get_wheel() и состояние радара через car_get_radar().
 * Отправляет данные только при изменении состояния парковки или при активном парковочном режиме.
 */
static void canbox_hiworld_vw_mqb_wheel_process(void)
{
	int16_t wmin = -540;
	int16_t wmax = 540;

	int8_t wheel = 0;
	if (!car_get_wheel(&wheel))
		return;

	struct radar_t radar;
	car_get_radar(&radar);

	uint8_t _park_is_on = (e_radar_on == radar.state) ? 1 : 0;
	static uint8_t park_is_on = 0;

	if (park_is_on != _park_is_on || park_is_on) {
		park_is_on = _park_is_on;

		int16_t sangle = scale(wheel, -100, 100, wmin, wmax);
		uint8_t wbuf[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		wbuf[0] = park_is_on ? 0x20 : 0x00;
		wbuf[4] = park_is_on ? 0x03 : 0x00;
		wbuf[6] = sangle >> 8;
		wbuf[7] = sangle;
		snd_canbox_hiworld_msg(0x11, wbuf, sizeof(wbuf));
	}
}

/**
 * @brief Отправка данных парктроника (радар)
 *
 * Формирует пакет 0x41, 12 байт — расстояния до препятствий для всех 8 датчиков.
 * Порядок: RL, RLM, RRM, RR, FR, FRM, FLM, FL.
 *
 * Диапазон масштабирования зависит от положения селектора:
 * - задний ход (e_selector_r): pmax = 165, pstart = 1
 * - остальные: pmax = 250, pstart = 5
 *
 * Использует car_get_radar() и car_get_selector().
 */
static void canbox_hiworld_vw_mqb_radar_process(void)
{
	uint8_t pmax = (e_selector_r == car_get_selector()) ? 165 : 250;
	uint8_t pstart = (e_selector_r == car_get_selector()) ? 1 : 5;

	struct radar_t radar;
	car_get_radar(&radar);
	if (radar.state == e_radar_undef)
		return;

	uint8_t park_is_on = (e_radar_on == radar.state) ? 1 : 0;

	if (park_is_on) {
		uint8_t data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		data[0] = pmax + pstart - scale(radar.rl, 0, 99, 0, pmax);
		data[1] = pmax + pstart - scale(radar.rlm, 0, 99, 0, pmax);
		data[2] = pmax + pstart - scale(radar.rrm, 0, 99, 0, pmax);
		data[3] = pmax + pstart - scale(radar.rr, 0, 99, 0, pmax);
		data[4] = pmax + pstart - scale(radar.fr, 0, 99, 0, pmax);
		data[5] = pmax + pstart - scale(radar.frm, 0, 99, 0, pmax);
		data[6] = pmax + pstart - scale(radar.flm, 0, 99, 0, pmax);
		data[7] = pmax + pstart - scale(radar.fl, 0, 99, 0, pmax);
		snd_canbox_hiworld_msg(0x41, data, sizeof(data));
	}
}

/**
 * @brief Отправка данных о состоянии дверей (HiWorld-формат)
 *
 * Формирует пакет 0x12, 7 байт:
 * - [2]: битовая маска дверей:
 *   - бит 2: капот
 *   - бит 3: багажник
 *   - бит 4: задняя правая дверь
 *   - бит 5: задняя левая дверь
 *   - бит 6: передняя правая дверь
 *   - бит 7: передняя левая дверь
 *
 * Использует car_get_door_*(), car_get_tailgate(), car_get_bonnet().
 */
static void canbox_hiworld_vw_mqb_door_process(void)
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
	if (rr_door)
		state |= 0x10;
	if (rl_door)
		state |= 0x20;
	if (fr_door)
		state |= 0x40;
	if (fl_door)
		state |= 0x80;

	uint8_t data[] = { 0x00, 0x00, state, 0x00, 0x00, 0x00, 0x00 };
	snd_canbox_hiworld_msg(0x12, data, sizeof(data));
}

/**
 * @brief Callback: кнопка "громкость +" на руле (заглушка)
 * @param val Величина изменения громкости (игнорируется)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_inc_volume(uint8_t val) { (void)val; }

/**
 * @brief Callback: кнопка "громкость -" на руле (заглушка)
 * @param val Величина изменения громкости (игнорируется)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_dec_volume(uint8_t val) { (void)val; }

/**
 * @brief Callback: кнопка "предыдущий трек" на руле (заглушка)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_prev(void) { }

/**
 * @brief Callback: кнопка "следующий трек" на руле (заглушка)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_next(void) { }

/**
 * @brief Callback: кнопка "режим" (MODE) на руле (заглушка)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_mode(void) { }

/**
 * @brief Callback: кнопка "продолжить/пауза" на руле (заглушка)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_cont(void) { }

/**
 * @brief Callback: кнопка "голосовой ввод / микрофон" на руле (заглушка)
 *
 * Для протокола HiWorld SWC не используется через canbox.
 */
void canbox_mici(void) { }

/**
 * @brief Публичный API: обработка входящего байта с USART (заглушка)
 * @param ch Полученный байт (игнорируется)
 *
 * Для протокола HiWorld RX state machine не используется.
 */
void canbox_rx_process(uint8_t ch) { (void)ch; }

/**
 * @brief Точка входа основного цикла (250 мс)
 *
 * Вызывает:
 * - canbox_hiworld_vw_mqb_wheel_process() — руль + парковка
 * - canbox_hiworld_vw_mqb_door_process() — двери
 */
static void canbox_hiworld_vw_mqb_process(void)
{
	canbox_hiworld_vw_mqb_wheel_process();
	canbox_hiworld_vw_mqb_door_process();
}

/**
 * @brief Точка входа цикла парктроника (100 мс)
 *
 * Вызывает canbox_hiworld_vw_mqb_radar_process() — данные парктроника.
 */
static void canbox_hiworld_vw_mqb_park_process(void)
{
	canbox_hiworld_vw_mqb_radar_process();
}
