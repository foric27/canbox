/**
 * @brief Универсальный обработчик активности CAN-шины
 * @param msg Указатель на буфер сообщения (не используется)
 * @param desc Указатель на дескриптор сообщения (для проверки таймаута)
 *
 * Устанавливает флаги acc и ign при получении любого CAN-пакета.
 * При таймауте сбрасывает acc и ign в 0.
 * Используется как fallback для автомобилей без детального разбора сообщений.
 */
static void anymsg_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	(void)msg;

	if (is_timeout(desc)) {

		carstate.acc = 0;
		carstate.ign = 0;
		return;
	}

	carstate.acc = 1;
	carstate.ign = 1;
}

/**
 * @brief Таблица дескрипторов CAN-сообщений для универсального режима
 *
 * Каждая запись: { CAN_ID, таймаут_мс, 0, 0, обработчик }
 * CAN_ID = 0x0 означает приём всех сообщений (wildcard).
 */
struct msg_desc_t anymsg_desc[] =
{
	{ 0x0, 100, 0, 0, anymsg_handler },
};
