/*
 * ============================================================
 * Death Race 2026 - Final RC Car Control Code
 * ============================================================
 * 
 * Components:
 *   - FlySky CT6B Receiver (CH1 Steering, CH3 Throttle)
 *   - BTS7960 43A Motor Driver
 *   - MG996R Steering Servo
 *   - Arduino Uno
 *   - 3S Li-po Battery 2200mAh + UBEC 5V/3A
 * 
 * Pin Connections (as per your diagram):
 *   - Pin 2 -> Receiver CH1 (Throttle signal)  [Interrupt 0]
 *   - Pin 3 -> Receiver CH2 (Steering signal) [Interrupt 1]
 *   - Pin 5 -> BTS7960 RPWM (Forward PWM)
 *   - Pin 6 -> BTS7960 LPWM (Reverse PWM)
 *   - Pin 9 -> Servo signal (Orange/Yellow)
 *   - 5V pin -> UBEC 5V output (powers Arduino)
 *   - GND   -> Common ground (UBEC, BTS7960, Battery)
 * 
 * Calibrated neutral values (from your measurements):
 *   - Throttle neutral = 1495 µs
 *   - Steering neutral = 1475 µs
 *   - Dead zones added to ignore jitter
 * 
 * Features:
 *   - Hardware interrupts for accurate pulse reading
 *   - Throttle dead zone & forced stop at neutral (prevents creeping)
 *   - Smooth acceleration ramping (prevents flipping)
 *   - Exponential steering (less twitchy at high speed)
 *   - Fail‑safe: stops motors if signal lost >500ms
 * 
 * ============================================================
 */

#include <Servo.h>

// ------------------- Pin Definitions ----------------------
#define THROTTLE_PIN  2   // Receiver CH1 (Interrupt 0)
#define STEERING_PIN  3   // Receiver CH2 (Interrupt 1)
#define RPWM_PIN      5   // BTS7960 forward PWM
#define LPWM_PIN      6   // BTS7960 reverse PWM
#define SERVO_PIN     9   // Servo signal

// ------------------- Calibrated Neutral Values ------------
#define THROTTLE_NEUTRAL  1495   // µs (from your data)
#define STEERING_NEUTRAL  1475   // µs (from your data)

// ------------------- Constants ----------------------------
const int THROTTLE_DEADZONE = 60;    // µs range around neutral to ignore
const int STEERING_DEADZONE = 50;    // optional, prevents servo twitch
const int RAMP_STEP = 5;             // speed change per loop (higher = faster accel)
const int EXPO_FACTOR = 40;          // steering expo: 0=linear, 100=very exponential
const unsigned long FAILSAFE_MS = 500; // no signal for this long -> stop

// ------------------- Servo Object -------------------------
Servo steeringServo;

// ------------------- Volatile Interrupt Variables ---------
volatile unsigned long throttleStart = 0;
volatile int throttlePulse = THROTTLE_NEUTRAL;   // start at neutral
volatile unsigned long steeringStart = 0;
volatile int steeringPulse = STEERING_NEUTRAL;   // start at neutral
volatile unsigned long lastSignalTime = 0;

// ------------------- Normal Variables ---------------------
int currentSpeed = 0;    // ramped speed (-255 to 255)
int targetSpeed;

// =================== INTERRUPT SERVICE ROUTINES ===========
void throttleISR() {
  if (digitalRead(THROTTLE_PIN) == HIGH) {
    throttleStart = micros();           // start of pulse
  } else {
    throttlePulse = (int)(micros() - throttleStart);
    lastSignalTime = millis();          // update fail‑safe timer
  }
}

void steeringISR() {
  if (digitalRead(STEERING_PIN) == HIGH) {
    steeringStart = micros();
  } else {
    steeringPulse = (int)(micros() - steeringStart);
    lastSignalTime = millis();
  }
}

// =================== SETUP ================================
void setup() {
  Serial.begin(9600);   // for debugging (optional)

  // Set pin modes
  pinMode(RPWM_PIN, OUTPUT);
  pinMode(LPWM_PIN, OUTPUT);
  pinMode(THROTTLE_PIN, INPUT);
  pinMode(STEERING_PIN, INPUT);

  // Attach servo
  steeringServo.attach(SERVO_PIN);

  // Attach interrupts (trigger on CHANGE)
  attachInterrupt(digitalPinToInterrupt(THROTTLE_PIN), throttleISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(STEERING_PIN), steeringISR, CHANGE);

  // Initial safe state
  analogWrite(RPWM_PIN, 0);
  analogWrite(LPWM_PIN, 0);
  steeringServo.write(90);   // centre servo
  currentSpeed = 0;

  Serial.println("Death Race Car Ready – Interrupts Active");
}

// =================== MAIN LOOP ============================
void loop() {
  // ----- Safely copy volatile variables -----
  noInterrupts();
  int throttle = throttlePulse;
  int steering = steeringPulse;
  unsigned long timeSinceLast = millis() - lastSignalTime;
  interrupts();

  // ----- FAIL‑SAFE: Signal lost? -----
  if (timeSinceLast > FAILSAFE_MS) {
    analogWrite(RPWM_PIN, 0);
    analogWrite(LPWM_PIN, 0);
    steeringServo.write(90);
    currentSpeed = 0;
    Serial.println("FAILSAFE: Signal lost");
    delay(20);
    return;
  }

  // ----- THROTTLE PROCESSING -----
  // Apply dead zone around calibrated neutral
  if (abs(throttle - THROTTLE_NEUTRAL) < THROTTLE_DEADZONE) {
    // Force stop: motors off, reset ramping
    analogWrite(RPWM_PIN, 0);
    analogWrite(LPWM_PIN, 0);
    currentSpeed = 0;
    targetSpeed = 0;
  } else {
    // Map pulse to target speed (-255..255)
    // Assuming full range 1000–2000 µs (adjust if your transmitter has narrower range)
    targetSpeed = map(throttle, 1120, 1813, -255, 255);
    targetSpeed = constrain(targetSpeed, -255, 255);

    // ----- Smooth acceleration (ramping) -----
    if (currentSpeed < targetSpeed) {
      currentSpeed += RAMP_STEP;
      if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
    } else if (currentSpeed > targetSpeed) {
      currentSpeed -= RAMP_STEP;
      if (currentSpeed < targetSpeed) currentSpeed = targetSpeed;
    }

    // ----- Apply motor speed (BTS7960) -----
    if (currentSpeed >= 0) {
      analogWrite(RPWM_PIN, currentSpeed);
      analogWrite(LPWM_PIN, 0);
    } else {
      analogWrite(RPWM_PIN, 0);
      analogWrite(LPWM_PIN, -currentSpeed);   // make positive for reverse PWM
    }
  }

  // ----- STEERING PROCESSING (with exponential) -----
  // Apply dead zone around calibrated steering neutral
  if (abs(steering - STEERING_NEUTRAL) < STEERING_DEADZONE) {
    steering = STEERING_NEUTRAL;   // centre the servo
  }

  // Normalise steering pulse to -100..100 range
  // Assuming full range 1000–2000 µs (adjust if needed)
  int steerNorm = map(steering, 1093, 1878, -100, 100);
  steerNorm = constrain(steerNorm, -100, 100);

  // Exponential curve: output = (input^3) / (100^2)
  long steerExp;
  if (steerNorm >= 0) {
    steerExp = ((long)steerNorm * steerNorm * steerNorm) / 10000L;
  } else {
    steerExp = -((long)(-steerNorm) * (-steerNorm) * (-steerNorm)) / 10000L;
  }

  // Mix linear and exponential based on EXPO_FACTOR
  int mixed = (steerNorm * (100 - EXPO_FACTOR) + (int)steerExp * EXPO_FACTOR) / 100;

  // Map mixed (-100..100) to servo angle (0..180)
  int servoAngle = map(mixed, -100, 100, 0, 180);
  servoAngle = constrain(servoAngle, 0, 180);
  steeringServo.write(servoAngle);

  // ----- LOOP TIMING -----
  delay(20);   // 20ms = 50Hz loop rate – stable and responsive
}
