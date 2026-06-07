#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/cortex.h>

#include <inttypes.h>

#include "hw_conf.h"

//#define debug 1
#ifdef debug
#include <string.h>
#include <stdio.h>
#include "hw_usart.h"
void mdelay(uint32_t msec);
#endif

/** Сектор 63, размер 1 КБ */
#define CONF_ADDR     0x0800fc00
#define CONF_SIZE     1024

/**
 * @brief Получить базовый адрес конфигурации во Flash
 * @return Адрес CONF_ADDR (0x0800FC00)
 */
uint32_t hw_conf_get_addr(void)
{
	return CONF_ADDR;
}

/**
 * @brief Получить размер области конфигурации
 * @return Размер в байтах (1024)
 */
uint32_t hw_conf_get_sz(void)
{
	return CONF_SIZE;
}

/**
 * @brief Разблокировать Flash для записи
 *
 * Запрещает прерывания и вызывает flash_unlock().
 */
void hw_conf_unlock(void)
{
	cm_disable_interrupts();
	flash_unlock();
}

/**
 * @brief Заблокировать Flash после записи
 *
 * Вызывает flash_lock() и разрешает прерывания.
 */
void hw_conf_lock(void)
{
	flash_lock();
	cm_enable_interrupts();
}

/**
 * @brief Стереть страницу конфигурации
 *
 * Разблокирует Flash, стирает страницу по CONF_ADDR,
 * затем блокирует Flash.
 */
void hw_conf_erase(void)
{
	hw_conf_unlock();

	flash_erase_page(CONF_ADDR);

	hw_conf_lock();
}

/**
 * @brief Пустая функция инициализации конфигурации
 *
 * Для STM32F103 дополнительная инициализация не требуется.
 */
void hw_conf_setup(void)
{
}

/**
 * @brief Прочитать 32-битное слово из Flash
 * @param address Адрес во Flash
 * @return Прочитанное значение
 */
uint32_t hw_conf_read_word(uint32_t address)
{
	uint32_t r = *(uint32_t *)(address);

#ifdef debug
	char buf[100];
	snprintf(buf, sizeof(buf), "read 0x%x:0x%x\r\n", (unsigned int)address, (unsigned int)r);
	hw_usart_write(hw_usart_get(), (uint8_t *)buf, strlen(buf));
	mdelay(100);
#endif

	return r;
}

/**
 * @brief Записать 32-битное слово во Flash
 * @param address Адрес во Flash
 * @param val     Значение для записи
 *
 * Flash должен быть предварительно разблокирован вызовом hw_conf_unlock().
 */
void hw_conf_write_word(uint32_t address, uint32_t val)
{
	flash_program_word(address, val);
}
