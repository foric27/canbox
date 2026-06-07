#ifndef WDG_DBG_H
#define WDG_DBG_H

#include <QWidget>
#include "log_levels.h"

namespace Ui
{
	class dbg;
}

/**
 * @brief Виджет отладочного журнала
 *
 * Отображает сообщения различных уровней (предупреждения, информация, отладка)
 * с цветовой маркировкой, возможностью очистки и сохранения лога в файл.
 */
class wdg_dbg_t : public QWidget
{
	Q_OBJECT

	private:
		Ui::dbg * ui;  /**< UI-форма (Qt Designer) */

	public:
		/**
		 * @brief Конструктор отладочного виджета
		 * @param parent Родительский виджет (по умолчанию nullptr)
		 *
		 * Заполняет выпадающий список уровней логирования и настраивает слоты кнопок.
		 */
		wdg_dbg_t(QWidget *parent = 0);

		/**
		 * @brief Деструктор
		 */
		~wdg_dbg_t();

	private slots:
		/**
		 * @brief Очистка содержимого журнала
		 */
		void slt_btn_clr();

		/**
		 * @brief Сохранение журнала в текстовый файл
		 *
		 * Открывает диалог выбора имени файла и записывает
		 * содержимое лога с временной меткой в имени по умолчанию.
		 */
		void slt_btn_save();

	public slots:
		/**
		 * @brief Добавление сообщения в журнал
		 * @param level Уровень важности (e_log_warn, e_log_info, e_log_debug, ...)
		 * @param log Текст сообщения
		 *
		 * Сообщения с уровнем выше текущего выбранного в cb_level игнорируются.
		 * Предупреждения выделяются красным, отладка — синим.
		 */
		void slt_log(uint8_t level, const QString & log);
};

#endif
