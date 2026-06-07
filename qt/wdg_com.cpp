#include <QDebug>
#include <QFile>
#include <QSerialPortInfo>

#include "wdg_com.h"
#include "log_levels.h"

/**
 * @brief Конструктор виджета COM-порта
 * @param parent Родительский виджет
 *
 * Заполняет выпадающий список доступными COM-портами системы,
 * устанавливает иконку "отключено" и связывает сигналы кнопки и порта
 * с соответствующими слотами.
 */
wdg_com_t::wdg_com_t(QWidget * parent) : QWidget(parent), ui(new Ui::com)
{
	ui->setupUi(this);

	QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
	for (int i = 0; i < list.size(); i++) {

		const QSerialPortInfo & spi = list[i];
		ui->cb_port->addItem(spi.portName());
	}

	ui->btn_open->setIcon(QIcon(":/images/disconnect.png"));

	connect(ui->btn_open, &QPushButton::clicked, this, &wdg_com_t::slt_open);
	connect(&sp, &QSerialPort::readyRead, this, &wdg_com_t::serial_read_cb);
	connect(&sp, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(slt_error(QSerialPort::SerialPortError)));
}

/**
 * @brief Деструктор
 */
wdg_com_t::~wdg_com_t()
{
}

/**
 * @brief Обработка ошибок последовательного порта
 * @param error Код ошибки QSerialPort
 *
 * Логирует любую ошибку, отличную от NoError.
 * При критической ошибке ResourceError (отключение устройства)
 * автоматически вызывает stop() для закрытия порта.
 */
void wdg_com_t::slt_error(QSerialPort::SerialPortError error)
{
	if (error != QSerialPort::NoError)
		emit sig_log(e_log_warn, "serial error: " + sp.errorString());

	if (error == QSerialPort::ResourceError)
		stop();
}

/**
 * @brief Открытие или закрытие COM-порта
 *
 * Если порт уже открыт — закрывает его (вызывает stop()).
 * Иначе настраивает параметры 38400 8N1, открывает порт в режиме
 * ReadWrite, устанавливает DTR и меняет иконку на "подключено".
 */
void wdg_com_t::slt_open()
{
	if (sp.isOpen()) {

		stop();
	}
	else {

		QString com = ui->cb_port->currentText();

		sp.setPortName(com);

		sp.setBaudRate(QSerialPort::Baud38400);
		sp.setDataBits(QSerialPort::Data8);
		sp.setParity(QSerialPort::NoParity);
		sp.setStopBits(QSerialPort::OneStop);
		sp.setFlowControl(QSerialPort::NoFlowControl);

		bool r = sp.open(QIODevice::ReadWrite);
		if (!r) {

			emit sig_log(e_log_warn, "Cannot open serial device: " + com + " " + sp.errorString());
			return;
		}

		sp.setDataTerminalReady(true);

		emit sig_log(e_log_info, "open serial device: " + com);

		ui->btn_open->setIcon(QIcon(":/images/connect.png"));
		ui->btn_open->setText("Close");

		emit sig_opened();
	}
}

/**
 * @brief Чтение данных из COM-порта
 *
 * Вызывается сигналом readyRead от QSerialPort.
 * Читает все доступные байты, формирует hex-строку для лога
 * и передает QByteArray через sig_msg для обработки протоколом canbox.
 */
void wdg_com_t::serial_read_cb()
{
	if (!sp.bytesAvailable())
		return;

	QByteArray msg = sp.readAll();

	QString s;
	for (int i = 0; i < msg.size(); i++)
		s += QString("%1 ").arg((uint8_t)msg[i], 2, 16, QChar('0'));
	emit sig_log(e_log_debug, "rx: " + s);

	emit sig_msg(msg);
}

/**
 * @brief Закрытие COM-порта и сброс интерфейса
 *
 * Закрывает QSerialPort, возвращает иконку "отключено",
 * меняет текст кнопки на "Open" и emits sig_closed.
 */
void wdg_com_t::stop()
{
	emit sig_log(e_log_info, "close comport");

	sp.close();

	ui->btn_open->setIcon(QIcon(":/images/disconnect.png"));
	ui->btn_open->setText("Open");

	emit sig_closed();
}

/**
 * @brief Передача данных в открытый COM-порт
 * @param msg Байтовый массив для отправки
 *
 * Если порт не открыт — вызов игнорируется.
 * Иначе данные логируются в hex и записываются в QSerialPort.
 */
void wdg_com_t::slt_msg(const QByteArray & msg)
{
	if (!sp.isOpen())
		return;

	QString s;
	for (int i = 0; i < msg.size(); i++)
		s += QString("%1 ").arg((uint8_t)msg[i], 2, 16, QChar('0'));
	emit sig_log(e_log_debug, "tx: " + s);

	sp.write(msg);
}
