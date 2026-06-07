#ifndef CANBOX_H
#define CANBOX_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Перечисление поддерживаемых протоколов canbox
 */
enum e_canbox_t
{
	e_cb_raise_vw_pq = 0,		/**< RAISE VW PQ */
	e_cb_raise_vw_mqb,			/**< RAISE VW MQB */
	e_cb_od_bmw_nbt_evo,		/**< Oudi BMW NBT EVO */
	e_cb_hiworld_vw_mqb,		/**< HiWorld VW MQB */
	e_cb_nums,					/**< Количество протоколов */
};

/**
 * @brief Основной цикл обработки canbox
 * @note Отправляет состояние автомобиля в Android-головное устройство
 */
void canbox_process(void);

/**
 * @brief Увеличение громкости
 * @param val Значение шага увеличения
 */
void canbox_inc_volume(uint8_t val);

/**
 * @brief Уменьшение громкости
 * @param val Значение шага уменьшения
 */
void canbox_dec_volume(uint8_t val);

/**
 * @brief Предыдущий трек/станция
 */
void canbox_prev(void);

/**
 * @brief Следующий трек/станция
 */
void canbox_next(void);

/**
 * @brief Нажатие кнопки MODE
 */
void canbox_mode(void);

/**
 * @brief Нажатие кнопки CONT/PHONE
 */
void canbox_cont(void);

/**
 * @brief Нажатие кнопки MICI/VOICE
 */
void canbox_mici(void);

/**
 * @brief Обработка принятого байта по USART
 * @param ch Принятый байт
 */
void canbox_rx_process(uint8_t ch);

/**
 * @brief Обработка парковочных датчиков
 * @note Отправляет данные радара в Android-головное устройство
 */
void canbox_park_process(void);

#ifdef __cplusplus
}
#endif

#endif
