#ifndef CONFIG_H
#define CONFIG_H

/**
 * @brief Конфигурация времени компиляции для прошивки canbox.
 *
 * Раскомментируйте ОДИН автомобиль и ОДИН протокол ниже, затем пересоберите.
 * Все настройки определяются на этапе компиляции — переключения в рантайме нет.
 */

/** @name Выбор автомобиля (раскомментировать ОДИН) */
/** @{ */
#define CONFIG_CAR_SKODA_FABIA		/**< Škoda Fabia */
// #define CONFIG_CAR_LR2_2007MY		/**< Land Rover Freelander 2 2007 */
// #define CONFIG_CAR_LR2_2013MY		/**< Land Rover Freelander 2 2013 */
// #define CONFIG_CAR_XC90_2007MY		/**< Volvo XC90 2007 */
// #define CONFIG_CAR_Q3_2015			/**< Audi Q3 2015 */
// #define CONFIG_CAR_TOYOTA_PREMIO_26X	/**< Toyota Premio 260/261 */
// #define CONFIG_CAR_ANYMSG			/**< Режим прослушивания всех сообщений */
/** @} */

/** @name Выбор протокола canbox (раскомментировать ОДИН) */
/** @{ */
#define USE_RAISE_VW_PQ				/**< RAISE VW PQ */
// #define USE_RAISE_VW_MQB			/**< RAISE VW MQB */
// #define USE_OD_BMW_NBT_EVO			/**< Oudi BMW NBT EVO */
// #define USE_HIWORLD_VW_MQB			/**< HiWorld VW MQB */
/** @} */

/** @brief Порог освещённости (0..255) */
#define CONFIG_ILLUM 50

/** @brief Задержка задней передачи (мс) */
#define CONFIG_REAR_DELAY 1500

#endif
