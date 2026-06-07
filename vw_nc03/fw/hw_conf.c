#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "NUC131.h"

#include "hw_conf.h"

/** Базовый адрес Data Flash */
#define CONF_ADDR 0x1F000
/** Размер Data Flash (4 КБ) */
#define CONF_SIZE 4096

/** Биты конфигурации CONFIG0 */
#define CONFIG0_DFEN     0x01
#define CONFIG0_LOCK     0x02
#define CONFIG0_DFVSEN   0x04
#define CONFIG0_CBS_MASK 0xc0
#define CONFIG0_CBS_APP  0x80

/**
 * @brief Получить базовый адрес конфигурации во Flash
 * @return Адрес CONF_ADDR (0x1F000)
 */
uint32_t hw_conf_get_addr(void)
{
	return CONF_ADDR;
}

/**
 * @brief Получить размер области конфигурации
 * @return Размер в байтах (4096)
 */
uint32_t hw_conf_get_sz(void)
{
	return CONF_SIZE;
}

/**
 * @brief Прочитать 32-битное слово из Flash
 * @param address Адрес во Flash
 * @return Прочитанное значение
 */
uint32_t hw_conf_read_word(uint32_t address)
{
	return FMC_Read(address);
}

/**
 * @brief Разблокировать Flash для записи
 *
 * Разблокирует системные регистры и открывает доступ к FMC.
 */
void hw_conf_unlock(void)
{
	SYS_UnlockReg();
	FMC_Open();
}

/**
 * @brief Заблокировать Flash после записи
 *
 * Закрывает FMC и блокирует системные регистры.
 */
void hw_conf_lock(void)
{
	FMC_Close();
	SYS_LockReg();
}

/**
 * @brief Стереть область конфигурации
 *
 * Разблокирует Flash, стирает все 4K-блоки (по 512 байт),
 * затем блокирует Flash.
 */
void hw_conf_erase(void)
{
	hw_conf_unlock();

	for (uint8_t i = 0; i < CONF_SIZE/512; i++)
		FMC_Erase(CONF_ADDR + i*512);

	hw_conf_lock();
}

/**
 * @brief Записать 32-битное слово во Flash
 * @param address Адрес во Flash
 * @param val     Значение для записи
 */
void hw_conf_write_word(uint32_t address, uint32_t val)
{
	FMC_Write(address, val);
}

/**
 * @brief Инициализация конфигурации Data Flash
 *
 * Проверяет текущую конфигурацию пользователя:
 * если Data Flash уже включён и CBS = APPROM — разрешает обновление APROM.
 * Иначе перезаписывает конфигурацию: включает Data Flash 4K,
 * LOCK = 1 (разблокировано), DFVSEN = 1, CBS = APPROM,
 * затем выполняет сброс чипа.
 */
void hw_conf_setup(void)
{
	uint32_t au32Config[2];

	SYS_UnlockReg();

	FMC_Open();

	/* Чтение текущей конфигурации пользователя */
	FMC_ReadConfig(au32Config, 2);

	/* Если Data Flash включён и CBS = APPROM — разрешить обновление */
	if (!(au32Config[0] & CONFIG0_DFEN) && (au32Config[0] & CONFIG0_DFVSEN) && !FMC_GetBootSource()) {

		FMC_EnableAPUpdate();
		return;
	}

	/* Разрешить обновление конфигурации */
	FMC_EnableConfigUpdate();

	/* Стереть область конфигурации */
	FMC_Erase(FMC_CONFIG_BASE);

	/* Запись конфигурации:
	 * LOCK = 1, Flash разблокирован
	 * DFVSEN = 1, размер Data Flash = 4 КБ
	 */
	au32Config[0] &= ~CONFIG0_DFEN;
	au32Config[0] |= CONFIG0_LOCK;
	au32Config[0] |= CONFIG0_DFVSEN;
	au32Config[0] &= ~CONFIG0_CBS_MASK;
	au32Config[0] |= CONFIG0_CBS_APP;

	if (FMC_WriteConfig(au32Config, 2))
		return;

	FMC_Close();

	/* Сброс чипа для применения новой конфигурации */
	SYS_ResetChip();

	SYS_LockReg();
}
