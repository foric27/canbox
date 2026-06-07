#ifndef HW_H
#define HW_H

#include <inttypes.h>

/**
 * @brief Инициализация аппаратного обеспечения
 * @note Настраивает тактирование, GPIO, USART, CAN и таймеры
 */
void hw_setup(void);

/**
 * @brief Отключение периферии
 */
void hw_disable(void);

/**
 * @brief Переход в режим сна
 * @note Отключает периферию и переводит процессор в режим ожидания
 */
void hw_sleep(void);

/**
 * @brief Перевод CPU в режим ожидания (sleep)
 */
void hw_cpu_sleep(void);

/**
 * @brief Настройка выводов GPIO
 */
void hw_gpio_setup(void);

/**
 * @brief Отключение выводов GPIO
 */
void hw_gpio_disable(void);

/**
 * @brief Включение управления задними устройствами
 */
void hw_gpio_rear_on(void);

/**
 * @brief Отключение управления задними устройствами
 */
void hw_gpio_rear_off(void);

/**
 * @brief Включение управления парковочными устройствами
 */
void hw_gpio_park_on(void);

/**
 * @brief Отключение управления парковочными устройствами
 */
void hw_gpio_park_off(void);

/**
 * @brief Включение линии ACC
 */
void hw_gpio_acc_on(void);

/**
 * @brief Отключение линии ACC
 */
void hw_gpio_acc_off(void);

/**
 * @brief Включение подсветки
 */
void hw_gpio_ill_on(void);

/**
 * @brief Отключение подсветки
 */
void hw_gpio_ill_off(void);

#endif
