/**
 * @brief Обработчик CAN-сообщения ID 0x2510020 (положение руля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[2:1] — угол поворота руля (16-битное значение)
 * Вызывает: обновление carstate.wheel
 */
static void skoda_fabia_ms_wheel_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.wheel = 0;
		return;
	}

	carstate.wheel = (((uint16_t)msg[2]) << 8 | msg[1]) & 0xffff;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x351 (скорость и задняя передача)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0x02 — задняя передача
 *   - msg[2:1] — скорость (16-бит), делится на 2
 *
 * Вызывает: обновление carstate.selector, carstate.speed
 */
static void skoda_fabia_ms_gear_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.selector = STATE_UNDEF;
		carstate.speed = 0;
		return;
	}

	if (msg[0] & 0x02)
		carstate.selector = e_selector_r;
	else
		carstate.selector = STATE_UNDEF;

	carstate.speed = (((uint16_t)msg[2]) << 8 | msg[1]) & 0xffff;
	carstate.speed /= 2;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x635 (освещённость/яркость)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[0] & 0x64 — состояние подсветки (100% или 0%)
 * Вызывает: обновление carstate.illum
 */
static void skoda_fabia_ms_illum_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.illum = STATE_UNDEF;

		return;
	}

	carstate.illum = (msg[0] & 0x64) ? 100 : 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x621 (стояночный тормоз)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[0] & 0x20 — стояночный тормоз
 * Вызывает: обновление carstate.park_break
 */
static void skoda_fabia_ms_park_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.park_break = STATE_UNDEF;
		return;
	}

	carstate.park_break = (msg[0] & 0x20) ? 1 : 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x371 (двери и парктроник)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] — двери (биты 0..3)
 *   - msg[1] — капот и багажник (биты 1 и 3)
 *   - msg[3] >> 3 & 0x1F — расстояние парктроника
 *   - msg[2] & 0x01 — активация парктроника (только при задней передаче)
 *
 * Вызывает: обновление carstate.door_*, bonnet, tailgate, carstate.radar
 *
 * @note Парктроник активируется только при включённой задней передаче.
 */
static void skoda_fabia_ms_rem_handler(const uint8_t * msg, struct msg_desc_t * desc)
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

	carstate.fl_door = (msg[0] & 0x01) ? 1 : 0;
	carstate.fr_door = (msg[0] & 0x02) ? 1 : 0;
	carstate.rl_door = (msg[0] & 0x04) ? 1 : 0;
	carstate.rr_door = (msg[0] & 0x08) ? 1 : 0;
	carstate.bonnet = (msg[1] & 0x08) ? 1 : 0;
	carstate.tailgate = (msg[1] & 0x02) ? 1 : 0;

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
 *   - msg[7] & 0x01 — кнопка PREV (по фронту 1→0)
 *   - msg[7] >> 1 & 0x01 — кнопка NEXT (по фронту 1→0)
 *
 * Вызывает: key_state.key_cb->prev(), key_state.key_cb->next()
 */
static void skoda_fabia_ms_swm_handler(const uint8_t * msg, struct msg_desc_t * desc)
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
 * @brief Обработчик CAN-сообщения ID 0x271 (ACC и зажигание)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] == 0x01 — ACC включён
 *   - msg[0] == 0x00 — ACC выключен
 *   - msg[0] & 0x07 — зажигание (IGN)
 *
 * Вызывает: обновление carstate.acc, carstate.ign
 */
static void skoda_fabia_ms_acc_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (msg[0] == 0x01)
		carstate.acc = 1;

	if (msg[0] == 0x00)
		carstate.acc = 0;

	else
		if (is_timeout(desc)) {

			carstate.ign = STATE_UNDEF;

			return;
		}

	carstate.ign = (msg[0] & 0x07) ? 1 : 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3E1 (кондиционер)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[0] & 0x10 — состояние AC (вкл/выкл)
 * Вызывает: обновление car_air_state.ac
 */
static void skoda_fabia_ms_aircon_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		car_air_state.ac = STATE_UNDEF;
		return;
	}

	car_air_state.ac = (msg[0] & 0x10) ? 1 : 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x353 (обороты двигателя — тахометр)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[2:1] — обороты двигателя (16-бит), делится на 4
 * Вызывает: обновление carstate.taho
 */
static void skoda_fabia_ms_taho_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
    if (is_timeout(desc)) {

		carstate.taho = 0;

		return;
	}

	carstate.taho = (((uint16_t)msg[2]) << 8 | msg[1]) & 0xffff;
	carstate.taho /= 4;
}

/**
 * @brief Таблица дескрипторов CAN-сообщений для Skoda Fabia
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 */
struct msg_desc_t skoda_fabia_ms[] =
{
	{ 0x635, 100, 0, 0, skoda_fabia_ms_illum_handler },
	{ 0x131726c, 25, 0, 0, skoda_fabia_ms_swm_handler },
	{ 0x371, 200, 0, 0, skoda_fabia_ms_rem_handler },
	{ 0x2510020, 80, 0, 0, skoda_fabia_ms_wheel_handler },
	{ 0x621, 200, 0, 0, skoda_fabia_ms_park_handler },
	{ 0x351, 100, 0, 0, skoda_fabia_ms_gear_handler },
	{ 0x271, 100, 0, 0, skoda_fabia_ms_acc_handler },
	{ 0x3e1, 200, 0, 0, skoda_fabia_ms_aircon_handler },
	{ 0x353, 100, 0, 0, skoda_fabia_ms_taho_handler },
};
