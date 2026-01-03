#include "main.h"

int devices_count = 2;
BLECharacteristic *pCharacteristic;

void save_to_EEPROM() { }

void load_from_EEPROM() { }

String convert_millis(long millis) {
  long days = millis / DAY;
  millis %= DAY;

  long hours = millis / HOUR;
  millis %= HOUR;

  long minutes = millis / MINUTE;
  millis %= MINUTE;

  long seconds = millis / SECOND;

  String result = "";
  if (days > 0) result += String(days) + "d";
  if (hours > 0) result += String(hours) + "h";
  if (minutes > 0) result += String(minutes) + "m";
  if (seconds > 0) result += String(seconds) + "s";

  return result;
}

long convert_to_millis(const char *time_str) {
  long total_ms = 0;
  long current_value = 0;

  for (; *time_str; ++time_str) {
    if (isdigit(*time_str)) {
      current_value = current_value * 10 + (*time_str - '0');
    } else {
      switch (*time_str) {
      case 'd': total_ms += current_value * DAY; break;
      case 'h': total_ms += current_value * HOUR; break;
      case 'm': total_ms += current_value * MINUTE; break;
      case 's': total_ms += current_value * SECOND; break;
      default: break;
      }
      current_value = 0;
    }
  }
  return total_ms;
}

void create_devices() {
  devices = (Device *)malloc(devices_count * sizeof(Device));
  
  if (!devices) {
    Serial.println("Memory allocation for devices failed");
  }
  
  for (int i = 0; i < devices_count; i++) {
    Device *device = &devices[i];
    Device *previous = NULL;
    
    char name_buffer[16];
    snprintf(name_buffer, 16, "Device %d", i);

    device->name = strdup(name_buffer);
    device->time = 5 * SECOND;
    device->period = 15 * SECOND;
    device->timer = 0;
    device->state = false;
    device->pin = DEVICE_FIRST_PIN + i;
  }
}

void write_data(const char* value) {
  int d;
  long period_ms, time_ms;

  char part1[5], part2[10], part3[10];
  if (sscanf(value, "%s %s %s", part1, part2, part3) != 3) {
    Serial.println("Ошибка парсинга строки!");
    return;
  }

  d = atoi(part1);
  period_ms = convert_to_millis(part2);
  time_ms = convert_to_millis(part3);

  if (d >= 0 && d < 10) {
    devices[d].period = period_ms;
    devices[d].time = time_ms;
      
    char response[64];
    snprintf(response, sizeof(response), "Устройство %d: period=%ldms, time=%ldms\n", d, period_ms, time_ms);
    pCharacteristic->setValue(response);
    Serial.println(response);
  } else {
    Serial.println("Неверный номер устройства!");
  }
}

class BLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.print("New value: ");
      Serial.println(value.c_str());
      write_data(value.c_str());
    }
  }
};

void setupBLE() {
  BLEDevice::init("Elara");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic =
    pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new BLECallbacks());
  pCharacteristic->setValue("You");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  Serial.println("BLE started");
}

void setup() {
  Serial.begin(115200);

  setupBLE();
  create_devices();
  
  for (int i = DEVICE_FIRST_PIN; i < DEVICE_FIRST_PIN + devices_count; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, 1);
  }
}

boolean is_it_time(long ctimer, long period) {
  return main_timer - ctimer > period;
}

void turn_device(Device *device) {  
  if (device->state) {
    digitalWrite(device->pin, 1);
    device->state = false;
  } else {
    digitalWrite(device->pin, 0);
    device->state = true;
  }
  device->timer = main_timer;
}

void loop() {
  main_timer = millis();

  if (main_timer - last_save_time >= 1 * HOUR) {
    save_to_EEPROM();
    last_save_time = main_timer;
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
