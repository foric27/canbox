#ifndef HW_USART_H
#define HW_USART_H

#include <inttypes.h>

struct usart_t;

/**
 * @brief Получение указателя на USART-контроллер
 * @return Указатель на структуру USART
 */
struct usart_t * hw_usart_get(void);

/**
 * @brief Настройка USART
 * @param usart Указатель на USART-контроллер
 * @param speed Скорость передачи (бод)
 * @param txbuf Указатель на буфер передачи
 * @param txbuflen Размер буфера передачи
 * @param rxbuf Указатель на буфер приёма
 * @param rxbuflen Размер буфера приёма
 */
void hw_usart_setup(struct usart_t *, uint32_t speed, uint8_t * txbuf, uint32_t txbuflen, uint8_t * rxbuf, uint32_t rxbuflen);

/**
 * @brief Отключение USART
 * @param usart Указатель на USART-контроллер
 */
void hw_usart_disable(struct usart_t *);

/**
 * @brief Запись данных в USART
 * @param usart Указатель на USART-контроллер
 * @param ptr Указатель на данные
 * @param len Длина данных
 * @return Количество записанных байт или код ошибки
 */
int hw_usart_write(struct usart_t *, const uint8_t * ptr, int len);

/**
 * @brief Ожидание завершения передачи
 * @param usart Указатель на USART-контроллер
 */
void hw_usart_wait_transfer(struct usart_t *);

/**
 * @brief Чтение одного символа из USART
 * @param usart Указатель на USART-контроллер
 * @param ch Указатель для сохранения символа
 * @return 0 если символ прочитан, иначе буфер пуст
 */
uint8_t hw_usart_read_ch(struct usart_t *, uint8_t *ch);

/**
 * @brief Получение количества переполнений буфера приёма
 * @param usart Указатель на USART-контроллер
 * @return Количество переполнений RX
 */
uint32_t hw_usart_get_rx_overflow(struct usart_t *);

/**
 * @brief Получение количества переполнений буфера передачи
 * @param usart Указатель на USART-контроллер
 * @return Количество переполнений TX
 */
uint32_t hw_usart_get_tx_overflow(struct usart_t *);

/**
 * @brief Получение количества принятых байт
 * @param usart Указатель на USART-контроллер
 * @return Количество RX-байт
 */
uint32_t hw_usart_get_rx(struct usart_t *);

/**
 * @brief Получение количества переданных байт
 * @param usart Указатель на USART-контроллер
 * @return Количество TX-байт
 */
uint32_t hw_usart_get_tx(struct usart_t *);

#endif
