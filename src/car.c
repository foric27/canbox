#include <string.h>

#include "car.h"
#include "hw_can.h"
#include "config.h"

#if defined(CONFIG_CAR_ANYMSG)
	#define USE_ANYMSG
	#define CONFIG_CAR e_car_anymsg
#elif defined(CONFIG_CAR_LR2_2007MY)
	#define USE_LR2_2007MY
	#define CONFIG_CAR e_car_lr2_2007my
#elif defined(CONFIG_CAR_LR2_2013MY)
	#define USE_LR2_2013MY
	#define CONFIG_CAR e_car_lr2_2013my
#elif defined(CONFIG_CAR_XC90_2007MY)
	#define USE_XC90_2007MY
	#define CONFIG_CAR e_car_xc90_2007my
#elif defined(CONFIG_CAR_SKODA_FABIA)
	#define USE_SKODA_FABIA
	#define CONFIG_CAR e_car_skoda_fabia
#elif defined(CONFIG_CAR_Q3_2015)
	#define USE_Q3_2015
	#define CONFIG_CAR e_car_q3_2015
#elif defined(CONFIG_CAR_TOYOTA_PREMIO_26X)
	#define USE_TOYOTA_PREMIO_26X
	#define CONFIG_CAR e_car_toyota_premio_26x
#else
	#error "No CONFIG_CAR_* defined in config.h"
#endif

/** @brief Значение "данные отсутствуют" для полей состояния (0xFF) */
#define STATE_UNDEF 0xff

/**
 * @brief Структура полного состояния автомобиля
 * @note Содержит все параметры, получаемые из CAN-шины:
 *       двигатель, скорость, двери, радар, климат и т.д.
 */
typedef struct car_state_t
{
	uint8_t car;            /**< Тип автомобиля (значение e_car_t) */

	uint8_t vin[18];        /**< VIN-код автомобиля (17 символов + нуль) */

	uint8_t acc;            /**< Состояние ACC (аксессуары) */
	uint8_t ign;            /**< Состояние зажигания (ignition) */
	uint8_t engine;         /**< Состояние работы двигателя */
	uint16_t taho;          /**< Обороты двигателя (RPM) */
	uint16_t speed;         /**< Скорость автомобиля */
	uint8_t illum;          /**< Уровень внешней освещенности */
	uint8_t selector;       /**< Положение селектора АКПП (e_selector_t) */

	radar_t radar;          /**< Данные парковочного радара */
	int8_t wheel;           /**< Угол поворота руля (со знаком) */

	uint8_t park_lights;    /**< Состояние парковочных огней */
	uint8_t near_lights;    /**< Состояние ближнего света */
	uint8_t park_break;     /**< Состояние стояночного тормоза */

	uint8_t fl_door;        /**< Передняя левая дверь */
	uint8_t fr_door;        /**< Передняя правая дверь */
	uint8_t rl_door;        /**< Задняя левая дверь */
	uint8_t rr_door;        /**< Задняя правая дверь */
	uint8_t tailgate;       /**< Багажник / задняя дверь */
	uint8_t bonnet;         /**< Капот */

	uint8_t low_washer;     /**< Низкий уровень омывающей жидкости */
	uint8_t ds_belt;        /**< Ремень безопасности водителя */

	uint32_t odometer;      /**< Пробег (одометр) */
	uint32_t voltage;       /**< Напряжение бортовой сети */
	uint32_t temp;          /**< Температура (наружная/двигателя) */
	uint8_t fuel_lvl;       /**< Уровень топлива */
	uint8_t low_voltage;    /**< Флаг низкого напряжения */
	uint8_t low_fuel_lvl;   /**< Флаг низкого уровня топлива */
} car_state_t;

/** @brief Глобальная переменная состояния автомобиля */
static car_state_t carstate =
{
	.vin = {STATE_UNDEF},
	.acc = STATE_UNDEF,
	.ign = STATE_UNDEF,
	.engine = STATE_UNDEF,
	.taho = 0,
	.speed = 0,
	.illum = STATE_UNDEF,
	.selector = STATE_UNDEF,
	.radar = {.state = STATE_UNDEF},
	.wheel = 0,
	.park_lights = STATE_UNDEF,
	.near_lights = STATE_UNDEF,
	.park_break = STATE_UNDEF,
	.fl_door = STATE_UNDEF,
	.fr_door = STATE_UNDEF,
	.rl_door = STATE_UNDEF,
	.rr_door = STATE_UNDEF,
	.tailgate = STATE_UNDEF,
	.bonnet = STATE_UNDEF,
	.low_washer = STATE_UNDEF,
	.ds_belt = STATE_UNDEF,
	.odometer = 0,
	.voltage = 0,
	.temp = 0,
	.fuel_lvl = 0,
	.low_voltage = STATE_UNDEF,
	.low_fuel_lvl = STATE_UNDEF,
};

/**
 * @brief Структура состояния климат-контроля
 * @note Содержит параметры кондиционера, обогрева, вентиляции
 *       и температурных режимов.
 */
typedef struct car_air_state_t
{
	uint8_t ac;             /**< Режим кондиционера */
	uint8_t ac_max;         /**< Режим MAX A/C */
	uint8_t recycling;      /**< Рециркуляция воздуха */
	uint8_t recycling_max;  /**< Максимальная рециркуляция */
	uint8_t recycling_min;  /**< Минимальная рециркуляция */
	uint8_t dual;           /**< Режим DUAL (двухзонный) */
	uint8_t rear;           /**< Обогрев заднего стекла */
	uint8_t rear_lock;      /**< Блокировка заднего управления */
	uint8_t aqs;            /**< Система качества воздуха AQS */

	uint8_t wind;           /**< Режим обдува ветрового стекла */
	uint8_t middle;         /**< Режим обдува в среднюю зону */
	uint8_t floor;          /**< Режим обдува в ноги */

	uint8_t powerfull;      /**< Режим POWERFULL (быстрое охлаждение/обогрев) */
	uint8_t fanspeed;       /**< Скорость вентилятора */
	uint8_t l_temp;         /**< Температура левой зоны */
	uint8_t r_temp;         /**< Температура правой зоны */

	uint8_t l_seat;         /**< Обогрев/вентиляция левого сиденья */
	uint8_t r_seat;         /**< Обогрев/вентиляция правого сиденья */
} car_air_state_t;

/** @brief Глобальная переменная состояния климат-контроля */
static car_air_state_t car_air_state =
{
	.ac = STATE_UNDEF,
	.ac_max = STATE_UNDEF,
	.recycling = STATE_UNDEF,
	.recycling_max = STATE_UNDEF,
	.recycling_min = STATE_UNDEF,
	.dual = STATE_UNDEF,
	.rear = STATE_UNDEF,
	.rear_lock = STATE_UNDEF,
	.aqs = STATE_UNDEF,
	.wind = STATE_UNDEF,
	.middle = STATE_UNDEF,
	.floor = STATE_UNDEF,
	.powerfull = STATE_UNDEF,
	.fanspeed = STATE_UNDEF,
	.l_temp = STATE_UNDEF,
	.r_temp = STATE_UNDEF,
	.l_seat = STATE_UNDEF,
	.r_seat = STATE_UNDEF,
};

/**
 * @brief Структура состояния кнопок рулевого управления (SWC)
 * @note Хранит флаги нажатия кнопок и указатель на таблицу
 *       обратных вызовов для обработки событий.
 */
typedef struct key_state_t
{
	uint8_t key_volume;     /**< Флаг кнопки громкости */
	uint8_t key_prev;       /**< Флаг кнопки "предыдущий трек" */
	uint8_t key_next;       /**< Флаг кнопки "следующий трек" */
	uint8_t key_mode;       /**< Флаг кнопки режима */
	uint8_t key_cont;       /**< Флаг кнопки продолжения/ play-pause */
	uint8_t key_navi;       /**< Флаг кнопки навигации */
	uint8_t key_mici;       /**< Флаг кнопки микрофона/голосового управления */

	struct key_cb_t * key_cb;   /**< Указатель на таблицу callback-функций */
} key_state_t;

/** @brief Глобальная переменная состояния кнопок рулевого управления */
static struct key_state_t key_state =
{
	.key_volume = STATE_UNDEF,
	.key_prev = STATE_UNDEF,
	.key_next = STATE_UNDEF,
	.key_mode = STATE_UNDEF,
	.key_cont = STATE_UNDEF,
	.key_navi = STATE_UNDEF,
	.key_mici = STATE_UNDEF,
	.key_cb = 0,
};

/**
 * @brief Описатель CAN-сообщения для диспетчера
 * @note Связывает CAN ID с обработчиком и отслеживает таймаут.
 *       При отсутствии сообщений в течение 2*period tick
 *       выставляется флаг таймаута.
 */
struct msg_desc_t;
typedef struct msg_desc_t
{
	uint32_t id;            /**< Идентификатор CAN-сообщения (0 = wildcard) */
	uint16_t period;        /**< Ожидаемый период поступления сообщения (мс) */
	uint16_t tick;          /**< Счетчик времени с момента последнего приема */
	uint32_t num;           /**< Номер последнего обработанного пакета */
	void (*in_handler)(const uint8_t * msg, struct msg_desc_t * desc);   /**< Обработчик сообщения */
} msg_desc_t;

/**
 * @brief Линейная интерполяция значения из одного диапазона в другой
 * @param value Исходное значение
 * @param in_min  Минимум исходного диапазона
 * @param in_max  Максимум исходного диапазона
 * @param out_min Минимум целевого диапазона
 * @param out_max Максимум целевого диапазона
 * @return Масштабированное значение
 */
static float scale(float value, float in_min, float in_max, float out_min, float out_max)
{
	return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Проверка таймаута CAN-сообщения
 * @param desc Указатель на описатель сообщения
 * @return 1 — таймаут превышен (нет данных >= 2*period), 0 — данные актуальны
 * @note При превышении таймаута счетчик tick фиксируется на максимуме
 */
uint8_t is_timeout(struct msg_desc_t * desc)
{
	if (desc->tick >= (2 * desc->period)) {
		desc->tick = 2 * desc->period;
		return 1;
	}
	return 0;
}

#include "cars/anymsg.c"

#ifdef USE_LR2_2007MY
#include "cars/lr2_2007my.c"
#endif

#ifdef USE_LR2_2013MY
#include "cars/lr2_2013my.c"
#endif

#ifdef USE_XC90_2007MY
#include "cars/xc90_2007my.c"
#endif

#ifdef USE_SKODA_FABIA
#include "cars/skoda_fabia.c"
#endif

#ifdef USE_Q3_2015
#include "cars/q3_2015.c"
#endif

#ifdef USE_TOYOTA_PREMIO_26X
#include "cars/toyota_premio_26x.c"
#endif

#ifdef QCAR
#include "qt/qcar.c"
#endif

/**
 * @brief Получить тип текущего автомобиля
 * @return Значение перечисления e_car_t
 */
enum e_car_t car_get_car(void)
{
	return carstate.car;
}

/**
 * @brief Диспетчер входящих CAN-сообщений
 * @param can     Указатель на CAN-интерфейс
 * @param ticks   Количество миллисекунд с момента последнего вызова
 * @param msg_desc    Массив описателей ожидаемых сообщений
 * @param desc_num    Количество элементов в массиве msg_desc
 * @note Для каждого принятого CAN-пакета ищет соответствующий описатель
 *       по ID и вызывает in_handler. Отслеживает таймауты через поле tick.
 *       Описатель с id=0 обрабатывает все сообщения (wildcard).
 */
static void in_process(struct can_t * can, uint8_t ticks, struct msg_desc_t * msg_desc, uint8_t desc_num)
{
	uint8_t msgs_num = hw_can_get_msg_nums(can);
	uint32_t all_packs = 0;
	for (uint8_t i = 0; i < msgs_num; i++) {
		struct msg_can_t msg;
		if (!hw_can_get_msg(can, &msg, i))
			continue;

		all_packs += msg.num;

		for (uint32_t j = 0; j < desc_num; j++) {
			struct msg_desc_t * desc = &msg_desc[j];

			if (0 == desc->id) {
				if (desc->in_handler) {
					if (i == (msgs_num - 1)) {
						if (all_packs == desc->num)
							desc->tick += ticks;
						else
							desc->tick = 0;

						desc->num = all_packs;
						desc->in_handler(msg.data, desc);
					}
				}
			}
			else if (msg.id == desc->id) {
				if (desc->in_handler) {
					if (msg.num == desc->num)
						desc->tick += ticks;
					else
						desc->tick = 0;

					desc->num = msg.num;
					desc->in_handler(msg.data, desc);
				}
				break;
			}
		}
	}
}

/**
 * @brief Инициализация подсистемы автомобиля
 * @param cb Указатель на структуру обратных вызовов кнопок SWC
 * @note Выполняет:
 *       1. Установку типа автомобиля из конфигурации
 *       2. Сброс всех полей состояния в STATE_UNDEF / 0
 *       3. Привязку callback-таблицы кнопок
 *       4. Настройку скорости CAN-шины (125/100/500 кбит/с в зависимости от авто)
 */
void car_init(struct key_cb_t * cb)
{
	carstate.car = CONFIG_CAR;

	carstate.vin[0] = STATE_UNDEF,
	carstate.acc = STATE_UNDEF,
	carstate.ign = STATE_UNDEF,
	carstate.engine = STATE_UNDEF,
	carstate.taho = 0,
	carstate.speed = 0,
	carstate.illum = STATE_UNDEF,
	carstate.selector = STATE_UNDEF,
	carstate.radar.state = STATE_UNDEF,
	carstate.wheel = 0,

	carstate.park_lights = STATE_UNDEF,
	carstate.near_lights = STATE_UNDEF,
	carstate.park_break = STATE_UNDEF,
	carstate.fl_door = STATE_UNDEF,
	carstate.fr_door = STATE_UNDEF,
	carstate.rl_door = STATE_UNDEF,
	carstate.rr_door = STATE_UNDEF,
	carstate.tailgate = STATE_UNDEF,
	carstate.bonnet = STATE_UNDEF,
	carstate.low_washer = STATE_UNDEF,
	carstate.ds_belt = STATE_UNDEF,
	carstate.odometer = 0,
	carstate.voltage = 0,
	carstate.temp = 0,
	carstate.fuel_lvl = 0,
	carstate.low_voltage = STATE_UNDEF,
	carstate.low_fuel_lvl = STATE_UNDEF,

	key_state.key_cb = cb;

	e_speed_t speed = e_speed_125;
#if defined(CONFIG_CAR_SKODA_FABIA) || defined(CONFIG_CAR_Q3_2015)
	speed = e_speed_100;
#elif defined(CONFIG_CAR_TOYOTA_PREMIO_26X)
	speed = e_speed_500;
#endif
	hw_can_set_speed(hw_can_get_mscan(), speed);
}

/**
 * @brief Основной цикл обработки CAN-сообщений автомобиля
 * @param ticks Количество миллисекунд с момента последнего вызова
 * @note Вызывает in_process с таблицей описателей, соответствующей
 *       выбранному автомобилю. Для Qt-эмулятора вызывает qcar_process().
 */
void car_process(uint8_t ticks)
{
	struct can_t * can = hw_can_get_mscan();

#if defined(CONFIG_CAR_ANYMSG)
	in_process(can, ticks, anymsg_desc, sizeof(anymsg_desc)/sizeof(anymsg_desc[0]));
#elif defined(CONFIG_CAR_LR2_2007MY)
	in_process(can, ticks, lr2_2007my_ms, sizeof(lr2_2007my_ms)/sizeof(lr2_2007my_ms[0]));
#elif defined(CONFIG_CAR_LR2_2013MY)
	in_process(can, ticks, lr2_2013my_ms, sizeof(lr2_2013my_ms)/sizeof(lr2_2013my_ms[0]));
#elif defined(CONFIG_CAR_XC90_2007MY)
	in_process(can, ticks, xc90_2007my_ms, sizeof(xc90_2007my_ms)/sizeof(xc90_2007my_ms[0]));
#elif defined(CONFIG_CAR_SKODA_FABIA)
	in_process(can, ticks, skoda_fabia_ms, sizeof(skoda_fabia_ms)/sizeof(skoda_fabia_ms[0]));
#elif defined(CONFIG_CAR_Q3_2015)
	in_process(can, ticks, q3_2015_ms, sizeof(q3_2015_ms)/sizeof(q3_2015_ms[0]));
#elif defined(CONFIG_CAR_TOYOTA_PREMIO_26X)
	in_process(can, ticks, toyota_premio_26x_ms, sizeof(toyota_premio_26x_ms)/sizeof(toyota_premio_26x_ms[0]));
#elif defined(QCAR) && defined(CONFIG_CAR_QCAR)
	qcar_process();
#endif
}

/* ============================================================
 * Геттеры основного состояния автомобиля
 * Все функции возвращают 0 если данные еще не получены (STATE_UNDEF)
 * ============================================================ */

/**
 * @brief Получить состояние ACC (аксессуаров)
 * @return 1 — ACC активно, 0 — нет / данные отсутствуют
 */
uint8_t car_get_acc(void)
{
	if (carstate.acc == STATE_UNDEF)
		return 0;
	return carstate.acc;
}

/**
 * @brief Получить состояние зажигания
 * @return 1 — зажигание включено, 0 — нет / данные отсутствуют
 */
uint8_t car_get_ign(void)
{
	if (carstate.ign == STATE_UNDEF)
		return 0;
	return carstate.ign;
}

/**
 * @brief Получить состояние работы двигателя
 * @return 1 — двигатель работает, 0 — нет / данные отсутствуют
 */
uint8_t car_get_engine(void)
{
	if (carstate.engine == STATE_UNDEF)
		return 0;
	return carstate.engine;
}

/**
 * @brief Получить уровень внешней освещенности
 * @return Значение освещенности, 0 если данные отсутствуют
 */
uint8_t car_get_illum(void)
{
	if (carstate.illum == STATE_UNDEF)
		return 0;
	return carstate.illum;
}

/**
 * @brief Получить состояние парковочных огней
 * @return 1 — включены, 0 — нет / данные отсутствуют
 */
uint8_t car_get_park_lights(void)
{
	if (carstate.park_lights == STATE_UNDEF)
		return 0;
	return carstate.park_lights;
}

/**
 * @brief Получить состояние ближнего света
 * @return 1 — включен, 0 — нет / данные отсутствуют
 */
uint8_t car_get_near_lights(void)
{
	if (carstate.near_lights == STATE_UNDEF)
		return 0;
	return carstate.near_lights;
}

/**
 * @brief Получить данные парковочного радара
 * @param r Указатель на структуру radar_t для сохранения данных
 * @note Копирует содержимое внутренней структуры radar в выходной буфер
 */
void car_get_radar(struct radar_t * r)
{
	memcpy(r, &carstate.radar, sizeof(radar_t));
}

/**
 * @brief Получить угол поворота руля
 * @param wheel Указатель для сохранения угла (со знаком, в градусах или условных единицах)
 * @return Всегда 8 (размер данных в битах)
 */
uint8_t car_get_wheel(int8_t * wheel)
{
	*wheel = carstate.wheel;
	return 8;
}

/**
 * @brief Получить VIN-код автомобиля
 * @param buf Буфер размером минимум 18 байт
 * @return Длина скопированных данных (17 при наличии VIN, 2 — "na" если данных нет)
 */
uint8_t car_get_vin(uint8_t * buf)
{
	memset(buf, 0x0, 18);
	if (carstate.vin[0] == STATE_UNDEF) {
		buf[0] = 'n';
		buf[1] = 'a';
		return 2;
	}
	memcpy(buf, carstate.vin, 17);
	return 17;
}

/**
 * @brief Получить пробег (одометр)
 * @return Значение одометра
 */
uint32_t car_get_odometer(void)
{
	return carstate.odometer;
}

/**
 * @brief Получить положение селектора АКПП
 * @return Значение перечисления e_selector_t
 */
enum e_selector_t car_get_selector(void)
{
	return carstate.selector;
}

/**
 * @brief Получить состояние передней левой двери
 * @return 1 — открыта, 0 — закрыта / данные отсутствуют
 */
uint8_t car_get_door_fl(void)
{
	if (carstate.fl_door == STATE_UNDEF)
		return 0;
	return carstate.fl_door;
}

/**
 * @brief Получить состояние передней правой двери
 * @return 1 — открыта, 0 — закрыта / данные отсутствуют
 */
uint8_t car_get_door_fr(void)
{
	if (carstate.fr_door == STATE_UNDEF)
		return 0;
	return carstate.fr_door;
}

/**
 * @brief Получить состояние задней левой двери
 * @return 1 — открыта, 0 — закрыта / данные отсутствуют
 */
uint8_t car_get_door_rl(void)
{
	if (carstate.rl_door == STATE_UNDEF)
		return 0;
	return carstate.rl_door;
}

/**
 * @brief Получить состояние задней правой двери
 * @return 1 — открыта, 0 — закрыта / данные отсутствуют
 */
uint8_t car_get_door_rr(void)
{
	if (carstate.rr_door == STATE_UNDEF)
		return 0;
	return carstate.rr_door;
}

/**
 * @brief Получить состояние капота
 * @return 1 — открыт, 0 — закрыт / данные отсутствуют
 */
uint8_t car_get_bonnet(void)
{
	if (carstate.bonnet == STATE_UNDEF)
		return 0;
	return carstate.bonnet;
}

/**
 * @brief Получить состояние багажника / задней двери
 * @return 1 — открыт, 0 — закрыт / данные отсутствуют
 */
uint8_t car_get_tailgate(void)
{
	if (carstate.tailgate == STATE_UNDEF)
		return 0;
	return carstate.tailgate;
}

/**
 * @brief Получить состояние стояночного тормоза
 * @return 1 — включен, 0 — отпущен / данные отсутствуют
 */
uint8_t car_get_park_break(void)
{
	if (carstate.park_break == STATE_UNDEF)
		return 0;
	return carstate.park_break;
}

/**
 * @brief Получить состояние индикатора низкого уровня омывающей жидкости
 * @return 1 — низкий уровень, 0 — норма / данные отсутствуют
 */
uint8_t car_get_low_washer(void)
{
	if (carstate.low_washer == STATE_UNDEF)
		return 0;
	return carstate.low_washer;
}

/**
 * @brief Получить состояние ремня безопасности водителя
 * @return 1 — не пристегнут, 0 — пристегнут / данные отсутствуют
 */
uint8_t car_get_ds_belt(void)
{
	if (carstate.ds_belt == STATE_UNDEF)
		return 0;
	return carstate.ds_belt;
}

/**
 * @brief Получить обороты двигателя
 * @return Значение тахометра (RPM)
 */
uint16_t car_get_taho(void)
{
	return carstate.taho;
}

/**
 * @brief Получить скорость автомобиля
 * @return Значение скорости
 */
uint16_t car_get_speed(void)
{
	return carstate.speed;
}

/**
 * @brief Получить напряжение бортовой сети
 * @return Значение напряжения (масштаб зависит от реализации)
 */
uint32_t car_get_voltage(void)
{
	return carstate.voltage;
}

/**
 * @brief Получить температуру
 * @return Значение температуры (масштаб зависит от реализации)
 */
uint32_t car_get_temp(void)
{
	return carstate.temp;
}

/**
 * @brief Получить уровень топлива
 * @return Значение уровня топлива
 */
uint8_t car_get_fuel_level(void)
{
	return carstate.fuel_lvl;
}

/**
 * @brief Получить состояние индикатора низкого напряжения
 * @return 1 — низкое напряжение, 0 — норма / данные отсутствуют
 */
uint8_t car_get_low_voltage(void)
{
	if (carstate.low_voltage == STATE_UNDEF)
		return 0;
	return carstate.low_voltage;
}

/**
 * @brief Получить состояние индикатора низкого уровня топлива
 * @return 1 — низкий уровень, 0 — норма / данные отсутствуют
 */
uint8_t car_get_low_fuel_level(void)
{
	if (carstate.low_fuel_lvl == STATE_UNDEF)
		return 0;
	return carstate.low_fuel_lvl;
}

/* ============================================================
 * Геттеры состояния климат-контроля
 * Все функции возвращают 0 если данные еще не получены (STATE_UNDEF)
 * ============================================================ */

/**
 * @brief Получить состояние кондиционера (AC)
 * @return 1 — включен, 0 — выключен / данные отсутствуют
 */
uint8_t car_get_air_ac(void)
{
	if (car_air_state.ac == STATE_UNDEF)
		return 0;
	return car_air_state.ac;
}

/**
 * @brief Получить состояние режима MAX A/C
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_ac_max(void)
{
	if (car_air_state.ac_max == STATE_UNDEF)
		return 0;
	return car_air_state.ac_max;
}

/**
 * @brief Получить состояние рециркуляции воздуха
 * @return 1 — активна, 0 — неактивна / данные отсутствуют
 */
uint8_t car_get_air_recycling(void)
{
	if (car_air_state.recycling == STATE_UNDEF)
		return 0;
	return car_air_state.recycling;
}

/**
 * @brief Получить состояние максимальной рециркуляции
 * @return 1 — активна, 0 — неактивна / данные отсутствуют
 */
uint8_t car_get_air_recycling_max(void)
{
	if (car_air_state.recycling_max == STATE_UNDEF)
		return 0;
	return car_air_state.recycling_max;
}

/**
 * @brief Получить состояние минимальной рециркуляции
 * @return 1 — активна, 0 — неактивна / данные отсутствуют
 */
uint8_t car_get_air_recycling_min(void)
{
	if (car_air_state.recycling_min == STATE_UNDEF)
		return 0;
	return car_air_state.recycling_min;
}

/**
 * @brief Получить состояние режима DUAL (двухзонного)
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_dual(void)
{
	if (car_air_state.dual == STATE_UNDEF)
		return 0;
	return car_air_state.dual;
}

/**
 * @brief Получить состояние обогрева заднего стекла
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_rear(void)
{
	if (car_air_state.rear == STATE_UNDEF)
		return 0;
	return car_air_state.rear;
}

/**
 * @brief Получить состояние блокировки заднего управления
 * @return 1 — заблокировано, 0 — разблокировано / данные отсутствуют
 */
uint8_t car_get_air_rear_lock(void)
{
	if (car_air_state.rear_lock == STATE_UNDEF)
		return 0;
	return car_air_state.rear_lock;
}

/**
 * @brief Получить состояние системы качества воздуха (AQS)
 * @return 1 — активна, 0 — неактивна / данные отсутствуют
 */
uint8_t car_get_air_aqs(void)
{
	if (car_air_state.aqs == STATE_UNDEF)
		return 0;
	return car_air_state.aqs;
}

/**
 * @brief Получить режим обдува ветрового стекла
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_wind(void)
{
	if (car_air_state.wind == STATE_UNDEF)
		return 0;
	return car_air_state.wind;
}

/**
 * @brief Получить режим обдува средней зоны
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_middle(void)
{
	if (car_air_state.middle == STATE_UNDEF)
		return 0;
	return car_air_state.middle;
}

/**
 * @brief Получить режим обдува в ноги
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_floor(void)
{
	if (car_air_state.floor == STATE_UNDEF)
		return 0;
	return car_air_state.floor;
}

/**
 * @brief Получить состояние режима POWERFULL
 * @return 1 — активен, 0 — неактивен / данные отсутствуют
 */
uint8_t car_get_air_powerfull(void)
{
	if (car_air_state.powerfull == STATE_UNDEF)
		return 0;
	return car_air_state.powerfull;
}

/**
 * @brief Получить скорость вентилятора
 * @return Значение скорости (0 если данные отсутствуют)
 */
uint8_t car_get_air_fanspeed(void)
{
	if (car_air_state.fanspeed == STATE_UNDEF)
		return 0;
	return car_air_state.fanspeed;
}

/**
 * @brief Получить температуру левой зоны
 * @return Значение температуры (0 если данные отсутствуют)
 */
uint8_t car_get_air_l_temp(void)
{
	if (car_air_state.l_temp == STATE_UNDEF)
		return 0;
	return car_air_state.l_temp;
}

/**
 * @brief Получить температуру правой зоны
 * @return Значение температуры (0 если данные отсутствуют)
 */
uint8_t car_get_air_r_temp(void)
{
	if (car_air_state.r_temp == STATE_UNDEF)
		return 0;
	return car_air_state.r_temp;
}

/**
 * @brief Получить состояние обогрева/вентиляции левого сиденья
 * @return Значение уровня (0 если данные отсутствуют)
 */
uint8_t car_get_air_l_seat(void)
{
	if (car_air_state.l_seat == STATE_UNDEF)
		return 0;
	return car_air_state.l_seat;
}

/**
 * @brief Получить состояние обогрева/вентиляции правого сиденья
 * @return Значение уровня (0 если данные отсутствуют)
 */
uint8_t car_get_air_r_seat(void)
{
	if (car_air_state.r_seat == STATE_UNDEF)
		return 0;
	return car_air_state.r_seat;
}
