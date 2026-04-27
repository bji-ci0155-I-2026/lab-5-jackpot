// Memory Measurement for Circuit Playground Classic (ATmega32u4)
// Mide Flash, SRAM y EEPROM disponibles en tiempo de ejecución

#include <Adafruit_CircuitPlayground.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// Constantes de hardware del ATmega32u4
const uint32_t FLASH_TOTAL    = 32768UL;  // 32 KB Flash total
const uint32_t FLASH_BOOTLOADER = 4096UL; // ~4 KB reservados para bootloader
const uint16_t SRAM_TOTAL     = 2560;     // 2.5 KB SRAM total
const uint16_t EEPROM_TOTAL   = 1024;     // 1 KB EEPROM total

// Medir SRAM libre usando el "stack/heap gap"
extern int __heap_start;
extern int *__brkval;

uint16_t freeMemorySRAM() {
  int freeValue;
  if ((int)__brkval == 0) {
    // Heap no ha sido usado aún
    freeValue = ((int)&freeValue) - ((int)&__heap_start);
  } else {
    freeValue = ((int)&freeValue) - ((int)__brkval);
  }
  return (uint16_t)freeValue;
}

// Medir EEPROM: contar bytes no escritos (0xFF = vacío)
uint16_t countEEPROMFree() {
  uint16_t freeCount = 0;
  for (uint16_t i = 0; i < EEPROM_TOTAL; i++) {
    if (EEPROM.read(i) == 0xFF) freeCount++;
  }
  return freeCount;
}

// Calcular Flash usada (tamaño del sketch compilado)
// __data_load_end es el símbolo del linker que marca el fin
// del sketch compilado cargado en Flash
extern char _end;
extern char __data_load_end;

uint32_t usedFlash() {
  // El compilador expone el símbolo FLASHEND del AVR
  // Usamos el símbolo del linker para precisión
  return (uint32_t)&__data_load_end;
}

// Utilidad: imprimir barra de uso
void printBar(float pct) {
  int filled = (int)(pct / 5.0);  // 20 bloques = 100%
  Serial.print("[");
  for (int i = 0; i < 20; i++) {
    Serial.print(i < filled ? "#" : "-");
  }
  Serial.print("] ");
  Serial.print((int)pct);
  Serial.println("%");
}

// ── Reporte completo por Serial ───────────────────────────
void reportMemory() {
  Serial.println(F("╔═══════════════════════════════════════════╗"));
  Serial.println(F("║  Circuit Playground Classic – ATmega32u4  ║"));
  Serial.println(F("╠═══════════════════════════════════════════╣"));

  // ── FLASH ────────────────────────────────────────────────
  uint32_t flashUsed  = usedFlash();
  uint32_t flashAvail = FLASH_TOTAL - FLASH_BOOTLOADER;
  uint32_t flashFree  = (flashUsed < flashAvail) ? (flashAvail - flashUsed) : 0;
  float    flashPct   = (flashUsed * 100.0f) / flashAvail;

  Serial.println(F("║  FLASH (memoria de programa)              ║"));
  Serial.print(F("  Total         : ")); Serial.print(FLASH_TOTAL);       Serial.println(F(" bytes (32 KB)"));
  Serial.print(F("  Bootloader    : ")); Serial.print(FLASH_BOOTLOADER);  Serial.println(F(" bytes (~4 KB)"));
  Serial.print(F("  Disponible    : ")); Serial.print(flashAvail);        Serial.println(F(" bytes (~28 KB)"));
  Serial.print(F("  Sketch usa    : ")); Serial.print(flashUsed);         Serial.println(F(" bytes"));
  Serial.print(F("  Libre         : ")); Serial.print(flashFree);         Serial.println(F(" bytes"));
  Serial.print(F("  Uso           : ")); printBar(flashPct);

  Serial.println(F("╠═══════════════════════════════════════════╣"));

  // SRAM 
  uint16_t sramFree = freeMemorySRAM();
  uint16_t sramUsed = SRAM_TOTAL - sramFree;
  float    sramPct  = (sramUsed * 100.0f) / SRAM_TOTAL;

  Serial.print(F("  Total         : ")); Serial.print(SRAM_TOTAL); Serial.println(F(" bytes (2.5 KB)"));
  Serial.print(F("  En uso        : ")); Serial.print(sramUsed);   Serial.println(F(" bytes"));
  Serial.print(F("  Libre aprox.  : ")); Serial.print(sramFree);   Serial.println(F(" bytes"));
  Serial.print(F("  Uso           : ")); printBar(sramPct);

  Serial.println(F("╠═══════════════════════════════════════════╣"));

  // ── EEPROM ───────────────────────────────────────────────
  uint16_t eepromFree = countEEPROMFree();
  uint16_t eepromUsed = EEPROM_TOTAL - eepromFree;
  float    eepromPct  = (eepromUsed * 100.0f) / EEPROM_TOTAL;

  Serial.println(F("║  EEPROM (memoria no volátil)              ║"));
  Serial.print(F("  Total         : ")); Serial.print(EEPROM_TOTAL); Serial.println(F(" bytes (1 KB)"));
  Serial.print(F("  Usada (!=0xFF): ")); Serial.print(eepromUsed);   Serial.println(F(" bytes"));
  Serial.print(F("  Libre (==0xFF): ")); Serial.print(eepromFree);   Serial.println(F(" bytes"));
  Serial.print(F("  Uso           : ")); printBar(eepromPct);

  Serial.println(F("╠═══════════════════════════════════════════╣"));

  if (flashPct > 90.0f) {
    Serial.println(F("  [AVISO]  Flash >90% llena. Optimizar codigo."));
  } else {
    Serial.println(F("  [OK]     Flash con espacio suficiente."));
  }

  Serial.println(F("╚═══════════════════════════════════════════╝"));
  Serial.println();
}

// Indicador visual con NeoPixel
void visualFeedback() {
  uint16_t sramFree = freeMemorySRAM();
  uint32_t color;

  // Verde > 1KB libre | Amarillo > 512B | Rojo <= 512B
  if      (sramFree > 1024) color = 0x002200;  // verde tenue
  else if (sramFree >  512) color = 0x222200;  // amarillo tenue
  else                       color = 0x220000;  // rojo tenue

  // Encender todos los NeoPixels con el color de estado
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, color);
  }
  delay(1000);
  CircuitPlayground.clearPixels();
}

void setup() {
  CircuitPlayground.begin();
  Serial.begin(9600);

  // Esperar a que el Monitor Serial esté listo (USB nativo)
  while (!Serial && millis() < 5000) { delay(10); }

  Serial.println(F("\n=== Iniciando medicion de memoria ===\n"));
  reportMemory();
  visualFeedback();
}

void loop() {
  // Repetir medición al presionar botón izquierdo (pin 4)
  if (CircuitPlayground.leftButton()) {
    reportMemory();
    visualFeedback();
    delay(500); // debounce
  }
}