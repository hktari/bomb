#include <TimeLib.h>
#define DEBUG

class Button
{
  int prev_state;
  int cur_state;

public:
  const int pin;

public:
  Button(int pin) : pin(pin)
  {
    prev_state = cur_state = 0;
  }

  void update()
  {
    set_state(digitalRead(pin));
  }

  bool transitioned_to(int state)
  {
    return prev_state != state && cur_state == state;
  }

  int get_state()
  {
    return cur_state;
  }

private:
  void set_state(int state)
  {
    prev_state = cur_state;
    cur_state = state;
  }
};

// const int SET_BOMB_TIME_BTN_PIN = 10;
// const int ARM_BOMB_BTN_PIN = 9;

enum BOMB_STATE
{
  IDLE,
  OPERATIONAL,
  EXPLODED,
  DEFUSED
};

const int DP_LATCH_PIN = 5; // Latch pin of 74HC595 is connected to Digital pin 5
const int DP_CLK_PIN = 6;   // Clock pin of 74HC595 is connected to Digital pin 6
const int DP_DATA_PIN = 4;  // Data pin of 74HC595 is connected to Digital pin 4

const int BOMB_WIRE_ONE_PIN = 12;
const int BOMB_WIRE_TWO_PIN = 11;
const int SET_BOMB_TIME_BTN_PIN = 10;
const int ARM_BOMB_BTN_PIN = 9;

const int BUZZER_PIN = A1;
const int RED_LED_PIN = 2;
const int GREEN_LED_PIN = 3;

BOMB_STATE cur_state = BOMB_STATE::IDLE;
Button arm_bomb_btn(ARM_BOMB_BTN_PIN), set_bomb_time_btn(SET_BOMB_TIME_BTN_PIN);

unsigned long bomb_explode_duration = 0;
unsigned long MAX_BOMB_TIME = 6UL * 3600UL * 1000UL; // 6 hrs

time_t bomb_started_time = 0;

int dp_digits[6] = {};

void switch_state(BOMB_STATE state)
{
  cur_state = state;
  Serial.print("Switching to: ");
  Serial.print(state);
  Serial.println();
}

void setup()
{
  pinMode(set_bomb_time_btn.pin, INPUT_PULLUP);
  pinMode(arm_bomb_btn.pin, INPUT_PULLUP);

  pinMode(BOMB_WIRE_ONE_PIN, INPUT_PULLUP);
  pinMode(BOMB_WIRE_TWO_PIN, INPUT_PULLUP);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Set all the pins of 74HC595 as OUTPUT
  pinMode(DP_LATCH_PIN, OUTPUT);
  pinMode(DP_CLK_PIN, OUTPUT);
  pinMode(DP_DATA_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Begin");

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
    arm_bomb_btn.update();

    if (set_bomb_time_btn.transitioned_to(LOW))
    {
      const unsigned long set_timer_step_sec = 30UL * 60UL; // 30 min
      bomb_explode_duration += set_timer_step_sec;
      tone(BUZZER_PIN, 700, 100);
      if (bomb_explode_duration > MAX_BOMB_TIME)
      {
        bomb_explode_duration = 0;
      }

      Serial.println(String("Set timer to: ") + String(bomb_explode_duration / 60) + String(" minutes"));

      duration_to_digits_arr(set_timer_step_sec, dp_digits);
      update_display(dp_digits);
    }
    else if (arm_bomb_btn.transitioned_to(LOW))
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
    else if (now() > bomb_started_time + bomb_explode_duration)
    {
      switch_state(BOMB_STATE::EXPLODED);
      // TODO: play explode sfx
    }
    else
    {
      // Pulse
      tone(BUZZER_PIN, 1300, 100);

      // Update display
      auto time_left = bomb_started_time + bomb_explode_duration - now();
      if (time_left < 0)
      {
        time_left = 0;
      }

      duration_to_digits_arr(time_left, dp_digits);
      update_display(dp_digits);
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

// A B C D E F G DP	Numbers	HEX code
// ---------------------------------
// 1 1 1 1 1 1 0 0	    0	      0x3F
// 0 1 1 0 0 0 0 0	    1	      0x06
// 1 1 0 1 1 0 1 0	    2	      0x5B
// 1 1 1 1 0 0 1 0	    3	      0x4F
// 0 1 1 0 0 1 1 0	    4	      0x66
// 1 0 1 1 0 1 1 0	    5	      0x6D
// 1 0 1 1 1 1 1 0	    6	      0x7D
// 1 1 1 0 0 0 0 0	    7	      0x07
// 1 1 1 1 1 1 1 0	    8	      0x7F
// 1 1 1 1 0 1 1 0	    9	      0x67
byte digit_to_DP_code(int digit)
{
  if (digit > 9 || digit < 0)
  {
    Serial.println("Wrong input to 'digit_to_DP_code' !");
    Serial.println(digit);
    return 0;
  }
  else
  {
    byte codes_map[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67};
    return codes_map[digit];
  }
}

void duration_to_digits_arr(time_t duration, int digits[6])
{
  int h = hour(duration);
  int min = minute(duration);
  int sec = second(duration);

#ifdef DEBUG
  Serial.print(h);
  Serial.print(":");
  Serial.print(min);
  Serial.print(":");
  Serial.print(sec);
  Serial.println();
#endif

  digits[0] = h / 10 % 10;
  digits[1] = h % 10;
  digits[2] = min / 10 % 10;
  digits[3] = min % 10;
  digits[4] = sec / 10 % 10;
  digits[5] = sec % 10;
}

// You've got 6 displays. 6 shift registers
// Wired from left most hour digit towards right most seconds digit.
// digits is an array of length 6, containing numbers between [0-9]
void update_display(int digits[6])
{
  digitalWrite(DP_LATCH_PIN, LOW);

  for (size_t i = 5; i > 0; i--)
  {
#ifdef DEBUG
    Serial.print("Displaying digit: ");
    Serial.print(digits[i], DEC);
#endif
    shiftOut(DP_DATA_PIN, DP_CLK_PIN, LSBFIRST, digit_to_DP_code(digits[i]));
  }

  digitalWrite(DP_LATCH_PIN, HIGH);
}