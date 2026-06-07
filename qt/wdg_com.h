#ifndef WDG_COM_H
#define WDG_COM_H

#include <QString>
#include <QObject>
#include <QSerialPort>

#include "ui_com.h"

/**
 * @brief Виджет управления последовательным COM-портом
 *
 * Предоставляет интерфейс для выбора порта, открытия/закрытия соединения
 * и обмена данными с Android-устройством через USART.
 * Входящие байты передаются в canbox_rx_process(), исходящие — через sig_msg.
 */
class wdg_com_t : public QWidget
{
	Q_OBJECT

	private:

		Ui::com * ui;        /**< UI-форма (Qt Designer) */
		QSerialPort sp;      /**< Объект последовательного порта Qt */

	private:
		/**
		 * @brief Закрытие COM-порта и сброс UI
		 */
		void stop();

	private slots:
		/**
		 * @brief Слот вызывается при появлении данных в порту
		 *
		 * Читает все доступные байты, логирует их в hex и emits sig_msg.
		 */
		void serial_read_cb();

		/**
		 * @brief Обработка ошибок QSerialPort
		 * @param error Код ошибки из QSerialPort::SerialPortError
		 *
		 * При ResourceError автоматически закрывает порт.
		 */
		void slt_error(QSerialPort::SerialPortError error);

		/**
		 * @brief Открытие/закрытие COM-порта по нажатию кнопки
		 *
		 * Если порт закрыт — открывает с параметрами 38400/8N1.
		 * Если открыт — вызывает stop().
		 */
		void slt_open();

	public slots:
		/**
		 * @brief Передача данных в COM-порт
		 * @param msg Байтовый массив для передачи
		 *
		 * Логирует исходящие данные в hex и записывает в QSerialPort.
		 * Игнорирует вызов, если порт не открыт.
		 */
		void slt_msg(const QByteArray & msg);

	signals:
		/**
		 * @brief Сигнализирует об успешном открытии порта
		 */
		void sig_opened();

		/**
		 * @brief Сигнализирует о закрытии порта
		 */
		void sig_closed();

		/**
		 * @brief Отправка строки лога в отладочный виджет
		 * @param lvl Уровень логирования (см. e_log_level)
		 * @param log Текст сообщения
		 */
		void sig_log(uint8_t, const QString & log);

		/**
		 * @brief Полученные из COM-порта данные для дальнейшей обработки
		 * @param msg Байтовый массив принятых данных
		 */
		void sig_msg(const QByteArray & msg);

	public:
		/**
		 * @brief Конструктор виджета COM-порта
		 * @param parent Родительский виджет (по умолчанию nullptr)
		 *
		 * Заполняет список доступных портов и настраивает слоты сигналов.
		 */
		wdg_com_t(QWidget * parent = 0);

		/**
		 * @brief Деструктор
		 */
		~wdg_com_t();
};

#endif
