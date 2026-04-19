/*
 Demonstration sketch for Adafruit i2c/SPI LCD backpack
 using MCP23008 I2C expander
 ( https://learn.adafruit.com/i2c-spi-lcd-backpack )

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * 5V to Arduino 5V pin
 * GND to Arduino GND pin
 * SCL to Digital: Micro PC5 or D19/SCL
 * SDA to Digital: Micro PC4 or D18/SDA
*/

// include the library code:
#include <Adafruit_LiquidCrystal.h>
#include "pitches.h"


// Connect via i2c, default address #0 (A0-A2 not jumpered)
// Group all instances into a single list named lcds
Adafruit_LiquidCrystal lcds[] = {
  Adafruit_LiquidCrystal(0), // LCD 0: Hello world and count
  Adafruit_LiquidCrystal(1), // LCD 1: Autoscroll
  Adafruit_LiquidCrystal(2)  // LCD 2: Casino
};

// Calculate the number of LCDs dynamically for flexibility
const uint8_t NUM_LCDS = sizeof(lcds) / sizeof(lcds[0]);

// ----------------------------------- Casino Config ---------------------------
const int BUTTON_PIN = 7;
const int BUZZER = 8; // Not working in Tinkercad.
const bool ALWAYS_WIN = false;
const char* symbols[] = { "7", "BAR", "$", "*", "#" };
const int NUM_SYMBOLS  = 5;

int melody_lose[] = {
  NOTE_C6, NOTE_B5, NOTE_AS5, NOTE_A5, NOTE_GS5, NOTE_FS5, NOTE_F5, NOTE_C2, NOTE_CS2, NOTE_C2, NOTE_CS2, NOTE_C2, NOTE_CS2, NOTE_C2, NOTE_CS2, NOTE_C2
};
int durations_lose[] = {
  8, 16, 16, 16, 16, 16, 4, 4, 4, 16, 16, 16, 16, 2, 16, 16
};

int melody_win[] = {
  NOTE_C2, NOTE_CS2, NOTE_C2, NOTE_CS2, NOTE_C2
};
int durations_win[] = {
  16, 16, 16, 16, 16
};

byte starChar[8] = {
  0b00100, 0b10101, 0b01110, 0b11111,
  0b01110, 0b10101, 0b00100, 0b00000
};
byte coinChar[8] = {
  0b01110, 0b10001, 0b10101, 0b10111,
  0b10101, 0b10001, 0b01110, 0b00000
};
byte heartChar[8] = {
  0b00000, 0b01010, 0b11111, 0b11111,
  0b01110, 0b00100, 0b00000, 0b00000
};

// Estado del juego
int  reel[3]       = {0, 0, 0};
bool lastButton    = false;
int  credits       = 10;
bool gameOver      = false;
// ------------------------------------------------------------------------------


// -----------------Function prototypes----------------------------------------
// ### LCD 0: Hello world and count ###
void run_lcd_hello_world(Adafruit_LiquidCrystal& lcd);
void run_lcd_counter(Adafruit_LiquidCrystal& lcd);

// ### LCD 1: Autoscroll ###
void run_lcd_autoscroll(Adafruit_LiquidCrystal& lcd);

// ### LCD 2: Casino ###
void run_lcd_casino(Adafruit_LiquidCrystal& lcd);
void run_ldc_casino_init(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_showIdle(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_spinAnimation(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_showResult(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_winAnimation(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_loseAnimation(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_gameOverScreen(Adafruit_LiquidCrystal& lcd);
void run_lcd_casino_PlayMusic(int melody[], int durations[], int size);


void setup() {
  Serial.begin(9600); // Initialize serial communication for error messages

  // Global Operations: Loop to handle common setup for all instances
  for (uint8_t i = 0; i < NUM_LCDS; i++) {
    // set up the LCD's number of rows and columns:
    if (!lcds[i].begin(16, 2)) {
      Serial.print("Could not init LCD ");
      Serial.print(i);
      Serial.println(". Check wiring.");
      while(1);
    }
    
    // Clear and reset the screen for each instance initially
    lcds[i].clear();
  }

  // Display "Hello World" on the first LCD
  run_lcd_hello_world(lcds[0]);
  run_ldc_casino_init(lcds[2]);
}

void loop() {
  // Main Loop Logic: Pass specific list elements to their respective routines
  run_lcd_counter(lcds[0]);
  run_lcd_autoscroll(lcds[1]);
  run_lcd_casino(lcds[2]);
}



// --------------------LCD 0: Hello World and Count--------------------
// [Full Implementation] Displays "Hello World" and a running counter
void run_lcd_hello_world(Adafruit_LiquidCrystal& lcd) {
  // Display message on the first line
  lcd.setCursor(0, 0);
  lcd.print("Hello World");
}

void run_lcd_counter(Adafruit_LiquidCrystal& lcd) {
  // Display the running counter on the second line
  lcd.setCursor(0, 1);
  lcd.print(millis() / 1000);
}

// If 500ms haven't passed, the function returns instantly without blocking.
void run_lcd_autoscroll(Adafruit_LiquidCrystal& lcd) {
  // Static variables to remember the state between each loop() cycle
  static uint32_t previous_time = 0;
  static uint8_t state = 0;       // 0 = first loop, 1 = second loop (autoscroll), 2 = clear
  static uint8_t thisChar = 0;    // Current character to be printed
  
  const uint32_t interval = 500; // 500 ms wait time
  uint32_t current_time = millis();

  // Only execute if 500ms have passed
  if (current_time - previous_time >= interval) {
    previous_time = current_time; // Save the new time

    // State machine to not block the loop of other LCDs
    switch (state) {
      case 0: // Equivalent to the first for-loop
        if (thisChar == 0) {
          lcd.setCursor(0, 0);
        }
        lcd.print(thisChar);
        thisChar++;
        
        if (thisChar >= 10) {
          state = 1;       // Move to the next state
          thisChar = 0;    // Reset character counter
          lcd.setCursor(16, 1);
          lcd.autoscroll();
        }
        break;

      case 1: // Equivalent to the second for-loop (with autoscroll)
        lcd.print(thisChar);
        thisChar++;
        
        if (thisChar >= 10) {
          state = 2;       // Move to the final clear state
          thisChar = 0;
        }
        break;

      case 2: // Equivalent to the final part
        lcd.noAutoscroll();
        lcd.clear();
        state = 0;         // Start over from scratch
        break;
    }
  }
}

// [Full Implementation] Casino-style slot machine animation
void run_lcd_casino(Adafruit_LiquidCrystal& lcd) {
  if (gameOver) {
    run_lcd_casino_gameOverScreen(lcd);
    return;
  }

  bool currentButton = (digitalRead(BUTTON_PIN) == HIGH);

  // Flanco de subida: botón recién presionado
  if (currentButton && !lastButton) {
    delay(50); // debounce
    if (digitalRead(BUTTON_PIN) == HIGH) {
      credits--;         // Cuesta 1 crédito jugar
      run_lcd_casino_spinAnimation(lcd);   // Animación girando
      run_lcd_casino_showResult(lcd);      // Muestra resultado y decide ganador

      if (credits <= 0) {
        gameOver = true;
      }
    }
  }

  lastButton = currentButton;
}

void run_ldc_casino_init(Adafruit_LiquidCrystal& lcd) {
  // Casino Initial Pin Setup
  pinMode(BUTTON_PIN, INPUT);
  randomSeed(analogRead(A0));

  lcds[2].createChar(0, starChar);
  lcds[2].createChar(1, coinChar);
  lcds[2].createChar(2, heartChar);
  run_lcd_casino_showIdle(lcds[2]);
}

// Pantalla de espera del Casino
void run_lcd_casino_showIdle(Adafruit_LiquidCrystal& lcd) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("** CASINO 777 **");
  lcd.setCursor(0, 1);
  lcd.print("Credits:");
  lcd.print(credits);
  lcd.print(" SPIN!");
}

// Animación de los rodillos girando
void run_lcd_casino_spinAnimation(Adafruit_LiquidCrystal& lcd) {
  int spinSteps = 18;   // Cuántas veces "gira"
  int delayMs   = 80;   // Velocidad inicial rápida

  for (int step = 0; step < spinSteps; step++) {
    int s0 = random(NUM_SYMBOLS);
    int s1 = random(NUM_SYMBOLS);
    int s2 = random(NUM_SYMBOLS);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  SPINNING...   ");
    lcd.setCursor(0, 1);

    String line = " ";
    line += symbols[s0];
    line += " | ";
    line += symbols[s1];
    line += " | ";
    line += symbols[s2];
    while (line.length() < 16) line += " ";
    lcd.print(line.substring(0, 16));

    if (step > spinSteps / 2) delayMs += 15;
    delay(delayMs);
  }

  if (ALWAYS_WIN) {
    int winner = random(NUM_SYMBOLS);
    reel[0] = winner;
    reel[1] = winner;
    reel[2] = winner;
  } else {
    reel[0] = random(NUM_SYMBOLS);
    reel[1] = random(NUM_SYMBOLS);
    reel[2] = random(NUM_SYMBOLS);
  }
}

void run_lcd_casino_PlayMusic(int melody[], int durations[], int size) {
  for (int note = 0; note < size; note++) {
    int duration = 1000 / durations[note];
    tone(BUZZER, melody[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER);
  }
}

// Muestra el resultado y decide si ganó
void run_lcd_casino_showResult(Adafruit_LiquidCrystal& lcd) {
  bool win = (reel[0] == reel[1] && reel[1] == reel[2]);

  lcd.clear();
  lcd.setCursor(0, 0);
  String line = " ";
  line += symbols[reel[0]];
  line += " | ";
  line += symbols[reel[1]];
  line += " | ";
  line += symbols[reel[2]];
  while (line.length() < 16) line += " ";
  lcd.print(line.substring(0, 16));

  lcd.setCursor(0, 1);
  if (win) {
    credits += 5;
    lcd.print("  3 IGUALES!!   ");
    delay(600);
    run_lcd_casino_winAnimation(lcd);
  } else {
    lcd.print(" Credits:");
    lcd.print(credits);
    lcd.print("     ");
    delay(800);
    run_lcd_casino_loseAnimation(lcd);
  }

  delay(400);
  run_lcd_casino_showIdle(lcd);
}

// Animación de victoria
void run_lcd_casino_winAnimation(Adafruit_LiquidCrystal& lcd) {
  String frames[] = {
    "*** YOU WIN! ***",
    "  * YOU WIN! *  ",
    "*** YOU WIN! ***",
    "  * YOU WIN! *  ",
  };
  run_lcd_casino_PlayMusic(melody_win, durations_win, sizeof(melody_win)/sizeof(int));

  for (int rep = 0; rep < 5; rep++) {
    for (int f = 0; f < 4; f++) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(frames[f]);
      lcd.setCursor(0, 1);
      lcd.print("  +5 CREDITOS!  ");
      delay(200);
    }
  }

  lcd.clear();
  String msg = "                YOU WIN!!               ";
  for (int i = 0; i < (int)msg.length() - 15; i++) {
    lcd.setCursor(0, 0);
    lcd.print(msg.substring(i, i + 16));
    lcd.setCursor(0, 1);
    lcd.print("  +5 CREDITOS!  ");
    delay(120);
  }
}

// Animación de derrota
void run_lcd_casino_loseAnimation(Adafruit_LiquidCrystal& lcd) {
  run_lcd_casino_PlayMusic(melody_lose, durations_lose, sizeof(melody_lose)/sizeof(int));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Suerte la   ");
  lcd.setCursor(0, 1);
  lcd.print(" proxima vez :(");
  delay(1000);

  for (int i = 0; i < 3; i++) {
    lcd.noDisplay();
    delay(150);
    lcd.display();
    delay(150);
  }
}

// Pantalla de game over
void run_lcd_casino_gameOverScreen(Adafruit_LiquidCrystal& lcd) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  -- GAME  --   ");
  lcd.setCursor(0, 1);
  lcd.print("  -- OVER  --   ");
  delay(600);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Sin creditos  ");
  lcd.setCursor(0, 1);
  lcd.print("Reset p/ jugar  ");
  while (true) {
    delay(1000);
  }
}
