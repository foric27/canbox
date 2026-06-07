/**
 * @brief Обработчик CAN-сообщения ID 0x010 (положение руля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[6] — угол поворота руля (0..59), масштабируется в -100..100
 * Вызывает: обновление carstate.wheel
 *
 * @note База lr2 = 2660 мм, радиус поворота 5700 мм, ширина 1600 мм.
 *       Максимальный угол ~0x2900.
 */
static void lr2_2013my_ms_10_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.wheel = 0;
		return;
	}

	uint8_t angle = msg[6];
	carstate.wheel = scale(angle, 0, 59, -100, 100);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x0B8 (ACC, зажигание, двигатель, подсветка)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0x80 + msg[1] & 0x04 — ACC
 *   - msg[1] & 0x02 — зажигание (IGN)
 *   - msg[0] & 0x80 + msg[1] & 0xE7 — состояние двигателя
 *   - msg[3] — уровень освещённости, масштабируется 0..100
 *
 * Вызывает: обновление carstate.acc, carstate.ign, carstate.engine, carstate.illum
 */
static void lr2_2013my_ms_b8_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.acc = STATE_UNDEF;
		carstate.ign = STATE_UNDEF;
		carstate.engine = STATE_UNDEF;
		carstate.illum = STATE_UNDEF;

		return;
	}

	carstate.acc = ((msg[0] & 0x80) && (msg[1] & 0x04)) ? 1 : 0;
	carstate.ign = (msg[1] & 0x02) ? 1 : 0;
	carstate.engine = ((msg[0] & 0x80) && (msg[1] & 0xe7)) ? 1 : 0;
	carstate.illum = scale(msg[3], 0, 0xff, 0, 100);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x4A6 (парктроник)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0x0F — состояние парктроника (0x04 = включён)
 *   - msg[2:4] — данные передних датчиков (3 байта, 4 зоны)
 *   - msg[5:7] — данные задних датчиков (3 байта, 4 зоны)
 *
 * Вызывает: обновление carstate.radar (state, fl, flm, frm, fr, rl, rlm, rrm, rr)
 *
 * @note msg[1] & 0xF8 — состояние, msg[1] & 0x07 — перед/зад:
 *       1 = зад, 2 = перед.
 */
static void lr2_2013my_ms_4a6_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.radar.state = e_radar_off;
		return;
	}

	uint32_t f = ((uint32_t)msg[2] << 16) | ((uint32_t)msg[3] << 8) | msg[4];
	uint8_t f0 = (f >> 15) & 0x1f;
	uint8_t f1 = (f >> 10) & 0x1f;
	uint8_t f2 = (f >> 5) & 0x1f;
	uint8_t f3 = f & 0x1f;

	carstate.radar.state = (0x04 == (msg[0] & 0x0f)) ? e_radar_on : e_radar_off;

	carstate.radar.fl = scale(f0, 0, 0x0f, 0, 99);
	carstate.radar.flm = scale(f1, 0, 0x0f, 0, 99);
	carstate.radar.frm = scale(f2, 0, 0x0f, 0, 99);
	carstate.radar.fr = scale(f3, 0, 0x0f, 0, 99);

	uint32_t r = ((uint32_t)msg[5] << 16) | ((uint32_t)msg[6] << 8) | msg[7];
	uint8_t r0 = (r >> 15) & 0x1f;
	uint8_t r1 = (r >> 10) & 0x1f;
	uint8_t r2 = (r >> 5) & 0x1f;
	uint8_t r3 = r & 0x1f;

	carstate.radar.rl = scale(r0, 0, 0x0f, 0, 99);
	carstate.radar.rlm = scale(r1, 0, 0x0f, 0, 99);
	carstate.radar.rrm = scale(r2, 0, 0x0f, 0, 99);
	carstate.radar.rr = scale(r3, 0, 0x0f, 0, 99);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x405 (VIN-номер автомобиля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: многофреймовое сообщение UDS
 *   - msg[0] == 0x10 — первый фрейм (3 байта VIN в msg[5:7])
 *   - msg[0] == 0x11 — второй фрейм (7 байт VIN в msg[1:7])
 *   - msg[0] == 0x12 — третий фрейм (7 байт VIN в msg[1:7])
 *
 * Вызывает: заполнение carstate.vin[0..16]
 */
static void lr2_2013my_ms_405_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.vin[0] = STATE_UNDEF;
		return;
	}

	if (msg[0] == 0x10) {

		memcpy(carstate.vin, msg + 5, 3);
	}
	else if (msg[0] == 0x11) {

		memcpy(carstate.vin + 3, msg + 1, 7);
	}
	else if (msg[0] == 0x12) {

		memcpy(carstate.vin + 10, msg + 1, 7);
	}
}

/**
 * @brief Таблица дескрипторов CAN-сообщений для Land Rover LR2 2013 model year
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 */
struct msg_desc_t lr2_2013my_ms[] =
{
	{ 0x10, 50, 0, 0, lr2_2013my_ms_10_handler },
	{ 0xb8, 60, 0, 0, lr2_2013my_ms_b8_handler },
	{ 0x4a6, 90, 0, 0, lr2_2013my_ms_4a6_handler },
	{ 0x405, 500, 0, 0, lr2_2013my_ms_405_handler },
};
