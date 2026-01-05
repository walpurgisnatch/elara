#pragma once
#include <Arduino.h>
#include <EEPROM.h>
//#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define EEPROM_MAGIC_NUMBER 0xABCC
#define EEPROM_SIZE 1024

#define SECOND 1000
#define MINUTE 60000
#define HOUR 3600000
#define DAY 86400000

#define DEVICE_FIRST_PIN 16
#define MENU_KNOB_A 34   // input-only, ок для энкодера
#define MENU_KNOB_B 35   // input-only
#define MENU_BUTTON 25   // любой GPIO с PullUp

#define DEBOUNCE 5
#define SERIAL_INPUT_SIZE 21

typedef enum {
    WATER,
    LIGHT
} DeviceType;

typedef struct Device Device;
typedef void (*FunctionPointer)(Device*);

typedef struct Device {
    const char *name;
    int pin;
    unsigned long timer;
    long time;
    long period;
    bool state;
} Device;

Device* devices = NULL;

volatile unsigned long last_interrupt_time;
unsigned long main_timer, last_save_time = 0;

void display_menu();

void loadFromEEPROM();
void saveToEEPROM();
void updateDevices();
void handleBT();
void onButton();
void onEncoder();
