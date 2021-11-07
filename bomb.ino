
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

const int BOMB_WIRE_ONE_PIN = 12;
const int BOMB_WIRE_TWO_PIN = 11;
const int SET_BOMB_TIME_BTN_PIN = 10;
const int ARM_BOMB_BTN_PIN = 9;

const int BUZZER_PIN = A1;
const int RED_LED_PIN = 2;
const int GREEN_LED_PIN = 3;

BOMB_STATE cur_state = BOMB_STATE::IDLE;
Button arm_bomb_btn(ARM_BOMB_BTN_PIN), set_bomb_time_btn(SET_BOMB_TIME_BTN_PIN);

unsigned long bomb_explode_time = 0;
unsigned long MAX_BOMB_TIME = 6UL * 3600UL * 1000UL; // 6 hrs

unsigned long bomb_timer = 0;

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

  Serial.begin(9600);
  Serial.println("Begin");
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
      const unsigned long set_timer_step_millis = 30UL * 60UL * 1000UL; // 30 min
      bomb_explode_time += set_timer_step_millis;
      tone(BUZZER_PIN, 700, 100);
      if (bomb_explode_time > MAX_BOMB_TIME)
      {
        bomb_explode_time = 0;
      }

      Serial.println(String("Set timer to: ") + String(bomb_explode_time / 60 / 1000UL) + String(" minutes"));
    }
    else if (arm_bomb_btn.transitioned_to(LOW))
    {
      switch_state(BOMB_STATE::OPERATIONAL);
      bomb_timer = 0;

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
    bomb_timer += millis();

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
    else if (bomb_timer > bomb_explode_time)
    {
      switch_state(BOMB_STATE::EXPLODED);
      // TODO: play explode sfx
    }
    else
    {
      // Pulse
      tone(BUZZER_PIN, 1300, 100);
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
