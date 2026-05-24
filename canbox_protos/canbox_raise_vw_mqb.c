/*
 * canbox_raise_vw_mqb.c — Raise VW(MQB) protocol
 *
 * Included into src/canbox.c. All symbols MUST be static.
 * Uses snd_canbox_msg() from core canbox.c.
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
