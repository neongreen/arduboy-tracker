#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <stdint.h>
#include <EEPROM.h>

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////

const uint8_t timer_height = 3; // pixels

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
  { 4, "FEEL ADDICT", true },
  { 1, "READ MESSG", true },
  { 2, "EXERT WILL", true },
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

// Print a string with font size 1 and non-monospace spaces.
void print_str_pretty (char *str) {
  uint8_t x = arduboy.getCursorX();
  uint8_t y = arduboy.getCursorY();
  char* c = str;
  while (*c) {
    arduboy.write(*c);
    if (*c == ' ') {
      x += 4;
    } else {
      x += 6;
    }
    arduboy.setCursor(x, y);
    c++;
  }
}

// Get string width in pixels
uint8_t str_width (char *str) {
  char* c = str;
  if (*c == NULL) {
    return 0;
  } else {
    uint8_t sum = 0;
    while (*c) {
      if (*c == ' ') {
        sum += 4;
      } else {
        sum += 6;
      }
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
      if (arduboy.pressed(A_BUTTON)) {
        String info = "";
        info += String(elapsed() / 1000);
        info += String("s");
        if (timer_minutes != 0) {
          info += String(" of ");
          info += String(timer_minutes * 60);
          info += String("s");
        }
        uint8_t width = str_width(info.c_str());
        uint8_t x = (128 - str_width(info.c_str())) / 2;
        arduboy.fillRect(x - 1, 54, width + 2, 9, BLACK);
        arduboy.setCursor(x, 55);
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
      const uint8_t Y_SIZE = 7 * COUNT + Y_GAP * (COUNT - 1);
      // Gap between the top and the block
      const uint8_t Y_TOP = (64 - timer_height - Y_SIZE) / 2;

      arduboy.setTextSize(1);
      for (int i = 0; i < COUNT; ++i) {
        uint8_t x = 18;
        uint8_t y = Y_TOP + (7 + Y_GAP) * i;
        if (i == current) {
          arduboy.fillTriangle(x, y, x + 3, y + 3, x, y + 6);
        }
        arduboy.setCursor(x + 8, y);
        print_str_pretty(counterSettings[i].caption);
        arduboy.setCursor(x + 80, y);
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
