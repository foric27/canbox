#ifndef HW_CAN_H
#define HW_CAN_H

#include <inttypes.h>

/**
 * @brief Перечисление скоростей CAN-шины (кбит/с)
 */
typedef enum e_speed_t
{
	e_speed_100 = 0,	/**< 100 кбит/с */
	e_speed_125,		/**< 125 кбит/с */
	e_speed_250,		/**< 250 кбит/с */
	e_speed_500,		/**< 500 кбит/с */
	e_speed_1000,		/**< 1000 кбит/с */
	e_speed_nums		/**< Количество вариантов скорости */
} e_speed_t;

/**
 * @brief Структура CAN-сообщения
 */
typedef struct msg_can_t
{
	uint32_t id;		/**< Идентификатор сообщения */
	uint32_t num;		/**< Номер сообщения */
	uint8_t type;		/**< Тип кадра (стандартный/расширенный) */
	uint8_t len;		/**< Длина полезных данных (0..8) */
	uint8_t data[8];	/**< Полезные данные */
} __attribute__ ((__packed__)) msg_can_t;

struct can_t;

/**
 * @brief Получение указателя на основной CAN-контроллер
 * @return Указатель на структуру CAN-контроллера
 */
struct can_t * hw_can_get_mscan(void);

/**
 * @brief Настройка CAN-контроллера
 * @param can Указатель на CAN-контроллер
 * @param speed Скорость шины
 * @return 0 при успехе, иначе код ошибки
 */
uint8_t hw_can_setup(struct can_t * can, e_speed_t speed);

/**
 * @brief Отключение CAN-контроллера
 * @param can Указатель на CAN-контроллер
 */
void hw_can_disable(struct can_t * can);

/**
 * @brief Перевод CAN-контроллера в режим сна
 * @param can Указатель на CAN-контроллер
 */
void hw_can_sleep(struct can_t * can);

/**
 * @brief Смена скорости CAN-шины
 * @param can Указатель на CAN-контроллер
 * @param speed Новая скорость
 * @return 0 при успехе, иначе код ошибки
 */
uint8_t hw_can_set_speed(struct can_t * can, e_speed_t speed);

/**
 * @brief Перевод CAN в пассивный (silent) режим
 */
void hw_can_silent(void);

/**
 * @brief Перевод CAN в активный режим
 */
void hw_can_active(void);

/**
 * @brief Получение количества принятых пакетов
 * @param can Указатель на CAN-контроллер
 * @return Количество пакетов
 */
uint32_t hw_can_get_pack_nums(struct can_t * can);

/**
 * @brief Получение количества сообщений в буфере
 * @param can Указатель на CAN-контроллер
 * @return Количество сообщений
 */
uint8_t hw_can_get_msg_nums(struct can_t * can);

/**
 * @brief Получение конкретного сообщения из буфера
 * @param can Указатель на CAN-контроллер
 * @param msg Указатель на структуру для сохранения сообщения
 * @param idx Индекс сообщения в буфере
 * @return 0 при успехе
 */
uint8_t hw_can_get_msg(struct can_t * can, struct msg_can_t * msg, uint8_t idx);

/**
 * @brief Получение статуса CAN-контроллера
 * @param can Указатель на CAN-контроллер
 * @return Код статуса
 */
uint8_t hw_can_get_sts(struct can_t * can);

/**
 * @brief Очистка буфера CAN-контроллера
 * @param can Указатель на CAN-контроллер
 */
void hw_can_clr(struct can_t * can);

/**
 * @brief Обработка принятого CAN-сообщения
 * @param can Указатель на CAN-контроллер
 * @param msg Указатель на принятое сообщение
 */
void hw_can_rcv_msg(struct can_t * can, msg_can_t * msg);

/**
 * @brief Отправка CAN-сообщения
 * @param can Указатель на CAN-контроллер
 * @param msg Указатель на сообщение для отправки
 */
void hw_can_snd_msg(struct can_t * can, struct msg_can_t * msg);

#endif
