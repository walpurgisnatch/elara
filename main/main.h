#include <string.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

#define DEBOUNCE 5

#define water_pin 5
#define light_pin 6
#define INPUT_SIZE 10

#define MENU_KNOB_A 3
#define MENU_KNOB_B 2
#define MENU_BUTTON 4

#define SECOND 1000
#define MINUTE 60000
#define HOUR 3600000
#define DAY 86400000

typedef struct MenuItem {
    const char *name;
    struct MenuItem *parent;
    struct MenuItem *next;
    struct MenuItem *prev;
    struct MenuItem *child;
    long *setting;
} MenuItem;

volatile byte a_flag = 0;
volatile byte b_flag = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial BTSerial(10, 11);

long water_time = 20 * SECOND;
long light_time = 6 * HOUR;
long water_period = 18 * HOUR;
long light_period = 18 * HOUR;

MenuItem mainMenu = {"Main manu", NULL, NULL, NULL, NULL, NULL};
MenuItem waterItem = {"Water", &mainMenu, NULL, NULL, NULL, NULL};
MenuItem lightItem = {"Light", &mainMenu, NULL, &waterItem, NULL, NULL};

MenuItem waterPeriod = {"Period", &waterItem, NULL, NULL, NULL, &water_period};
MenuItem waterAmount = {"Time", &waterItem, NULL, &waterPeriod, NULL, &water_time};
MenuItem backFromWater = {"Back", &lightItem, NULL, &waterAmount, &waterItem, NULL};

MenuItem lightPeriod = {"Period", &lightItem, NULL, NULL, NULL, &light_period};
MenuItem lightTime = {"Time", &lightItem, NULL, &lightPeriod, NULL, &light_time};
MenuItem backFromLight = {"Back", &lightItem, NULL, &lightTime, &lightItem, NULL};

void display_menu();
void navigate_up(MenuItem **current);
void navigate_down(MenuItem **current);
void select_item(MenuItem **current);
