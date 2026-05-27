#ifndef CONFIG_H
#define CONFIG_H

/*
 * Compile-time configuration for canbox firmware.
 *
 * Uncomment ONE car and ONE protocol below, then rebuild.
 * All settings are resolved at compile time — no runtime switching.
 */

/* ===== Car selection (uncomment ONE) ===== */
#define CONFIG_CAR_SKODA_FABIA
// #define CONFIG_CAR_LR2_2007MY
// #define CONFIG_CAR_LR2_2013MY
// #define CONFIG_CAR_XC90_2007MY
// #define CONFIG_CAR_Q3_2015
// #define CONFIG_CAR_TOYOTA_PREMIO_26X
// #define CONFIG_CAR_ANYMSG

/* ===== Canbox protocol selection (uncomment ONE) ===== */
#define USE_RAISE_VW_PQ
// #define USE_RAISE_VW_MQB
// #define USE_OD_BMW_NBT_EVO
// #define USE_HIWORLD_VW_MQB

/* ===== Other compile-time settings ===== */
#define CONFIG_ILLUM 50
#define CONFIG_REAR_DELAY 1500

#endif
