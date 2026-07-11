#include <cstdint>
void bootloader_random_enable(void) {}
void bootloader_random_disable(void) {}
uint32_t esp_random(void)
{
    return 0;
}
