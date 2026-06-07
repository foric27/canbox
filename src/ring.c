#include "ring.h"

/**
 * @brief Инициализация кольцевого буфера
 * @param ring Указатель на структуру кольцевого буфера
 * @param buf Указатель на выделенный массив памяти для данных
 * @param size Размер буфера в байтах
 * @note Обнуляет указатели begin/end и счетчик переполнения.
 *       Вызывающий код должен гарантировать, что buf существует
 *       на протяжении всего времени использования буфера.
 */
void ring_init(struct ring_t *ring, uint8_t *buf, ring_size_t size)
{
	ring->data = buf;
	ring->size = size;
	ring->begin = 0;
	ring->end = 0;
	ring->overflow = 0;
}

/**
 * @brief Записать один байт в кольцевой буфер
 * @param ring Указатель на структуру кольцевого буфера
 * @param ch Байт для записи
 * @return Записанный байт (uint32_t)ch при успехе, -1 при переполнении
 * @note При переполнении инкрементирует счетчик ring->overflow
 */
int32_t ring_write_ch(struct ring_t *ring, uint8_t ch)
{
	if (((ring->end + 1) % ring->size) != ring->begin) {

		ring->data[ring->end++] = ch;
		ring->end %= ring->size;

		return (uint32_t)ch;
	}

	ring->overflow++;

	return -1;
}

/**
 * @brief Записать массив байт в кольцевой буфер
 * @param ring Указатель на структуру кольцевого буфера
 * @param data Указатель на массив данных для записи
 * @param size Количество байт для записи
 * @return Количество успешно записанных байт,
 *         или отрицательное значение (-i) если запись прервана
 * @note Запись прекращается при первом переполнении буфера
 */
int32_t ring_write(struct ring_t *ring, uint8_t *data, ring_size_t size)
{
	int32_t i;

	for (i = 0; i < size; i++) {

		if (ring_write_ch(ring, data[i]) < 0)
			return -i;
	}

	return i;
}

/**
 * @brief Прочитать один байт из кольцевого буфера
 * @param ring Указатель на структуру кольцевого буфера
 * @param ch Указатель для сохранения прочитанного байта
 * @return 1 — байт успешно прочитан, 0 — буфер пуст
 */
uint8_t ring_read_ch(struct ring_t *ring, uint8_t *ch)
{
	if (ring->begin != ring->end) {

		*ch = ring->data[ring->begin++];
		ring->begin %= ring->size;

		return 1;
	}

	return 0;
}

/**
 * @brief Получить количество событий переполнения буфера
 * @param ring Указатель на структуру кольцевого буфера
 * @return Значение счетчика переполнений
 * @note Счетчик не обнуляется автоматически; вызывающий код
 *       может использовать его для диагностики
 */
uint32_t ring_get_overflow(struct ring_t *ring)
{
	return ring->overflow;
}
