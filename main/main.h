#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

#define water_pin 3
#define light_pin 2
#define INPUT_SIZE 10
#define MENU_ITEMS_COUNT 2

#define MENU_KNOB_A 4
#define MENU_KNOB_B 5

volatile byte a_flag = 0;
volatile byte b_flag = 0;

long water_time = 300000;
long light_time = 600000;
long water_period = 15000000;
long light_period = 10000000;

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial BTSerial(10, 11);

long main_timer, light_timer, water_timer = 0;
boolean light_state = false;
boolean water_state = false;

typedef enum { WATER, LIGHT } Name;
typedef enum { UP, DOWN } Operation;

// Определение структуры меню
typedef struct MenuItem {
    const char *name; // Название пункта меню
    struct MenuItem *parent; // Родительский пункт меню
    struct MenuItem *next;   // Следующий пункт на том же уровне
    struct MenuItem *prev;   // Предыдущий пункт на том же уровне
    struct MenuItem *child;  // Дочерний пункт (вложенное меню)
} MenuItem;

// Прототипы функций
void displayMenu();
void navigateUp(MenuItem **current);
void navigateDown(MenuItem **current);
void selectItem(MenuItem **current);

struct MenuItem menu[MENU_ITEMS_COUNT] = {
    { WATER, "water" },
    { LIGHT, "light" }
};

MenuItem mainMenu = {"Главное меню", NULL, NULL, NULL, NULL};
MenuItem waterItem = {"Вода", &mainMenu, NULL, NULL, NULL};
MenuItem lightItem = {"Свет", &mainMenu, NULL, &waterItem, NULL};

MenuItem waterPeriod = {"Период", &waterItem, NULL, NULL, NULL};
MenuItem waterAmount = {"Количество", &waterItem, NULL, &waterPeriod, NULL};

MenuItem lightPeriod = {"Период", &lightItem, NULL, NULL, NULL};
MenuItem lightAmount = {"Время", &lightItem, NULL, &lightPeriod, NULL};

MenuItem *current = &mainMenu;
