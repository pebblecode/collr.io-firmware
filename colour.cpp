#include <Arduino.h>


typedef union Rgb {
    uint32_t word;
    uint8_t  byte[3];
    struct {
        uint8_t  blue;
        uint8_t  green;
        uint8_t  red;
    } color ;
} Rgb ;

typedef union Hsv {
    uint32_t word;
    uint8_t  byte[3];
} Hsv;

Rgb color(uint8_t r, uint8_t g, uint8_t b)
{
    Rgb result;
    result.color.red = r;
    result.color.red = g;
    result.color.red = b;
    return result;
}

Rgb HsvToRgb(Hsv hsv)
{
    uint8_t h = hsv.byte[0];
    uint8_t s = hsv.byte[1];
    uint8_t v = hsv.byte[2];
    
    uint8_t r,g,b;
    unsigned char region, remainder, p, q, t;

    if (s == 0)
    {
        r = v;
        g = v;
        b = v;
        return color(r,g,b);
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6; 

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            r = v; g = t; b = p;
            break;
        case 1:
            r = q; g = v; b = p;
            break;
        case 2:
            r = p; g = v; b = t;
            break;
        case 3:
            r = p; g = q; b = v;
            break;
        case 4:
            r = t; g = p; b = v;
            break;
        default:
            r = v; g = p; b = q;
            break;
    }

    return color(r,g,b);
}




Hsv RgbToHsv(Rgb rgb)
{
    uint8_t r = rgb.color.red, g = rgb.color.green, b = rgb.color.blue;
    uint8_t h,s,v;
    unsigned char rgbMin, rgbMax;
    Hsv hsv;

    rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
    rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

    v = rgbMax;
    if (v == 0)
    {
        h = 0;
        s = 0;
        hsv.byte[0] = h;
        hsv.byte[1] = s;
        hsv.byte[2] = v;
        
        return hsv;
    }

    s = 255 * long(rgbMax - rgbMin) / v;
    if (s == 0)
    {
        h = 0;
        
        hsv.byte[0] = h;
        hsv.byte[1] = s;
        hsv.byte[2] = v;
        
        return hsv;
    }

    if (rgbMax == r)
        h = 0 + 43 * (g - b) / (rgbMax - rgbMin);
    else if (rgbMax == g)
        h = 85 + 43 * (b - r) / (rgbMax - rgbMin);
    else
        h = 171 + 43 * (r - g) / (rgbMax - rgbMin);

    hsv.byte[0] = h;
    hsv.byte[1] = s;
    hsv.byte[2] = v;
    
    return hsv;
}

uint32_t lerpRgb(uint32_t start, uint32_t end, float amt)
{
  Rgb srgb;
  srgb.word = start;

  Rgb ergb;
  ergb.word = end;
  
  Hsv hsvStart = RgbToHsv(srgb);
  Hsv hsvEnd = RgbToHsv(ergb);

  //hsvStart.byte[0]

  return 0;    
}

template<typename T, typename T2>
T linear(T a, T b, T2 t)
{
    return a * (1 - t) + b * t;
}

uint32_t calculateColor(uint8_t pixelIndex, uint32_t colours[], uint8_t count, int32_t cycleSpeedMs, uint8_t numPixels, bool forward)
{
    uint32_t currentTime = millis();
    int32_t rotationOffset = (forward ? 1 : -1) * numPixels * currentTime / cycleSpeedMs;

//    int32_t rotationOffset = numPixels * (sin((float)currentTime / cycleSpeedMs) + 1) / 2;
    
  //int32_t rotationOffset = numPixels * tri(10*(float)currentTime/cycleSpeedMs) / 0xfff;

    pixelIndex += rotationOffset;
    
    uint8_t currentColour = (count * pixelIndex / numPixels) % count;
    return colours[currentColour];
}

