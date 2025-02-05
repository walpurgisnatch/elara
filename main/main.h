#include <string.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

#define DEBOUNCE 5

#define DEVICE_FIRST_PIN 5
#define INPUT_SIZE 10

#define MENU_KNOB_A 3
#define MENU_KNOB_B 2
#define MENU_BUTTON 4

#define SECOND 1000
#define MINUTE 60000
#define HOUR 3600000
#define DAY 86400000

typedef enum {
    WATER,
    LIGHT
} DeviceType;

typedef struct MenuItem {
    const char *name;
    struct MenuItem *parent;
    struct MenuItem *next;
    struct MenuItem *prev;
    struct MenuItem *child;
    long *setting;
} MenuItem;

typedef struct Device {
    struct MenuItem *parent_item;
    struct MenuItem *period_item;
    struct MenuItem *time_item;
    struct MenuItem *back_item;
    unsigned long timer;
    long time;
    long period;
    bool state;
} Device;

volatile byte a_flag = 0;
volatile byte b_flag = 0;
bool button_pressed = false;
bool setting_selected = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial BTSerial(10, 11);

Device *devices = NULL;

MenuItem mainMenu = {"Main manu", NULL, NULL, NULL, NULL, NULL};
MenuItem *current = NULL;

unsigned long main_timer, last_interrupt_time = 0;

void display_menu();
void navigate_up(MenuItem **current);
void navigate_down(MenuItem **current);
void select_item(MenuItem **current);
