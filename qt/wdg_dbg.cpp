#include <QDateTime>
#include <QFileDialog>
#include <QDebug>

#include "wdg_dbg.h"
#include "ui_dbg.h"

/**
 * @brief Конструктор отладочного виджета
 * @param parent Родительский виджет
 *
 * Заполняет выпадающий список cb_level уровнями логирования
 * (Warn, Status, Info, Debug, All), устанавливает фильтр "All"
 * и связывает кнопки Clear/Save со слотами.
 */
wdg_dbg_t::wdg_dbg_t(QWidget * parent) : QWidget(parent), ui(new Ui::dbg)
{
	ui->setupUi(this);

	ui->cb_level->addItem("Warn", e_log_warn);
	ui->cb_level->addItem("Status", e_log_status);
	ui->cb_level->addItem("Info", e_log_info);
	ui->cb_level->addItem("Debug", e_log_debug);
	ui->cb_level->addItem("All", e_log_all);

	ui->cb_level->setCurrentIndex(e_log_all);

	connect(ui->btn_clr, &QPushButton::clicked, this, &wdg_dbg_t::slt_btn_clr);
	connect(ui->btn_save, &QPushButton::clicked, this, &wdg_dbg_t::slt_btn_save);
}

/**
 * @brief Деструктор
 */
wdg_dbg_t::~wdg_dbg_t()
{
}

/**
 * @brief Добавление сообщения в журнал с фильтрацией по уровню
 * @param lvl Уровень важности сообщения
 * @param txt Текст сообщения
 *
 * Сравнивает уровень сообщения с выбранным в cb_level.
 * Если сообщение отфильтровано — возврат без вывода.
 * Иначе добавляет временную метку и выводит:
 * - Warn — красным цветом (HTML)
 * - Debug — синим цветом (HTML)
 * - Остальные — обычным текстом
 */
void wdg_dbg_t::slt_log(uint8_t lvl, const QString & txt)
{
	QVariant variant = ui->cb_level->currentData();
	int log_level = variant.value<int>();

	if (lvl > log_level)
		return;

	QDateTime dt = QDateTime::currentDateTime();
	QString st = dt.toString("hh:mm:ss.z");
	if (lvl == e_log_warn)
		ui->log->appendHtml(st + " " + "<font color = \"red\">" + txt.toHtmlEscaped() + "</font>");
	else if (lvl == e_log_debug)
		ui->log->appendHtml(st + " " + "<font color = \"blue\">" + txt.toHtmlEscaped() + "</font>");
	else
		ui->log->appendPlainText(st + " " + txt);
}

/**
 * @brief Очистка содержимого журнала
 */
void wdg_dbg_t::slt_btn_clr()
{
	ui->log->clear();
}

/**
 * @brief Сохранение журнала в текстовый файл
 *
 * Формирует имя файла по умолчанию: qcanbox-log-<гггг-ММ-дд-hhmmss>.txt.
 * Открывает диалог сохранения, записывает содержимое QPlainTextEdit
 * в файл в локальной кодировке.
 */
void wdg_dbg_t::slt_btn_save()
{
	QDateTime dt = QDateTime::currentDateTime();
	QString st = dt.toString("yyyy-MM-dd-hhmmss");

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save log file"), QString(".") + QDir::separator() + "qcanbox-log-" + st + ".txt", tr("text (*.txt)"));

	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
		return;

	file.write(ui->log->toPlainText().toLocal8Bit());

	file.close();
}
