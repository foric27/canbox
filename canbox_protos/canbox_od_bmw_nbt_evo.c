/*
 * canbox_od_bmw_nbt_evo.c — Raise Oudi BMW(NBT/EVO) protocol
 *
 * Included into src/canbox.c. All symbols MUST be static.
 * Uses snd_canbox_msg() from core canbox.c.
 * Oudi reuses the same Raise functions as VW(MQB) with different constants.
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
