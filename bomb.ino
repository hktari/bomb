class Button{
  int prev_state;
  int cur_state;
  
  public:
    const int pin;

  public:
    Button(int pin) : pin(pin) {
      prev_state = cur_state = 0;
    }
    int get_state(){
      return cur_state;
    }

  inline void set_state(int state){
    prev_state = cur_state;
    cur_state = state;
  }
};

// const int SET_BOMB_TIME_BTN = 10;
// const int ARM_BOMB_BTN = 9;

enum BOMB_STATE {
  IDLE,
  OPERATIONAL,
  EXPLODED,
  DEFUSED
};

const int BOMB_WIRE_ONE = 12;
const int BOMB_WIRE_TWO = 11;
 

BOMB_STATE cur_state = BOMB_STATE::IDLE;
Button arm_bomb_btn(9), set_bomb_time_btn(10);

unsigned long bomb_explode_time = 0;
unsigned long MAX_BOMB_TIME = 6UL * 3600UL * 1000UL; // 6 hrs

unsigned long bomb_timer = 0;

void setup() {
  pinMode(set_bomb_time_btn.pin, INPUT_PULLUP);
  pinMode(arm_bomb_btn.pin, INPUT_PULLUP);  

  pinMode(BOMB_WIRE_ONE, INPUT_PULLUP);
  pinMode(BOMB_WIRE_TWO, INPUT_PULLUP);

}

void loop() {
  if(cur_state == BOMB_STATE::IDLE){

    set_bomb_time_btn.set_state(digitalRead(set_bomb_time_btn.pin));
    arm_bomb_btn.set_state(digitalRead(arm_bomb_btn.pin));

    if(set_bomb_time_btn.get_state() == LOW){
      const unsigned long set_timer_step_millis = 30UL * 60UL * 1000UL; // 30 min
      bomb_explode_time += set_timer_step_millis;
      bomb_explode_time %= MAX_BOMB_TIME;
    } else if(arm_bomb_btn.get_state() == LOW){
      cur_state == BOMB_STATE::OPERATIONAL;
      bomb_timer = 0;
      // TODO: play armed sfx
      
    }
  }
  else if (cur_state == BOMB_STATE::OPERATIONAL){
    bomb_timer += millis();

    if(digitalRead(BOMB_WIRE_ONE) == LOW){
      cur_state == BOMB_STATE::DEFUSED;
    }
    else if(bomb_timer > bomb_explode_time){
      cur_state == BOMB_STATE::EXPLODED;
      // TODO: play explode sfx
    } 
  }
  else if (cur_state == BOMB_STATE::DEFUSED){
    // TODO: play defused sfx
    // TODO: show green lights
    // TODO: go to idle
  }
  else if (cur_state == BOMB_STATE::EXPLODED){
    // TODO: after 5 sec go to idle
  }
}