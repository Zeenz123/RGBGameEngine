#ifndef PIXELCONVERTER_H
#define PIXELCONVERTER_H

#include <Arduino.h>
#include "Color.h"

class PixelConverter
{
public:

    static uint16_t toRGB565(const Color &color);

};

#endif