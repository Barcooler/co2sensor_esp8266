// Based on https://github.com/fe-c/MT8060-data-read code
// All rights for reading code owned https://geektimes.ru/users/fedorro/
// and https://github.com/revspace

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    HUMIDITY    = 0x41,
    TEMPERATURE = 0x42,
    CO2_PPM     = 0x50,
} dataType;

typedef struct {
    dataType type;
    uint16_t value;
    bool checksumIsValid;
} mt8060_message;

mt8060_message* mt8060_process(unsigned long ms, bool data);

