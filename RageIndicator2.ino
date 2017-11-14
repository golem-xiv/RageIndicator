#include <FastLED.h>
#include <Wire.h>
#include "IRLibAll.h"
#include <Servo.h>

enum PinAssignments {
  irReceiverPin    = 3,
  encoderPinA      = 2,
  encoderPinB      = 6,
  encoderSwitchPin = 4, //TODO
  ledPin           = 5
};

volatile int encoderSwitchPinState;             // the current reading from the input pin
volatile int lastEncoderSwitchPinState = LOW;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

Servo servoFuckYou;
volatile unsigned pos = 0;

//IR Reveicer
IRrecvPCI irReceiver(irReceiverPin);
IRdecode  irDecoder;

#define BUTTON_MORE 0xffa857
#define BUTTON_LESS 0xffe01f
#define BUTTON_FUCK 0xff906f
#define IR_ENCODER_STEP 50

//LED and Rotary Encoder
#define NUM_LEDS    10
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define MAXENCODERPOS 500
#define COLOR_INDEX_STEP 7
#define LED_FLASHSTEP 5
#define UPDATES_PER_SECOND 100

CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 ragePalette PROGMEM;
volatile uint16_t ledFlashCounter = 0;


//Encoder Settings
volatile unsigned int encoderPos = 0;
volatile unsigned int oldEncoderPos = 0;
volatile boolean encoderIsrFired = false;
volatile boolean encoderUpwards  = false;

void setup() {
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  attachInterrupt(0, encoderIsr, RISING);
  pinMode(encoderSwitchPin, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, 5, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  currentPalette = ragePalette;
  currentBlending = LINEARBLEND;
  irReceiver.enableIRIn();
  Serial.begin(9600);
  Serial.println("KheRageIndicator");
  drawGradient(1);
}
void debounceButton() {
  int reading = digitalRead(encoderSwitchPin);
  if (reading != lastEncoderSwitchPinState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != encoderSwitchPinState) {
      encoderSwitchPinState = reading;

      if (encoderSwitchPinState == HIGH) {
        fuckYou();
        Serial.println("Button Pressed");
      }
    }
  }

  lastEncoderSwitchPinState = reading;
}

void encoderChanged() {
  if (encoderPos != oldEncoderPos) {
    oldEncoderPos = encoderPos;
    Serial.print("RageCounter:");
    Serial.print(encoderPos, DEC);
    Serial.println();
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}
void loop()
{
  encoderProcess();
  processIrSignals();
  drawGradient(encoderPos);
  encoderChanged();
  ledFlashCounter += LED_FLASHSTEP;
  debounceButton();
}

void fuckYou() {
  Serial.println("Fuck you was triggered!");
  servoFuckYou.attach(7);
  //servoFuckYou.write(355);              // tell servo to go to position in variable 'pos'
  //delay(1000);                       // waits 15ms for the servo to reach the position
  //servoFuckYou.write(-180);              // tell servo to go to position in variable 'pos'
  //delay(100o);
  //servoFuckYou.detach();

  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servoFuckYou.write(pos);              // tell servo to go to position in variable 'pos'
    delay(2);                       // waits 15ms for the servo to reach the position
  }
  delay(5000);                       // waits 15ms for the servo to reach the position
  Serial.println("And back!");
  for (pos = 180; pos >= 1; pos -= 1) { // goes from 180 degrees to 0 degrees
    servoFuckYou.write(pos);              // tell servo to go to position in variable 'pos'
    delay(10);                       // waits 15ms for the servo to reach the position
  }
  servoFuckYou.detach();
  Serial.println("Servo fuckYou was detached!");
}
//Ir Receiver
void processIrSignals() {
  if (irReceiver.getResults()) {
    irDecoder.decode();
    switch (irDecoder.value) {
      case BUTTON_MORE: {
          encoderPos += IR_ENCODER_STEP;
          normalizeEncodePos();
          break;
        }
      case BUTTON_LESS: {
          encoderPos -= IR_ENCODER_STEP;
          normalizeEncodePos();
          break;
        }
      case BUTTON_FUCK: {
          fuckYou();
          break;
        }
    }
    irDecoder.dumpResults(false);
    irReceiver.enableIRIn(); //Reveiver can receive next signal
  }
}
//Encoder Interrupt
void encoderIsr () {
  if (digitalRead (encoderPinA)) {
    encoderUpwards = digitalRead(encoderPinB);
  }
  else {
    encoderUpwards = !digitalRead(encoderPinB);
  }
  encoderIsrFired = true;
}

void encoderProcess() {
  if (encoderIsrFired) {
    if (encoderUpwards) {
      encoderPos-=10;
    }
    else {
      encoderPos+=10;
    }
    encoderIsrFired = false;
    normalizeEncodePos();
  }
}

void normalizeEncodePos() {
  encoderPos = (encoderPos  < 0 || encoderPos > 60000)   ?  0   :  encoderPos;
  encoderPos = (encoderPos  > MAXENCODERPOS) ? MAXENCODERPOS :  encoderPos;
}

//Led Gradient
void drawGradient(int rageValue)
{
  float ragePercent = MAXENCODERPOS / (float) rageValue;
  int numLedsLit    =  round(NUM_LEDS / ragePercent);
  numLedsLit = (numLedsLit < 1) ? 1 : numLedsLit;
  uint8_t colorIndex = round(255 / ragePercent);
  uint8_t brightness = 0;
  uint8_t brightnessValue;
  if (rageValue < (MAXENCODERPOS - 75))
  {
    brightnessValue = colorIndex > 55 ? colorIndex : 55;
  }
  else
  {
    brightnessValue = 255;
  }

  for ( int i = 9; i >= 0; i--) {
    if (i >= numLedsLit)
    {
      brightness = 0;
    }
    else
    {
      brightness = brightnessValue;
    }
    colorIndex = (colorIndex  <=  COLOR_INDEX_STEP + 1) ? 1 : colorIndex - COLOR_INDEX_STEP;
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
  }
}

const TProgmemPalette16 ragePalette PROGMEM =
{
  CRGB::Green,
  0x2FFF00,
  0x4FFF00,
  0x6FFF00,

  0x8FFF00,
  0xAFFF00,
  0xCFFF00,
  0xEFFF00,

  0xFFDF00,
  0xFFBF00,
  0xFF9F00,
  0xFF7F00,

  0xFF5F00,
  0xFF3F00,
  0xFF1F00,
  CRGB::Red
};


