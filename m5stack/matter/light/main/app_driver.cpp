/*
   M5Stack Core S3 specific driver for Matter Light Demo
   
   Handles Matter attribute updates and translates them to M5Stack display changes
*/

#include <esp_log.h>
#include <esp_matter.h>
#include <app_priv.h>

using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t light_endpoint_id;

static bool light_state = false;

esp_err_t app_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing M5Stack Core S3 driver");
    
    /* Initialize display */
    esp_err_t err = display_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display: %d", err);
        return err;
    }
    
    /* Set initial light state */
    display_set_light_state(light_state);
    
    ESP_LOGI(TAG, "M5Stack Core S3 driver initialized successfully");
    return ESP_OK;
}

static esp_err_t app_driver_light_set_power(esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    bool new_state = val->val.b;
    
    if (new_state != light_state) {
        light_state = new_state;
        ESP_LOGI(TAG, "Light %s", light_state ? "ON" : "OFF");
        
        /* Update display to show new state */
        err = display_set_light_state(light_state);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to update display: %d", err);
        }
    }
    
    return err;
}

esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;

    if (type == attribute::PRE_UPDATE) {
        /* Handle pre-update operations */
        return ESP_OK;
    }

    if (endpoint_id == light_endpoint_id) {
        if (cluster_id == OnOff::Id) {
            switch (attribute_id) {
            case OnOff::Attributes::OnOff::Id:
                err = app_driver_light_set_power(val);
                break;
            default:
                ESP_LOGI(TAG, "Unhandled OnOff attribute: 0x%04lX", attribute_id);
                break;
            }
        } else {
            ESP_LOGI(TAG, "Unhandled cluster: 0x%04lX", cluster_id);
        }
    }

    return err;
}