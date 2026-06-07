#ifndef HW_CONF_H
#define HW_CONF_H

#include <inttypes.h>

/**
 * @brief Инициализация подсистемы хранения конфигурации
 */
void hw_conf_setup(void);

/**
 * @brief Получение базового адреса области конфигурации
 * @return Адрес начала сектора конфигурации
 */
uint32_t hw_conf_get_addr(void);

/**
 * @brief Получение размера области конфигурации
 * @return Размер в байтах
 */
uint32_t hw_conf_get_sz(void);

/**
 * @brief Блокировка доступа к Flash-памяти
 */
void hw_conf_lock(void);

/**
 * @brief Разблокировка доступа к Flash-памяти
 */
void hw_conf_unlock(void);

/**
 * @brief Стирание сектора конфигурации
 */
void hw_conf_erase(void);

/**
 * @brief Запись 32-битного слова во Flash
 * @param address Адрес записи
 * @param v Значение для записи
 */
void hw_conf_write_word(uint32_t address, uint32_t v);

/**
 * @brief Чтение 32-битного слова из Flash
 * @param address Адрес чтения
 * @return Прочитанное значение
 */
uint32_t hw_conf_read_word(uint32_t address);

#endif
