/**
 * @brief Обработчик CAN-сообщения ID 0x2C3 (ACC и зажигание)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0x01 — ACC (аксессуары)
 *   - msg[0] & 0x02 — IGN (зажигание)
 *
 * Вызывает: обновление carstate.acc, carstate.ign
 *
 * @note Примеры значений msg[0]:
 *       0x10 — ключ не вставлен
 *       0x01 — ключ вставлен, зажигание выключено
 *       0x07 — зажигание включено
 *       0x0B — стартер
 */
static void q3_2015_ms_2c3_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.acc = STATE_UNDEF;
		carstate.ign = STATE_UNDEF;
		return;
	}

	if (msg[0] & 0x01)
		carstate.acc = 1;
	else
		carstate.acc = 0;

	if ((msg[0] & 0x02) == 0x02)
		carstate.ign = 1;
	else
		carstate.ign = 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x65F (VIN-номер автомобиля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: многофреймовое сообщение UDS
 *   - msg[0] == 0x00 — первый фрейм (3 байта VIN в msg[5:7])
 *   - msg[0] == 0x01 — второй фрейм (7 байт VIN в msg[1:7])
 *   - msg[0] == 0x02 — третий фрейм (7 байт VIN в msg[1:7])
 *
 * Вызывает: заполнение carstate.vin[0..16]
 */
static void q3_2015_ms_65F_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.vin[0] = STATE_UNDEF;
		return;
	}

	if (msg[0] == 0x00)
		memcpy(carstate.vin, msg + 5, 3);
	else if (msg[0] == 0x01)
		memcpy(carstate.vin + 3, msg + 1, 7);
	else if (msg[0] == 0x02)
		memcpy(carstate.vin + 10, msg + 1, 7);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x65D (одометр/пробег)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[3], msg[2], msg[1] — пробег в км (BCD-подобное представление)
 * Вызывает: обновление carstate.odometer
 */
static void q3_2015_ms_65D_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.odometer = STATE_UNDEF;
		return;
	}

	uint8_t odo[3] = { (msg[3] & 0x0F), msg[2], msg[1] };
	uint32_t value = 0;

	for (int i = 0; i < 3; i++) {
		value = (value << 8) + (odo[i] & 0xFF);
	}

	carstate.odometer = value;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x571 (напряжение бортовой сети)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[0] — напряжение (формула: 5 + 0.05 * msg[0])
 * Вызывает: обновление carstate.voltage
 *
 * @note Пример: msg[0] = 0xA6 → 5 + 0.05 * 166 = 13.3 В
 */
static void q3_2015_ms_571_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.voltage = STATE_UNDEF;
		return;
	}

	carstate.voltage = 5 + (0.05 * msg[0]);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x470 (состояние дверей)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[1] — состояние дверей (биты 0..5)
 *   - bit 0 — передняя левая дверь
 *   - bit 1 — передняя правая дверь
 *   - bit 2 — задняя левая дверь
 *   - bit 3 — задняя правая дверь
 *   - bit 4 — капот
 *   - bit 5 — багажник
 *
 * Вызывает: обновление carstate.fl_door, fr_door, rl_door, rr_door, bonnet, tailgate
 */
static void q3_2015_ms_470_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.fl_door = STATE_UNDEF;
		carstate.fr_door = STATE_UNDEF;
		carstate.rl_door = STATE_UNDEF;
		carstate.rr_door = STATE_UNDEF;
		carstate.bonnet = STATE_UNDEF;
		carstate.tailgate = STATE_UNDEF;

		return;
	}

	carstate.fl_door  = (msg[1] & 0x01) ? 1 : 0;
	carstate.fr_door  = (msg[1] & 0x02) ? 1 : 0;
	carstate.rl_door  = (msg[1] & 0x04) ? 1 : 0;
	carstate.rr_door  = (msg[1] & 0x08) ? 1 : 0;
	carstate.bonnet   = (msg[1] & 0x10) ? 1 : 0;
	carstate.tailgate = (msg[1] & 0x20) ? 1 : 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x359 (селектор АКПП и скорость)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - (msg[7] >> 4) & 0x0F — положение селектора
 *   - msg[2:1] — скорость автомобиля (км/ч, делится на 100)
 *
 * Вызывает: обновление carstate.selector, carstate.speed
 *
 * @note Кодировка селектора:
 *       0x08 = P, 0x07 = R, 0x06 = N, 0x05 = D,
 *       0x0A = M+, 0x0B = M-, 0x0C = S, 0x0E = M (ручной).
 */
static void q3_2015_ms_359_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.selector = STATE_UNDEF;
		return;
	}

	switch ((msg[7] >> 4) & 0x0f) {

		case 0x08:
			carstate.selector = e_selector_p;
			break;
		case 0x07:
			carstate.selector = e_selector_r;
			break;
		case 0x06:
			carstate.selector = e_selector_n;
			break;
		case 0x05:
			carstate.selector = e_selector_d;
			break;
		case 0x0a:
			carstate.selector = e_selector_m_p;
			break;
		case 0x0b:
			carstate.selector = e_selector_m_m;
			break;
		case 0x0c:
			carstate.selector = e_selector_s;
			break;
		case 0x0e:
			carstate.selector = e_selector_m;
			break;
		default:
			carstate.selector = e_selector_p;
			break;
	}

	carstate.speed =  ((msg[2] * 256) + msg[1]) / 100;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x5BF (кнопки на руле — SWC)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[0] — код нажатой кнопки
 *   - 0x21 + msg[2] & 0x06 — NEXT / CONT
 *   - 0x1B — NAVI
 *   - 0x19 — MICI (голосовое управление)
 *
 * Вызывает: key_state.key_cb->next(), cont(), navi(), mici()
 *
 * @note Реакция на отпускание кнопки (фронт 1→0).
 */
static void q3_2015_ms_5BF_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		key_state.key_next = STATE_UNDEF;
		key_state.key_navi = STATE_UNDEF;
		key_state.key_cont = STATE_UNDEF;
		key_state.key_mici = STATE_UNDEF;
		return;
	}

	uint8_t key_next = 0;
	uint8_t key_navi = 0;
	uint8_t key_cont = 0;
	uint8_t key_mici = 0;

	if (msg[0] == 0x21) {

		if ((msg[2] & 0x06) == 0x06)
			key_cont = 1;
		else
			key_next = 1;
	}

	if (msg[0] == 0x1B)
		key_navi = 1;

	if (msg[0] == 0x19)
		key_mici = 1;


	if ((key_state.key_cont == 1) && (key_cont == 0) && key_state.key_cb && key_state.key_cb->cont) key_state.key_cb->cont();

	if ((key_state.key_next == 1) && (key_next == 0) && key_state.key_cb && key_state.key_cb->next) key_state.key_cb->next();

	if ((key_state.key_navi == 1) && (key_navi == 0) && key_state.key_cb && key_state.key_cb->navi)	key_state.key_cb->navi();

	if ((key_state.key_mici == 1) && (key_mici == 0) && key_state.key_cb && key_state.key_cb->mici)	key_state.key_cb->mici();

	key_state.key_navi = key_navi;
	key_state.key_cont = key_cont;
	key_state.key_next = key_next;
	key_state.key_mici = key_mici;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x635 (освещённость/яркость)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит: msg[1] — уровень освещённости (0..0x63), масштабируется в 0..100
 * Вызывает: обновление carstate.illum
 */
static void q3_2015_ms_635_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.illum = STATE_UNDEF;
		return;
	}

	carstate.illum = scale(msg[1], 0x00, 0x63, 0, 100);
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3C3 (положение руля)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[1] & 0x7F — угол поворота
 *   - msg[1] & 0x80 — направление (1 = право, 0 = лево)
 *
 * Вызывает: обновление carstate.wheel
 *
 * @note Данные по углу поворота руля (байты 0-1) и моменту (байты 2-3).
 *       Угол масштабируется из 0..0x44 в 0..100.
 */
static void q3_2015_ms_3c3_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.wheel = 0;
		return;
	}

	uint8_t angle = msg[1] & 0x7f;
	uint8_t wheel = scale(angle, 0, 0x44, 0, 100);

	if (msg[1] & 0x80) {
		// поворот направо
		carstate.wheel = wheel;
	} else {
		// поворот налево
		carstate.wheel = -wheel;
	}
}

/**
 * @brief Обработчик CAN-сообщения ID 0x35B (обороты, двигатель, температура ОЖ)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[2:1] — обороты двигателя (RPM, делится на 4)
 *   - msg[3] — температура охлаждающей жидкости (формула: (msg[3] - 64) * 0.75)
 *
 * Вызывает: обновление carstate.taho, carstate.engine, carstate.temp
 *
 * @note Двигатель считается заведённым при оборотах > 500 RPM.
 */
static void q3_2015_ms_35b_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.taho = STATE_UNDEF;
		carstate.engine = STATE_UNDEF;
		return;
	}

	carstate.taho = ((msg[2] * 256) + msg[1]) / 4;

	if (carstate.taho > 500)
		carstate.engine = 1;
	else
		carstate.engine = 0;

	carstate.temp = (msg[3] - 64) * 0.75;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x621 (стояночный тормоз, уровень топлива, омывайка)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] & 0x20 — стояночный тормоз
 *   - msg[0] & 0x04 — низкий уровень омывающей жидкости
 *   - msg[3] & 0x7F — уровень топлива
 *   - msg[3] & 0x80 — низкий уровень топлива
 *
 * Вызывает: обновление carstate.park_break, low_washer, fuel_lvl, low_fuel_lvl
 */
static void q3_2015_ms_621_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.park_break = STATE_UNDEF;
		carstate.low_fuel_lvl = STATE_UNDEF;
		carstate.low_washer = STATE_UNDEF;
		return;
	}

	carstate.fuel_lvl = msg[3] & 0x7F;

	if ((msg[0] & 0x20) == 0x20)
		carstate.park_break = 1;
	else
		carstate.park_break = 0;


	if ((msg[0] & 0x04) == 0x04)
		carstate.low_washer = 1;
	else
		carstate.low_washer = 0;

	if ((msg[3] & 0x80) == 0x80)
		carstate.low_fuel_lvl = 1;
	else
		carstate.low_fuel_lvl = 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3E1 (климат-контроль: скорость вентилятора, AC, обогрев заднего стекла)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - (msg[4] >> 4) & 0x0F — скорость вентилятора (0..15)
 *   - (msg[6] >> 1) & 0x01 — состояние AC
 *   - (msg[0] >> 3) & 0x01 — обогрев заднего стекла
 *
 * Вызывает: обновление car_air_state.fanspeed, ac, rear
 */
static void q3_2015_ms_3E1_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		car_air_state.fanspeed = STATE_UNDEF;
		car_air_state.ac = STATE_UNDEF;
		car_air_state.rear = STATE_UNDEF;
		return;
	}

	car_air_state.fanspeed = (msg[4] >> 4) & 0x0F;
	car_air_state.ac = (msg[6] >> 1) & 0x01;
	car_air_state.rear = (msg[0] >> 3) & 0x01;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x3E3 (обогрев сидений и мощный режим)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - (msg[3] >> 4) & 0x03 — обогрев правого сиденья (0..3)
 *   - (msg[3] >> 1) & 0x03 — обогрев левого сиденья (0..3)
 *   - (msg[3] >> 6) & 0x01 — мощный режим (powerfull)
 *
 * Вызывает: обновление car_air_state.r_seat, l_seat, powerfull
 */
static void q3_2015_ms_3E3_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		car_air_state.l_seat = STATE_UNDEF;
		car_air_state.r_seat = STATE_UNDEF;
		car_air_state.powerfull = STATE_UNDEF;
		return;
	}

	switch ((msg[3] >> 4) & 0x03) {
		case 0x01:
			car_air_state.r_seat = 1;
			break;
		case 0x02:
			car_air_state.r_seat = 2;
			break;
		case 0x03:
			car_air_state.r_seat = 3;
			break;
		case 0x00:
			car_air_state.r_seat = 0;
			break;
		default:
			car_air_state.r_seat = 0;
			break;
	}

	switch ((msg[3] >> 1) & 0x03) {
		case 0x01:
			car_air_state.l_seat = 1;
			break;
		case 0x02:
			car_air_state.l_seat = 2;
			break;
		case 0x03:
			car_air_state.l_seat = 3;
			break;
		case 0x00:
			car_air_state.l_seat = 0;
			break;
		default:
			car_air_state.l_seat = 0;
			break;
	}

	if ((msg[3] >> 6) & 0x01)
		car_air_state.powerfull = 1;
	else
		car_air_state.powerfull = 0;
}

/**
 * @brief Обработчик CAN-сообщения ID 0x6DA (парктроник)
 * @param msg Указатель на буфер сообщения (8 байт)
 * @param desc Указатель на дескриптор сообщения (для таймаута)
 *
 * Парсит:
 *   - msg[0] — состояние парктроника (0x42 = вкл, 0x32 = выкл)
 *   - msg[1] & 0x01 — перед/зад (0 = перед, 1 = зад)
 *   - msg[2..5] — расстояния по 4 зонам (левый, левый-средний, правый-средний, правый)
 *
 * Вызывает: обновление carstate.radar (state, fl, flm, frm, fr, rl, rlm, rrm, rr)
 *
 * @note Данные масштабируются в диапазон 0..99 с инверсией (99 — близко, 0 — далеко).
 */
static void q3_2015_ms_6DA_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.radar.state = e_radar_off;
		return;
	}

	if (msg[0] == 0x42) {

		carstate.radar.state = e_radar_on;

		if (msg[1] & 0x01) {
			// задние датчики
			carstate.radar.rl = 99 - scale(msg[2], 0xf, 0x55, 0, 99);
			carstate.radar.rlm = 99 - scale(msg[3], 0x13, 0x98, 0, 99);
			carstate.radar.rrm = 99 - scale(msg[4], 0x13, 0x98, 0, 99);
			carstate.radar.rr = 99 - scale(msg[5], 0xf, 0x55, 0, 99);
		} else {
			// передние датчики
			carstate.radar.fl = 99 - scale(msg[2], 0xf, 0x55, 0, 99);
			carstate.radar.flm = 99 - scale(msg[3], 0xc, 0x77, 0, 99);
			carstate.radar.frm = 99 - scale(msg[4], 0xc, 0x77, 0, 99);
			carstate.radar.fr = 99 - scale(msg[5], 0xf, 0x55, 0, 99);
		}
	}
	else if (msg[0] == 0x32)
		carstate.radar.state = e_radar_off;
	else
		carstate.radar.state = e_radar_off;
}

/**
 * @brief Таблица дескрипторов CAN-сообщений для Audi Q3 2015 model year
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 */
static struct msg_desc_t q3_2015_ms[] =
{
	{ 0x2c3,  100, 0, 0, q3_2015_ms_2c3_handler },
	{ 0x65F,  200, 0, 0, q3_2015_ms_65F_handler },
	{ 0x65D, 1000, 0, 0, q3_2015_ms_65D_handler },
	{ 0x571,  600, 0, 0, q3_2015_ms_571_handler },
	{ 0x470,   50, 0, 0, q3_2015_ms_470_handler },
	{ 0x359,  100, 0, 0, q3_2015_ms_359_handler },
	{ 0x5BF,  100, 0, 0, q3_2015_ms_5BF_handler },
	{ 0x635,  100, 0, 0, q3_2015_ms_635_handler },
	{ 0x3c3,  100, 0, 0, q3_2015_ms_3c3_handler },
	{ 0x35b,  100, 0, 0, q3_2015_ms_35b_handler },
	{ 0x621,  100, 0, 0, q3_2015_ms_621_handler },
	{ 0x6DA,   50, 0, 0, q3_2015_ms_6DA_handler },
	{ 0x3E1,  500, 0, 0, q3_2015_ms_3E1_handler },
	{ 0x3E3,  500, 0, 0, q3_2015_ms_3E3_handler },
};
