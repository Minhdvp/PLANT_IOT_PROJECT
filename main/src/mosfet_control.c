#include "mosfet_control.h"
#include "driver/gpio.h"

#define MOSFET_GPIO GPIO_NUM_32 // Chọn chân phù hợp với mạch của bạn

void mosfet_init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOSFET_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
    pump_off(); // Tắt bơm ban đầu
}

void pump_on()
{
    gpio_set_level(MOSFET_GPIO, 0);
}

void pump_off()
{
    gpio_set_level(MOSFET_GPIO, 1);
}