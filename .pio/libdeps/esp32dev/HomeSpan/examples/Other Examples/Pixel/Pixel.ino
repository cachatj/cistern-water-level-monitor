/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2022 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
// HomeSpan Addressable RGB LED Examples.  Demonstrates use of:
//
//  * HomeSpan Pixel Class that provides for control of single-wire addressable RGB RGBW, and RGBWC LEDs using the NeoPixel protocol, such as the WS2812 and SK6812
//  * HomeSpan Dot Class that provides for control of two-wire addressable RGB LEDs using the DotStar protocol, such as the APA102 and SK9822
//  * HomeSpan WS2801_LED Class that provides for control of two-wire addressable RGB LEDs using the WS2801 protocol
//

#ifndef HS_FEATHER_PINS

  #define NEOPIXEL_RGB_PIN       26         // CHANGE THESE PINS TO MATCH YOUR BOARD
  #define NEOPIXEL_RGBW_PIN      32
  #define DOTSTAR_DATA_PIN       33
  #define DOTSTAR_CLOCK_PIN      27
  #define WS2801_DATA_PIN        14
  #define WS2801_CLOCK_PIN       22
  
#else

  #define NEOPIXEL_RGB_PIN       F26        // FEATHER PIN MAPPING - FOR HOMESPAN DEVELOPER USE ONLY
  #define NEOPIXEL_RGBW_PIN      F32
  #define DOTSTAR_DATA_PIN       F33
  #define DOTSTAR_CLOCK_PIN      F27
  #define WS2801_DATA_PIN        F14
  #define WS2801_CLOCK_PIN       F22
  
#endif

#include "HomeSpan.h"

///////////////////////////////

struct NeoPixel_RGB : Service::LightBulb {      // Addressable single-wire RGB NeoPixel LED Strand
 
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  Pixel *pixel;
  int nPixels;
  
  NeoPixel_RGB(uint8_t pin, int nPixels) : Service::LightBulb(){

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new Pixel(pin);                     // creates Pixel RGB LED on specified pin
    this->nPixels=nPixels;                    // save number of Pixels in this LED Strand
    update();                                 // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();
    
    float h=H.getNewVal<float>();       // range = [0,360]
    float s=S.getNewVal<float>();       // range = [0,100]
    float v=V.getNewVal<float>();       // range = [0,100]

    Pixel::Color color;

    pixel->set(color.HSV(h*p, s*p, v*p),nPixels);       // sets all nPixels to the same HSV color
          
    return(true);  
  }
};

///////////////////////////////

struct NeoPixel_RGBW : Service::LightBulb {      // Addressable single-wire RGBW NeoPixel LED Strand
 
  Characteristic::On power{0,true};
  Characteristic::Brightness V{100,true};
  Characteristic::ColorTemperature T{140,true};
  Pixel *pixel;
  int nPixels;
  
  NeoPixel_RGBW(uint8_t pin, int nPixels) : Service::LightBulb(){

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new Pixel(pin,"GRBW");              // creates Pixel RGBW LED on specified pin (with order of colors changed to reflect this specific NeoPixel device)
    this->nPixels=nPixels;                    // save number of Pixels in this LED Strand
    update();                                 // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();
    
    float v=V.getNewVal<float>();       // range = [0,100]
    float t=T.getNewVal<float>();       // range = [140,500] (140=coldest, 500=warmest)

    float hue=240-(t-140)/3;            // add in a splash of color between blue and green to simulated change of color temperature

    // Pixel::Color color;              // if static HSV method is used (below), there is no need to first create a Color object

    pixel->set(pixel->HSV(hue, 100, v*p, v*p),nPixels);       // sets all nPixels to the same HSV color (note use of static method pixel->HSV, instead of defining and setting Pixel::Color)
          
    return(true);  
  }
};

///////////////////////////////

struct DotStar_RGB : Service::LightBulb {      // Addressable two-wire RGB DotStar LED Strand
 
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  Dot *pixel;
  int nPixels;
  
  DotStar_RGB(uint8_t dataPin, uint8_t clockPin, int nPixels) : Service::LightBulb(){

    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new Dot(dataPin,clockPin);          // creates Dot LED on specified pins
    this->nPixels=nPixels;                    // save number of Pixels in this LED Strand
    update();                                 // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();
    
    float h=H.getNewVal<float>();       // range = [0,360]
    float s=S.getNewVal<float>();       // range = [0,100]
    float v=V.getNewVal<float>();       // range = [0,100]

    Dot::Color color[nPixels];          // create an arrary of Colors

    float hueStep=360.0/nPixels;        // step size for change in hue from one pixel to the next

    for(int i=0;i<nPixels;i++)
      color[i].HSV(h+i*hueStep,s,100,v*p);   // create spectrum of all hues starting with specified Hue; use current-limiting parameter (4th argument) to control overall brightness, instead of PWM
      
    pixel->set(color,nPixels);          // set the colors according to the array
          
    return(true);  
  }
};

///////////////////////////////

struct WS2801_RGB : Service::LightBulb {      // Addressable two-wire RGB WS2801 LED Strand
 
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  WS2801_LED *pixel;
  WS2801_LED::Color *color;
  int nPixels;

  WS2801_RGB(uint8_t dataPin, uint8_t clockPin, int nPixels) : Service::LightBulb(){

    V.setRange(5,100,1);                        // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new WS2801_LED(dataPin, clockPin);    // creates WS2801 RGB LED on specified pins
    this->nPixels=nPixels;                      // save number of Pixels in this LED Strand
    color=WS2801_LED::getMem(nPixels);          // get memory to store nPixels worth of Colors
        
    update();                                   // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();
    
    float h=H.getNewVal<float>();       // range = [0,360]
    float s=S.getNewVal<float>();       // range = [0,100]
    float v=V.getNewVal<float>();       // range = [0,100]

    float hueStep=120.0/nPixels;        // step size for change in hue from one pixel to the next

    for(int i=0;i<nPixels;i++)
      color[i].HSV(h+i*hueStep,s,v*p);  // create spectrum of all hues over 1/3 of color wheel starting with specified Hue

    pixel->set(color,nPixels);          // sets all nPixels to the same HSV color
          
    return(true);  
  }
};

///////////////////////////////

void setup() {
  
  Serial.begin(115200);
 
  homeSpan.begin(Category::Lighting,"Pixel LEDS");

  SPAN_ACCESSORY();                                             // create Bridge (note this sketch uses the SPAN_ACCESSORY() macro, introduced in v1.5.1 --- see the HomeSpan API Reference for details on this convenience macro)

  SPAN_ACCESSORY("Neo RGB");
    new NeoPixel_RGB(NEOPIXEL_RGB_PIN,8);                       // create 8-LED NeoPixel RGB Strand with full color control

  SPAN_ACCESSORY("Neo RGBW");
    new NeoPixel_RGBW(NEOPIXEL_RGBW_PIN,60);                    // create 60-LED NeoPixel RGBW Strand  with simulated color temperature control 

  SPAN_ACCESSORY("Dot RGB");
    new DotStar_RGB(DOTSTAR_DATA_PIN,DOTSTAR_CLOCK_PIN,30);     // create 30-LED DotStar RGB Strand displaying a full spectrum of colors and using the current-limiting feature of DotStars to create flicker-free dimming

  SPAN_ACCESSORY("WS2801 RGB");
    new WS2801_RGB(WS2801_DATA_PIN,WS2801_CLOCK_PIN,25);        // create 25-LED WS2801 RGB Strand displaying a smooth spectrum of colors across 1/3 of the color wheel
}

///////////////////////////////

void loop() {
  homeSpan.poll();
}

///////////////////////////////
