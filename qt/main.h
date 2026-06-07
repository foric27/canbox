#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QDialog>

namespace Ui {
	class main;
}

namespace Ui {
	class about;
}

/**
 * @brief Главное окно приложения qCanBox
 *
 * Центральный виджет, объединяющий панель COM-порта (wdg_com_t),
 * панель отладки (wdg_dbg_t) и кнопки управления виртуальным автомобилем.
 * Загружает прошивочный код canbox/car и запускает таймер обработки.
 */
class main_t : public QMainWindow
{
	Q_OBJECT

	public:
		/**
		 * @brief Конструктор главного окна
		 * @param parent Родительский виджет (по умолчанию nullptr)
		 *
		 * Собирает UI, заполняет список протоколов canbox,
		 * связывает сигналы COM-порта и кнопок, инициализирует car_init().
		 */
		main_t(QWidget *parent = 0);

		/**
		 * @brief Передача данных из прошивки в Qt-сигнал
		 * @param ptr Указатель на буфер с данными
		 * @param len Длина данных в байтах
		 * @return Количество переданных байтов (всегда len)
		 *
		 * Заглушка hw_usart_write — преобразует буфер в QByteArray
		 * и emits sig_msg для передачи в wdg_com_t.
		 */
		int usart_write(const uint8_t * ptr, int len);

	public:
		Ui::main *m_ui;  /**< UI-форма главного окна (Qt Designer) */

	private:
		QTimer timer;    /**< Таймер периодической обработки прошивки */

		/**
		 * @brief Инициализация кнопок виртуального автомобиля
		 *
		 * Связывает кнопки дверей, капота, багажника и ремня
		 * со слотом slt_btns() для обновления qcar_state[].
		 */
		void init_buttons();

	signals:
		/**
		 * @brief Сигнал передачи строки лога в отладочный виджет
		 * @param lvl Уровень логирования
		 * @param log Текст сообщения
		 */
		void sig_log(uint8_t lvl, const QString & log);

		/**
		 * @brief Данные от прошивки к COM-порту (исходящий USART)
		 * @param ba Байтовый массив пакета canbox
		 */
		void sig_msg(const QByteArray & ba);

	private slots:
		/**
		 * @brief Показ диалога "О программе"
		 * @param Не используется (сигнатура для совместимости с clicked)
		 */
		void slt_btn_about(int);

		/**
		 * @brief Таймер обработки прошивки
		 *
		 * Вызывает car_process(1) и canbox_process() каждые 500 мс.
		 * В эмуляторе период увеличен относительно реальной прошивки
		 * (5 мс/250 мс) для удобства ручного тестирования.
		 */
		void slt_timer();

		/**
		 * @brief Обработка входящих данных от COM-порта
		 * @param msg Байтовый массив от wdg_com_t
		 *
		 * Передает каждый байт в canbox_rx_process() для парсинга протокола.
		 */
		void slt_msg(const QByteArray & msg);

		/**
		 * @brief Обработка переключения кнопок автомобиля
		 *
		 * Считывает состояние всех checkable-кнопок дверей, капота,
		 * багажника и ремня безопасности в массив qcar_state[].
		 */
		void slt_btns();
};

/**
 * @brief Диалоговое окно "О программе"
 *
 * Отображает версию Qt, тип ОС, дату сборки и ссылку на репозиторий.
 */
class about_t : public QDialog
{
	Q_OBJECT

	public:
		/**
		 * @brief Конструктор диалога
		 * @param parent Родительский виджет
		 */
		about_t(QWidget *parent = 0);

	private:
		Ui::about *m_ui;  /**< UI-форма диалога (Qt Designer) */
};

#endif
