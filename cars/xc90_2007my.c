/**
 * @brief Обработчик CAN-сообщения ID 0x2510020 (положение руля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[6] & 0x3F — угол поворота руля (0..0x3F)
 *   - msg[5] & 0x04 — направление (1 = право, 0 = лево)
 *
 * Вызывает: обновление carstate.wheel (масштабируется в -100..100)
 *
 * @note Шина: MSCAN (0x2510020).
 */
static void xc90_2007my_ms_wheel_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.wheel = 0;
		return;
	}

	uint8_t angle = msg[6] & 0x3f;
	uint8_t wheel = scale(angle, 0, 0x3f, 0, 100);

	if (msg[5] & 0x04)
		carstate.wheel = wheel;
	else
		carstate.wheel = -wheel;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3200428 (положение селектора АКПП)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: (msg[6] >> 4) & 0x07 — положение селектора
 * Вызывает: обновление carstate.selector
 *
 * @note Кодировка:
 *       1 = P, 2 = R, 3 = N, 4 = D.
 *       По умолчанию устанавливается P.
 */
static void xc90_2007my_ms_gear_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.selector = STATE_UNDEF;
		return;
	}

	switch ((msg[6] >> 4) & 0x07) {

		case 1:
			carstate.selector = e_selector_p;
			break;
		case 2:
			carstate.selector = e_selector_r;
			break;
		case 3:
			carstate.selector = e_selector_n;
			break;
		case 4:
			carstate.selector = e_selector_d;
			break;

		default:
			carstate.selector = e_selector_p;
			break;
	}
}

/**
 * @brief Обработчик CAN-сообщения ID 0x2803008 (освещённость/яркость)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[2] — уровень освещённости (0..0xFF), масштабируется в 0..100
 * Вызывает: обновление carstate.illum
 */
static void xc90_2007my_ms_lsm1_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.illum = STATE_UNDEF;

		return;
	}

	carstate.illum = scale(msg[2], 0, 0xff, 0, 100);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x0217FFC (габариты и ближний свет)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[3] — состояние световых приборов
 *   - bit 2 (0x04) — габаритные огни
 *   - bit 3 (0x08) — ближний свет
 *
 * Вызывает: обновление carstate.park_lights, carstate.near_lights
 */
static void xc90_2007my_ms_lsm0_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.park_lights = STATE_UNDEF;
		carstate.near_lights = STATE_UNDEF;
		return;
	}

	carstate.park_lights = msg[3] & 0x04 ? 1 : 0;
	carstate.near_lights = msg[3] & 0x08 ? 1 : 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x12173BE (двери и парктроник)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[5] — состояние дверей (биты 1..5)
 *   - msg[3] >> 3 & 0x1F — расстояние до препятствия (парктроник)
 *   - msg[2] & 0x01 — включение парктроника (только при задней передаче)
 *
 * Вызывает: обновление carstate.fl_door, fr_door, rl_door, rr_door,
 *           bonnet, tailgate, carstate.radar (все зоны)
 *
 * @note Парктроник активируется только при включённой задней передаче.
 */
static void xc90_2007my_ms_rem_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.fl_door = STATE_UNDEF;
		carstate.fr_door = STATE_UNDEF;
		carstate.rl_door = STATE_UNDEF;
		carstate.rr_door = STATE_UNDEF;
		carstate.bonnet = STATE_UNDEF;
		carstate.tailgate = STATE_UNDEF;

		carstate.radar.state = e_radar_off;
		return;
	}

	carstate.fl_door = (msg[5] & 0x02) ? 1 : 0;
	carstate.fr_door = (msg[5] & 0x04) ? 1 : 0;
	carstate.rl_door = (msg[5] & 0x08) ? 1 : 0;
	carstate.rr_door = (msg[5] & 0x10) ? 1 : 0;
	carstate.bonnet = (msg[5] & 0x41) ? 1 : 0;
	carstate.tailgate = (msg[5] & 0x20) ? 1 : 0;

	uint8_t v = (msg[3] >> 3) & 0x1f;
	v = scale(v, 0x00, 0x1f, 0, 99);

	uint8_t on = (msg[2] & 0x01) ? 0x1 : 0x0;
	if (e_selector_r != car_get_selector())
		on = 0x0;

	carstate.radar.state = on ? e_radar_on : e_radar_off;
	carstate.radar.fl = v;
	carstate.radar.flm = v;
	carstate.radar.frm = v;
	carstate.radar.fr = v;
	carstate.radar.rl = v;
	carstate.radar.rlm = v;
	carstate.radar.rrm = v;
	carstate.radar.rr = v;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x131726C (кнопки на руле — SWC)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[7] & 0x01 — кнопка PREV (по фронту 1→0 вызывает callback)
 *   - msg[7] >> 1 & 0x01 — кнопка NEXT (по фронту 1→0 вызывает callback)
 *
 * Вызывает: key_state.key_cb->prev(), key_state.key_cb->next()
 */
static void xc90_2007my_ms_swm_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		key_state.key_volume = STATE_UNDEF;
		key_state.key_mode = STATE_UNDEF;
		key_state.key_prev = STATE_UNDEF;
		key_state.key_next = STATE_UNDEF;

		return;
	}

	//PREV
	uint8_t key_prev = msg[7] & 0x01;
	//1->0 short release
	if ((key_state.key_prev == 1) && (key_prev == 0) && key_state.key_cb && key_state.key_cb->prev)
		key_state.key_cb->prev();
	key_state.key_prev = key_prev;

	//NEXT
	uint8_t key_next = (msg[7] >> 1) & 0x01;
	//1->0 short release
	if ((key_state.key_next == 1) && (key_next == 0) && key_state.key_cb && key_state.key_cb->next)
		key_state.key_cb->next();
	key_state.key_next = key_next;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x2006428 (ACC и зажигание)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[1] & 0x40 — ACC (аксессуары)
 *   - msg[1] & 0x20 — IGN (зажигание)
 *
 * Вызывает: обновление carstate.acc, carstate.ign
 */
static void xc90_2007my_ms_acc_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.acc = STATE_UNDEF;
		carstate.ign = STATE_UNDEF;

		return;
	}

	if (msg[1] & 0x40)
		carstate.acc = 1;
	else
		carstate.acc = 0;

	if (msg[1] & 0x20)
		carstate.ign = 1;
	else
		carstate.ign = 0;
}

/**
 * @brief Таблица дескрипторов CAN-сообщений для Volvo XC90 2007 model year
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 *
 * @note Используются две CAN-шины:
 *       - MSCAN: 0x2510020, 0x2803008, 0x3200428, 0x2006428
 *       - HSCAN: 0x0217FFC, 0x131726C, 0x12173BE
 */
struct msg_desc_t xc90_2007my_ms[] =
{
	{ 0x0217ffc, 20, 0, 0, xc90_2007my_ms_lsm0_handler },
	{ 0x131726c, 25, 0, 0, xc90_2007my_ms_swm_handler },
	{ 0x12173be, 45, 0, 0, xc90_2007my_ms_rem_handler },
	{ 0x2510020, 80, 0, 0, xc90_2007my_ms_wheel_handler },
	{ 0x2803008, 60, 0, 0, xc90_2007my_ms_lsm1_handler },
	{ 0x3200428, 90, 0, 0, xc90_2007my_ms_gear_handler },
	{ 0x2006428, 120, 0, 0, xc90_2007my_ms_acc_handler },
};
