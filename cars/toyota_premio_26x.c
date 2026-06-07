/**
 * @brief Обработчик CAN-сообщения ID 0x025 (положение руля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0x0F + msg[1] — угол поворота руля (12-бит, центр = 2048)
 *
 * Вызывает: обновление carstate.wheel (масштабируется -100..100)
 *
 * @note Угол центрального положения ~2048, диапазон ±380 единиц.
 */
static void toyota_premio_26x_ms_wheel_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.wheel = 0;
		return;
	}

	int16_t angle = (int16_t)(((uint16_t)(msg[0] & 0x0F)) << 8 | msg[1]);
	angle = (angle < 2048) ? angle : (angle - 4096);

	carstate.wheel = scale(angle, -380, 380, -100, 100);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x0B4 (скорость автомобиля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[5:6] — скорость (16-бит), коррекция +50 и деление на 100
 * Вызывает: обновление carstate.speed
 */
static void toyota_premio_26x_ms_speed_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.speed = 0;
		return;
	}
	carstate.speed = ((((uint16_t)msg[5]) << 8 | msg[6]) + 50) / 100;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x620 (зажигание, тормоз, двери, ремень)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[4] & 0x10 — ACC
 *   - msg[4] & 0x20 — зажигание (IGN)
 *   - msg[7] & 0x10 — стояночный тормоз (инверсная логика)
 *   - msg[5] — состояние дверей (биты 0..5)
 *   - msg[7] & 0x40 — ремень водителя
 *
 * Вызывает: обновление carstate.acc, ign, park_break, door_*, tailgate, ds_belt
 */
static void toyota_premio_26x_ms_ign_brake_doors_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.acc = STATE_UNDEF;
		carstate.ign = STATE_UNDEF;
		carstate.park_break = STATE_UNDEF;
		carstate.fl_door = STATE_UNDEF;
		carstate.fr_door = STATE_UNDEF;
		carstate.rl_door = STATE_UNDEF;
		carstate.rr_door = STATE_UNDEF;
		carstate.tailgate = STATE_UNDEF;
		carstate.ds_belt = STATE_UNDEF;
		return;
	}
	carstate.acc        = (msg[4] & 0x10) ? 1:0;
	carstate.ign 		= (msg[4] & 0x20) ? 1:0;
	carstate.park_break = (msg[7] & 0x10) ? 0:1;
	carstate.fl_door 	= (msg[5] & 0x20) ? 1:0;
	carstate.fr_door 	= (msg[5] & 0x10) ? 1:0;
	carstate.rl_door 	= (msg[5] & 0x08) ? 1:0;
	carstate.rr_door 	= (msg[5] & 0x04) ? 1:0;
	carstate.tailgate 	= (msg[5] & 0x01) ? 1:0;
	carstate.ds_belt    = (msg[7] & 0x40) ? 1:0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x622 (освещение)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[3] — состояние световых приборов
 *   - bit 4 (0x10) — освещённость/габариты
 *   - bit 5 (0x20) — ближний свет
 *
 * Вызывает: обновление carstate.illum, near_lights, park_lights
 */
static void toyota_premio_26x_ms_light_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.illum = STATE_UNDEF;
		carstate.near_lights = STATE_UNDEF;
		carstate.park_lights = STATE_UNDEF;
		return;
	}
	carstate.illum 			= (msg[3] & 0x10) ? 100:0;
	carstate.near_lights 	= (msg[3] & 0x20) ? 1:0;
	carstate.park_lights    = (msg[3] & 0x10) ? 1:0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3B4 (селектор АКПП)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[4] & 0xF0 — основное положение селектора
 *   - msg[5] — дополнительное уточнение (для D/S)
 *
 * Вызывает: обновление carstate.selector
 *
 * @note Кодировка:
 *       0x80 = P, 0x40 = R, 0x20 = N,
 *       0x00 + msg[5] = 0x40 → D,
 *       0x00 + msg[5] = 0x00/0x01 → S.
 */
static void toyota_premio_26x_ms_drive_mode_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
	 	carstate.selector = STATE_UNDEF;
		return;
	}

	if ((msg[4] & 0xF0) == 0x80)
		carstate.selector = e_selector_p;
	else if ((msg[4] & 0xF0) == 0x40)
		carstate.selector = e_selector_r;
	else if ((msg[4] & 0xF0) == 0x20)
		carstate.selector = e_selector_n;
	else if ((msg[4] & 0xF0) == 0x00 && msg[5] == 0x40)
		carstate.selector = e_selector_d;
	else if ((msg[4] & 0xF0) == 0x00 && (msg[5] == 0x00 || msg[5] == 0x01))
		carstate.selector = e_selector_s;
	else
		carstate.selector = STATE_UNDEF;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x611 (одометр/пробег)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[5:7] — пробег (24-битное значение)
 * Вызывает: обновление carstate.odometer
 */
static void toyota_premio_26x_ms_odometer(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.odometer = 0;
		return;
	}
	carstate.odometer = ((uint32_t)msg[5] << 16) | ((uint32_t)msg[6] << 8) | ((uint32_t)msg[7] << 0);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x2C4 (обороты двигателя — тахометр)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[0:1] — обороты двигателя (16-бит), коррекция (×3+2)/4
 * Вызывает: обновление carstate.taho
 */
static void toyota_premio_26x_ms_tacho_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.taho = 0;
		return;
	}
	carstate.taho = ((uint32_t)msg[0] << 8) | ((uint32_t)msg[1] << 0);
	carstate.taho = ((carstate.taho * 3) + 2) / 4;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3B0 (температура ОЖ)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[3] — температура охлаждающей жидкости (со смещением -0x30)
 * Вызывает: обновление carstate.temp
 */
static void toyota_premio_26x_ms_temp_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {
		carstate.temp = 0;
		return;
	}

	carstate.temp = (int32_t)((int8_t)msg[3]-0x30);
}

/**
 * @brief Таблица дескрипторов CAN-сообщений для Toyota Premio (260 series)
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 */
struct msg_desc_t toyota_premio_26x_ms[] =
{
	{ 0x025,  	 80, 0, 0, toyota_premio_26x_ms_wheel_handler },
	{ 0x0b4,	100, 0, 0, toyota_premio_26x_ms_speed_handler },
	{ 0x620,    200, 0, 0, toyota_premio_26x_ms_ign_brake_doors_handler },
	{ 0x622,   1000, 0, 0, toyota_premio_26x_ms_light_handler },
	{ 0x3b4,   1000, 0, 0, toyota_premio_26x_ms_drive_mode_handler},
	{ 0x611,   1000, 0, 0, toyota_premio_26x_ms_odometer},
	{ 0x2c4,    100, 0, 0, toyota_premio_26x_ms_tacho_handler},
	{ 0x3b0,   2000, 0, 0, toyota_premio_26x_ms_temp_handler},
};
