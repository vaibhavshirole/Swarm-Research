#include <stdio.h>
#include <mqtt_client.h> //temporary, for easy testing 

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
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

/* Motor 2 pins */
#define BDC_MCPWM_GPIO_C              32
#define BDC_MCPWM_GPIO_D              26

/* Configure WiFi connection settings here */
#define EXAMPLE_ESP_WIFI_SSID      "username"
#define EXAMPLE_ESP_WIFI_PASS      "password"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "example"; //for cleaner debugging prints

static int s_retry_num = 0;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void wifi_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* set a custom speed */
void explorer_set_speed(bdc_motor_handle_t left_motor, bdc_motor_handle_t right_motor, uint16_t speed)
{
    ESP_LOGI(TAG, "Setting speed...");
    bdc_motor_set_speed(right_motor, (uint32_t)speed);
    bdc_motor_set_speed(left_motor, (uint32_t)speed);
}

/* run the left motors */
void explorer_set_left(bdc_motor_handle_t left_motor, bdc_motor_handle_t right_motor)
{
    ESP_LOGI(TAG, "Forward: right - Reverse: left");
    ESP_ERROR_CHECK(bdc_motor_forward(right_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(left_motor));
}

/* run the right motors */
void explorer_set_right(bdc_motor_handle_t left_motor, bdc_motor_handle_t right_motor)
{
    ESP_LOGI(TAG, "Forward: left - Reverse: right");
    ESP_ERROR_CHECK(bdc_motor_forward(left_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(right_motor));
}

/* run motors in counterclockwise */
void explorer_set_reverse(bdc_motor_handle_t left_motor, bdc_motor_handle_t right_motor)
{
    ESP_LOGI(TAG, "Forward: none - Reverse: left, right");
    ESP_ERROR_CHECK(bdc_motor_reverse(left_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(right_motor));
}

/* run the motors clockwise */
void explorer_set_forward(bdc_motor_handle_t left_motor, bdc_motor_handle_t right_motor)
{
    ESP_LOGI(TAG, "Forward: left, right - Reverse: none");
    ESP_ERROR_CHECK(bdc_motor_forward(left_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(right_motor));
}

/* go for a certain amount of time */
void explorer_drive_ms(bdc_motor_handle_t left_motor, bdc_motor_handle_t right_motor, uint32_t time){
    vTaskDelay(pdMS_TO_TICKS(time));
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(){
    const esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtt://mqtt.eclipseprojects.io", //provide address for the broker being used
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* TASK: PWM control */
static void mcpwm_bdc(void *arg){
    /* DC motor config */
    bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = 0,
        .resolution_hz = BDC_MCPWM_TIMER_RESOLUTION_HZ,
    };

    ESP_LOGI(TAG, "Create DC motors");

    /* right motor handle */
    bdc_motor_handle_t right_motor = NULL;
    bdc_motor_config_t right_motor_config = {
        .pwm_freq_hz = BDC_MCPWM_FREQ_HZ,
        .pwma_gpio_num = BDC_MCPWM_GPIO_A,
        .pwmb_gpio_num = BDC_MCPWM_GPIO_B,
    };
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&right_motor_config, &mcpwm_config, &right_motor));
    bdc_motor_set_speed(right_motor, (uint32_t)BDC_INIT_SPEED);
    ESP_LOGI(TAG, "Enable right_motor");
    ESP_ERROR_CHECK(bdc_motor_enable(right_motor));

    /* left motor handle */
    bdc_motor_handle_t left_motor = NULL;
    bdc_motor_config_t left_motor_config = {
        .pwm_freq_hz = BDC_MCPWM_FREQ_HZ,
        .pwma_gpio_num = BDC_MCPWM_GPIO_C,
        .pwmb_gpio_num = BDC_MCPWM_GPIO_D,
    };
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&left_motor_config, &mcpwm_config, &left_motor));
    bdc_motor_set_speed(left_motor, (uint32_t)BDC_INIT_SPEED);
    ESP_LOGI(TAG, "Enable left_motor");
    ESP_ERROR_CHECK(bdc_motor_enable(left_motor));

    while (1) {
        /* if it looks like it isn't moving, set speed higher. It's a PWM thing */
        uint16_t speed = BDC_MIN_SPEED;

        uint32_t drive_time_ms = 500;

        explorer_set_speed(left_motor, right_motor, speed);
        explorer_set_forward(left_motor, right_motor);
        explorer_drive_ms(left_motor, right_motor, drive_time_ms);
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* run tasks in here - avoid writing blocking code */
void app_main(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    ESP_LOGI(TAG, "ESP_MQTT");
    mqtt_app_start();

    ESP_LOGI(TAG, "Running task...");
    xTaskCreate(mcpwm_bdc, "mcpwm_bdc", 4096, NULL, 5, NULL);
}
