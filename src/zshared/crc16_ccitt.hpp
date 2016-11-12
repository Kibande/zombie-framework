#pragma once

#include <littl/Base.hpp>

namespace zshared
{
    uint16_t crc16_ccitt(const uint8_t* data, size_t length);
}
