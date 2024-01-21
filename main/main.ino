#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

#define water_time 300000
#define light_time 600000
#define water_pin 3
#define light_pin 2
#define INPUT_SIZE 10


long water_period = 15000000;
long light_period = 10000000;

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial BTSerial(10, 11);

long main_timer, light_timer, water_timer = 0;
boolean light_state = false;
boolean water_state = false;

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  
  pinMode(water_pin, OUTPUT);
  pinMode(light_pin, OUTPUT);
  digitalWrite(light_pin, 1);
  digitalWrite(water_pin, 1);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(200);
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  delay(1000);
  write_vars();
}

void write_vars() {
  char water[16];
  char light[16];
  sprintf(water, "water %ds", water_period/100000);
  sprintf(light, "light %ds", light_period/100000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(water);
  lcd.setCursor(0, 1);
  lcd.print(light);
}

void write_state() {
  lcd.clear();
  if (water_state) {
    lcd.setCursor(0, 0);
    lcd.print("watering is on");    
  }
  if (light_state) {
    lcd.setCursor(0, 1);
    lcd.print("light is on");
  }
}

void refresh_screen() {
  if (water_state || light_state)
    write_state();
  else
    write_vars();
}

boolean is_it_time(long ctimer, long period) {
  return main_timer - ctimer > period;
}

int to_minute(int m) {
  return m*60*1000;
}

void turn_light() {
  if (light_state)
    digitalWrite(light_pin, 1);
  else
    digitalWrite(light_pin, 0);
  light_timer = main_timer;
  light_state = !light_state;
}

void turn_water() {
  if (water_state)
    digitalWrite(water_pin, 1);
  else
    digitalWrite(water_pin, 0);
  water_timer = main_timer;
  water_state = !water_state;
}

void loop() {
  main_timer++;

  while (BTSerial.available()) {
    Serial.write(BTSerial.read());
    char c = 0;
    int v = 0;
    char input[INPUT_SIZE + 1];
    byte size = BTSerial.readBytes(input, INPUT_SIZE);
    input[size] = 0;
    
    char* pch = strtok(input, " ");
    c = pch[0];
    pch = strtok(NULL, " ");
    v = atol(pch);

    switch (c) {
    case 'w':
      water_period = v * 100000;
      break;
    case 'l':
      light_period = v * 100000;
      break;
    }
    refresh_screen();
  }
      
  if (!light_state) {
    if (is_it_time(light_timer, light_period)) {
      turn_light();
      refresh_screen();
    }
  } else {
    if (is_it_time(light_timer, light_time)) {
      turn_light();
      refresh_screen();
    }
  }
  
  if (!water_state) {
    if (is_it_time(water_timer, water_period)) {
      turn_water();
      refresh_screen();
    }
  } else {
    if (is_it_time(water_timer, water_time)) {
      turn_water();
      refresh_screen();
    }
  }
}
