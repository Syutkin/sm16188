/*--------------------------------------------------------------------------------------
 sm16188.h - Function and support library for the Quang Li 512 LED matrix display based on sm16188 chips
             panel arranged in a 32 x 16 layout.
			 
			 
This Library (sm16188) is a fork of original DMD library
Modified by: Andrey Syutkin  // syutkin@gmail.com
 
 ---
 
 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.
 
 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.
--------------------------------------------------------------------------------------*/

#ifndef SM16188_H_
#define SM16188_H_

#include "Arduino.h"
#ifdef __AVR__
#include <DigitalIO.h>
#endif

//display screen (and subscreen) sizing
#define SM16188_PIXELS_ACROSS 32                           //pixels across x axis (base 2 size expected)
#define SM16188_PIXELS_DOWN 16                             //pixels down y axis
#define SM16188_BITSPERPIXEL 1                             //1 bit per pixel
#define SM16188_HALF_PIXELS_DOWN (SM16188_PIXELS_DOWN / 2) //half pixels down y axis
#define SM16188_RAM_SIZE_BYTES (SM16188_PIXELS_ACROSS * SM16188_PIXELS_DOWN * SM16188_BITSPERPIXEL / 8)

//Pixel/graphics writing modes (bGraphicsMode)
#define GRAPHICS_NORMAL 0
#define GRAPHICS_INVERSE 1
#define GRAPHICS_TOGGLE 2
#define GRAPHICS_OR 3
#define GRAPHICS_NOR 4

// Font Indices
#define FONT_LENGTH 0
#define FONT_FIXED_WIDTH 2
#define FONT_HEIGHT 3
#define FONT_FIRST_CHAR 4
#define FONT_CHAR_COUNT 5
#define FONT_WIDTH_TABLE 6

//drawTestPattern Patterns
#define PATTERN_ALT_0 0
#define PATTERN_ALT_1 1
#define PATTERN_STRIPE_0 2
#define PATTERN_STRIPE_1 3

typedef uint8_t (*FontCallback)(const uint8_t *);

//The main class of SM16188 library functions
template <uint8_t d1, uint8_t d2>
class SM16188
{
public:
    void begin(byte panelsWide, byte panelsHigh)
    {
        _panelsWide = panelsWide;
        _panelsHigh = panelsHigh;
        _brightness = 15;

        panelsTotal = _panelsWide * _panelsHigh;
        bSM16188ScreenRAM = (byte *)malloc(panelsTotal * SM16188_RAM_SIZE_BYTES);

        pinMode(d1, OUTPUT);
        pinMode(d2, OUTPUT);
        digitalWrite(d1, LOW);
        digitalWrite(d2, LOW);

        // fastPinConfig(d1, OUTPUT, LOW);
        // fastPinConfig(d2, OUTPUT, LOW);

        clearScreen(true);
    }

    void end()
    {
        pinMode(d1, INPUT);
        pinMode(d2, INPUT);
        // fastPinMode(d1, INPUT);
        // fastPinMode(d2, INPUT);
    }

    //Set brightness from 0 to 15
    void setBrightness(uint8_t brightness)
    {
        if (brightness > 15)
        {
            _brightness = 15;
        }
        else
        {
            _brightness = brightness;
        }
    }

    //Set or clear a pixel at the x and y location (0,0 is the top left corner)
    void writePixel(unsigned int bX, unsigned int bY, byte bGraphicsMode, byte bPixel)
    {
        noInterrupts();
        unsigned int uiSM16188RAMPointer;

        if (bX >= (SM16188_PIXELS_ACROSS * _panelsWide) || bY >= (SM16188_PIXELS_DOWN * _panelsHigh))
        {
            return;
        }

        uiSM16188RAMPointer = bX * 2 * _panelsHigh + int(bY / SM16188_HALF_PIXELS_DOWN);

        bY = bY - SM16188_HALF_PIXELS_DOWN * int(bY / SM16188_HALF_PIXELS_DOWN);

        switch (bGraphicsMode)
        {
        case GRAPHICS_NORMAL:
            if (bPixel == true)
                bitSet(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // zero bit is pixel off
            else
                bitClear(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // one bit is pixel on
            break;
        case GRAPHICS_INVERSE:
            if (bPixel == false)
                bitSet(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // one bit is pixel on
            else
                bitClear(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // zero bit is pixel off
            break;
        case GRAPHICS_TOGGLE:
            if (bPixel == true)
            {
                if (bitRead(bSM16188ScreenRAM[uiSM16188RAMPointer], bY))
                    bitClear(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // zero bit is pixel off
                else
                    bitSet(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // one bit is pixel on
            }
            break;
        case GRAPHICS_OR:
            //only set pixels on
            if (bPixel == true)
                bitSet(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // one bit is pixel on
            break;
        case GRAPHICS_NOR:
            //only clear on pixels
            if ((bPixel == true) &&
                bitRead(bSM16188ScreenRAM[uiSM16188RAMPointer], bY))
                bitClear(bSM16188ScreenRAM[uiSM16188RAMPointer], bY); // zero bit is pixel off
            break;
        }
        interrupts();
    }

    //Draw a string
    void drawString(int bX, int bY, const char *bChars, byte length, byte bGraphicsMode)
    {
        if (bX >= (SM16188_PIXELS_ACROSS * _panelsWide) || bY >= SM16188_PIXELS_DOWN * _panelsHigh)
            return;
        uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
        if (bY + height < 0)
            return;

        int strWidth = 0;
        this->drawLine(bX - 1, bY, bX - 1, bY + height, GRAPHICS_INVERSE);

        for (int i = 0; i < length; i++)
        {
            int charWide = this->drawChar(bX + strWidth, bY, bChars[i], bGraphicsMode);
            if (charWide > 0)
            {
                strWidth += charWide;
                this->drawLine(bX + strWidth, bY, bX + strWidth, bY + height, GRAPHICS_INVERSE);
                strWidth++;
            }
            else if (charWide < 0)
            {
                return;
            }
            if ((bX + strWidth) >= SM16188_PIXELS_ACROSS * _panelsWide || bY >= SM16188_PIXELS_DOWN * _panelsHigh)
                return;
        }
    }

    //Select a text font
    void selectFont(const uint8_t *font)
    {
        this->Font = font;
    }

    //Draw a single character
    int drawChar(const int bX, const int bY, const unsigned char letter, byte bGraphicsMode)
    {
        if (bX > (SM16188_PIXELS_ACROSS * _panelsWide) || bY > (SM16188_PIXELS_DOWN * _panelsHigh))
            return -1;
        unsigned char c = letter;
        uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
        if (c == ' ')
        {
            int charWide = charWidth(' ');
            this->drawFilledBox(bX, bY, bX + charWide, bY + height, GRAPHICS_INVERSE);
            return charWide;
        }
        uint8_t width = 0;
        uint8_t bytes = (height + 7) / 8;

        uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
        uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

        uint16_t index = 0;

        if (c < firstChar || c >= (firstChar + charCount))
            return 0;
        c -= firstChar;

        if (pgm_read_byte(this->Font + FONT_LENGTH) == 0 && pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0)
        {
            // zero length is flag indicating fixed width font (array does not contain width data entries)
            width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
            index = c * bytes * width + FONT_WIDTH_TABLE;
        }
        else
        {
            // variable width font, read width data, to get the index
            for (uint8_t i = 0; i < c; i++)
            {
                index += pgm_read_byte(this->Font + FONT_WIDTH_TABLE + i);
            }
            index = index * bytes + charCount + FONT_WIDTH_TABLE;
            width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
        }
        if (bX < -width || bY < -height)
            return width;

        // last but not least, draw the character
        for (uint8_t j = 0; j < width; j++)
        { // Width
            for (uint8_t i = bytes - 1; i < 254; i--)
            { // Vertical Bytes
                uint8_t data = pgm_read_byte(this->Font + index + j + (i * width));
                int offset = (i * 8);
                if ((i == bytes - 1) && bytes > 1)
                {
                    offset = height - 8;
                }
                for (uint8_t k = 0; k < 8; k++)
                { // Vertical bits
                    if ((offset + k >= i * 8) && (offset + k <= height))
                    {
                        if (data & (1 << k))
                        {
                            writePixel(bX + j, bY + offset + k, bGraphicsMode, true);
                        }
                        else
                        {
                            writePixel(bX + j, bY + offset + k, bGraphicsMode, false);
                        }
                    }
                }
            }
        }
        return width;
    }

    //Find the width of a character
    int charWidth(const unsigned char letter)
    {
        unsigned char c = letter;
        // Space is often not included in font so use width of 'n'
        if (c == ' ')
            c = 'n';
        uint8_t width = 0;

        uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
        uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

        // uint16_t index = 0;

        if (c < firstChar || c >= (firstChar + charCount))
        {
            return 0;
        }
        c -= firstChar;

        if (pgm_read_byte(this->Font + FONT_LENGTH) == 0 && pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0)
        {
            // zero length is flag indicating fixed width font (array does not contain width data entries)
            width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
        }
        else
        {
            // variable width font, read width data
            width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
        }
        return width;
    }

    //Draw a scrolling string
    void drawMarquee(const char *bChars, byte length, int left, int top)
    {
        marqueeWidth = 0;
        for (int i = 0; i < length; i++)
        {
            marqueeText[i] = bChars[i];
            marqueeWidth += charWidth(bChars[i]) + 1;
        }
        marqueeHeight = pgm_read_byte(this->Font + FONT_HEIGHT);
        marqueeText[length] = '\0';
        marqueeOffsetY = top;
        marqueeOffsetX = left;
        marqueeLength = length;
        drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
                   GRAPHICS_NORMAL);
    }

    //Move the maquee accross by amount
    // boolean stepMarquee(int amountX, int amountY)
    // {
    //     boolean ret = false;
    //     marqueeOffsetX += amountX;
    //     marqueeOffsetY += amountY;
    //     if (marqueeOffsetX < -marqueeWidth)
    //     {
    //         marqueeOffsetX = SM16188_PIXELS_ACROSS * _panelsWide;
    //         clearScreen(true);
    //         ret = true;
    //     }
    //     else if (marqueeOffsetX > SM16188_PIXELS_ACROSS * _panelsWide)
    //     {
    //         marqueeOffsetX = -marqueeWidth;
    //         clearScreen(true);
    //         ret = true;
    //     }

    //     if (marqueeOffsetY < -marqueeHeight)
    //     {
    //         marqueeOffsetY = SM16188_PIXELS_DOWN * _panelsHigh;
    //         clearScreen(true);
    //         ret = true;
    //     }
    //     else if (marqueeOffsetY > SM16188_PIXELS_DOWN * _panelsHigh)
    //     {
    //         marqueeOffsetY = -marqueeHeight;
    //         clearScreen(true);
    //         ret = true;
    //     }

    //     // Special case horizontal scrolling to improve speed
    //     if (amountY == 0 && amountX == -1)
    //     {
    //         // Shift entire screen one bit
    //         for (int i = 1; i < SM16188_RAM_SIZE_BYTES * panelsTotal; i++)
    //         {
    //             if ((i % (_panelsWide * 4)) == (_panelsWide * 4) - 1)
    //             {
    //                 bSM16188ScreenRAM[i] = (bSM16188ScreenRAM[i] << 1) + 1;
    //             }
    //             else
    //             {
    //                 bSM16188ScreenRAM[i] = (bSM16188ScreenRAM[i] << 1) + ((bSM16188ScreenRAM[i + 1] & 0x80) >> 7);
    //             }
    //         }

    //         // Redraw last char on screen
    //         int strWidth = marqueeOffsetX;
    //         for (byte i = 0; i < marqueeLength; i++)
    //         {
    //             int wide = charWidth(marqueeText[i]);
    //             if (strWidth + wide >= _panelsWide * SM16188_PIXELS_ACROSS)
    //             {
    //                 drawChar(strWidth, marqueeOffsetY, marqueeText[i], GRAPHICS_NORMAL);
    //                 return ret;
    //             }
    //             strWidth += wide + 1;
    //         }
    //     }
    //     else if (amountY == 0 && amountX == 1)
    //     {
    //         // Shift entire screen one bit
    //         for (int i = (SM16188_RAM_SIZE_BYTES * panelsTotal) - 1; i >= 0; i--)
    //         {
    //             if ((i % (_panelsWide * 4)) == 0)
    //             {
    //                 bSM16188ScreenRAM[i] = (bSM16188ScreenRAM[i] >> 1) + 128;
    //             }
    //             else
    //             {
    //                 bSM16188ScreenRAM[i] = (bSM16188ScreenRAM[i] >> 1) + ((bSM16188ScreenRAM[i - 1] & 1) << 7);
    //             }
    //         }

    //         // Redraw last char on screen
    //         int strWidth = marqueeOffsetX;
    //         for (byte i = 0; i < marqueeLength; i++)
    //         {
    //             int wide = charWidth(marqueeText[i]);
    //             if (strWidth + wide >= 0)
    //             {
    //                 drawChar(strWidth, marqueeOffsetY, marqueeText[i], GRAPHICS_NORMAL);
    //                 return ret;
    //             }
    //             strWidth += wide + 1;
    //         }
    //     }
    //     else
    //     {
    //         drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
    //                    GRAPHICS_NORMAL);
    //     }

    //     return ret;
    // }

    //Clear the screen in DMD RAM
    void clearScreen(byte bNormal)
    {
        if (bNormal) // clear all pixels
            memset(bSM16188ScreenRAM, 0, SM16188_RAM_SIZE_BYTES * panelsTotal);
        else // set all pixels
            memset(bSM16188ScreenRAM, 255, SM16188_RAM_SIZE_BYTES * panelsTotal);
    }

    //Draw or clear a line from x1,y1 to x2,y2
    void drawLine(int x1, int y1, int x2, int y2, byte bGraphicsMode)
    {
        int dy = y2 - y1;
        int dx = x2 - x1;
        int stepx, stepy;

        if (dy < 0)
        {
            dy = -dy;
            stepy = -1;
        }
        else
        {
            stepy = 1;
        }
        if (dx < 0)
        {
            dx = -dx;
            stepx = -1;
        }
        else
        {
            stepx = 1;
        }
        dy <<= 1; // dy is now 2*dy
        dx <<= 1; // dx is now 2*dx

        writePixel(x1, y1, bGraphicsMode, true);
        if (dx > dy)
        {
            int fraction = dy - (dx >> 1); // same as 2*dy - dx
            while (x1 != x2)
            {
                if (fraction >= 0)
                {
                    y1 += stepy;
                    fraction -= dx; // same as fraction -= 2*dx
                }
                x1 += stepx;
                fraction += dy; // same as fraction -= 2*dy
                writePixel(x1, y1, bGraphicsMode, true);
            }
        }
        else
        {
            int fraction = dx - (dy >> 1);
            while (y1 != y2)
            {
                if (fraction >= 0)
                {
                    x1 += stepx;
                    fraction -= dy;
                }
                y1 += stepy;
                fraction += dx;
                writePixel(x1, y1, bGraphicsMode, true);
            }
        }
    }

    //Draw or clear a circle of radius r at x,y centre
    void drawCircle(int xCenter, int yCenter, int radius, byte bGraphicsMode)
    {
        int x = 0;
        int y = radius;
        int p = (5 - radius * 4) / 4;

        drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
        while (x < y)
        {
            x++;
            if (p < 0)
            {
                p += 2 * x + 1;
            }
            else
            {
                y--;
                p += 2 * (x - y) + 1;
            }
            drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
        }
    }

    //Draw or clear a box(rectangle) with a single pixel border
    void drawBox(int x1, int y1, int x2, int y2, byte bGraphicsMode)
    {
        drawLine(x1, y1, x2, y1, bGraphicsMode);
        drawLine(x2, y1, x2, y2, bGraphicsMode);
        drawLine(x2, y2, x1, y2, bGraphicsMode);
        drawLine(x1, y2, x1, y1, bGraphicsMode);
    }

    //Draw or clear a filled box(rectangle) with a single pixel border
    void drawFilledBox(int x1, int y1, int x2, int y2, byte bGraphicsMode)
    {
        for (int b = x1; b <= x2; b++)
        {
            drawLine(b, y1, b, y2, bGraphicsMode);
        }
    }

    //Draw the selected test pattern
    void drawTestPattern(byte bPattern)
    {
        unsigned int ui;

        int numPixels = panelsTotal * SM16188_PIXELS_ACROSS * SM16188_PIXELS_DOWN;
        int pixelsWide = SM16188_PIXELS_ACROSS * _panelsWide;
        for (ui = 0; ui < numPixels; ui++)
        {
            switch (bPattern)
            {
            case PATTERN_ALT_0: // every alternate pixel, first pixel on
                if ((ui & pixelsWide) == 0)
                    //even row
                    writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
                else
                    //odd row
                    writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
                break;
            case PATTERN_ALT_1: // every alternate pixel, first pixel off
                if ((ui & pixelsWide) == 0)
                    //even row
                    writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
                else
                    //odd row
                    writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
                break;
            case PATTERN_STRIPE_0: // vertical stripes, first stripe on
                writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
                break;
            case PATTERN_STRIPE_1: // vertical stripes, first stripe off
                writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
                break;
            }
        }
    }

    // Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
    void updateScreen()
    {
        for (int i = SM16188_PIXELS_ACROSS * _panelsWide * 2 - 1; i >= 0; i -= 2)
        {
            transfer(bSM16188ScreenRAM[i], d2);
        }
        transferBrightness(_brightness, d2);
        for (int i = SM16188_PIXELS_ACROSS * _panelsWide * 2 - 2; i >= 0; i -= 2)
        {
            transfer(bSM16188ScreenRAM[i], d1);
        }
        transferBrightness(_brightness, d1);
    }

private:
#ifdef __AVR__
    inline __attribute__((always_inline)) void transfer(byte val, uint8_t pin)
    {
        noInterrupts();
        switch (pin)
        {
        case d1:
            for (int i = 7; i >= 0; i--)
            {
                if (bitRead(val, i))
                {
                    delay = micros();
                    fastDigitalWrite(d1, HIGH);
                    while (micros() - delay < 3)
                    {
                    }
                    fastDigitalWrite(d1, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
                else
                {
                    delay = micros();
                    fastDigitalWrite(d1, HIGH);
                    // while (micros() - delay < 1)
                    // {
                    // }
                    fastDigitalWrite(d1, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
            }
            break;
        case d2:
            for (int i = 7; i >= 0; i--)
            {
                if (bitRead(val, i))
                {
                    delay = micros();
                    fastDigitalWrite(d2, HIGH);
                    while (micros() - delay < 3)
                    {
                    }
                    fastDigitalWrite(d2, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
                else
                {
                    delay = micros();
                    fastDigitalWrite(d2, HIGH);
                    // while (micros() - delay < 1)
                    // {
                    // }
                    fastDigitalWrite(d2, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
            }
            break;
        }
        interrupts();
    }

    inline __attribute__((always_inline)) void transferBrightness(byte val, uint8_t pin)
    {
        noInterrupts();
        switch (pin)
        {
        case d1:
            for (int i = 3; i >= 0; i--)
            {
                if (bitRead(val, i))
                {
                    delay = micros();
                    fastDigitalWrite(d1, HIGH);
                    while (micros() - delay < 3)
                    {
                    }
                    fastDigitalWrite(d1, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
                else
                {
                    delay = micros();
                    fastDigitalWrite(d1, HIGH);
                    // while (micros() - delay < 1)
                    // {
                    // }
                    fastDigitalWrite(d1, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
            }
            break;
        case d2:
            for (int i = 3; i >= 0; i--)
            {
                if (bitRead(val, i))
                {
                    delay = micros();
                    fastDigitalWrite(d2, HIGH);
                    while (micros() - delay < 3)
                    {
                    }
                    fastDigitalWrite(d2, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
                else
                {
                    delay = micros();
                    fastDigitalWrite(d2, HIGH);
                    // while (micros() - delay < 1)
                    // {
                    // }
                    fastDigitalWrite(d2, LOW);
                    while (micros() - delay < 4)
                    {
                    }
                }
            }
            break;
        }
        interrupts();
    }

#elif defined(ESP32)
    inline __attribute__((always_inline)) void transfer(byte val, uint8_t pin)
    {
        noInterrupts();
        for (int i = 7; i >= 0; i--)
        {
            if (bitRead(val, i))
            {
                delay = micros();
                digitalWrite(pin, HIGH);
                while (micros() - delay < 3)
                {
                }
                digitalWrite(pin, LOW);
                while (micros() - delay < 4)
                {
                }
            }
            else
            {
                delay = micros();
                digitalWrite(pin, HIGH);
                // while (micros() - delay < 1)
                // {
                // }
                digitalWrite(pin, LOW);
                while (micros() - delay < 4)
                {
                }
            }
        }
        interrupts();
    }

    inline __attribute__((always_inline)) void transferBrightness(byte val, uint8_t pin)
    {
        noInterrupts();
        for (int i = 3; i >= 0; i--)
        {
            if (bitRead(val, i))
            {
                delay = micros();
                digitalWrite(pin, HIGH);
                while (micros() - delay < 3)
                {
                }
                digitalWrite(pin, LOW);
                while (micros() - delay < 4)
                {
                }
            }
            else
            {
                delay = micros();
                digitalWrite(pin, HIGH);
                // while (micros() - delay < 1)
                // {
                // }
                digitalWrite(pin, LOW);
                while (micros() - delay < 4)
                {
                }
            }
        }
        interrupts();
    }
#endif

    void
    drawCircleSub(int cx, int cy, int x, int y, byte bGraphicsMode)
    {

        if (x == 0)
        {
            writePixel(cx, cy + y, bGraphicsMode, true);
            writePixel(cx, cy - y, bGraphicsMode, true);
            writePixel(cx + y, cy, bGraphicsMode, true);
            writePixel(cx - y, cy, bGraphicsMode, true);
        }
        else if (x == y)
        {
            writePixel(cx + x, cy + y, bGraphicsMode, true);
            writePixel(cx - x, cy + y, bGraphicsMode, true);
            writePixel(cx + x, cy - y, bGraphicsMode, true);
            writePixel(cx - x, cy - y, bGraphicsMode, true);
        }
        else if (x < y)
        {
            writePixel(cx + x, cy + y, bGraphicsMode, true);
            writePixel(cx - x, cy + y, bGraphicsMode, true);
            writePixel(cx + x, cy - y, bGraphicsMode, true);
            writePixel(cx - x, cy - y, bGraphicsMode, true);
            writePixel(cx + y, cy + x, bGraphicsMode, true);
            writePixel(cx - y, cy + x, bGraphicsMode, true);
            writePixel(cx + y, cy - x, bGraphicsMode, true);
            writePixel(cx - y, cy - x, bGraphicsMode, true);
        }
    }

private:
    unsigned long delay;
    byte _panelsWide;
    byte _panelsHigh;
    byte _brightness;
    byte panelsTotal;

    //Pointer to current font
    const uint8_t *Font;

    //Marquee values
    char marqueeText[256];
    byte marqueeLength;
    int marqueeWidth;
    int marqueeHeight;
    int marqueeOffsetX;
    int marqueeOffsetY;

    //Mirror of SM16188 pixels in RAM, ready to be clocked out by the main loop or high speed timer calls
    byte *bSM16188ScreenRAM;
};

#endif /* SM16188_H_ */
