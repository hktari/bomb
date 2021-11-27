class Button
{
  int prev_state;
  int cur_state;
  time_t last_transition_time = 0;
  bool consider_long_press = false;

public:
  const int pin;
  const int long_press_trigger;

public:
  Button(int pin, int long_press_trigger = 2000) : pin(pin), long_press_trigger(long_press_trigger)
  {
    prev_state = cur_state = 0;
  }

  void update()
  {
    set_state(digitalRead(pin));

    if (transitioned_to(LOW))
    {
      last_transition_time = millis();
      consider_long_press = true;
    }
    else if (transitioned_to(HIGH))
    {
      consider_long_press = false;
    }
  }

  bool transitioned_to(int state)
  {
    return prev_state != state && cur_state == state;
  }

  int get_state()
  {
    return cur_state;
  }

  bool long_press()
  {
    if (consider_long_press)
    {
#ifdef DEBUG
      Serial.print("TRIGGER AT: ");
      Serial.print(last_transition_time + long_press_trigger);
      Serial.println();
      Serial.println(millis(), DEC);
#endif
      return (last_transition_time + long_press_trigger) < millis();
    }
    return false;
  }

private:
  void set_state(int state)
  {
    prev_state = cur_state;
    cur_state = state;
  }
};
