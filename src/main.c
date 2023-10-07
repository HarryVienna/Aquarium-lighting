#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"

#include "time.h"

#include "wifi.h"

// PWM-Konfiguration
#define PWM_FREQ 1000 // 1000Hz PWM
#define PWM_RESOLUTION LEDC_TIMER_12_BIT // Auflösung von 10 Bit
#define MIN_DUTY 0 // Minimaler duty cycle Wert
#define MAX_DUTY 4095 // Maximaler duty cycle Wert
#define LEDC_CHANNEL LEDC_CHANNEL_0 // Verwenden Sie ein beliebiges LEDC-Kanal

// PIN für LED-Ausgang
#define LED_PIN 16 

#define TAG "main"


void app_main(void)
{
    ESP_LOGI(TAG, "Starting");

    esp_err_t ret;

    // ------------------ Starte WIFI -------------------

    static wifi_conf_t wifi_conf = {
        .aes_key = "ESP32EXAMPLECODE", // ESP32---AQUARIUM                    
        .hostname = "ESP32",
        .ntp_server = "pool.ntp.org",
    };
    wifi_t *smartconfig = wifi_new_smartconfig(&wifi_conf);

    if (smartconfig->init(smartconfig) == ESP_OK) {

        do {
            ret = smartconfig->connect(smartconfig);
        }
        while (ret != ESP_OK);

        smartconfig->init_sntp(smartconfig);
        smartconfig->init_timezone(smartconfig);
    }


    // ------------------ Starte Licht -------------------

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL,
        .duty = 0,
        .gpio_num = LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };

    time_t now;
    struct tm timeinfo;
    
    int brightnessArray[24] = { 0, // 00
                                0, // 01
                                0, // 02
                                0, // 03
                                0, // 04
                                0, // 05
                                0, // 06
                                0, // 07
                                1, // 08
                                1, // 09
                                1, // 10
                                1, // 11
                                1, // 12
                                0, // 13
                                0, // 14
                                1, // 15
                                1, // 16
                                1, // 17
                                1, // 18
                                1, // 19
                                1, // 20
                                0, // 21
                                0, // 22
                                0  // 23
                            };


    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);

    // Warten, bis die RTC Zeit synchronisiert ist
    time(&now);
    localtime_r(&now, &timeinfo);
    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // Beim Start LEDs dunkel
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL, 0);
    int current_state = 0;

    // Maximalwert hängt von PWM_RESOLUTION ab
    int max_duty = (1 << PWM_RESOLUTION) - 1;  // Bei 10 Bit ist das 1023
    ESP_LOGI(TAG, "max_duty %d", max_duty);

    while (1) {

        time(&now);
        localtime_r(&now, &timeinfo);
        
        int hour = timeinfo.tm_hour;
        ESP_LOGI(TAG, "hour %d", hour);
        int target_state = brightnessArray[hour];
        ESP_LOGI(TAG, "current_state target_state  %d %d", current_state, target_state);

        if (target_state != current_state) {
            ESP_LOGI(TAG, "target_state != current_state");
            current_state = target_state;

            if (target_state == 0) {
                ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL, MIN_DUTY, 10 * 60 * 1000); // Ausschalten, 10 Minuten Fade-Zeit
            }
            else {
                ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL, MAX_DUTY, 10 * 60 * 1000); // Einschalten, 10 Minuten Fade-Zeit
            }
            
            ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL, LEDC_FADE_WAIT_DONE);
        }
               
        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS); // Warten für 10 Sekunden
    }
}
