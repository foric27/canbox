/*
 * canbox_raise_vw_pq.c — Raise VW(PQ) protocol
 *
 * Included into src/canbox.c. All symbols MUST be static.
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

	if ((conf_get_car() == e_car_skoda_fabia) || (conf_get_car() == e_car_q3_2015) || (conf_get_car() == e_car_toyota_premio_26x)) {

		if (ds_belt)
			state |= 0x80;
		if (low_washer)
			state |= 0x40;
		if (park_break)
			state |= 0x20;
	}
	else if (bonnet)
		state |= 0x20;

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

/*
 * Entry points — called from canbox.c dispatcher
 */
static void canbox_raise_vw_pq_process(void)
{
	canbox_raise_vw_wheel_process(0x26, -540, 540);
	canbox_raise_vw_door_process();
	canbox_raise_vw_ac_process();
	canbox_raise_vw_vehicle_info();
}

static void canbox_raise_vw_pq_park_process(void)
{
	uint8_t fmax[4] = { 10, 10, 10, 10 };
	uint8_t rmax[4] = { 10, 10, 10, 10 };
	canbox_raise_vw_radar_process(fmax, rmax);
}
