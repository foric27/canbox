#include <stdint.h>
#include <errno.h>
#include <malloc.h>

/** @brief Максимальный резерв стека под прерывания и вызовы (в байтах) */
#define MAX_STACK_SIZE  1024

/**
 * @brief Настройка границ динамической памяти (heap)
 * @param start Указатель для записи начального адреса heap
 * @param end   Указатель для записи конечного адреса heap
 * @note Функция помечена как #pragma weak; пользователь может
 *       переопределить её для размещения heap во внешней памяти.
 */
void local_heap_setup(uint8_t **start, uint8_t **end);

#pragma weak local_heap_setup = __local_ram

/* Символы, определенные линкер-скриптом */
#ifdef STM32F1
extern uint8_t _ebss, _stack;
#else
extern uint8_t __bss_end__, __stack;
#endif

/** @brief Текущий указатель "break" (граница выделенной памяти) */
static uint8_t *_cur_brk = NULL;
/** @brief Конец доступной динамической памяти */
static uint8_t *_heap_end = NULL;

/**
 * @brief Реализация настройки heap по умолчанию
 * @param start Указатель для записи начального адреса heap
 * @param end   Указатель для записи конечного адреса heap
 * @note Размещает heap в свободном ОЗУ между концом секции .bss
 *       и началом стека, оставляя MAX_STACK_SIZE байт под стек.
 *       Для STM32F1 используются символы _ebss/_stack,
 *       для остальных платформ — __bss_end__/__stack.
 */
static void
__local_ram(uint8_t **start, uint8_t **end)
{
#ifdef STM32F1
    *start = &_ebss;
    *end = (uint8_t *)(&_stack - MAX_STACK_SIZE);
#else
    *start = &__bss_end__;
    *end = (uint8_t *)(&__stack - MAX_STACK_SIZE);
#endif
}

/* Прототип для совместимости с newlib */
void *_sbrk_r(struct _reent *, ptrdiff_t );

/**
 * @brief Реализация _sbrk_r для newlib (malloc/FreeRTOS)
 * @param reent Указатель на структуру reentrancy newlib
 * @param diff  Количество байт для расширения/сжатия heap
 * @return Указатель на начало выделенного блока при успехе,
 *         (void *)-1 при нехватке памяти (ENOMEM)
 * @note При первом вызове инициализирует границы heap через
 *       local_heap_setup(). Память должна быть непрерывной.
 */
void *_sbrk_r(struct _reent *reent, ptrdiff_t diff)
{
    uint8_t *_old_brk;

    if (_heap_end == NULL) {
        local_heap_setup(&_cur_brk, &_heap_end);
    }

    _old_brk = _cur_brk;
    if (_cur_brk + diff > _heap_end) {
        reent->_errno = ENOMEM;
        return (void *)-1;
    }
    _cur_brk += diff;
    return _old_brk;
}
