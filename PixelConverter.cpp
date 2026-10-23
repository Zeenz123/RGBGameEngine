#include "PixelConverter.h"

uint16_t PixelConverter::toRGB565(const Color &color)
{
    return color.rgb565();
}