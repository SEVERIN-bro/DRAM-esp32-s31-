#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPENDRAM_WORD_COUNT 16384U
#define OPENDRAM_ADDR_MASK  0x3FFFU

typedef struct {
    uint8_t addr_gpio[14];
    uint8_t tx_gpio[16];
    uint8_t rx_gpio[16];
    uint8_t clk_gpio;
} opendram_bus_config_t;

esp_err_t opendram_bus_init(const opendram_bus_config_t *cfg);

esp_err_t opendram_bus_transfer(
    uint16_t address,
    uint16_t tx_data,
    uint16_t *rx_data
);

uint16_t opendram_bus_read(uint16_t address);

void opendram_bus_write(uint16_t address, uint16_t value);

#ifdef __cplusplus
}
#endif
