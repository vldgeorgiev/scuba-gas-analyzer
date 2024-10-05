#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

typedef enum {
    logLevel_None = 0,
    logLevel_Warning = 1,
    logLevel_Error = 2
} logLevel;

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_O2_ENABLED = 0,
    FLOW_GLOBAL_VARIABLE_O2_VALUE = 1,
    FLOW_GLOBAL_VARIABLE_O2_MILLIVOLTS = 2,
    FLOW_GLOBAL_VARIABLE_CO_ENABLED = 3,
    FLOW_GLOBAL_VARIABLE_CO_VALUE = 4,
    FLOW_GLOBAL_VARIABLE_CO_MILLIVOLTS = 5,
    FLOW_GLOBAL_VARIABLE_MOD_PO2_BOTTOM = 6,
    FLOW_GLOBAL_VARIABLE_MOD_PO2_DECO = 7,
    FLOW_GLOBAL_VARIABLE_PO2_MAX_BOTTOM = 8,
    FLOW_GLOBAL_VARIABLE_PO2_MAX_DECO = 9,
    FLOW_GLOBAL_VARIABLE_HE_ENABLED = 10,
    FLOW_GLOBAL_VARIABLE_HE_VALUE = 11,
    FLOW_GLOBAL_VARIABLE_HE_MILLIVOLTS = 12,
    FLOW_GLOBAL_VARIABLE_UI_LOG_LEVEL = 13,
    FLOW_GLOBAL_VARIABLE_BATT_VOLTAGE = 14
};

// Native global variables

extern const char *get_var_ui_log();
extern void set_var_ui_log(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/