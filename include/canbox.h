#ifndef CANBOX_H
#define CANBOX_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum e_canbox_t
{
	e_cb_raise_vw_pq = 0,
	e_cb_raise_vw_mqb,
	e_cb_od_bmw_nbt_evo,
	e_cb_hiworld_vw_mqb,
	e_cb_nums,
};

void canbox_process(void);

void canbox_inc_volume(uint8_t val);
void canbox_dec_volume(uint8_t val);
void canbox_prev(void);
void canbox_next(void);
void canbox_mode(void);
void canbox_cont(void);
void canbox_mici(void);

void canbox_rx_process(uint8_t ch);
void canbox_park_process(void);

#ifdef __cplusplus
}
#endif

#endif
