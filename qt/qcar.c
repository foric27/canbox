#include "qcar.h"

/**
 * @brief Глобальный массив состояний виртуального автомобиля
 *
 * Хранит состояния дверей, капота, багажника и ремня безопасности,
 * управляемые через GUI главного окна. Значения: 0 — закрыто/отстегнут,
 * 1 — открыто/пристегнут.
 */
uint8_t qcar_state[e_qcar_nums] = { 0 };

/**
 * @brief Обновление глобального состояния автомобиля из виртуального
 *
 * Копирует значения из qcar_state[] в глобальную структуру carstate,
 * которая используется прошивочным кодом (src/car.c).
 *
 * @note Вызывается из car_process() при сборке с QCAR.
 *       Позволяет тестировать логику canbox без реального CAN-шины.
 */
static void qcar_process(void)
{
	carstate.fl_door  = qcar_state[e_fl_door] ? 1 : 0;
	carstate.fr_door  = qcar_state[e_fr_door] ? 1 : 0;
	carstate.rl_door  = qcar_state[e_rl_door] ? 1 : 0;
	carstate.rr_door  = qcar_state[e_rr_door] ? 1 : 0;
	carstate.bonnet   = qcar_state[e_bonnet] ? 1 : 0;
	carstate.tailgate = qcar_state[e_tailgate] ? 1 : 0;

	carstate.ds_belt = qcar_state[e_belt] ? 1 : 0;
}
