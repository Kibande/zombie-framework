
#include <zshared/crc16_ccitt.hpp>

namespace zshared
{
    static uint16_t crc_ccitt_update (uint16_t crc, uint8_t data)
    {
        data ^= crc & 0xFF;
        data ^= data << 4;

        return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
    }

    uint16_t crc16_ccitt(const uint8_t* data, size_t length)
    {
        uint16_t crc = 0xFFFF;

        while (length--)
            crc = crc_ccitt_update(crc, *data++);

        return crc;
    }
}
