#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>

#include "hw_gpio.h"
#include "hw_can.h"

/**
 * @brief Структура параметров битрейта CAN (заглушка)
 * @note QEMU target: не используется, сохранена для совместимости
 */
typedef struct speed_t
{
	uint32_t sjw;
	uint32_t ts1;
	uint32_t ts2;
	uint32_t brp;
} speed_t;

#if 0
/* APB1 36 МГц, sample point 87.5 %, SJW = 1 */
static speed_t speeds[e_speed_nums] =
{
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 22 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 18 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_2TQ, 9 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_15TQ, CAN_BTR_TS2_2TQ, 4 },
	{ CAN_BTR_SJW_1TQ, CAN_BTR_TS1_15TQ, CAN_BTR_TS2_2TQ, 2 },
};
#else
/* APB1 36 МГц, sample point 75 %, SJW = 2 */
static speed_t speeds[e_speed_nums] =
{
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 20 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 16 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 8 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 4 },
	{ CAN_BTR_SJW_2TQ, CAN_BTR_TS1_13TQ, CAN_BTR_TS2_4TQ, 2 },
};
#endif

/** Размер буфера CAN-сообщений */
#define MSGS_SIZE 80

/**
 * @brief Структура CAN-интерфейса (заглушка для QEMU)
 *
 * @note QEMU target: эмулирует работу CAN через виртуальный UART.
 * Поля структуры идентичны реальному STM32F103 для совместимости.
 */
typedef struct can_t
{
	uint32_t rcc;
	uint32_t baddr;
	uint8_t fid;

	uint32_t irq;

	struct gpio_t tx;
	struct gpio_t rx;
	struct gpio_t s;

	uint32_t nums;
	msg_can_t msgs[MSGS_SIZE];
	uint8_t msgs_size;
} can_t;

/** Экземпляр CAN1 (базовый адрес CAN1, пины PA12/PA11, silent PB6) */
static struct can_t can1 =
{
	.rcc = RCC_CAN,
	.baddr = CAN1,
	.irq = NVIC_USB_LP_CAN_RX0_IRQ,
	.fid = 0,
	.tx = GPIO_INIT(A, 12),
	.rx = GPIO_INIT(A, 11),
	.s = GPIO_INIT(B, 6),

	.nums = 0,
	.msgs = { },
	.msgs_size = 0,
};

/**
 * @brief Получить указатель на основной CAN-интерфейс (заглушка)
 * @return Указатель на структуру can1
 * @note QEMU target: возвращает статический экземпляр для эмуляции
 */
struct can_t * hw_can_get_mscan(void)
{
	return &can1;
}

/**
 * @brief Заглушка установки скорости CAN
 * @param can   Указатель на структуру CAN
 * @param speed Индекс скорости
 * @return 0 (успех)
 * @note QEMU target: скорость не имеет значения в эмуляции
 */
uint8_t hw_can_set_speed(struct can_t * can, e_speed_t speed)
{
	nvic_disable_irq(can->irq);
	can_disable_irq(can->baddr, CAN_IER_FMPIE0);

	/* Сброс CAN-контроллера. */
	can_reset(can->baddr);

	/* Инициализация CAN-ячейки, APB1 = 36 МГц. */
	int ret = can_init(can->baddr,
		     false,           /* TTCM: режим синхронизации по времени? */
		     true,            /* ABOM: автоматический выход из bus-off? */
		     false,           /* AWUM: автоматическое пробуждение? */
		     false,           /* NART: отключить автоповтор? */
		     false,           /* RFLM: блокировка FIFO приёмника? */
		     false,           /* TXFP: приоритет FIFO передачи? */
		     speeds[speed].sjw,
		     speeds[speed].ts1,
		     speeds[speed].ts2,
		     speeds[speed].brp,
		     false,
		     false
		     );

	if (ret)
		return ret;

	/* Инициализация фильтра 0: пропускать все ID. */
	can_filter_id_mask_32bit_init(0,     /* ID фильтра */
				0,     /* CAN ID */
				0,     /* Маска CAN ID */
				0,     /* Назначение FIFO (здесь: FIFO0) */
				true); /* Включить фильтр. */

	/* Разрешить прерывание по приёму в FIFO0. */
	can_enable_irq(can->baddr, CAN_IER_FMPIE0);
	nvic_enable_irq(can->irq);

	return 0;
}

/** Типы CAN-кадров (битовые флаги) */
enum e_can_types
{
	e_can_simple = 0x0,
	e_can_statistic = 0x1,
	e_can_odd = 0x2,
	e_can_ext = 0x40,
	e_can_rtr = 0x80,
};

/**
 * @brief Заглушка полной инициализации CAN
 * @param can   Указатель на структуру CAN
 * @param speed Индекс скорости
 * @return Результат hw_can_set_speed()
 * @note QEMU target: использует реальные регистры STM32F100 для совместимости
 */
uint8_t hw_can_setup(struct can_t * can, e_speed_t speed)
{
	/* Включение тактирования периферии. */
	rcc_periph_clock_enable(can->rcc);
	rcc_periph_clock_enable(can->rx.rcc);
	rcc_periph_clock_enable(can->tx.rcc);

	exti_disable_request(EXTI11);
	nvic_disable_irq(NVIC_EXTI15_10_IRQ);

	/* Настройка пина CAN RX: вход с подтяжкой к VDD. */
	gpio_set_mode(can->rx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, can->rx.pin);
	gpio_set(can->rx.port, can->rx.pin);

	/* Настройка пина CAN TX: альтернативная функция, push-pull. */
	gpio_set_mode(can->tx.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, can->tx.pin);

	rcc_periph_clock_enable(can->s.rcc);
	/* Silent mode — отключение передатчика ZL1040. */
	gpio_set(can->s.port, can->s.pin);
	gpio_set_mode(can->s.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, can->s.pin);

	/* Настройка NVIC. */
	nvic_enable_irq(can->irq);
	nvic_set_priority(can->irq, 1);

	return hw_can_set_speed(can, speed);
}

/**
 * @brief Заглушка отключения CAN
 * @param can Указатель на структуру CAN
 * @note QEMU target: выполняет полную последовательность отключения
 */
void hw_can_disable(struct can_t * can)
{
	nvic_disable_irq(can->irq);

	rcc_periph_clock_enable(can->rcc);
	can_reset(can->baddr);
	rcc_periph_clock_disable(can->rcc);

	rcc_periph_clock_enable(can->s.rcc);
	/* Silent mode — отключение передатчика ZL1040. */
	gpio_set(can->s.port, can->s.pin);
	rcc_periph_clock_disable(can->s.rcc);

	rcc_periph_clock_enable(can->rx.rcc);
	hw_gpio_set_float(&can->tx);
	rcc_periph_clock_disable(can->rx.rcc);

	rcc_periph_clock_enable(can->tx.rcc);
	hw_gpio_set_float(&can->tx);
	rcc_periph_clock_disable(can->tx.rcc);
}

/**
 * @brief Заглушка: получить количество сообщений в буфере
 * @param can Указатель на структуру CAN
 * @return Текущий размер msgs_size
 */
uint8_t hw_can_get_msg_nums(can_t * can)
{
	return can->msgs_size;
}

/**
 * @brief Заглушка: получить количество принятых пакетов
 * @param can Указатель на структуру CAN
 * @return Счётчик nums
 */
uint32_t hw_can_get_pack_nums(struct can_t * can)
{
	return can->nums;
}

/**
 * @brief Заглушка: получить CAN-сообщение по индексу
 * @param can Указатель на структуру CAN
 * @param msg Буфер для копирования сообщения
 * @param idx Индекс в массиве msgs
 * @return 1 при успехе, 0 если индекс вне диапазона
 */
uint8_t hw_can_get_msg(struct can_t * can, struct msg_can_t * msg, uint8_t idx)
{
	if (idx >= can->msgs_size)
		return 0;

	*msg = can->msgs[idx];

	return 1;
}

/** Счётчик входов в CAN-прерывание (для отладки) */
uint32_t can_isr_cnt = 0;

/**
 * @brief Заглушка обработчика CAN-прерывания
 * @param can Указатель на структуру CAN
 * @note QEMU target: использует реальные регистры STM32F100
 */
static void can_isr(struct can_t * can)
{
	uint8_t fmi;
	struct msg_can_t msg;
	uint8_t i, j;
	uint32_t id = 0;
	uint16_t timestamp;

	can_isr_cnt++;
	bool rtr = 0, ext = 0;

	can_receive(can->baddr, 0, false, &id, &ext, &rtr, &fmi, &msg.len, msg.data, &timestamp);
	msg.id = id;

	msg.type = 0;
	if (rtr)
		msg.type |= e_can_rtr;
	if (ext)
		msg.type |= e_can_ext;

	can->nums++;

	uint8_t found = 0;
	for (i = 0; i < can->msgs_size; i++) {

		if (can->msgs[i].id == msg.id) {

			can->msgs[i].len = msg.len;
			for (j = 0; j < 8; j++)
				can->msgs[i].data[j] = msg.data[j];
			can->msgs[i].num++;
			found = 1;
			break;
		}
	}

	if (!found && can->msgs_size < MSGS_SIZE) {

		can->msgs[can->msgs_size] = msg;
		can->msgs[can->msgs_size].num = 1;
		can->msgs_size++;
	}

	can_fifo_release(can->baddr, 0);
}

/**
 * @brief Заглушка обработчика прерывания USB_LP / CAN_RX0
 * @note QEMU target: перенаправляет в can_isr()
 */
void usb_lp_can_rx0_isr(void)
{
	can_isr(hw_can_get_mscan());
}

/**
 * @brief Заглушка передачи CAN-сообщения
 * @param can Указатель на структуру CAN
 * @param msg Указатель на сообщение для передачи
 * @note QEMU target: передача через виртуальный UART
 */
void hw_can_snd_msg(struct can_t * can, struct msg_can_t * msg)
{
	if (!can_available_mailbox(can->baddr)) {

		CAN_TSR(can->baddr) |= CAN_TSR_ABRQ0 | CAN_TSR_ABRQ1 | CAN_TSR_ABRQ2;
	}

	bool rtr = msg->type & e_can_rtr;
	bool ext = msg->type & e_can_ext;
	can_transmit(can->baddr, msg->id, ext, rtr, msg->len, msg->data);
}

/**
 * @brief Заглушка очистки буфера CAN-сообщений
 * @param can Указатель на структуру CAN
 */
void hw_can_clr(struct can_t * can)
{
	can->nums = 0;
	can->msgs_size = 0;
}

/**
 * @brief Заглушка перевода CAN в режим сна
 * @param can Указатель на структуру CAN
 * @note QEMU target: отключает CAN и настраивает EXTI для пробуждения
 */
void hw_can_sleep(struct can_t * can)
{
	/* Silent mode — отключение передатчика ZL1040. */
	rcc_periph_clock_enable(can->s.rcc);
	gpio_set(can->s.port, can->s.pin);
	gpio_set_mode(can->s.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, can->s.pin);

#if 1
	hw_can_disable(can);

	rcc_periph_clock_enable(can->rx.rcc);
	gpio_set_mode(can->rx.port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, can->rx.pin);
	/* Подтяжка к VDD. */
	gpio_set(can->rx.port, can->rx.pin);
	rcc_periph_clock_disable(can->rx.rcc);

	exti_select_source(EXTI11, can->rx.port);
	exti_set_trigger(EXTI11, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI11);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);
#endif
}

/**
 * @brief Заглушка обработчика прерывания EXTI15_10
 * @note QEMU target: сбрасывает флаг EXTI11
 */
void exti15_10_isr(void)
{
	exti_reset_request(EXTI11);
}
