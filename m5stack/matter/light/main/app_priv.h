/*
   Private header file for M5Stack Core S3 Matter Light Demo
*/

#pragma once

#include <esp_err.h>
#include <esp_matter.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_POWER 0
#define DEFAULT_BRIGHTNESS 64

/* Display related functions */
esp_err_t display_init(void);
esp_err_t display_set_light_state(bool on);

/* Driver related functions */
esp_err_t app_driver_init(void);
esp_err_t app_attribute_update_cb(esp_matter::attribute::callback_type_t type, uint16_t endpoint_id, 
                                  uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val);

#ifdef __cplusplus
}
#endif