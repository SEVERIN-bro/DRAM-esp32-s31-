#include "opendram_bus.h"

#include <string.h>
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "opendram_bus";
static opendram_bus_config_t g_cfg;
static bool g_initialized;

static void set_address(uint16_t value)
{
    for (int bit = 0; bit < OPENDRAM_ROW_BITS; ++bit) {
        gpio_set_level(g_cfg.addr_gpio[bit], (value >> bit) & 1U);
    }
}

static void set_dq_output(uint16_t value)
{
    for (int bit = 0; bit < 16; ++bit) {
        gpio_set_level(g_cfg.dq_gpio[bit], (value >> bit) & 1U);
    }
}

static uint16_t get_dq_input(void)
{
    uint16_t value = 0;

    for (int bit = 0; bit < 16; ++bit) {
        if (gpio_get_level(g_cfg.dq_gpio[bit])) {
            value |= (uint16_t)(1U << bit);
        }
    }

    return value;
}

static void set_dq_direction(gpio_mode_t mode)
{
    for (int bit = 0; bit < 16; ++bit) {
        gpio_set_direction(g_cfg.dq_gpio[bit], mode);
    }
}

static void pulse_clock(void)
{
    gpio_set_level(g_cfg.clk_gpio, 1);
    gpio_set_level(g_cfg.clk_gpio, 0);
}

static esp_err_t check_ready(void)
{
    return g_initialized ? ESP_OK : ESP_ERR_INVALID_STATE;
}

static void begin_cycle(void)
{
    gpio_set_level(g_cfg.cs_gpio, 0);
    gpio_set_level(g_cfg.ras_gpio, 0);
    gpio_set_level(g_cfg.cas_gpio, 0);
    gpio_set_level(g_cfg.we_gpio, 0);
}

static void end_cycle(void)
{
    gpio_set_level(g_cfg.ras_gpio, 0);
    gpio_set_level(g_cfg.cas_gpio, 0);
    gpio_set_level(g_cfg.we_gpio, 0);
    gpio_set_level(g_cfg.cs_gpio, 1);
    set_dq_direction(GPIO_MODE_INPUT);
}

static void activate_row(uint16_t row)
{
    set_address(row);
    gpio_set_level(g_cfg.ras_gpio, 1);
    gpio_set_level(g_cfg.cas_gpio, 0);
    gpio_set_level(g_cfg.we_gpio, 0);
    pulse_clock();
    gpio_set_level(g_cfg.ras_gpio, 0);
}

esp_err_t opendram_bus_init(const opendram_bus_config_t *cfg)
{
    ESP_RETURN_ON_FALSE(cfg != NULL, ESP_ERR_INVALID_ARG, TAG, "cfg is NULL");

    memcpy(&g_cfg, cfg, sizeof(g_cfg));

    for (int i = 0; i < 14; ++i) {
        gpio_config_t io = {
            .pin_bit_mask = 1ULL << g_cfg.addr_gpio[i],
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "address GPIO setup failed");
        gpio_set_level(g_cfg.addr_gpio[i], 0);
    }

    for (int i = 0; i < 16; ++i) {
        gpio_config_t io = {
            .pin_bit_mask = 1ULL << g_cfg.dq_gpio[i],
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "DQ GPIO setup failed");
    }

    const gpio_num_t control[] = {
        g_cfg.clk_gpio, g_cfg.cs_gpio, g_cfg.ras_gpio,
        g_cfg.cas_gpio, g_cfg.we_gpio
    };

    for (size_t i = 0; i < sizeof(control) / sizeof(control[0]); ++i) {
        gpio_config_t io = {
            .pin_bit_mask = 1ULL << control[i],
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "control GPIO setup failed");
        gpio_set_level(control[i], 0);
    }

    gpio_set_level(g_cfg.cs_gpio, 1);
    g_initialized = true;
    return ESP_OK;
}

esp_err_t opendram_bus_write16(uint32_t address, uint16_t value)
{
    ESP_RETURN_ON_ERROR(check_ready(), TAG, "bus is not initialized");
    ESP_RETURN_ON_FALSE(address <= OPENDRAM_MAX_ADDRESS, ESP_ERR_INVALID_ARG, TAG, "address out of range");

    uint16_t row;
    uint16_t column;
    opendram_decode_address(address, &row, &column);

    begin_cycle();
    activate_row(row);

    set_address(column);
    set_dq_direction(GPIO_MODE_OUTPUT);
    set_dq_output(value);
    gpio_set_level(g_cfg.cas_gpio, 1);
    gpio_set_level(g_cfg.we_gpio, 1);
    pulse_clock();

    end_cycle();
    return ESP_OK;
}

esp_err_t opendram_bus_read16(uint32_t address, uint16_t *value)
{
    ESP_RETURN_ON_ERROR(check_ready(), TAG, "bus is not initialized");
    ESP_RETURN_ON_FALSE(value != NULL, ESP_ERR_INVALID_ARG, TAG, "value is NULL");
    ESP_RETURN_ON_FALSE(address <= OPENDRAM_MAX_ADDRESS, ESP_ERR_INVALID_ARG, TAG, "address out of range");

    uint16_t row;
    uint16_t column;
    opendram_decode_address(address, &row, &column);

    begin_cycle();
    activate_row(row);

    set_address(column);
    set_dq_direction(GPIO_MODE_INPUT);
    gpio_set_level(g_cfg.cas_gpio, 1);
    gpio_set_level(g_cfg.we_gpio, 0);
    pulse_clock();
    *value = get_dq_input();

    end_cycle();
    return ESP_OK;
}

uint32_t opendram_make_address(uint16_t row, uint16_t column)
{
    if (row >= (1U << OPENDRAM_ROW_BITS) ||
        column >= (1U << OPENDRAM_COL_BITS)) {
        return OPENDRAM_MAX_ADDRESS + 1U;
    }

    return ((uint32_t)row << OPENDRAM_COL_BITS) | column;
}

void opendram_decode_address(uint32_t address, uint16_t *row, uint16_t *column)
{
    if (row != NULL) {
        *row = (uint16_t)(address >> OPENDRAM_COL_BITS);
    }
    if (column != NULL) {
        *column = (uint16_t)(address & ((1U << OPENDRAM_COL_BITS) - 1U));
    }
}
