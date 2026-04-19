/*
  Casino Slot Machine - LCD 16x2
  ================================
  Pines (mismo circuito que lcd_display_autoscroll):
    LCD RS     -> pin 12
    LCD Enable -> pin 11
    LCD D4     -> pin 5
    LCD D5     -> pin 4
    LCD D6     -> pin 3
    LCD D7     -> pin 2
    LCD R/W    -> GND
    Potenciómetro 10K (contraste) -> VO pin del LCD

  Botón de SPIN -> pin 7
*/

#include <LiquidCrystal.h>
#include "pitches.h"

// Pines
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const int BUTTON_PIN = 7;
const int BUZZER = 8;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Modo de prueba
// TRUE para que el jugador SIEMPRE gane (fuerza 3 símbolos iguales)
const bool ALWAYS_WIN = false;

// Símbolos de la slot
// 5 símbolos disponibles: 7  BAR  $  *  #
const char* symbols[] = { "7", "BAR", "$", "*", "#" };
const int NUM_SYMBOLS  = 5;

// Musica
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

// Caracteres personalizados (iconos pequeños para la pantalla)
// https://naylampmechatronics.com/blog/34_tutorial-lcd-conectando-tu-arduino-a-un-lcd1602-y-lcd2004.html
// Estrella
// --X--
// X-X-X
// -XXX-
// XXXXX
// -XXX-
// X-X-X
// --X--
byte starChar[8] = {
  0b00100, 0b10101, 0b01110, 0b11111,
  0b01110, 0b10101, 0b00100, 0b00000
};
// Moneda
byte coinChar[8] = {
  0b01110, 0b10001, 0b10101, 0b10111,
  0b10101, 0b10001, 0b01110, 0b00000
};
// Corazón
byte heartChar[8] = {
  0b00000, 0b01010, 0b11111, 0b11111,
  0b01110, 0b00100, 0b00000, 0b00000
};

// Estado del juego
int  reel[3]       = {0, 0, 0};
bool lastButton    = false;
int  credits       = 10;
bool gameOver      = false;

// Prototipos
void showIdle();
void spinAnimation();
void showResult();
void winAnimation();
void loseAnimation();
void gameOverScreen();

// ══════════════════════════════════════════════════════════════════════════

void setup() {
  lcd.begin(16, 2);
  lcd.createChar(0, starChar);
  lcd.createChar(1, coinChar);
  lcd.createChar(2, heartChar);

  pinMode(BUTTON_PIN, INPUT);
  randomSeed(analogRead(A0));   // Semilla aleatoria del ruido analógico https://docs.arduino.cc/language-reference/en/functions/random-numbers/randomSeed/

  showIdle();
}

// ══════════════════════════════════════════════════════════════════════════
void loop() {
  if (gameOver) {
    gameOverScreen();
    return;
  }

  bool currentButton = (digitalRead(BUTTON_PIN) == HIGH);

  // Flanco de subida: botón recién presionado
  if (currentButton && !lastButton) {
    delay(50); // debounce
    if (digitalRead(BUTTON_PIN) == HIGH) {
      credits--;         // Cuesta 1 crédito jugar
      spinAnimation();   // Animación girando
      showResult();      // Muestra resultado y decide ganador

      if (credits <= 0) {
        gameOver = true;
      }
    }
  }

  lastButton = currentButton;
}

// Pantalla de espera
void showIdle() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("** CASINO 777 **");
  lcd.setCursor(0, 1);
  lcd.print("Credits:");
  lcd.print(credits);
  lcd.print(" SPIN!");
}

// Animación de los rodillos girando
void spinAnimation() {
  int spinSteps = 18;   // Cuántas veces "gira"
  int delayMs   = 80;   // Velocidad inicial rápida

  for (int step = 0; step < spinSteps; step++) {
    // Genera valores aleatorios para mostrar mientras gira
    int s0 = random(NUM_SYMBOLS);
    int s1 = random(NUM_SYMBOLS);
    int s2 = random(NUM_SYMBOLS);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  SPINNING...   ");
    lcd.setCursor(0, 1);

    // Centra los 3 símbolos con separadores
    String line = " ";
    line += symbols[s0];
    line += " | ";
    line += symbols[s1];
    line += " | ";
    line += symbols[s2];
    // Pad para completar 16 chars
    while (line.length() < 16) line += " ";
    lcd.print(line.substring(0, 16));

    // Desacelera hacia el final
    if (step > spinSteps / 2) delayMs += 15;
    delay(delayMs);
  }

  // Determina resultado final
  if (ALWAYS_WIN) {
    // Fuerza los 3 rodillos al mismo símbolo
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

// Muestra el resultado y decide si ganó
void showResult() {
  bool win = (reel[0] == reel[1] && reel[1] == reel[2]);

  // Fila 0: los tres símbolos
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

  // Fila 1: resultado
  lcd.setCursor(0, 1);
  if (win) {
    credits += 5;   // Premio: +5 créditos
    lcd.print("  3 IGUALES!!   ");
    delay(600);
    winAnimation();
  } else {
    lcd.print(" Credits:");
    lcd.print(credits);
    lcd.print("     ");
    delay(800);
    loseAnimation();
  }

  delay(400);
  showIdle();
}

// Animación de victoria
void winAnimation() {
  // Parpadeo "YOU WIN!!" con asteriscos que se mueven
  String frames[] = {
    "*** YOU WIN! ***",
    "  * YOU WIN! *  ",
    "*** YOU WIN! ***",
    "  * YOU WIN! *  ",
  };
  PlayMusic(melody_win, durations_win, sizeof(melody_win)/sizeof(int));

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

  // Scroll del texto "YOU WIN!!" de izquierda a derecha
  lcd.clear();
  String msg = "                YOU WIN!!               ";
  lcd.setCursor(0, 0);
  for (int i = 0; i < (int)msg.length() - 15; i++) {
    lcd.setCursor(0, 0);
    lcd.print(msg.substring(i, i + 16));
    lcd.setCursor(0, 1);
    lcd.print("  +5 CREDITOS!  ");
    delay(120);
  }
}

// Animación de derrota
void loseAnimation() {
  PlayMusic(melody_lose, durations_lose, sizeof(melody_lose)/sizeof(int));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Suerte la   ");
  lcd.setCursor(0, 1);
  lcd.print(" proxima vez :(");
  delay(1000);

  // Parpadeo rápido
  for (int i = 0; i < 3; i++) {
    lcd.noDisplay();
    delay(150);
    lcd.display();
    delay(150);
  }
}

// Pantalla de game over
void gameOverScreen() {
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
  // Se queda aquí hasta que el usuario haga reset del Arduino
  while (true) {
    delay(1000);
  }
}

// Credits https://github.com/arashjafari/audio-to-arduino?tab=readme-ov-file
void PlayMusic(int melody[], int durations[], int size) {
  for (int note = 0; note < size; note++) {
    int duration = 1000 / durations[note];
    tone(BUZZER, melody[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER);
  }
}