#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/cortex.h>

#include <inttypes.h>

#include "hw_conf.h"

/** Сектор 63, размер 1 КБ (заглушка) */
#define CONF_ADDR     0x0800fc00
#define CONF_SIZE     1024

/**
 * @brief Заглушка: получить базовый адрес конфигурации
 * @return Адрес CONF_ADDR (0x0800FC00)
 * @note QEMU target: адрес виртуального Flash
 */
uint32_t hw_conf_get_addr(void)
{
	return CONF_ADDR;
}

/**
 * @brief Заглушка: получить размер области конфигурации
 * @return Размер в байтах (1024)
 * @note QEMU target: размер виртуального Flash
 */
uint32_t hw_conf_get_sz(void)
{
	return CONF_SIZE;
}

/**
 * @brief Заглушка разблокировки Flash для QEMU
 * @note QEMU target: нет реального Flash, операция пустая
 */
void hw_conf_unlock(void)
{
}

/**
 * @brief Заглушка блокировки Flash для QEMU
 * @note QEMU target: нет реального Flash, операция пустая
 */
void hw_conf_lock(void)
{
}

/**
 * @brief Заглушка стирания Flash для QEMU
 * @note QEMU target: нет реального Flash, операция пустая
 */
void hw_conf_erase(void)
{
}

/**
 * @brief Заглушка инициализации конфигурации для QEMU
 * @note QEMU target: дополнительная инициализация не требуется
 */
void hw_conf_setup(void)
{
}

/**
 * @brief Заглушка чтения слова из Flash для QEMU
 * @param address Адрес (игнорируется)
 * @return Всегда 0xFFFFFFFF (пустая Flash)
 * @note QEMU target: возвращает значение незаписанной Flash
 */
uint32_t hw_conf_read_word(uint32_t address)
{
	(void)address;

	return 0xffffffff;
}

/**
 * @brief Заглушка записи слова во Flash для QEMU
 * @param address Адрес (игнорируется)
 * @param val     Значение (игнорируется)
 * @note QEMU target: запись не сохраняется
 */
void hw_conf_write_word(uint32_t address, uint32_t val)
{
	(void)address;
	(void)val;
}
