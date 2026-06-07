/**
 * @brief Обработчик CAN-сообщения ID 0x06C (положение селектора АКПП)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[7] & 0x07 — положение селектора
 * Вызывает: car_set_selector() через carstate.selector
 *
 * @note Кодировка положения:
 *       0 = P, 1 = R, 2 = N, 3 = D
 */
static void lr2_2007my_ms_6c_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.selector = STATE_UNDEF;
		return;
	}

	switch (msg[7] & 0x7) {

		case 0:
			carstate.selector = e_selector_p;
			break;
		case 1:
			carstate.selector = e_selector_r;
			break;
		case 2:
			carstate.selector = e_selector_n;
			break;
		case 3:
			carstate.selector = e_selector_d;
			break;
	}
}

/**
 * @brief Обработчик CAN-сообщения ID 0x07E (состояние зажигания/двигателя/тахометра)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[2] & 0x80 — ACC (аксессуары)
 *   - msg[2] & 0xA0 — IGN (зажигание)
 *   - msg[2] & 0x98 — запуск двигателя
 *   - msg[6] & 0x80 + msg[3:4] — обороты двигателя (тахометр)
 *
 * Вызывает: обновление carstate.acc, carstate.ign, carstate.engine, carstate.taho
 *
 * @note Значения msg[2]:
 *       0x003b8x — ключ вставлен
 *       0x003bax — старт
 *       0x003b9x — работа
 */
static void lr2_2007my_ms_7e_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.acc = STATE_UNDEF;
		carstate.ign = STATE_UNDEF;
		carstate.taho = 0;

		return;
	}

	if (msg[2] & 0x80)
		carstate.acc = 1;
	else
		carstate.acc = 0;

	if ((msg[2] & 0xa0) == 0xa0)
		carstate.ign = 1;
	else
		carstate.ign = 0;

	if ((msg[2] & 0x98) == 0x98) {

		carstate.engine = 1;
		carstate.ign = 1;
	}
	else
		carstate.engine = 0;

	if (msg[6] & 0x80)
		carstate.taho = (((uint16_t)msg[3]) << 8 | msg[4]) & 0xfff;
	else
		carstate.taho = 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x0FD (скорость автомобиля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[6:7] — скорость в условных единицах (делится на 100)
 * Вызывает: обновление carstate.speed
 */
static void lr2_2007my_ms_fd_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.speed = 0;
		return;
	}

	carstate.speed = (((uint16_t)msg[6]) << 8 | msg[7]) & 0xffff;
	carstate.speed /= 100;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x110 (стоянточный тормоз и габариты)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[1] & 0x08 — стояночный тормоз
 *   - msg[0] & 0x20 — габаритные огни (инверсная логика)
 *
 * Вызывает: обновление carstate.park_break, carstate.park_lights
 */
static void lr2_2007my_ms_110_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.park_lights = STATE_UNDEF;
		carstate.park_break = STATE_UNDEF;
		return;
	}

	carstate.park_break = (msg[1] & 0x08) ? 1 : 0;
	carstate.park_lights = (msg[0] & 0x20) ? 0 : 1;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x188 (парктроник)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0xF0 — состояние парктроника (0x70/0x60 = включён)
 *   - msg[5:7] — данные передних датчиков (3 байта, 4 зоны)
 *   - msg[2:4] — данные задних датчиков (3 байта, 4 зоны)
 *
 * Вызывает: обновление carstate.radar (state, fl, flm, frm, fr, rl, rlm, rrm, rr)
 *
 * @note Формат данных датчиков:
 *       Каждая зона кодируется 5 битами, масштабируется в 0..99.
 *       Зоны: fl (front left), flm (front left middle),
 *             frm (front right middle), fr (front right),
 *             rl (rear left), rlm (rear left middle),
 *             rrm (rear right middle), rr (rear right).
 */
static void lr2_2007my_ms_188_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.radar.state = e_radar_off;
		return;
	}

	uint32_t f = ((uint32_t)msg[5] << 16) | ((uint32_t)msg[6] << 8) | msg[7];
	uint8_t f0 = (f >> 15) & 0x1f;
	uint8_t f1 = (f >> 10) & 0x1f;
	uint8_t f2 = (f >> 5) & 0x1f;
	uint8_t f3 = f & 0x1f;

	carstate.radar.state = ((0x70 == (msg[0] & 0xf0)) || (0x60 == (msg[0] & 0xf0))) ? e_radar_on : e_radar_off;

	carstate.radar.fl = scale(f0, 0, 0x0f, 0, 99);
	carstate.radar.flm = scale(f1, 0, 0x0f, 0, 99);
	carstate.radar.frm = scale(f2, 0, 0x0f, 0, 99);
	carstate.radar.fr = scale(f3, 0, 0x0f, 0, 99);

	uint32_t r = ((uint32_t)msg[2] << 16) | ((uint32_t)msg[3] << 8) | msg[4];
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
 * @brief Обработчик CAN-сообщения ID 0x2A0 (освещённость/яркость подсветки)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[1] & 0x0F — уровень освещённости, масштабируется 0..100
 * Вызывает: обновление carstate.illum
 */
static void lr2_2007my_ms_2a0_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.illum = STATE_UNDEF;
		return;
	}

	carstate.illum = scale(msg[1] & 0x0f, 0, 0x0f, 0, 100);
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
static void lr2_2007my_ms_405_handler(const uint8_t * msg, struct msg_desc_t * desc)
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
 * @brief Таблица дескрипторов CAN-сообщений для Land Rover LR2 2007 model year
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 */
static struct msg_desc_t lr2_2007my_ms[] =
{
	{ 0x06c, 30, 0, 0, lr2_2007my_ms_6c_handler },
	{ 0x07e, 50, 0, 0, lr2_2007my_ms_7e_handler },
	{ 0x0fd, 50, 0, 0, lr2_2007my_ms_fd_handler },
	{ 0x110, 60, 0, 0, lr2_2007my_ms_110_handler },
	{ 0x188, 70, 0, 0, lr2_2007my_ms_188_handler },
	{ 0x2a0, 115, 0, 0, lr2_2007my_ms_2a0_handler },
	{ 0x405, 500, 0, 0, lr2_2007my_ms_405_handler },
};
