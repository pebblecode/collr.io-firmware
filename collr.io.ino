/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
  #include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#include <Adafruit_NeoPixel.h>

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    PIN                       Which pin on the Arduino is connected to the NeoPixels?
    NUMPIXELS                 How many NeoPixels are attached to the Arduino?
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE     1

    #define PIN                     6
    #define NUMPIXELS               31
/*=========================================================================*/

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN);

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];


/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  while (!Serial);  // required for Flora & Micro
  //delay(500);

  Serial.println(F("Init"));

  // turn off neopixel
  pixel.begin(); // This initializes the NeoPixel library.
  for(uint8_t i=0; i<NUMPIXELS; i++) {
    pixel.setPixelColor(i, pixel.Color(20,0,20)); // off
  }
  pixel.show();

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Neopixel Color Picker Example"));
  Serial.println(F("------------------------------------------------"));

  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  while (! ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("***********************"));

  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("***********************"));
}

/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/

uint32_t calculateColor(uint8_t pixelIndex, uint32_t colours[], 
uint8_t count, int32_t cycleSpeedMs, uint8_t numPixels, bool forward);


bool solidColor = true;
uint32_t currentColor = 0x00444400;
char lastButton = '1';

uint8_t brightness = 200;

void loop(void)
{
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, 10);// BLE_READPACKET_TIMEOUT);
  if (len != 0) {
      // Color
    if (packetbuffer[1] == 'C') {
      uint8_t red = packetbuffer[2];
      uint8_t green = packetbuffer[3];
      uint8_t blue = packetbuffer[4];
      Serial.print ("RGB #");
      if (red < 0x10) Serial.print("0");
      Serial.print(red, HEX);
      if (green < 0x10) Serial.print("0");
      Serial.print(green, HEX);
      if (blue < 0x10) Serial.print("0");
      Serial.println(blue, HEX);
      solidColor = true;

      currentColor = pixel.Color(red, green, blue);

      Serial.print("Color Picker: ");
      printHex(packetbuffer, 6);
    } // Control pad
    else if (packetbuffer[1] == 'B') {
      Serial.print("Control Pad: ");


      
      printHex(packetbuffer, 5);
      solidColor = false;
      uint8_t buttnum = packetbuffer[2] - '0';
      boolean pressed = packetbuffer[3] - '0';

      //if (pressed) {
        Serial.println(buttnum);
        lastButton = packetbuffer[2];
      //}
    }
  }

  if (solidColor)
  {
    for(uint8_t i=0; i<NUMPIXELS; i++) {
      pixel.setPixelColor(i, currentColor);
    }
  }
  else 
  {
    if (lastButton == '2')
    {
        uint32_t colours[] = { pixel.Color(brightness,0,0), pixel.Color(0,0,0),
            pixel.Color(0,0,0), pixel.Color(0,0,0), pixel.Color(0,0,0), };
        for(uint8_t i=0; i<NUMPIXELS; i++) {
            uint32_t rgb = calculateColor(i, colours, 5, 1000, NUMPIXELS, false);
            pixel.setPixelColor(i, rgb);
        }
    }
    else if (lastButton == '1')
    {
        uint32_t colours[] = { pixel.Color(brightness,0,0), pixel.Color(brightness,brightness,brightness), pixel.Color(0,0,brightness) };
        for(uint8_t i=0; i<NUMPIXELS; i++) {
            uint32_t rgb = calculateColor(i, colours, 3, 2000, NUMPIXELS, true);
            pixel.setPixelColor(i, rgb);
        }
    }
    else if (lastButton == '3')
    {
        uint32_t colours[] = { pixel.Color(brightness,0,0), pixel.Color(brightness,brightness,0), pixel.Color(0,brightness,0),
            pixel.Color(0,brightness,brightness), pixel.Color(0,0,brightness), pixel.Color(brightness,0,brightness),
        };
        for(uint8_t i=0; i<NUMPIXELS; i++) {
            uint32_t rgb = calculateColor(i, colours, 6, 500, NUMPIXELS, false);
            pixel.setPixelColor(i, rgb);
        }
        
    }
    else if (lastButton == '4')
    {
        uint32_t colours[] = { pixel.Color(brightness,0,0), pixel.Color(0,brightness,0) };
        for(uint8_t i=0; i<NUMPIXELS; i++) {
            uint32_t rgb = calculateColor(i, colours, 2, 700, NUMPIXELS, true);
            pixel.setPixelColor(i, rgb);
        }
    }         
  }
  pixel.show();
}
