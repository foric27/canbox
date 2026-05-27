/*
 * canbox_od_bmw_nbt_evo.c — Raise Oudi BMW(NBT/EVO) protocol
 *
 * Fully self-contained. All symbols static except public API.
 */

#include <string.h>

static float scale(float value, float in_min, float in_max, float out_min, float out_max)
{
	return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}

static uint8_t canbox_checksum(uint8_t * buf, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++)
		sum += buf[i];
	sum = sum ^ 0xff;
	return sum;
}

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

/*
 * SWC key callbacks — empty stubs (OD BMW does not use SWC via canbox)
 */
void canbox_inc_volume(uint8_t val) { (void)val; }
void canbox_dec_volume(uint8_t val) { (void)val; }
void canbox_prev(void) { }
void canbox_next(void) { }
void canbox_mode(void) { }
void canbox_cont(void) { }
void canbox_mici(void) { }

/*
 * RX state machine — empty stub (OD BMW does not use Raise RX)
 */
void canbox_rx_process(uint8_t ch) { (void)ch; }

/*
 * Entry points
 */
static void canbox_od_bmw_nbt_evo_process(void)
{
	canbox_raise_vw_wheel_process(0x29, -5400, 5400);
	canbox_raise_vw_mqb_door_process();
}

static void canbox_od_bmw_nbt_evo_park_process(void)
{
	uint8_t fmax[4] = { 10, 10, 10, 10 };
	uint8_t rmax[4] = { 10, 10, 10, 10 };
	canbox_raise_vw_radar_process(fmax, rmax);
}
