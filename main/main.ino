#include "main.h"

void set_menu() {
  mainMenu.child = &waterItem;
  lightItem.next = &waterItem;
  waterItem.next = &lightItem;
  
  waterItem.prev = &lightItem;

  waterItem.child = &waterPeriod;
  lightItem.child = &lightPeriod;
}

// Функция отображения текущего пункта меню
void displayMenu() {
  if (!(current->child)) return;
    
  lcd.clear();
    MenuItem *item = current->child;
    
    lcd.setCursor(0, 0);
    lcd.print(item->name);
    item = item->next;
    
    lcd.setCursor(0, 1);
    lcd.print(item->name);
}

void navigateUp(MenuItem **current) {
  if ((*current)->prev)
    *current = (*current)->prev;
}

void navigateDown(MenuItem **current) {
  if ((*current)->next)
    *current = (*current)->next;
}

void selectItem(MenuItem **current) {
  if ((*current)->child)
    *current = (*current)->child;
}

void pin_a() {
  volatile byte value = 0;
  cli();
  value = PIND & 0xC;
  if(value == B00001100 && a_flag) {
    navigateDown(&current);
    b_flag = 0;
    a_flag = 0;
  }
  else if (value == B00000100) b_flag = 1;
  sei();
}

void pin_b() {
  volatile byte value = 0;
  cli();
  value = PIND & 0xC;
  if (value == B00001100 && b_flag) {
    navigateUp(&current);
    b_flag = 0;
    a_flag = 0;
  }
  else if (value == B00001000) a_flag = 1;
  sei();
}

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);

  pinMode(MENU_KNOB_A, INPUT_PULLUP);
  pinMode(MENU_KNOB_B, INPUT_PULLUP);
  attachInterrupt(0, pin_a, RISING);
  attachInterrupt(1, pin_b, RISING);
  
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

  /* while (BTSerial.available()) { */
  /*   bluetooth_read(); */
  /* } */
      
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

void bluetooth_read() {
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
