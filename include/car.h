#ifndef CAR_H
#define CAR_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Перечисление поддерживаемых автомобилей
 */
enum e_car_t
{
	e_car_anymsg = 0,		/**< Режим прослушивания всех сообщений */
	e_car_lr2_2007my,		/**< Land Rover Freelander 2 2007 */
	e_car_lr2_2013my,		/**< Land Rover Freelander 2 2013 */
	e_car_xc90_2007my,		/**< Volvo XC90 2007 */
	e_car_skoda_fabia,		/**< Škoda Fabia */
	e_car_q3_2015,			/**< Audi Q3 2015 */
	e_car_toyota_premio_26x,	/**< Toyota Premio 260/261 кузов */
#ifdef QCAR
	e_car_qcar,			/**< Qt-эмулятор */
#endif
	e_car_nums,			/**< Количество поддерживаемых автомобилей */
};

/**
 * @brief Перечисление положений селектора АКПП
 */
enum e_selector_t
{
	e_selector_p = 0,	/**< Паркинг (P) */
	e_selector_r,		/**< Задний ход (R) */
	e_selector_n,		/**< Нейтраль (N) */
	e_selector_d,		/**< Драйв (D) */
	e_selector_s,		/**< Спорт (S) */
	e_selector_m,		/**< Ручной режим (M) */
	e_selector_m_p,		/**< Ручной режим: плюс */
	e_selector_m_m,		/**< Ручной режим: минус */
};

/**
 * @brief Перечисление состояний парковочных датчиков
 */
enum e_radar_t
{
	e_radar_undef = 0,	/**< Состояние не определено */
	e_radar_off,		/**< Датчики выключены */
	e_radar_on,		/**< Датчики включены (все) */
	e_radar_on_front,	/**< Включены только передние */
	e_radar_on_rear,	/**< Включены только задние */
};

/**
 * @brief Структура данных парковочных датчиков
 */
typedef struct radar_t
{
	uint8_t state;		/**< Состояние системы (см. e_radar_t) */
	uint8_t fl;		/**< Передний левый датчик */
	uint8_t flm;		/**< Передний левый средний датчик */
	uint8_t frm;		/**< Передний правый средний датчик */
	uint8_t fr;		/**< Передний правый датчик */
	uint8_t rl;		/**< Задний левый датчик */
	uint8_t rlm;		/**< Задний левый средний датчик */
	uint8_t rrm;		/**< Задний правый средний датчик */
	uint8_t rr;		/**< Задний правый датчик */
} radar_t;

/**
 * @brief Структура обратных вызовов кнопок на руле (SWC)
 */
typedef struct key_cb_t
{
	void (*mode)(void);			/**< Нажатие кнопки MODE */
	void (*inc_volume)(uint8_t val);	/**< Увеличение громкости */
	void (*dec_volume)(uint8_t val);	/**< Уменьшение громкости */
	void (*prev)(void);			/**< Предыдущий трек/станция */
	void (*next)(void);			/**< Следующий трек/станция */
	void (*navi)(void);			/**< Нажатие кнопки NAVI */
	void (*cont)(void);			/**< Нажатие кнопки CONT/PHONE */
	void (*mici)(void);			/**< Нажатие кнопки MICI/VOICE */
} key_cb_t;

/**
 * @brief Инициализация подсистемы автомобиля
 * @param cb Указатель на структуру обратных вызовов кнопок
 */
void car_init(struct key_cb_t * cb);

/**
 * @brief Обработка CAN-сообщений автомобиля
 * @param ms Текущее время в миллисекундах
 */
void car_process(uint8_t);

/**
 * @brief Получение текущего типа автомобиля
 * @return Значение из перечисления e_car_t
 */
enum e_car_t car_get_car(void);

/**
 * @brief Получение состояния ACC
 * @return 0 — выключено, 1 — включено
 */
uint8_t car_get_acc(void);

/**
 * @brief Получение состояния зажигания (IGN)
 * @return 0 — выключено, 1 — включено
 */
uint8_t car_get_ign(void);

/**
 * @brief Получение состояния двигателя
 * @return 0 — заглушен, 1 — работает
 */
uint8_t car_get_engine(void);

/**
 * @brief Получение данных парковочных датчиков
 * @param radar Указатель на структуру для сохранения данных
 */
void car_get_radar(struct radar_t * radar);

/**
 * @brief Получение угла поворота рулевого колеса
 * @param wheel Указатель для сохранения значения
 * @return 0 при успехе
 */
uint8_t car_get_wheel(int8_t * wheel);

/**
 * @brief Получение состояния габаритных огней
 * @return 0 — выключены, 1 — включены
 */
uint8_t car_get_park_lights(void);

/**
 * @brief Получение состояния ближнего света
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_near_lights(void);

/**
 * @brief Получение уровня освещённости
 * @return Значение освещённости (0..255)
 */
uint8_t car_get_illum(void);

/**
 * @brief Получение VIN-номера автомобиля
 * @param buf Указатель на буфер для сохранения VIN
 * @return Длина VIN или 0 при отсутствии
 */
uint8_t car_get_vin(uint8_t * buf);

/**
 * @brief Получение положения селектора АКПП
 * @return Значение из перечисления e_selector_t
 */
enum e_selector_t car_get_selector(void);

/**
 * @brief Получение скорости автомобиля
 * @return Скорость в км/ч
 */
uint16_t car_get_speed(void);

/**
 * @brief Получение оборотов двигателя (тахометр)
 * @return Обороты в минуту
 */
uint16_t car_get_taho(void);

/**
 * @brief Получение состояния передней левой двери
 * @return 0 — закрыта, 1 — открыта
 */
uint8_t car_get_door_fl(void);

/**
 * @brief Получение состояния передней правой двери
 * @return 0 — закрыта, 1 — открыта
 */
uint8_t car_get_door_fr(void);

/**
 * @brief Получение состояния задней левой двери
 * @return 0 — закрыта, 1 — открыта
 */
uint8_t car_get_door_rl(void);

/**
 * @brief Получение состояния задней правой двери
 * @return 0 — закрыта, 1 — открыта
 */
uint8_t car_get_door_rr(void);

/**
 * @brief Получение состояния капота
 * @return 0 — закрыт, 1 — открыт
 */
uint8_t car_get_bonnet(void);

/**
 * @brief Получение состояния багажника
 * @return 0 — закрыт, 1 — открыт
 */
uint8_t car_get_tailgate(void);

/**
 * @brief Получение состояния стояночного тормоза
 * @return 0 — отпущен, 1 — затянут
 */
uint8_t car_get_park_break(void);

/**
 * @brief Получение состояния низкого уровня омывающей жидкости
 * @return 0 — норма, 1 — низкий уровень
 */
uint8_t car_get_low_washer(void);

/**
 * @brief Получение состояния ремня водителя
 * @return 0 — пристёгнут, 1 — не пристёгнут
 */
uint8_t car_get_ds_belt(void);

/**
 * @brief Получение значения одометра
 * @return Пробег в километрах
 */
uint32_t car_get_odometer(void);

/**
 * @brief Получение напряжения бортовой сети
 * @return Напряжение (мВ)
 */
uint32_t car_get_voltage(void);

/**
 * @brief Получение наружной температуры
 * @return Температура (в десятых градуса Цельсия)
 */
uint32_t car_get_temp(void);

/**
 * @brief Получение уровня топлива
 * @return Уровень топлива (0..255)
 */
uint8_t car_get_fuel_level(void);

/**
 * @brief Получение состояния низкого напряжения
 * @return 0 — норма, 1 — низкое напряжение
 */
uint8_t car_get_low_voltage(void);

/**
 * @brief Получение состояния низкого уровня топлива
 * @return 0 — норма, 1 — низкий уровень
 */
uint8_t car_get_low_fuel_level(void);

/**
 * @brief Получение состояния кондиционера (AC)
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_ac(void);

/**
 * @brief Получение состояния режима AC MAX
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_ac_max(void);

/**
 * @brief Получение состояния рециркуляции воздуха
 * @return 0 — выключена, 1 — включена
 */
uint8_t car_get_air_recycling(void);

/**
 * @brief Получение состояния максимальной рециркуляции
 * @return 0 — выключена, 1 — включена
 */
uint8_t car_get_air_recycling_max(void);

/**
 * @brief Получение состояния минимальной рециркуляции
 * @return 0 — выключена, 1 — включена
 */
uint8_t car_get_air_recycling_min(void);

/**
 * @brief Получение состояния режима DUAL
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_dual(void);

/**
 * @brief Получение состояния обогрева заднего стекла
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_rear(void);

/**
 * @brief Получение состояния блокировки заднего обогрева
 * @return 0 — разблокирован, 1 — заблокирован
 */
uint8_t car_get_air_rear_lock(void);

/**
 * @brief Получение состояния AQS (качества воздуха)
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_aqs(void);

/**
 * @brief Получение состояния обдува ветрового стекла
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_wind(void);

/**
 * @brief Получение состояния среднего обдува
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_middle(void);

/**
 * @brief Получение состояния обдува в пол
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_floor(void);

/**
 * @brief Получение состояния режима POWERFULL
 * @return 0 — выключен, 1 — включен
 */
uint8_t car_get_air_powerfull(void);

/**
 * @brief Получение скорости вентилятора климат-контроля
 * @return Скорость (0..макс)
 */
uint8_t car_get_air_fanspeed(void);

/**
 * @brief Получение температуры левой зоны
 * @return Температура (в десятых градуса)
 */
uint8_t car_get_air_l_temp(void);

/**
 * @brief Получение температуры правой зоны
 * @return Температура (в десятых градуса)
 */
uint8_t car_get_air_r_temp(void);

/**
 * @brief Получение уровня обогрева левого сиденья
 * @return Уровень обогрева (0..макс)
 */
uint8_t car_get_air_l_seat(void);

/**
 * @brief Получение уровня обогрева правого сиденья
 * @return Уровень обогрева (0..макс)
 */
uint8_t car_get_air_r_seat(void);

#ifdef __cplusplus
}
#endif

#endif
