#include <DueTimer.h>

// set pins
auto const LED_RED = 11;
auto const LED_GREEN = 8;
auto const LED_BLUE = 7;
auto const BUTTON1 = 3;
auto const BUTTON2 = 5;
auto const MOTOR_PWM = 6;
auto const MOTOR_IN1 = 49;
auto const MOTOR_IN2 = 47;
// set variables for debouncing buttons
int button1_pressed = 0;
int button1_unpressed = 0;
int button2_pressed = 0;
int button2_unpressed = 0;
bool timer1_running = false;
bool timer2_running = false;
// system mode
int const ROTATION_MODE = 0;
int const POWER_MODE = 1;
bool system_mode = ROTATION_MODE;
// rotation
int const CW = 1;
int const CCW = -1;
int const STOP = 0;
int rotation = STOP;
// power
int const POWER_INTERVAL = 51;
int const INCREASE = 1;
int const DECREASE = -1;
int power = 255;
// timer
DueTimer timer1;
DueTimer timer2;

bool wait_for_response = false;

void button1Interrupt() {
  if (!timer1_running && !wait_for_response) {
    startTimer(1);
  }
}

void button2Interrupt() {
  if (!timer2_running && !wait_for_response) {
    startTimer(2);
  }
}

void startTimer(int btn) {
  if (btn == 1) {
    timer1.start();
    timer1_running = true;
  }
  else {
    timer2.start();
    timer2_running = true;
  }
}

void stopTimer(int btn) {
  if (btn == 1) {
    timer1.stop();
    button1_pressed = 0;
    button1_unpressed = 0;
    timer1_running = false;
  }
  else {
    timer2.stop();
    button2_pressed = 0;
    button2_unpressed = 0;
    timer2_running = false;
  }
}

void debounce1() {
  if (digitalRead(BUTTON1) == LOW) {
    button1_pressed++;
    button1_unpressed = 0;
  }
  else {
    button1_unpressed++;
    button1_pressed = 0;
  }

  if (button1_pressed > 50) {
    if (button2_pressed != 0 || button2_unpressed != 0) {
      wait_for_response = true;
    }
    else {
      if (wait_for_response) {
        triggerboth();
        wait_for_response = false;
      }
      else {
        trigger1();
      }
    }
    stopTimer(1);
  }
  else if (button1_unpressed > 50) {
    if (wait_for_response) {
      trigger2();
      wait_for_response = false;
    }
    stopTimer(1);
  }
}

void debounce2() {
  if (digitalRead(BUTTON2) == LOW) {
    button2_pressed++;
    button2_unpressed = 0;
  }
  else {
    button2_unpressed++;
    button2_pressed = 0;
  }

  if (button2_pressed > 50) {
    if (button1_pressed != 0 || button1_unpressed != 0) {
      wait_for_response = true;
    }
    else {
      if (wait_for_response) {
        triggerboth();
        wait_for_response = false;
      }
      else {
        trigger2();
      }
    }
    stopTimer(2);
  }
  else if (button2_unpressed > 50) {
    if (wait_for_response) {
      trigger1();
      wait_for_response = false;
    }
    stopTimer(2);
  }
}

void trigger1() {
  if (system_mode == ROTATION_MODE) {
    changeRotation(CCW);
  }
  else {
    changePower(INCREASE);
  }
}

void trigger2() {
  if (system_mode == ROTATION_MODE) {
    changeRotation(CW);
  }
  else {
    changePower(DECREASE);
  }
}

void triggerboth() {
  changeSystemMode();
}

// swaps the system mode
void changeSystemMode() {
  system_mode = system_mode ? ROTATION_MODE : POWER_MODE;
  Serial.println("------------------------------------------------------------");
  printInfo();
}

// changes the rotation direction according to the specification
void changeRotation(int direction) {
  if (rotation == STOP) {
    rotation = direction;
  }
  else {
    rotation = STOP;
  }
  printInfo();
}

// increases or decreases the motor power by POWER_INTERVAL
void changePower(int change) {
  int oldpower = power;
  if (change == INCREASE) {
    power = power <= 255 - POWER_INTERVAL ? power + POWER_INTERVAL : 255;
  }
  else if (change == DECREASE) {
    power = power >= 0 + POWER_INTERVAL ? power - POWER_INTERVAL : 0;
  }
  if (rotation == STOP) {
    Serial.println("ATTENTION: Motor is stopped! Please select direction.");
  }
  else if(oldpower == 0 && change == DECREASE) {
    Serial.println("ATTENTION: Motor is stopped! Cant decrease power.");
  }
  else if(oldpower == 255 && change == INCREASE) {
    Serial.println("ATTENTION: Motor is running at top speed! Cant increase power.");
  }
  else {
    printInfo();
  }
}

void applyRotationChanges() {
  if (rotation == CW) {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
  }
  else if (rotation == CCW) {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
  }
  else if (rotation == STOP) {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
  }
}

void applyPowerChanges() {
  analogWrite(MOTOR_PWM, power);
}

// sets the RGB-LED to "red" or "green" depending on system mode
void setLED() {
  if (system_mode == ROTATION_MODE) {
    analogWrite(LED_RED, 255);
    analogWrite(LED_GREEN, 0);
    analogWrite(LED_BLUE, 255);
  }
  else {
    analogWrite(LED_RED, 0);
    analogWrite(LED_GREEN, 255);
    analogWrite(LED_BLUE, 255);
  }
}

void printInfo() {
  Serial.println(
    (system_mode == ROTATION_MODE ? String("[ROTATION MODE]") : String("[POWER MODE]"))
     + " Motor direction: " + (rotation == 0 ? "STOP" : (rotation == -1 ? "CCW" : "CW"))
     + " | Motor power: " + String((int)((power / 255.0) * 100)) + " %");
}

void setup() {
  // set pin modes
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  // set interrupts for buttons
  attachInterrupt(digitalPinToInterrupt(BUTTON1), button1Interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON2), button2Interrupt, FALLING);
  // configure timer for debouncing
  timer1.configure(1000, debounce1);
  timer2.configure(1000, debounce2);
  // configure Serial interface
  Serial.begin(9600);
  Serial.flush();
  timer1_running = false;
  timer2_running = false;
}

void loop() {
  applyRotationChanges();
  applyPowerChanges();
  setLED();
}
