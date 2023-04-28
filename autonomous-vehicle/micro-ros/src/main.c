#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "bdc_motor.h"

/* General BDC motor defines */
#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000 // 10MHz, 1 tick = 0.1us
#define BDC_MCPWM_FREQ_HZ             25000    // 25KHz PWM
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ) // maximum value we can set for the duty cycle, in ticks
#define BDC_INIT_SPEED                0        //speed can be 0-400
#define BDC_MIN_SPEED                 200

/* Motor 1 pins */
#define BDC_MCPWM_GPIO_A              33
#define BDC_MCPWM_GPIO_B              25

static const char *TAG = "example";   //for prints

static void mcpwm_bdc(void *arg){
    /* DC motor config */
    bdc_motor_config_t motor_config = {         //TO-DO: make init a function to config multiple motors
        .pwm_freq_hz = BDC_MCPWM_FREQ_HZ,
        .pwma_gpio_num = BDC_MCPWM_GPIO_A,
        .pwmb_gpio_num = BDC_MCPWM_GPIO_B,
    };
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = 0,
        .resolution_hz = BDC_MCPWM_TIMER_RESOLUTION_HZ,
    };

    ESP_LOGI(TAG, "Create DC motors");

    /* right motor handle */
    bdc_motor_handle_t right_motor = NULL;
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&motor_config, &mcpwm_config, &right_motor));
    bdc_motor_set_speed(right_motor, (uint32_t)BDC_INIT_SPEED);
    ESP_LOGI(TAG, "Enable right_motor");
    ESP_ERROR_CHECK(bdc_motor_enable(right_motor));

    while (1) {
        bdc_motor_set_speed(right_motor, (uint32_t)BDC_MIN_SPEED);

        ESP_LOGI(TAG, "Forward right_motor");
        ESP_ERROR_CHECK(bdc_motor_forward(right_motor));
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "Reverse right_motor");
        ESP_ERROR_CHECK(bdc_motor_reverse(right_motor));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Running task...");
    xTaskCreate(mcpwm_bdc, "mcpwm_bdc", 4096, NULL, 5, NULL);
}
