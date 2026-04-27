/**
 * =============================================================================
 * memory_monitor.ino
 * =============================================================================
 * Sketch de diagnóstico de memoria para Arduino Uno R3 (ATmega328P)
 *
 * Mide y reporta:
 *   - Flash (programa): total, usado por bootloader, disponible para usuario
 *   - SRAM: total, usada (estática + heap), libre (entre heap y stack)
 *   - EEPROM: total, bytes escritos (no 0xFF), integridad básica
 *
 * Buenas prácticas aplicadas:
 *   - Strings en Flash con F() para no consumir SRAM
 *   - Constantes con constexpr en lugar de #define
 *   - Tipos de ancho fijo (uint8_t, uint16_t, uint32_t)
 *   - Sin allocación dinámica (malloc/new)
 *   - Funciones pequeñas con responsabilidad única
 *   - Volatile donde corresponde (acceso a símbolos del linker)
 *   - Watchdog deshabilitado explícitamente para diagnóstico
 *
 * Hardware: Arduino Uno R3 / ATmega328P @ 16 MHz
 * Autor:    Generado con Claude (Anthropic)
 * Licencia: MIT
 * =============================================================================
 */

#include <avr/boot.h>
#include <avr/pgmspace.h>      // pgm_read_word() para leer Flash
#include <avr/wdt.h>
#include <EEPROM.h>

/* ─────────────────────────────────────────────
 *  Constantes de arquitectura ATmega328P
 * ───────────────────────────────────────────── */
static constexpr uint32_t FLASH_TOTAL_BYTES    = 32768UL;   // 32 KB
static constexpr uint16_t BOOTLOADER_BYTES     = 512U;      // 0.5 KB (Optiboot)
static constexpr uint16_t SRAM_TOTAL_BYTES     = 2048U;     // 2 KB
static constexpr uint16_t EEPROM_TOTAL_BYTES   = 1024U;     // 1 KB
static constexpr uint8_t  EEPROM_ERASED_VALUE  = 0xFF;      // Valor de fábrica

static constexpr uint32_t SERIAL_BAUD          = 9600UL;
static constexpr uint16_t LOOP_INTERVAL_MS     = 5000U;     // Intervalo entre reportes

/* ─────────────────────────────────────────────
 *  Símbolos del linker (fronteras de secciones)
 *
 *  __data_start  → inicio de .data en SRAM
 *  __data_end    → fin de .data
 *  __bss_start   → inicio de .bss
 *  __bss_end     → fin de .bss  (= inicio del heap)
 *  __heap_start  → base del heap
 *  __heap_end    → tope del heap (si se usa malloc)
 * ───────────────────────────────────────────── */
extern uint8_t __data_start;
extern uint8_t __data_end;
extern uint8_t __bss_start;
extern uint8_t __bss_end;
extern uint8_t __heap_start;
extern uint8_t *__brkval;          // Puntero actual del heap (0 si no se usó malloc)

/* ─────────────────────────────────────────────
 *  Prototipos
 * ───────────────────────────────────────────── */
static uint16_t getSramFree(void);
static uint16_t getSramUsedStatic(void);
static uint16_t getSramUsedHeap(void);
static uint16_t getEepromUsedBytes(void);
static uint32_t getFlashUsedBytes(void);
static void     printSeparator(void);
static void     printFlashReport(void);
static void     printSramReport(void);
static void     printEepromReport(void);
static void     printBar(uint16_t used, uint16_t total, uint8_t width);

/* ═════════════════════════════════════════════
 *  SRAM libre: espacio entre el tope del heap
 *  (o fin de .bss) y el stack pointer actual.
 * ═════════════════════════════════════════════ */
static uint16_t getSramFree(void)
{
    uint8_t stackMarker;  // Variable local → vive en el stack
    uint16_t heapTop;

    if (__brkval == 0) {
        heapTop = (uint16_t)&__heap_start;
    } else {
        heapTop = (uint16_t)__brkval;
    }

    return (uint16_t)&stackMarker - heapTop;
}

/* ═════════════════════════════════════════════
 *  SRAM usada por variables globales/estáticas
 *  (.data + .bss)
 * ═════════════════════════════════════════════ */
static uint16_t getSramUsedStatic(void)
{
    return (uint16_t)&__bss_end - (uint16_t)&__data_start;
}

/* ═════════════════════════════════════════════
 *  SRAM usada por el heap (allocación dinámica)
 * ═════════════════════════════════════════════ */
static uint16_t getSramUsedHeap(void)
{
    if (__brkval == 0) {
        return 0;
    }
    return (uint16_t)__brkval - (uint16_t)&__heap_start;
}

/* ═════════════════════════════════════════════
 *  Bytes de EEPROM que difieren del valor de
 *  fábrica (0xFF). Indicador aproximado de uso.
 * ═════════════════════════════════════════════ */
static uint16_t getEepromUsedBytes(void)
{
    uint16_t count = 0;

    for (uint16_t addr = 0; addr < EEPROM_TOTAL_BYTES; ++addr) {
        if (EEPROM.read(addr) != EEPROM_ERASED_VALUE) {
            ++count;
        }
    }

    return count;
}

/* ═════════════════════════════════════════════
 *  Flash usada por el programa (aproximación).
 *  Busca desde el final hacia atrás la última
 *  palabra distinta de 0xFFFF.
 *
 *  ATmega328P tiene 32 KB de Flash → cabe en
 *  direcciones de 16 bits → pgm_read_word()
 *  (variante _near) es suficiente y portable.
 *
 *  pgm_read_word_far() solo es necesario en MCUs
 *  con >64 KB (ATmega2560, etc.) y puede no estar
 *  implementado en todas las toolchains para 328P.
 * ═════════════════════════════════════════════ */
static uint32_t getFlashUsedBytes(void)
{
    uint16_t appEnd = (uint16_t)(FLASH_TOTAL_BYTES - BOOTLOADER_BYTES);

    /* Retroceder de 2 en 2 bytes (tamaño de palabra Flash) */
    for (uint16_t addr = appEnd - 2; addr > 0; addr -= 2) {
        uint16_t word = pgm_read_word(addr);
        if (word != 0xFFFF) {
            return (uint32_t)(addr + 2);  // Siguiente posición = tamaño usado
        }
    }

    return 0;
}

/* ─────────────────────────────────────────────
 *  Helpers de impresión
 * ───────────────────────────────────────────── */
static void printSeparator(void)
{
    Serial.println(F("────────────────────────────────────────"));
}

/**
 * Barra ASCII proporcional: [████░░░░░░] 45%
 */
static void printBar(uint16_t used, uint16_t total, uint8_t width)
{
    if (total == 0) return;

    uint8_t filled = (uint8_t)((uint32_t)used * width / total);

    Serial.print(F("  ["));
    for (uint8_t i = 0; i < width; ++i) {
        Serial.print(i < filled ? '#' : '.');
    }
    Serial.print(F("] "));

    uint8_t pct = (uint8_t)((uint32_t)used * 100 / total);
    Serial.print(pct);
    Serial.println(F("%"));
}

/* ─────────────────────────────────────────────
 *  Reporte de Flash
 * ───────────────────────────────────────────── */
static void printFlashReport(void)
{
    uint32_t flashUsed     = getFlashUsedBytes();
    uint32_t flashAvail    = FLASH_TOTAL_BYTES - BOOTLOADER_BYTES;
    uint32_t flashFree     = (flashUsed < flashAvail) ? (flashAvail - flashUsed) : 0;

    Serial.println(F("[FLASH - Memoria de Programa]"));
    Serial.print(F("  Total:          "));
    Serial.print(FLASH_TOTAL_BYTES);
    Serial.println(F(" bytes (32 KB)"));

    Serial.print(F("  Bootloader:     "));
    Serial.print(BOOTLOADER_BYTES);
    Serial.println(F(" bytes"));

    Serial.print(F("  Disponible app: "));
    Serial.print(flashAvail);
    Serial.println(F(" bytes"));

    Serial.print(F("  Usada (sketch): ~"));
    Serial.print(flashUsed);
    Serial.println(F(" bytes"));

    Serial.print(F("  Libre:          ~"));
    Serial.print(flashFree);
    Serial.println(F(" bytes"));

    printBar((uint16_t)(flashUsed >> 0), (uint16_t)(flashAvail >> 0), 20);
}

/* ─────────────────────────────────────────────
 *  Reporte de SRAM
 * ───────────────────────────────────────────── */
static void printSramReport(void)
{
    uint16_t staticUsed = getSramUsedStatic();
    uint16_t heapUsed   = getSramUsedHeap();
    uint16_t free       = getSramFree();
    uint16_t stackEst   = SRAM_TOTAL_BYTES - staticUsed - heapUsed - free;

    Serial.println(F("[SRAM - Datos en Ejecucion]"));
    Serial.print(F("  Total:       "));
    Serial.print(SRAM_TOTAL_BYTES);
    Serial.println(F(" bytes (2 KB)"));

    Serial.print(F("  .data+.bss:  "));
    Serial.print(staticUsed);
    Serial.println(F(" bytes (variables globales)"));

    Serial.print(F("  Heap:        "));
    Serial.print(heapUsed);
    Serial.println(F(" bytes (alloc dinamico)"));

    Serial.print(F("  Stack (est): ~"));
    Serial.print(stackEst);
    Serial.println(F(" bytes"));

    Serial.print(F("  >>> LIBRE:   "));
    Serial.print(free);
    Serial.println(F(" bytes <<<"));

    printBar(SRAM_TOTAL_BYTES - free, SRAM_TOTAL_BYTES, 20);

    /* Alerta de SRAM baja */
    if (free < 256) {
        Serial.println(F("  !! ALERTA: SRAM libre < 256 bytes !!"));
        Serial.println(F("     Riesgo de colision stack/heap."));
    } else if (free < 512) {
        Serial.println(F("  >> Advertencia: SRAM libre < 512 bytes."));
    }
}

/* ─────────────────────────────────────────────
 *  Reporte de EEPROM
 * ───────────────────────────────────────────── */
static void printEepromReport(void)
{
    uint16_t eepUsed = getEepromUsedBytes();
    uint16_t eepFree = EEPROM_TOTAL_BYTES - eepUsed;

    Serial.println(F("[EEPROM - Almacenamiento Persistente]"));
    Serial.print(F("  Total:         "));
    Serial.print(EEPROM_TOTAL_BYTES);
    Serial.println(F(" bytes (1 KB)"));

    Serial.print(F("  Escritas:      "));
    Serial.print(eepUsed);
    Serial.println(F(" bytes (valor != 0xFF)"));

    Serial.print(F("  Disponibles:   "));
    Serial.print(eepFree);
    Serial.println(F(" bytes"));

    Serial.println(F("  Ciclos W/E:    ~100,000 por celda"));

    printBar(eepUsed, EEPROM_TOTAL_BYTES, 20);
}

/* ═════════════════════════════════════════════
 *  setup()
 * ═════════════════════════════════════════════ */
void setup(void)
{
    wdt_disable();                       // Desactivar watchdog durante diagnóstico

    Serial.begin(SERIAL_BAUD);
    while (!Serial) { /* Esperar conexión USB (solo Leonardo, pero es buena práctica */ }

    Serial.println();
    printSeparator();
    Serial.println(F(" MONITOR DE MEMORIA - Arduino Uno R3"));
    Serial.println(F(" ATmega328P @ 16 MHz"));
    printSeparator();
    Serial.println();
}

/* ═════════════════════════════════════════════
 *  loop() — Imprime reporte periódicamente
 * ═════════════════════════════════════════════ */
void loop(void)
{
    static uint32_t lastReport = 0;
    uint32_t now = millis();

    /* Protección contra overflow de millis() (~49 días) */
    if ((now - lastReport) >= LOOP_INTERVAL_MS || lastReport == 0) {
        lastReport = now;

        Serial.println();
        printSeparator();
        Serial.print(F(" Reporte @ "));
        Serial.print(now / 1000UL);
        Serial.println(F(" s"));
        printSeparator();
        Serial.println();

        printFlashReport();
        Serial.println();
        printSramReport();
        Serial.println();
        printEepromReport();

        Serial.println();
        printSeparator();
        Serial.println(F(" Proximo reporte en 5 segundos..."));
        printSeparator();
    }
}