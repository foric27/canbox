/*
 * canbox_hiworld_vw_mqb.c — HiWorld VW(MQB) protocol
 *
 * Fully self-contained. All symbols static except public API.
 */

#include <string.h>

static float scale(float value, float in_min, float in_max, float out_min, float out_max)
{
	return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}

static uint8_t canbox_hiworld_checksum(uint8_t * buf, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++)
		sum += buf[i];
	sum = sum - 1;
	return sum;
}

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

/*
 * SWC key callbacks — empty stubs (HiWorld does not use SWC via canbox)
 */
void canbox_inc_volume(uint8_t val) { (void)val; }
void canbox_dec_volume(uint8_t val) { (void)val; }
void canbox_prev(void) { }
void canbox_next(void) { }
void canbox_mode(void) { }
void canbox_cont(void) { }
void canbox_mici(void) { }

/*
 * RX state machine — empty stub (HiWorld does not use Raise RX)
 */
void canbox_rx_process(uint8_t ch) { (void)ch; }

/*
 * Entry points
 */
static void canbox_hiworld_vw_mqb_process(void)
{
	canbox_hiworld_vw_mqb_wheel_process();
	canbox_hiworld_vw_mqb_door_process();
}

static void canbox_hiworld_vw_mqb_park_process(void)
{
	canbox_hiworld_vw_mqb_radar_process();
}
