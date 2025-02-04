#include "main.h"

unsigned long last_interrupt_time = 0;

unsigned long main_timer, light_timer, water_timer = 0;
bool light_state = false;
bool water_state = false;
bool button_pressed = false;
bool setting_selected = false;

MenuItem *current = NULL;

String convert_millis(long millis) {
  long days = millis / (DAY);
  millis %= (DAY);

  long hours = millis / (HOUR);
  millis %= (HOUR);

  long minutes = millis / (MINUTE);
  millis %= (MINUTE);

  long seconds = millis / SECOND;

  // Формируем строку с результатом
  String result = "";
  if (days > 0) result += String(days) + "d";
  if (hours > 0) result += String(hours) + "h";
  if (minutes > 0) result += String(minutes) + "m";
  if (seconds > 0) result += String(seconds) + "s";

  return result;
}

void create_menu() {
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
    device->back_item = (MenuItem *)malloc(sizeof(MenuItem));

    if (!device->parent_item || !device->period_item || !device->time_item || !device->back_item) {
      Serial.println("Memory allocation for menu items failed");
    }

    // Заполняем parent_item
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
    device->parent_item->setting = NULL;

    // Заполняем period_item
    device->period_item->name = "Period";
    device->period_item->parent = device->parent_item;
    device->period_item->next = device->time_item;
    device->period_item->prev = device->back_item;
    device->period_item->child = NULL;
    device->period_item->setting = &device->period;

    // Заполняем time_item
    device->time_item->name = "Time";
    device->time_item->parent = device->parent_item;
    device->time_item->next = device->back_item;
    device->time_item->prev = device->period_item;
    device->time_item->child = NULL;
    device->time_item->setting = &device->time;

    // Заполняем back_item
    device->back_item->name = "Back";
    device->back_item->parent = device->parent_item;
    device->back_item->next = device->period_item;
    device->back_item->prev = device->time_item;
    device->back_item->child = device->parent_item;
    device->back_item->setting = NULL;
    
    device->time = 20 * SECOND;
    device->period = 18 * HOUR;
    device->timer = 0;
    device->state = false;

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

void setup_menu() {
  create_menu();
  current = devices[0].parent_item;
  mainMenu.child = devices[0].parent_item;
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
  }
}

void value_up() {
  if (*(current->setting) > 3600000) {
    *(current->setting) += 2 * HOUR;
  } else {
    *(current->setting) += 2 * SECOND;
  }
}

void value_down() {
  if (*(current->setting) > 3600000) {
    *(current->setting) -= 2 * HOUR;
  } else {
    *(current->setting) -= 2 * SECOND;
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

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  setup_menu();
  
  pinMode(water_pin, OUTPUT);
  pinMode(light_pin, OUTPUT);
  digitalWrite(light_pin, 1);
  digitalWrite(water_pin, 1);
  
  pinMode(MENU_KNOB_A, INPUT_PULLUP);
  pinMode(MENU_KNOB_B, INPUT_PULLUP);
  attachInterrupt(0, pin_a, RISING);
  attachInterrupt(1, pin_b, RISING);

  pinMode(MENU_BUTTON, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(200);
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  delay(1000);
  display_menu();
}

boolean is_it_time(long ctimer, long period) {
  return main_timer - ctimer > period;
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
  bool btn_state = !digitalRead(MENU_BUTTON);

  if (btn_state && !button_pressed) {
    button_pressed = true;
  }
  if (!btn_state && button_pressed) {
    button_pressed = false;
    select_item(&current);
  }
      
  if (!light_state) {
    if (is_it_time(light_timer, light_period)) {
      turn_light();
    }
  } else {
    if (is_it_time(light_timer, light_time)) {
      turn_light();
    }
  }
  
  if (!water_state) {
    if (is_it_time(water_timer, water_period)) {
      turn_water();
    }
  } else {
    if (is_it_time(water_timer, water_time)) {
      turn_water();
    }
  }

  /* while (BTSerial.available()) { */
  /*   bluetooth_read(); */
  /* } */
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
}
