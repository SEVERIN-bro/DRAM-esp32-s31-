#include "opendram_bus.h"
#include "esp_log.h"

static const char *TAG = "app";

void app_main(void)
{
    const opendram_bus_config_t cfg = {
        .addr_gpio = { /* Set ADDR[13:0] GPIOs for your ESP32-S31 board. */ },
        .dq_gpio = { /* Set shared DQ[15:0] GPIOs for your ESP32-S31 board. */ },
        .clk_gpio = 0,
        .cs_gpio = 0,
        .ras_gpio = 0,
        .cas_gpio = 0,
        .we_gpio = 0
    };

    ESP_ERROR_CHECK(opendram_bus_init(&cfg));

    const uint32_t address = opendram_make_address(0, 0);
    ESP_ERROR_CHECK(opendram_bus_write16(address, 0xABCD));

    uint16_t value = 0;
    ESP_ERROR_CHECK(opendram_bus_read16(address, &value));
    ESP_LOGI(TAG, "address=0x%07lx value=0x%04X",
             (unsigned long)address, value);
}
