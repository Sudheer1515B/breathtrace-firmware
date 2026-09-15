// Minimal stub of the Arduino/ESP32 API surface the firmware touches, so the
// sources can be syntax-checked on the host. NOT for flashing.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define SERIAL_8N1 0x800001c
enum { ADC_0db, ADC_2_5db, ADC_6db, ADC_11db };
unsigned long millis();
void delay(unsigned long);
void delayMicroseconds(unsigned int);
void pinMode(int, int);
void digitalWrite(int, int);
int  digitalRead(int);
int  analogRead(int);
void analogReadResolution(int);
void analogSetPinAttenuation(int, int);
void ledcAttach(int, uint32_t, uint8_t);
void ledcWrite(int, uint32_t);
void ledcSetup(int, uint32_t, uint8_t);
void ledcAttachPin(int, int);
void ledcWriteTone(int, uint32_t);
struct HardwareSerial {
    void begin(unsigned long, uint32_t = SERIAL_8N1, int8_t = -1, int8_t = -1);
    int  available();
    int  read();
    size_t write(const uint8_t*, size_t);
    void flush();
    void println(const char* = "");
    int  printf(const char*, ...);
};
extern HardwareSerial Serial;
extern HardwareSerial Serial2;
