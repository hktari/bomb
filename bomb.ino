#include <TimeLib.h>
#include <TM1637Display.h>
#include "button.h"

#define DEBUG

enum DEFUSED_STATE
{
  WIRE_1_CUT = 1,
  WIRE_2_CUT = 2,
  WIRE_3_CUT = 4
};

enum BOMB_STATE
{
  IDLE,
  OPERATIONAL,
  EXPLODED,
  DEFUSED
};

#define DP_2_DATA_PIN 6 // TM1637
#define DP_2_CLK_PIN 5  // TM1637

#define DP_1_LATCH_PIN 3 // Latch pin of 74HC595 is connected to Digital pin 5
#define DP_1_CLK_PIN 4   // Clock pin of 74HC595 is connected to Digital pin 6
#define DP_1_DATA_PIN 2  // Data pin of 74HC595 is connected to Digital pin 4

#define BOMB_WIRE_ONE_PIN 12
#define BOMB_WIRE_TWO_PIN 11
#define BOMB_WIRE_THREE_PIN 10

#define SET_BOMB_TIME_BTN_PIN 9
#define RED_LED_PIN 8
#define GREEN_LED_PIN 7

#define BUZZER_PIN A1

#define MOTOR_A A2
#define MOTOR_B A3
#define MOTOR_C A4
#define MOTOR_D A5

uint8_t state_defused = 0;
BOMB_STATE cur_state = BOMB_STATE::IDLE;
Button set_bomb_time_btn(SET_BOMB_TIME_BTN_PIN);

TM1637Display display(DP_2_CLK_PIN, DP_2_DATA_PIN);

unsigned long bomb_explode_duration = 0;
unsigned long MAX_BOMB_TIME = 6UL * 3600UL * 1000UL; // 6 hrs

time_t bomb_started_time = 0;

uint8_t dp_digits[5] = {};

void write_motor(int a, int b, int c, int d)
{
  digitalWrite(MOTOR_A, a);
  digitalWrite(MOTOR_B, b);
  digitalWrite(MOTOR_C, c);
  digitalWrite(MOTOR_D, d);
}

void onestep()
{
  const int motor_delay = 1000;

  write_motor(1, 0, 0, 0);
  delayMicroseconds(motor_delay);
  write_motor(1, 1, 0, 0);
  delayMicroseconds(motor_delay);
  write_motor(0, 1, 0, 0);
  delayMicroseconds(motor_delay);
  write_motor(0, 1, 1, 0);
  delayMicroseconds(motor_delay);
  write_motor(0, 0, 1, 0);
  delayMicroseconds(motor_delay);
  write_motor(0, 0, 1, 1);
  delayMicroseconds(motor_delay);
  write_motor(0, 0, 0, 1);
  delayMicroseconds(motor_delay);
  write_motor(1, 0, 0, 1);
  delayMicroseconds(motor_delay);
}

void explode()
{
  switch_state(BOMB_STATE::EXPLODED);
  int i;
  i = 1;
  #define NUMBER_OF_STEPS_PER_REV 312
  while (i < NUMBER_OF_STEPS_PER_REV)
  {
    onestep();
    i++;

    if(i%80 == 0) tone(BUZZER_PIN, 1300, 150);

  }


  write_motor(0,0,0,0);
}

void switch_state(BOMB_STATE state)
{
  cur_state = state;
  Serial.print("Switching to: ");
  String state_str = "";
  switch (state)
  {
  case BOMB_STATE::IDLE:
    state_str = "IDLE";
    break;
  case BOMB_STATE::OPERATIONAL:
    state_str = "OPERATIONAL";
    break;
  case BOMB_STATE::EXPLODED:
    state_str = "EXPLODED";
    break;
  case BOMB_STATE::DEFUSED:
    state_str = "DEFUSED";
    break;
  }
  Serial.print(state_str);
  Serial.println();
}

void setup()
{
  pinMode(set_bomb_time_btn.pin, INPUT_PULLUP);

  pinMode(BOMB_WIRE_ONE_PIN, INPUT_PULLUP);
  pinMode(BOMB_WIRE_TWO_PIN, INPUT_PULLUP);
  pinMode(BOMB_WIRE_THREE_PIN, INPUT_PULLUP);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Set all the pins of 74HC595 as OUTPUT
  pinMode(DP_1_LATCH_PIN, OUTPUT);
  pinMode(DP_1_CLK_PIN, OUTPUT);
  pinMode(DP_1_DATA_PIN, OUTPUT);

  pinMode(MOTOR_A, OUTPUT);
  pinMode(MOTOR_B, OUTPUT);
  pinMode(MOTOR_C, OUTPUT);
  pinMode(MOTOR_D, OUTPUT);

  Serial.begin(115200);

  display.setBrightness(8);

  duration_to_digits_arr(0, dp_digits);
  update_display(dp_digits);
}

void loop()
{
  if (cur_state == BOMB_STATE::IDLE)
  {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);

    set_bomb_time_btn.update();

    if (set_bomb_time_btn.transitioned_to(HIGH))
    {
      const unsigned long set_timer_step_sec = 30UL * 60UL; // 30 min
      bomb_explode_duration += set_timer_step_sec;
      tone(BUZZER_PIN, 700, 100);
      if (bomb_explode_duration > MAX_BOMB_TIME)
      {
        bomb_explode_duration = 0;
      }

      Serial.println(String("Set timer to: ") + String(bomb_explode_duration / 60) + String(" minutes"));

      duration_to_digits_arr(bomb_explode_duration, dp_digits);
      update_display(dp_digits);
    }
    else if (set_bomb_time_btn.long_press())
    {
      switch_state(BOMB_STATE::OPERATIONAL);
      bomb_started_time = now();

      digitalWrite(RED_LED_PIN, LOW);
      tone(BUZZER_PIN, 1300, 350);
      delay(350);
      tone(BUZZER_PIN, 1300, 350);
      delay(350);
      digitalWrite(RED_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1300, 1000);
    }
  }
  else if (cur_state == BOMB_STATE::OPERATIONAL)
  {
    if (digitalRead(BOMB_WIRE_ONE_PIN) == HIGH)
    {
      Serial.println("1 is HIGH");
      state_defused |= DEFUSED_STATE::WIRE_1_CUT;
    }
    if (digitalRead(BOMB_WIRE_TWO_PIN) == HIGH)
      state_defused |= DEFUSED_STATE::WIRE_2_CUT;
    if (digitalRead(BOMB_WIRE_THREE_PIN) == HIGH)
      state_defused |= DEFUSED_STATE::WIRE_3_CUT;

    const uint8_t DEFUSED_SUCCESS = DEFUSED_STATE::WIRE_1_CUT | DEFUSED_STATE::WIRE_2_CUT | DEFUSED_STATE::WIRE_3_CUT;
    const uint8_t DEFUSED_WRONG = DEFUSED_STATE::WIRE_3_CUT | DEFUSED_STATE::WIRE_2_CUT;

    if ((state_defused & DEFUSED_SUCCESS) == DEFUSED_SUCCESS)
    {
      switch_state(BOMB_STATE::DEFUSED);

      digitalWrite(GREEN_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1300, 350);
      delay(250);
      digitalWrite(GREEN_LED_PIN, LOW);
      delay(100);

      digitalWrite(GREEN_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1300, 350);
      delay(250);
      digitalWrite(GREEN_LED_PIN, LOW);
      delay(100);

      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1300, 350);
    }
    // Wrong wires cut
    else if ((state_defused & DEFUSED_WRONG) == DEFUSED_WRONG)
    {
      explode();
    }
    // If wire 1 hasn't been cut
    else if ((state_defused & DEFUSED_STATE::WIRE_1_CUT) != DEFUSED_STATE::WIRE_1_CUT && (state_defused & DEFUSED_WRONG) > 0)
    {
      static bool err_handled = false;

      if (!err_handled)
      {
        Serial.println("ERR HANDLED");
        bomb_started_time = now() - bomb_explode_duration + (2L * 60L); // set time left to 2 minutes

        digitalWrite(RED_LED_PIN, HIGH);
        tone(BUZZER_PIN, 1300, 350);
        delay(250);
        digitalWrite(RED_LED_PIN, LOW);
        delay(100);

        digitalWrite(RED_LED_PIN, HIGH);
        tone(BUZZER_PIN, 1300, 350);
        delay(250);

        err_handled = true;
      }
    }

    // Pulse
    tone(BUZZER_PIN, 1300, 100);

    // Update display
    auto time_left = bomb_started_time + bomb_explode_duration - now();
    if (time_left < 0)
    {
      time_left = 0;
    }
#ifdef DEBUG
    Serial.print("TIME LEFT (s): ");
    Serial.print(time_left);
    Serial.println();
#endif
    duration_to_digits_arr(time_left, dp_digits);
    update_display(dp_digits);

    // Times up
    if (time_left == 0)
    {
      explode();
    }
  }
  else if (cur_state == BOMB_STATE::DEFUSED)
  {
  }
  else if (cur_state == BOMB_STATE::EXPLODED)
  {
    // TODO: after 5 sec go to idle
  }

  if (cur_state == BOMB_STATE::OPERATIONAL)
  {
    delay(1300);
  }
  else
  {
    delay(100);
  }
}

// duration is in sec
void duration_to_digits_arr(time_t duration, uint8_t digits[5])
{
  // duration = duration / 1000L;
  int h = numberOfHours(duration);
  int min = numberOfMinutes(duration);
  int sec = numberOfSeconds(duration);

  int mins_combined = h * 60 + min;

#ifdef DEBUG
  Serial.print(h);
  Serial.print(":");
  Serial.print(min);
  Serial.print(":");
  Serial.print(sec);
  Serial.println();

  Serial.print("TOTAL MINS: ");
  Serial.print(mins_combined);
  Serial.println();
#endif

  digits[0] = mins_combined / 100 % 10;
  digits[1] = mins_combined / 10 % 10;
  digits[2] = mins_combined % 10;
  digits[3] = sec / 10 % 10;
  digits[4] = sec % 10;
}

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
const uint8_t digitToSegment[] = {
    // XGFEDCBA
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b01110111, // A
    0b01111100, // B
    0b00111001, // C
    0b01011110, // D
    0b01111001, // E
    0b01110001  // F
};

uint8_t encoded_digits[4] = {}; // TM1637 buffer

void update_display(uint8_t digits[5])
{
  // Update sev seg display for digit 0
  digitalWrite(DP_1_LATCH_PIN, LOW);
  shiftOut(DP_1_DATA_PIN, DP_1_CLK_PIN, MSBFIRST, digitToSegment[digits[0]]);
  digitalWrite(DP_1_LATCH_PIN, HIGH);

  //  Update tm1637 for digits 1-4
  encoded_digits[0] = display.encodeDigit(digits[1]);
  encoded_digits[1] = display.encodeDigit(digits[2]);
  encoded_digits[1] |= 0b10000000; // Show ':'
  encoded_digits[2] = display.encodeDigit(digits[3]);
  encoded_digits[3] = display.encodeDigit(digits[4]);

  display.setSegments(encoded_digits);
}