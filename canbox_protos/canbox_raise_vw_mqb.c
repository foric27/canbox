/*
 * canbox_raise_vw_mqb.c — Raise VW(MQB) protocol
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
 * SWC key callbacks
 */
void canbox_inc_volume(uint8_t val)
{
	(void)val;
	uint8_t buf[] = { 0x01, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

void canbox_dec_volume(uint8_t val)
{
	(void)val;
	uint8_t buf[] = { 0x02, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

void canbox_prev(void)
{
	uint8_t buf[] = { 0x04, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

void canbox_next(void)
{
	uint8_t buf[] = { 0x03, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

void canbox_mode(void)
{
	uint8_t buf[] = { 0x0a, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

void canbox_cont(void)
{
	uint8_t buf[] = { 0x09, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

void canbox_mici(void)
{
	uint8_t buf[] = { 0x0c, 0x01 };
	snd_canbox_msg(0x20, buf, sizeof(buf));
	buf[1] = 0x00;
	snd_canbox_msg(0x20, buf, sizeof(buf));
}

/*
 * RX state machine
 */
static void canbox_raise_cmd_process(uint8_t * cmdbuf, uint8_t len)
{
	(void)len;
	uint8_t cmd = cmdbuf[1];

	if (cmd == 0x81) {
	}
	else if (cmd == 0x90) {
	}
	else if (cmd == 0xa0) {
	}
	else if (cmd == 0xa6) {
	}
}

enum rx_state
{
	RX_WAIT_START,
	RX_LEN,
	RX_CMD,
	RX_DATA,
	RX_CRC
};

#define RX_BUFFER_SIZE 32
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_idx = 0;
static uint8_t rx_state = RX_WAIT_START;

static void canbox_raise_rx_process(uint8_t ch)
{
	switch (rx_state) {
		case RX_WAIT_START:
			if (ch != 0x2e)
				break;
			rx_idx = 0;
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_CMD;
			break;
		case RX_CMD:
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_LEN;
			break;
		case RX_LEN:
			rx_buffer[rx_idx++] = ch;
			rx_state = ch ? RX_DATA : RX_CRC;
			break;
		case RX_DATA:
			rx_buffer[rx_idx++] = ch;
			{
				uint8_t len = rx_buffer[2];
				rx_state = ((rx_idx - 2) > len) ? RX_CRC : RX_DATA;
			}
			break;
		case RX_CRC:
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_WAIT_START;
			{
				uint8_t ack = 0xff;
				hw_usart_write(hw_usart_get(), (uint8_t *)&ack, 1);
				canbox_raise_cmd_process(rx_buffer, rx_idx);
			}
			break;
	}
	if (rx_idx > RX_BUFFER_SIZE)
		rx_state = RX_WAIT_START;
}

void canbox_rx_process(uint8_t ch)
{
	canbox_raise_rx_process(ch);
}

/*
 * Entry points
 */
static void canbox_raise_vw_mqb_process(void)
{
	canbox_raise_vw_wheel_process(0x29, -19980, 19980);
	canbox_raise_vw_mqb_door_process();
}

static void canbox_raise_vw_mqb_park_process(void)
{
	uint8_t fmax[4] = { 60, 120, 120, 60 };
	uint8_t rmax[4] = { 60, 165, 165, 60 };
	canbox_raise_vw_radar_process(fmax, rmax);
}
