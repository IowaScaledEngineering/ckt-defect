#pragma once
#include <cstdint>
#include <cstddef>

typedef int esp_err_t;
#define ESP_OK 0

struct nvs_stats_t {
    size_t used_entries;
    size_t free_entries;
    size_t total_entries;
    size_t namespace_count;
};

inline esp_err_t nvs_get_stats(const char*, nvs_stats_t* stats) {
    if (stats) {
        stats->used_entries = 0;
        stats->free_entries = 0;
        stats->total_entries = 0;
        stats->namespace_count = 0;
    }
    return ESP_OK;
}
