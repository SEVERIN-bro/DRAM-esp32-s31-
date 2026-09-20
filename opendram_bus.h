#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPENDRAM_ROW_BITS     14
#define OPENDRAM_COL_BITS     13
#define OPENDRAM_WORD_BITS    27
#define OPENDRAM_MAX_ADDRESS  ((1UL << OPENDRAM_WORD_BITS) - 1)
#define OPENDRAM_WORD_COUNT   (1UL << OPENDRAM_WORD_BITS)

typedef struct {
    uint8_t addr_gpio[14];
    uint8_t dq_gpio[16];
    uint8_t clk_gpio;
    uint8_t cs_gpio;
    uint8_t ras_gpio;
    uint8_t cas_gpio;
    uint8_t we_gpio;
} opendram_bus_config_t;

esp_err_t opendram_bus_init(const opendram_bus_config_t *cfg);
esp_err_t opendram_bus_write16(uint32_t address, uint16_t value);
esp_err_t opendram_bus_read16(uint32_t address, uint16_t *value);
uint32_t opendram_make_address(uint16_t row, uint16_t column);
void opendram_decode_address(uint32_t address, uint16_t *row, uint16_t *column);

#ifdef __cplusplus
}
#endif
