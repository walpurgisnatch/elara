#define water_period 700000
#define water_time 300000
#define light_period 1500000
#define light_time 600000
#define water 3
#define light 4

long main_timer, light_timer, water_timer = 0;
boolean light_state = false;
boolean water_state = false;

void setup() {
  Serial.begin(9600);
  pinMode(water, OUTPUT);
  pinMode(light, OUTPUT);
  digitalWrite(light, 1);
  digitalWrite(water, 1);
}

long is_it_time(long qmer, long period) {
  main_timer - qmer > period;
}

void loop() {
  main_timer++;
      
  if (!light_state) {
    if (main_timer - light_timer > light_period) {
      light_timer = main_timer;
      light_state = true;
      digitalWrite(light, 0);
    }
  } else {
    if (main_timer - light_timer > light_time) {
      light_timer = main_timer;
      light_state = false;
      digitalWrite(light, 1);
    }
  }

  if (!water_state) {
    if (main_timer - water_timer > water_period) {
      water_timer = main_timer;
      water_state = true;
      digitalWrite(water, 0);
    }
  } else {
    if (main_timer - water_timer > water_time) {
      water_timer = main_timer;
      water_state = false;
      digitalWrite(water, 1);
    }
  }

  
}
