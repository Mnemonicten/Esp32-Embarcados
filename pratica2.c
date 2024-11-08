#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_OUTPUT_LED    2  // LED azul no GPIO2
#define GPIO_INPUT_BTN_0   21 // Botão 0 no GPIO21
#define GPIO_INPUT_BTN_1   22 // Botão 1 no GPIO22
#define GPIO_INPUT_BTN_2   23 // Botão 2 no GPIO23
#define ESP_INTR_FLAG_DEFAULT 0

static const char *TAG = "Pratica2"; // Identificador para o log
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg) {
    uint32_t io_num;
    int led_state = 0;

    for(;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            switch (io_num) {
                case GPIO_INPUT_BTN_0:
                    ESP_LOGI(TAG, "Botão 0 pressionado: LED ACESO");
                    gpio_set_level(GPIO_OUTPUT_LED, 1);
                    break;
                case GPIO_INPUT_BTN_1:
                    ESP_LOGI(TAG, "Botão 1 pressionado: LED APAGADO");
                    gpio_set_level(GPIO_OUTPUT_LED, 0);
                    break;
                case GPIO_INPUT_BTN_2:
                    led_state = !led_state;
                    ESP_LOGI(TAG, "Botão 2 pressionado: LED estado invertido para %d", led_state);
                    gpio_set_level(GPIO_OUTPUT_LED, led_state);
                    break;
                default:
                    break;
            }
        }
    }
}

void app_main(void) {
    // Configuração do LED como saída
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_OUTPUT_LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configuração dos botões como entrada com PULL UP e interrupção
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = ((1ULL << GPIO_INPUT_BTN_0) | (1ULL << GPIO_INPUT_BTN_1) | (1ULL << GPIO_INPUT_BTN_2));
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // Criar uma fila para gerenciar eventos GPIO
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // Instalar o serviço de interrupção GPIO
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_BTN_0, gpio_isr_handler, (void*) GPIO_INPUT_BTN_0);
    gpio_isr_handler_add(GPIO_INPUT_BTN_1, gpio_isr_handler, (void*) GPIO_INPUT_BTN_1);
    gpio_isr_handler_add(GPIO_INPUT_BTN_2, gpio_isr_handler, (void*) GPIO_INPUT_BTN_2);

    ESP_LOGI(TAG, "Configuração inicializada. Aguardando pressionamento de botões...");
}
