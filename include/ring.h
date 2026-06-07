#ifndef RING_H
#define RING_H

#include <inttypes.h>

/** @brief Тип для размеров и индексов кольцевого буфера */
typedef int32_t ring_size_t;

/**
 * @brief Структура кольцевого буфера
 */
struct ring_t
{
	uint8_t *data;			/**< Указатель на буфер данных */
	ring_size_t size;		/**< Размер буфера */
	volatile uint32_t begin;	/**< Индекс начала (чтение) */
	volatile uint32_t end;		/**< Индекс конца (запись) */
	uint32_t overflow;		/**< Счётчик переполнений */
};

/**
 * @brief Инициализация кольцевого буфера
 * @param ring Указатель на структуру буфера
 * @param buf Указатель на массив данных
 * @param size Размер массива
 */
void ring_init(struct ring_t *ring, uint8_t *buf, ring_size_t size);

/**
 * @brief Запись одного байта в буфер
 * @param ring Указатель на структуру буфера
 * @param ch Байт для записи
 * @return 0 при успехе, отрицательное значение при переполнении
 */
int32_t ring_write_ch(struct ring_t *ring, uint8_t ch);

/**
 * @brief Запись блока данных в буфер
 * @param ring Указатель на структуру буфера
 * @param data Указатель на данные
 * @param size Длина данных
 * @return Количество записанных байт или код ошибки
 */
int32_t ring_write(struct ring_t *ring, uint8_t *data, ring_size_t size);

/**
 * @brief Чтение одного байта из буфера
 * @param ring Указатель на структуру буфера
 * @param ch Указатель для сохранения прочитанного байта
 * @return 0 если байт прочитан, иначе буфер пуст
 */
uint8_t ring_read_ch(struct ring_t *ring, uint8_t *ch);

/**
 * @brief Получение количества переполнений буфера
 * @param ring Указатель на структуру буфера
 * @return Количество переполнений
 */
uint32_t ring_get_overflow(struct ring_t *ring);

#endif
