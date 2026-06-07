#include "config.h"
#include "canbox.h"
#include "car.h"
#include "hw_usart.h"

#if defined(USE_RAISE_VW_PQ)
#include "canbox_protos/canbox_raise_vw_pq.c"
#elif defined(USE_RAISE_VW_MQB)
#include "canbox_protos/canbox_raise_vw_mqb.c"
#elif defined(USE_OD_BMW_NBT_EVO)
#include "canbox_protos/canbox_od_bmw_nbt_evo.c"
#elif defined(USE_HIWORLD_VW_MQB)
#include "canbox_protos/canbox_hiworld_vw_mqb.c"
#endif

/**
 * @brief Диспетчер основного цикла протокола canbox
 * @note Вызывает соответствующую функцию обработки в зависимости от
 *       выбранного протокола (Raise VW PQ/MQB, Oudi BMW, HiWorld).
 *       Отправляет состояние автомобиля на Android-устройство по USART.
 */
void canbox_process(void)
{
#if defined(USE_RAISE_VW_PQ)
	canbox_raise_vw_pq_process();
#elif defined(USE_RAISE_VW_MQB)
	canbox_raise_vw_mqb_process();
#elif defined(USE_OD_BMW_NBT_EVO)
	canbox_od_bmw_nbt_evo_process();
#elif defined(USE_HIWORLD_VW_MQB)
	canbox_hiworld_vw_mqb_process();
#endif
}

/**
 * @brief Диспетчер парковочных данных протокола canbox
 * @note Вызывает соответствующую функцию отправки парковочных данных
 *       (радар, датчики расстояния) в зависимости от выбранного протокола.
 *       Выполняется с периодичностью 100 мс.
 */
void canbox_park_process(void)
{
#if defined(USE_RAISE_VW_PQ)
	canbox_raise_vw_pq_park_process();
#elif defined(USE_RAISE_VW_MQB)
	canbox_raise_vw_mqb_park_process();
#elif defined(USE_OD_BMW_NBT_EVO)
	canbox_od_bmw_nbt_evo_park_process();
#elif defined(USE_HIWORLD_VW_MQB)
	canbox_hiworld_vw_mqb_park_process();
#endif
}
