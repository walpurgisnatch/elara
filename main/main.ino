#include "main.h"

int devices_count = 2;

void save_to_EEPROM() {
  int addr = 0;

  int magic = EEPROM_MAGIC_NUMBER;
  EEPROM.put(addr, magic);
  addr += sizeof(magic);
  
  EEPROM.put(addr, main_timer);
  addr += sizeof(main_timer);

  for (int i = 0; i < devices_count; i++) {
    EEPROM.put(addr, devices[i]);
    addr += sizeof(Device);
  }
}

void load_from_EEPROM() {
  int addr = 0;
  int magic;

  EEPROM.get(addr, magic);
  if (magic != EEPROM_MAGIC_NUMBER)
    return;
  addr += sizeof(magic);
  
  EEPROM.get(addr, main_timer);
  addr += sizeof(main_timer);
  
  memset(devices, 0, sizeof(Device) * devices_count);
  
  for (int i = 0; i < devices_count; i++) {
    EEPROM.get(addr, devices[i]);
    addr += sizeof(Device);
  }
}

String convert_millis(long millis) {
  long days = millis / (DAY);
  millis %= (DAY);

  long hours = millis / (HOUR);
  millis %= (HOUR);

  long minutes = millis / (MINUTE);
  millis %= (MINUTE);

  long seconds = millis / SECOND;

  String result = "";
  if (days > 0) result += String(days) + "d";
  if (hours > 0) result += String(hours) + "h";
  if (minutes > 0) result += String(minutes) + "m";
  if (seconds > 0) result += String(seconds) + "s";

  return result;
}

void create_devices() {
  devices = (Device *)malloc(devices_count * sizeof(Device));
  
  if (!devices) {
    Serial.println("Memory allocation for devices failed");
  }
  
  for (int i = 0; i < devices_count; i++) {
    Device *device = &devices[i];
    Device *previous = NULL;
    if (i > 0) {
      previous = &devices[i-1];
    }

    device->parent_item = (MenuItem *)malloc(sizeof(MenuItem));
    device->period_item = (MenuItem *)malloc(sizeof(MenuItem));
    device->time_item = (MenuItem *)malloc(sizeof(MenuItem));
    device->func_item = (MenuItem *)malloc(sizeof(MenuItem));
    device->back_item = (MenuItem *)malloc(sizeof(MenuItem));

    if (!device->parent_item || !device->period_item || !device->time_item || !device->back_item) {
      Serial.println("Memory allocation for menu items failed");
    }
    
    char nameBuffer[16];
    snprintf(nameBuffer, 16, "Device %d", i);
    device->parent_item->name = strdup(nameBuffer);

    if (!device->parent_item->name) {
      Serial.println("Memory allocation for name failed");
    }
    
    device->parent_item->parent = &mainMenu;
    device->parent_item->next = NULL;
    device->parent_item->prev = NULL;
    device->parent_item->child = device->period_item;
    device->parent_item->func = NULL;
    device->parent_item->setting = NULL;
    device->parent_item->device = device;

    // Заполняем period_item
    device->period_item->name = "Period";
    device->period_item->parent = device->parent_item;
    device->period_item->next = device->time_item;
    device->period_item->prev = device->back_item;
    device->period_item->child = NULL;
    device->period_item->func = NULL;
    device->period_item->setting = &device->period;
    device->period_item->device = device;

    // Заполняем time_item
    device->time_item->name = "Time";
    device->time_item->parent = device->parent_item;
    device->time_item->next = device->func_item;
    device->time_item->prev = device->period_item;
    device->time_item->child = NULL;
    device->time_item->func = NULL;
    device->time_item->setting = &device->time;
    device->time_item->device = device;

    // Заполняем func_item
    device->func_item->name = "Try it";
    device->func_item->parent = device->parent_item;
    device->func_item->next = device->back_item;
    device->func_item->prev = device->time_item;
    device->func_item->child = NULL;
    device->func_item->func = turn_device;
    device->func_item->setting = NULL;
    device->func_item->device = device;

    // Заполняем back_item
    device->back_item->name = "Back";
    device->back_item->parent = device->parent_item;
    device->back_item->next = device->period_item;
    device->back_item->prev = device->func_item;
    device->back_item->child = device->parent_item;
    device->back_item->func = NULL;
    device->back_item->setting = NULL;
    device->back_item->device = device;
    
    device->time = 20 * SECOND;
    device->period = 18 * HOUR;
    device->timer = 0;
    device->state = false;
    device->pin = DEVICE_FIRST_PIN + i;

    if (devices_count > 1) {
      if (previous) {
        previous->parent_item->next = device->parent_item;
        device->parent_item->prev = previous->parent_item;
      }
      if (i == devices_count-1) {
        previous = &devices[0];
        previous->parent_item->prev = device->parent_item;
        device->parent_item->next = previous->parent_item;
      }
    }
  }
}

void display_menu() {
  lcd.clear();

  MenuItem *item = current;
  
  lcd.setCursor(0, 0);
  lcd.print("> ");
  lcd.print(item->name);
  if (item->setting) {
    lcd.print(" ");
    lcd.print(convert_millis(*item->setting));
  }
  if (item->next) {
    item = item->next;
    
    lcd.setCursor(2, 1);
    lcd.print(item->name);
    if (item->setting) {
      lcd.print(" ");
      lcd.print(convert_millis(*item->setting));
    }
  }
}

void navigate_up(MenuItem **current) {
  if ((*current)->prev) {
    *current = (*current)->prev;
    display_menu();
  }
}

void navigate_down(MenuItem **current) {
  if ((*current)->next) {
    *current = (*current)->next;
    display_menu();
  }
}

void select_item(MenuItem **current) {
  if ((*current)->child) {
    *current = (*current)->child;
    display_menu();
  } else if ((*current)->setting && !setting_selected) {
    setting_selected = true;
  } else if ((*current)->setting && setting_selected) {
    setting_selected = false;
  } else if ((*current)->func) {
    (*current)->func((*current)->device);
  }
}

void value_up() {
  if (*(current->setting) > 7200000) { // 2h
    *(current->setting) += 1 * HOUR;
  } else if (*(current->setting) > 3600000) { //1h
    *(current->setting) += 10 * MINUTE;
  } else if (*(current->setting) > 1800000) { //30m
    *(current->setting) += 5 * MINUTE;
  } else if (*(current->setting) > 600000) { //10m
    *(current->setting) += 1 * MINUTE;
  } else if (*(current->setting) > 60000) { //1m
    *(current->setting) += 30 * SECOND;
  } else {
    *(current->setting) += 2.5 * SECOND;
  }
}

void value_down() {
  if (*(current->setting) > 7200000) { // 2h
    *(current->setting) -= 1 * HOUR;
  } else if (*(current->setting) > 3600000) { //1h
    *(current->setting) -= 10 * MINUTE;
  } else if (*(current->setting) > 1800000) { //30m
    *(current->setting) -= 5 * MINUTE;
  } else if (*(current->setting) > 600000) { //10m
    *(current->setting) -= 1 * MINUTE;
  } else if (*(current->setting) > 60000) { //1m
    *(current->setting) -= 30 * SECOND;
  } else {
    *(current->setting) -= 2.5 * SECOND;
  }
}

void nullify_flags() {
  b_flag = 0;
  a_flag = 0;
}

void pin_a() {  
  volatile byte value = 0;
  bool changed = false;
  
  if (main_timer - last_interrupt_time < DEBOUNCE) {
    return;
  }
  last_interrupt_time = main_timer;
  
  cli();
  value = PIND & 0xC;
  if(value == 8 && a_flag) {
    changed = true;
    nullify_flags();
  } else if (value == 12) b_flag = 1;
  sei();

  if (changed && !setting_selected) {
    navigate_down(&current);
  } else if (changed && setting_selected) {
    value_down();
    display_menu();
  }
}

void pin_b() {
  volatile byte value = 0;
  bool changed = false;
  
  cli();
  value = PIND & 0xC;
  if (value == 12 && b_flag) {
    changed = true;
    nullify_flags();
  } else if (value == 8) a_flag = 1;
  sei();

  if (changed && !setting_selected) {
    navigate_up(&current);
  } else if (changed && setting_selected) {
    value_up();
    display_menu();
  }
}

void LCD_setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(200);
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  delay(1000);
  display_menu();
}

void setup_menu() {
  current = devices[0].parent_item;
  mainMenu.child = devices[0].parent_item;
}

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  
  create_devices();
  load_from_EEPROM();
    
  setup_menu();

  for (int i = DEVICE_FIRST_PIN; i < DEVICE_FIRST_PIN + devices_count; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, 1);
  }
  
  pinMode(MENU_BUTTON, INPUT_PULLUP);
  pinMode(MENU_KNOB_A, INPUT_PULLUP);
  pinMode(MENU_KNOB_B, INPUT_PULLUP);
  attachInterrupt(0, pin_a, RISING);
  attachInterrupt(1, pin_b, RISING);

  LCD_setup();
}

boolean is_it_time(long ctimer, long period) {
  return main_timer - ctimer > period;
}

void turn_device(Device *device) {
  if (device->state) 
    digitalWrite(device->pin, 1);
  else
    digitalWrite(device->pin, 0);
  device->timer = main_timer;
  device->state = !device->state;
}

void loop() {
  main_timer = millis();
  bool btn_state = !digitalRead(MENU_BUTTON);

  if (main_timer - last_save_time >= 1 * HOUR) {
    save_to_EEPROM();
    last_save_time = main_timer;
  }

  if (btn_state && !button_pressed) {
    button_pressed = true;
  }
  if (!btn_state && button_pressed) {
    button_pressed = false;
    select_item(&current);
  }

  for (int d = 0; d < devices_count; d++) {
    if (!devices[d].state) {
      if (is_it_time(devices[d].timer, devices[d].period))
        turn_device(&devices[d]);
    } else {
      if (is_it_time(devices[d].timer, devices[d].time))
        turn_device(&devices[d]);
    }
  }
}
