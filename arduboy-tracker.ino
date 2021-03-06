#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <stdint.h>
#include <EEPROM.h>

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////

const uint8_t timer_height = 3;
const uint8_t char_height = 7;
const uint8_t counters_width = 110;

struct CounterSettings {
  // Index of the counter in persistent memory. Useful if you
  // want your counters to survive when you reorder them.
  int memory_index;

  // Counter text.
  char *caption;

  // Whether you want to hear a Special Sound after every ten
  // steps.
  bool achievement_enabled;
};

// Put your own counters here.
const CounterSettings counterSettings[] = {
  { 4, "FEEL ADDICTION", true },
  { 8, "FEEL SLEEPY", true },
  { 1, "READ MESSAGES", true },
  { 2, "EXERT OWN WILL", true },
  { 5, "DAYS NO TWITTER", true },
  // 6
  { 7, "GRATITUDE ADMIT", true },
};

////////////////////////////////////////////////////////////
// LED
////////////////////////////////////////////////////////////

class LED {
  private:
    unsigned long last_flash = 0;

  public:
    void flash() {
      arduboy.setRGBled(GREEN_LED, 35);
      last_flash = millis();
    }
    void update() {
      if (last_flash == 0 || millis() - last_flash > 100) {
        arduboy.setRGBled(GREEN_LED, 0);
        last_flash = 0;
      }
    }
};

LED _LED;

////////////////////////////////////////////////////////////
// Sounds
////////////////////////////////////////////////////////////

class Sounds {
  public:
    void up_sound() {
      sound.tone(NOTE_C4H, 50);
    }
    void down_sound() {
      sound.tone(NOTE_A3H, 50);
    }
};

const uint16_t achievement_song[] PROGMEM = {
  NOTE_E4H, 200, NOTE_REST, 10,
  NOTE_D4H, 200, NOTE_REST, 10,
  NOTE_D4H, 400, NOTE_REST, 60,
  NOTE_FS4H, 200, NOTE_REST, 10,
  NOTE_G4H, 100,
  TONES_END
};

const uint16_t timer_song[] PROGMEM = {
  NOTE_B4H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_F5H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_B4H, 300, NOTE_REST, 160,
  NOTE_B4H, 300, NOTE_REST, 160,

  NOTE_B4H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_F5H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_B4H, 300, NOTE_REST, 580,

  NOTE_B4H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_F5H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_B4H, 300, NOTE_REST, 160,
  NOTE_B4H, 300, NOTE_REST, 160,

  NOTE_B4H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_F5H, 200, NOTE_REST, 20,
  NOTE_C5H, 200, NOTE_REST, 20,
  NOTE_B4H, 300, NOTE_REST, 160,
  TONES_END
};

Sounds _Sounds;

////////////////////////////////////////////////////////////
// Strings
////////////////////////////////////////////////////////////

// Shift the cursor
void shiftX(int16_t delta) {
  uint8_t x = arduboy.getCursorX();
  uint8_t y = arduboy.getCursorY();
  arduboy.setCursor(int16_t(x) + delta, y);
}

int8_t char_left_correction(char c) {
  if (c == ' ') return -1;
  if (c == ':') return -1;
  return 0;
}
int8_t char_right_correction(char c) {
  if (c == ' ') return -1;
  if (c == ':') return -1;
  return 0;
}

// Print a string with font size 1 and nice widths.
void print_str_pretty (char *str) {
  char* c = str;
  while (*c) {
    shiftX(char_left_correction(*c));
    arduboy.write(*c);
    shiftX(char_right_correction(*c));
    c++;
  }
}

// Get string width in pixels
uint8_t str_width (char *str) {
  char* c = str;
  if (*c == NULL) {
    return 0;
  } else {
    uint16_t sum = 0;
    while (*c) {
      sum += 5 + char_left_correction(*c) + char_right_correction(*c) + 1;
      c++;
    }
    sum--;
    return sum;
  }
}

////////////////////////////////////////////////////////////
// Timer
////////////////////////////////////////////////////////////

class Timer {
  private:
    unsigned long timer_minutes = 0;
    unsigned long timer_started = 0;

  public:
    bool is_stopped() {
      return timer_minutes == 0;
    }

    unsigned long elapsed() {
      return millis() - timer_started;
    }

    void start_or_inc() {
      if (is_stopped()) {
        timer_started = millis();
      }
      timer_minutes++;
      _Sounds.up_sound();
      _LED.flash();
    }

    void stop() {
      timer_minutes = 0;
      _Sounds.down_sound();
      _LED.flash();
    }

    void update() {
      if (!is_stopped() && elapsed() > timer_minutes * 60 * 1000) {
        timer_minutes = 0;
        sound.tones(timer_song);
      }
    }

    void render() {
      const uint8_t bar_height = 1;
      if (!is_stopped()) {
        // Draw the bar
        uint8_t width = round(128.0 * (1.0 - elapsed() / float(timer_minutes * 60 * 1000)));
        arduboy.fillRect(0, 64 - bar_height, width, bar_height);
        // Draw ticks
        for (int i = 0; i < timer_minutes; ++i) {
          uint8_t x = round(float(i) / float(timer_minutes) * 128.0);
          arduboy.fillRect(x, 64 - timer_height, 1, timer_height);
        }
        // Draw the last tick
        arduboy.fillRect(127, 64 - timer_height, 1, timer_height);
      }
      // Draw timer info when A is pressed
      if (arduboy.pressed(A_BUTTON)) {
        String info = "";
        unsigned long mm = (elapsed() / 1000) / 60;
        unsigned long ss = (elapsed() / 1000) % 60;
        info += String(mm) + ":";
        if (ss < 10) {
          info += "0" + String(ss);
        } else {
          info += String(ss);
        }
        if (timer_minutes != 0) {
          info += " / " + String(timer_minutes) + ":00";
        }
        uint8_t width = str_width(info.c_str());
        uint8_t x = (128 - str_width(info.c_str())) / 2; // text X
        uint8_t y_gap = 3;
        uint8_t y = 64 - timer_height - y_gap - char_height; // text Y
        arduboy.drawRect(0, y - y_gap - 1, 128, 1);
        arduboy.fillRect(0, y - y_gap, 128, char_height + y_gap * 2, BLACK);
        arduboy.setCursor(x, y);
        print_str_pretty(info.c_str());
      }
    }
};

Timer _Timer;

////////////////////////////////////////////////////////////
// Counters
////////////////////////////////////////////////////////////

class Counters {
  private:
    int current = 0;
    static const int COUNT = sizeof(counterSettings) / sizeof(CounterSettings);
    int values[COUNT];

  public:
    void move_up() {
      if (current == 0) {
        current = COUNT - 1;
      } else {
        current--;
      }
    }
    void move_down() {
      if (current == COUNT - 1) {
        current = 0;
      } else {
        current++;
      }
    }

    void inc_current() {
      values[current]++;
      if (values[current] % 10 == 0 &&
          values[current] != 0 &&
          counterSettings[current].achievement_enabled) {
        sound.tones(achievement_song);
      }
      else {
        _Sounds.up_sound();
      }
      _LED.flash();
      store();
    }
    void dec_current() {
      values[current]--;
      _Sounds.down_sound();
      _LED.flash();
      store();
    }
    void zero_current() {
      values[current] = 0;
      _Sounds.down_sound();
      _LED.flash();
      store();
    }

    void load() {
      for (int i = 0; i < COUNT; ++i) {
        EEPROM.get(100 + counterSettings[i].memory_index * 2, values[i]);
      }
    }
    void store() {
      for (int i = 0; i < COUNT; ++i) {
        // 'put' uses 'update' so this does not lead to extraneous EEPROM writes
        EEPROM.put(100 + counterSettings[i].memory_index * 2, values[i]);
      }
    }

    void render() {
      // Gap between lines
      const uint8_t Y_GAP = 9 - COUNT;
      // Size of the whole block
      const uint8_t X_SIZE = counters_width;
      const uint8_t Y_SIZE = 7 * COUNT + Y_GAP * (COUNT - 1);
      // Margins
      const uint8_t X_MARGIN = (128 - X_SIZE) / 2;
      const uint8_t Y_MARGIN = (64 - timer_height - Y_SIZE) / 2;

      arduboy.setTextSize(1);
      for (int i = 0; i < COUNT; ++i) {
        uint8_t x = X_MARGIN;
        uint8_t y = Y_MARGIN + (7 + Y_GAP) * i;
        if (i == current) {
          arduboy.fillTriangle(x - 8, y, x - 5, y + 3, x - 8, y + 6);
        }
        arduboy.setCursor(x, y);
        print_str_pretty(counterSettings[i].caption);
        arduboy.setCursor(x + X_SIZE - 9, y);
        arduboy.print(values[i]);
      }
    }
};

Counters _Counters;

////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////

void handle_input() {
  arduboy.pollButtons();

  if (arduboy.justPressed(UP_BUTTON)) {
    _Counters.move_up();
  }
  else if (arduboy.justPressed(DOWN_BUTTON)) {
    _Counters.move_down();
  }
  else if (arduboy.justPressed(RIGHT_BUTTON)) {
    _Counters.inc_current();
  }
  else if (arduboy.justPressed(LEFT_BUTTON) && arduboy.pressed(A_BUTTON)) {
    _Counters.zero_current();
  }
  else if (arduboy.justPressed(LEFT_BUTTON)) {
    _Counters.dec_current();
  }
  else if (arduboy.justPressed(B_BUTTON) && arduboy.pressed(A_BUTTON)) {
    _Timer.stop();
  }
  else if (arduboy.justPressed(B_BUTTON)) {
    _Timer.start_or_inc();
  }
}

void setup()
{
  // Used instead of arduboy.begin() to get rid of the logo
  arduboy.boot();
  arduboy.display();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.audio.begin();
  arduboy.waitNoButtons();

  // Initialize LEDs
  arduboy.setRGBled(0, 0, 0);

  _Counters.load();
}

void loop()
{
  if (!arduboy.nextFrame()) return;
  handle_input();
  _LED.update();
  _Timer.update();
  _Counters.render();
  _Timer.render();
  arduboy.display(CLEAR_BUFFER);
}
