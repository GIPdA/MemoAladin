/*
 * MemoAladin - Aladin MemoMouse replacement
 * 
 * Aladin Input:
 * Idle: 100Hz square signal (~1.5V to 2V peak AC)
 * Send: 19200 bps (2050 bytes)
 * 
 * LED:
 * - Idle, no mouse: breath
 * - Idle, Aladin connected: ON constant (todo)
 * - PC transfer, waiting Aladin: blink slow
 * - PC transfer: blink fast
 * - Aladin data in, waiting PC: blink slow
 * - Aladin transfer: blink fast
 */

#include <LEDEffect.h>
#include "utils.h"
#include "aladin.h"

#include "testdata.h"


int const ledPin = PB1;
int const input = PA3; // RX2, Timer 2 Capture CH4
int const input2 = PA4;

LEDEffect led(ledPin);

uint8_t data[2050] = {0}; // Dive computer log

uint16_t dataIndex {0};
enum class DecoderState { Idle, Header, Data, Ready };
DecoderState decoderState {DecoderState::Idle};

uint8_t pcDecoderState {0};

static uint32_t resetTimeout {0};


void blinkSlow()
{
  led.blink(500);
}

void blinkFast()
{
  led.blink(50);
}

void breath()
{
  led.breath(30);
}


// check dive mouse log header
bool checkCurrentHeader()
{
  for (uint8_t i {0}; i < dataIndex; ++i) {
    if (i == 3)
      return data[i] == 0x00;
      
    if (data[i] != 0xAA)
      return false;
  }
  return true;
}

bool checkData()
{
  uint16_t checksum {0};
  for (uint16_t i {0}; i < 2044; ++i) {
    checksum += data[i];
  }
  checksum += 0x1fe; // "UUU\0"
  return checksum == ((uint16_t)data[2044] + ((uint16_t)data[2045]<<8));
}

void resetMouseDecoder()
{
  flushMouseSerial();
  decoderState = DecoderState::Idle;
  dataIndex = 0;
  memset(data, 0, 2050);
}


void decode(uint8_t c)
{
  if (decoderState == DecoderState::Idle && pcDecoderState == 0 && c == 0x00) {
    // Idle mode, no PC transfer, getting mouse 100Hz idle signal
    //led.on();
    return;
  }

  switch (decoderState) {
    case DecoderState::Ready:
      if (c != 0xAA)
        break;
      // fallthrough for 'U' (new frame incoming)

    case DecoderState::Idle:
      if (c == 0xAA) {
        dataIndex = 0;
        decoderState = DecoderState::Header;
        blinkFast();
        // Fall through
      } else {
        break;
      }

    case DecoderState::Header: {
      data[dataIndex++] = c;
      bool const cok = checkCurrentHeader();
      
      if (!cok) {
        dataIndex = 0;
        decoderState = DecoderState::Idle;
        breath();
      } else if (cok && dataIndex == 4) {
        dataIndex = 0;
        decoderState = DecoderState::Data;
      }
      break;
    }

    case DecoderState::Data:
      data[dataIndex++] = c;

      if (dataIndex == 2046) {
        // All data in!
        decoderState = DecoderState::Ready;

        // check data
        if (!checkData()) {
          delay(1000);
          resetMouseDecoder();
          reset();
          return;
        }

        blinkSlow();
        // Wait for PC transfer (or PC already waiting)

        //Serial.write((uint8_t*)data, 2050);
        //memset(data, 0, 2050);
      }
      break;
  }
}


void setup()
{
  memset(data, 0, 2050);
  
  Serial.begin(9600);
  delay(1000);
  
  pinMode(ledPin, OUTPUT);
  pinMode(input, INPUT);
  pinMode(input2, INPUT);

  Serial2.begin(19200); // Mouse serial comm

  reset();

  /*if (checkTestData()) {
    blinkFast();
  }//*/
}


void flushPCSerial()
{
  while (Serial.available()) Serial.read();
}

void flushMouseSerial()
{
  while (Serial2.available()) Serial2.read();
}


void reset()
{
  pcDecoderState = 0;
  resetTimeout = 0;
  flushPCSerial();
  breath();
}

void loop()
{
  led.update();

  uint8_t maxCount = 0;
  while (Serial2.available()) {
    uint8_t const c { reverse(Serial2.read()) };
    //Serial.write(c);
    decode(c);
    if (maxCount++ > 50)
      break;
  }


  // Data from PC
  while (Serial.available()) {
    uint8_t const c { reverse(Serial.read()) };

    switch (pcDecoderState) {
      case 0:
        if (c == 0x15) {
          blinkFast();
          aladin_print("IFV1.00");
          ++pcDecoderState;

          // Start timeout
          resetTimeout = millis();
          if (!resetTimeout) ++resetTimeout;
        }
        break;

      case 1:
        if (c == 0x06)
          ++pcDecoderState; // ack
        break;

      // outer and inner packet len (ignore)
      case 2:
      case 3:
      case 4:
        ++pcDecoderState;
        break;

      // command (only check 'U')
      case 5:
        if (c != 0x55) { // Send NAK and reset
          aladin_print(0x15);
          delay(1000);
          reset();
        } else {
          ++pcDecoderState;
        }
        break;

      // time (ignore)
      case 6:
      case 7:
      case 8:
      case 9:
        ++pcDecoderState;
        break;

      // outer packet checksum (no inner packet checksum!)
      case 10:
        ++pcDecoderState;
        blinkSlow();

        // send ack
        aladin_print(0x06);
        
        // Start timeout
        resetTimeout = millis();
        if (!resetTimeout) ++resetTimeout;
        break;

      case 11:
        break;

      default:
        // Error!?
        reset();
        break;
    }
  }

  // Send dive log (wait for PC or Aladin)
  if (pcDecoderState == 11 && decoderState == DecoderState::Ready) {
    digitalWrite(ledPin, HIGH);
    
    delay(500);
    
    // send data
    sendAladin(data);

    digitalWrite(ledPin, LOW);

    resetMouseDecoder();
    reset();
  }

  if (resetTimeout > 0 && (millis() - resetTimeout) > 65000) {
    reset();
  }
}
