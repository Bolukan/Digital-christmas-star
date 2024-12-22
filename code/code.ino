#define FASTLED_ALLOW_INTERRUPTS 0
#include <arduino.h>
#include <FastLED.h>

constexpr static const bool DEBUG = true;  /// en-/disable debug output

constexpr static const uint8_t DATA_PIN = 19;

constexpr static const uint16_t NUM_FINS = 5;
constexpr static const uint16_t NUM_LEDS_SPOKE = 9;
constexpr static const uint16_t NUM_LEDS_TIP = 1;
constexpr static const uint16_t NUM_LEDS_PIT = 1;

constexpr static const uint16_t NUM_LEDS_IN_RAY = NUM_LEDS_PIT + NUM_LEDS_SPOKE + NUM_LEDS_TIP;
constexpr static const uint16_t NUM_LEDS_IN_FIN = NUM_LEDS_SPOKE + NUM_LEDS_TIP + NUM_LEDS_SPOKE;
constexpr static const uint16_t NUM_LEDS_IN_FIN_AND_PIT = NUM_LEDS_IN_FIN + NUM_LEDS_PIT;
constexpr static const uint16_t NUM_LEDS_IN_FIN_AND_PITS =  NUM_LEDS_PIT + NUM_LEDS_IN_FIN + NUM_LEDS_PIT;
constexpr static const uint16_t NUM_LEDS = NUM_FINS * NUM_LEDS_IN_FIN_AND_PIT;
constexpr static const uint16_t LED_OFFSET = 19; // first PIT led

// The time after which to change animation mode, 60000 milliseconds = 1 minute
constexpr static const uint16_t animationTime = 60000;
constexpr static const uint8_t maxMode = 8;  /// The mode count
uint8_t currentMode = 0;          /// The current mode/animation we are in
uint32_t lastAnimationTime = 0;   /// The last time we changed the animation
uint32_t lastAnimationStep = 0;   /// The last time we animated a step 1
uint32_t lastAnimationStep2 = 0;  /// The last time we animated a step 2
uint8_t animationStep = 0;
uint8_t finIndex = 0;

#define BRIGHTNESS 92
#define COLOR_BLACK 0x000000
#define COLOR_ISLAMIC_GREEN 0x00B315
#define COLOR_VENETIAN_RED 0xB3000C
uint8_t hue = 0;                  /// The color of the animation

CRGBArray<NUM_LEDS> leds;  // Our leds

// forward declaration of function
bool animate(const uint32_t currentTime, const uint16_t animationStepTime);
bool animate2(const uint32_t currentTime, const uint16_t animationStepTime);
void lerp8(CPixelView<CRGB>& ledArray, const CHSV& color, const uint8_t fraction);
void lerp8(CRGB& color1, const CRGB& color2, const uint8_t fraction);
constexpr uint8_t convertPercent(const double percentage);
void mirrorFirstFin();
void copyFirstFinToAllFins();
template <typename T>
void logParameter(const String& name, const T& value);

bool circleRainbowAnimation(const uint32_t time);
bool rainbow(const uint32_t time);
bool randomTwinkle(const uint32_t time, const uint32_t color1, const uint32_t color2);
bool outsideRainbow(const uint32_t time);
bool circleAnimation(const uint32_t time, const CHSV& color);
bool staticColor(const CHSV& color);
bool outsideWoosh(const uint32_t time);
bool simpleColorFade(const uint32_t time);
bool staticTwinkle(const uint32_t time, const CHSV& color);

void setup() {
  if (DEBUG) {
    Serial.begin(115200);  // Enable serial output when debugging
  }
  // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PIN>(leds, NUM_LEDS);
  // Limit power to 2 Watts (5V * 0.4A)
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); 
  FastLED.setBrightness(BRIGHTNESS);
  leds.fill_solid(COLOR_BLACK);  // Clear all leds
  FastLED.show();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastAnimationTime > animationTime)  // Change the mode after the given animationTime
  {
    lastAnimationTime = currentMillis;
    ++currentMode;
    hue += 30;
    if (currentMode > maxMode)  // Reset to first mode after last mode
    {
      currentMode = 0;
    }
  }

  logParameter("Time", currentMillis);

  bool show = false;
  // Run the animation associated with the mode
  switch (currentMode) {
    case 0:
      show = circleRainbowAnimation(currentMillis);
      break;
    case 1:
      show = rainbow(currentMillis);
      break;
    case 2:
      show = randomTwinkle(currentMillis, COLOR_ISLAMIC_GREEN, COLOR_VENETIAN_RED);
      break;
    case 3:
      show = outsideRainbow(currentMillis);
      break;
    case 4:
      show = circleAnimation(currentMillis, CHSV(hue, 255, 255));
      break;
    case 5:
      show = staticColor(CHSV(hue, 255, BRIGHTNESS));
      break;
    case 6:
      show = outsideWoosh(currentMillis);
      break;
    case 7:
      show = simpleColorFade(currentMillis);
      break;
    case 8:
      show = staticTwinkle(currentMillis, CHSV(hue, 255, BRIGHTNESS));
      break;
    default:
      currentMode = 0;
  }

  if (show) {
    LEDS.show();  // Show the animation
  }
}

//  0
bool circleRainbowAnimation(const uint32_t time) {
  // Every 60 ms update the color
  if (!animate(time, 60)) {
    return false;
  }

#if defined(NO_TIP_AND_PIT_LEDS)
  // TODO implement
#else
  const uint16_t ledInFinIndex = animationStep % NUM_LEDS_IN_FIN_AND_PITS;
  // 5 fins -> 0 -> 2 -> 4 -> 1 -> 3 -> 0 -> 2 -> 4 -> 1 -> 3 -> 0
  if (ledInFinIndex == 0) {
    finIndex = (finIndex + NUM_FINS / 2) % NUM_FINS;
  }
  const uint16_t ledIndex = ((ledInFinIndex + finIndex * NUM_LEDS_IN_FIN_AND_PIT) + LED_OFFSET) % NUM_LEDS;

  leds.fadeToBlackBy(convertPercent(200.0 / (NUM_LEDS_IN_FIN + 1)));
  leds[ledIndex] = CHSV(hue, 255, 255);
  ++hue;

  ++animationStep;
  if (animationStep >= NUM_LEDS_IN_FIN_AND_PITS * NUM_FINS) {
    animationStep = 0;
  }
#endif
  return true;
}

//  1
bool rainbow(const uint32_t time) {
  // Every 30 ms update the color
  if (!animate(time, 30)) {
    return false;
  }

  ++hue;
  for (uint8_t i = 0; i < NUM_LEDS; ++i) {
    uint16_t offset = (256 * i) / NUM_LEDS;  // Calculate offset with higher precision
    leds[i].setHSV((hue - offset) % 256, 255, BRIGHTNESS);
  }
  return true;
}

//  2
bool randomTwinkle(const uint32_t time, const uint32_t color1, const uint32_t color2) {
  // Every 100 ms fade by 20 and set random leds
  if (!animate(time, 100)) {
    return false;
  }

  leds.fadeToBlackBy(20);
  leds[random8(0, NUM_LEDS)] = color1;
  leds[random8(0, NUM_LEDS)] = color2;
  return true;
}

//  3
bool outsideRainbow(const uint32_t time) {
  // Every 30 ms update the color
  if (!animate(time, 30)) {
    return false;
  }

  ++hue;
  // Fill rainbow between first tips
  for (uint8_t i = 0; i < NUM_LEDS_IN_RAY; ++i) {
    leds[(i + LED_OFFSET) % NUM_LEDS].setHue((hue - (255 / (NUM_LEDS_IN_FIN)) * i) % 256);
  }

  mirrorFirstFin();
  copyFirstFinToAllFins();
  return true;
}

//  4
bool circleAnimation(const uint32_t time, const CHSV& color) {
  // Every 60 ms update the color
  if (!animate(time, 60)) {
    return false;
  }

  const uint16_t ledInFinIndex = animationStep % NUM_LEDS_IN_FIN_AND_PITS;
  // 5 fins -> 0 -> 2 -> 4 -> 1 -> 3 -> 0 -> 2 -> 4 -> 1 -> 3 -> 0
  if (ledInFinIndex == 0) {
    finIndex = (finIndex + NUM_FINS / 2) % NUM_FINS;
  }
  const uint16_t ledIndex = ((ledInFinIndex + finIndex * NUM_LEDS_IN_FIN_AND_PIT) + LED_OFFSET) % NUM_LEDS;

  leds.fadeToBlackBy(convertPercent(200.0 / NUM_LEDS_IN_FIN_AND_PIT));
  leds[ledIndex] = color;

  ++animationStep;
  if (animationStep >= NUM_LEDS_IN_FIN_AND_PITS * NUM_FINS) {
    animationStep = 0;
  }
  return true;
}

//  5
bool staticColor(const CHSV& color) {
  leds.fill_solid(color);
  return true;
}

//  6
bool outsideWoosh(const uint32_t time) {
  static uint8_t animationStep = 0;

  // Every 100 ms update the color
  if (!animate(time, 100)) {
    return false;
  }

  leds.fadeToBlackBy(convertPercent(300.0 / NUM_LEDS_IN_RAY));

  // Don't animate all the time to have time to fade to black
  if (animationStep < NUM_LEDS_IN_RAY) {
    leds[(animationStep + LED_OFFSET) % NUM_LEDS] = CHSV(hue, 255, 255);
    mirrorFirstFin();
    copyFirstFinToAllFins();
    ++hue;
  }

  ++animationStep;
  if (animationStep >= 2 * NUM_LEDS_IN_RAY) {
    animationStep = 0;
  }
  return true;
}

//  7
bool simpleColorFade(const uint32_t time) {
  // Every 30 ms update the color
  if (!animate(time, 30)) {
    return false;
  }

  ++hue;
  staticColor(CHSV(hue, 255, 255));
  return true;
}

//  8
bool staticTwinkle(const uint32_t time, const CHSV& color) {
  bool update = false;
  // Every 50 ms fade to black by 12.5 %
  if (animate2(time, 50)) {
    lerp8(leds, color, convertPercent(12.5));
    update = true;
  }
  // Every 150 ms set random led
  if (animate(time, 150)) {
    leds[random8(0, NUM_LEDS)] = CRGB::White;
    update = true;
  }
  return update;
}

//  reserve
bool weirdAnimation(const uint32_t time) {
  // Every 30 ms update the color
  if (!animate(time, 30)) {
    return false;
  }
  ++hue;

#if defined(NO_TIP_AND_PIT_LEDS)
  // Fill rainbow between first tips
  for (uint8_t i = 0; i < NUM_LEDS_SPOKE; ++i) {
    leds[i].setHue((hue - (255 / NUM_LEDS_SPOKE) * i) % 256);
  }
#else
  // Set tip and inner tip green
  leds[0] = leds[NUM_FINS + 1] = COLOR_ISLAMIC_GREEN;
  // Fill rainbow between first tips
  for (uint8_t i = 0; i < NUM_LEDS_SPOKE; ++i) {
    leds[i + 1].setHue((hue - (255 / NUM_LEDS_SPOKE) * i) % 256);
  }
#endif
  mirrorFirstFin();
  copyFirstFinToAllFins();
  return true;
}

// Converts a given percentage to the FastLED percentage
constexpr uint8_t convertPercent(const double percentage) {
  return percentage * 2.56;
}

// Mirrors the first half of the first fin to the second half
void mirrorFirstFin() {
#if defined(NO_TIP_AND_PIT_LEDS)
  leds(NUM_LEDS_IN_FIN + LED_OFFSET, NUM_LEDS_SPOKE + LED_OFFSET) =
      leds(0 + LED_OFFSET, NUM_LEDS_SPOKE + LED_OFFSET);
#else
  leds(LED_OFFSET + NUM_LEDS_IN_FIN, LED_OFFSET + NUM_LEDS_SPOKE + 2) =
      leds(LED_OFFSET + 1, LED_OFFSET + NUM_LEDS_SPOKE + 1);
#endif
}

// Copies the first fin to all other fins
void copyFirstFinToAllFins() {
  if (LED_OFFSET > 0) {
    leds(0, LED_OFFSET - 1) = leds(NUM_LEDS_IN_FIN + 1, NUM_LEDS_IN_FIN + LED_OFFSET);
  }

  for (uint16_t fin = 1; fin < (NUM_FINS - 1); ++fin) {
    const uint16_t finStart = fin * NUM_LEDS_IN_FIN_AND_PIT;
    leds(LED_OFFSET + finStart, LED_OFFSET + finStart + NUM_LEDS_IN_FIN) = leds(LED_OFFSET, LED_OFFSET + NUM_LEDS_IN_FIN);
  }

  uint16_t fin = NUM_FINS - 1;
  const uint16_t finStart = fin * NUM_LEDS_IN_FIN_AND_PIT;
  leds(LED_OFFSET + finStart, NUM_LEDS - 1) = leds(LED_OFFSET + finStart - (NUM_LEDS_IN_FIN + 1), NUM_LEDS - 1 - (NUM_LEDS_IN_FIN + 1));
}

// Helper function that returns true after the given animationStepTime has
// passed
bool animate(const uint32_t currentTime, const uint16_t animationStepTime) {
  if (currentTime - lastAnimationStep > animationStepTime) {
    lastAnimationStep = currentTime;
    return true;
  }
  return false;
}

// second Helper function does the same as above but on a second timer
bool animate2(const uint32_t currentTime, const uint16_t animationStepTime) {
  if (currentTime - lastAnimationStep2 > animationStepTime) {
    lastAnimationStep2 = currentTime;
    return true;
  }
  return false;
}

// Do a linear interpolation between the current color of eacg led in ledArray
// and the given color with the given fraction
void lerp8(CPixelView<CRGB>& ledArray, const CHSV& color, const uint8_t fraction) {
  const CRGB rgbColor = CRGB(color);
  for (int i = 0; i < ledArray.size(); ++i) {
    lerp8(ledArray[i], rgbColor, fraction);
  }
}

// Do a linear interpolation between the two given colors and store the result
// in the first color object
void lerp8(CRGB& color1, const CRGB& color2, const uint8_t fraction) {
  color1.r = lerp8by8(color1.r, color2.r, fraction);
  color1.g = lerp8by8(color1.g, color2.g, fraction);
  color1.b = lerp8by8(color1.b, color2.b, fraction);
}

// Log the given parameter when logging is enabled
// Ecpected output: "<name>: <value>"
template <typename T>
void logParameter(const String& name, const T& value) {
  if (DEBUG) {
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value);
  }
}
