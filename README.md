# lab-5-jackpot

#### Texto sencillo

Diseño y codigo LCD normal: [Laboratorio #4 - Liquid Crystal Display (LCD) Hello World](https://www.tinkercad.com/things/cIjcsSK1FTP-laboratorio-4-liquid-crystal-display-lcd-hello-world?sharecode=g8UwlarT3QCvULCgqoQ0eayPNNs1qK-MCkgNnNc4xHk)

Diseño y codigo LCD I2C: [Laboratorio #5 - Arduino LCD I2C 3 leds](https://www.tinkercad.com/things/3ak9taXIvaF-laboratorio-5-arduino-lcd-i2c-3-leds?sharecode=AHoXLpBPg6qnkTKg0Dd6PVaCOfvxm3K15Y1JRLvMJJw)

#### Texto scrolling

Diseño y codigo LCD normal: [Laboratorio #4 - Liquid Crystal Display (LCD) Autoscroll](https://www.tinkercad.com/things/cih2mpGdvnV-laboratorio-4-liquid-crystal-display-lcd-autoscroll?sharecode=WtUM4_9ktNBCt_ETgbniWzJcT_eFNPF8749XIPibCFY)

Diseño y codigo LCD I2C: [Laboratorio #5 - Arduino LCD I2C 3 leds](https://www.tinkercad.com/things/3ak9taXIvaF-laboratorio-5-arduino-lcd-i2c-3-leds?sharecode=AHoXLpBPg6qnkTKg0Dd6PVaCOfvxm3K15Y1JRLvMJJw)


#### Animaciones

Diseño y codigo LCD normal: [Laboratorio #4 - Liquid Crystal Display (LCD) Casino](https://www.tinkercad.com/things/dqve9ozAi33-laboratorio-4-liquid-crystal-display-lcd-casino?sharecode=3_SfphVUupbPQ_M8Ji9SK-Rcx6fAe9pp9RoYP-7b-BE)

Diseño y codigo LCD I2C: [Laboratorio #5 - Arduino LCD I2C 3 leds](https://www.tinkercad.com/things/3ak9taXIvaF-laboratorio-5-arduino-lcd-i2c-3-leds?sharecode=AHoXLpBPg6qnkTKg0Dd6PVaCOfvxm3K15Y1JRLvMJJw)

## Guía Técnica de Memoria EEPROM en Microcontroladores AVR y Arduino

1. Fundamentos Teóricos y Arquitectura

La EEPROM es una memoria no volátil integrada en los microcontroladores que permite conservar datos incluso cuando la placa se apaga. Se comporta de manera similar a un "pequeño disco duro" para el almacenamiento de variables persistentes.

### Especificaciones de Memoria y Fiabilidad

De acuerdo con los datasheets de Atmel y la documentación de Arduino, las características clave de fiabilidad son:
* Ciclos de Escritura/Borrado: La memoria tiene un límite físico de aproximadamente 100,000 ciclos por ubicación individual antes de que la celda se desgaste.
* Retención de Datos: Se proyecta una retención de 20 años a 85°C o hasta 100 años a 25°C.
* Tiempo de Escritura: Cada operación de escritura toma aproximadamente 3.3 ms.
* Organización: La memoria está orientada a bytes. El tamaño total es siempre una potencia de dos, determinada por el número de líneas de dirección (por ejemplo, 10 líneas de dirección resultan en 2^{10} = 1024 posiciones).

### Lógica de Direccionamiento y Desbordamiento (Wrapping)

Dado que los tamaños son potencias de dos, es posible prevenir desbordamientos de direcciones mediante una operación de bits AND con el valor de longitud - 1. Por ejemplo, si la longitud es 1024 `(0b10000000000)`, la máscara será 1023 `(0b0111111111)`. Al aplicar un AND bitwise entre cualquier dirección y 1023, una dirección inválida como 1024 volverá automáticamente a 0, asegurando que siempre se acceda a un rango válido.

### Hardware y Capacidades por Dispositivo

Los microcontroladores de la familia AVR (ATmega48A/PA/88A/PA/168A/PA/328/P) varían principalmente en sus capacidades de memoria.

Tabla de Memoria por Microcontrolador

|Dispositivo|Flash|EEPROM|RAM|
|-|-|-|-|
|ATmega48A/PA|	4 KBytes|	256 Bytes|	512 Bytes|
|ATmega88A/PA|	8 KBytes|	512 Bytes|	1 KBytes|
|ATmega168A/PA|	16 KBytes|	512 Bytes|	1 KBytes|
|ATmega328/P|	32 KBytes|	1 KBytes|	2 KBytes|

Placas Arduino Compatibles

* Arduino UNO R4 (Minima/WiFi): Incluyen EEPROM.
* Arduino UNO Rev.3: Utiliza el ATmega328P (1 KB EEPROM).
* Arduino Mega 2560 Rev.3: Ofrece 4 KB de EEPROM.
* Otras: Arduino Nano, Micro, Leonardo y Nano Every.


### Implementación: Librería EEPROM de Arduino

La biblioteca oficial proporciona una interfaz de C++ para interactuar con el almacenamiento interno.

Funciones Principales:

1. `EEPROM.read(address)`: Lee un solo byte (0-255) de la dirección especificada.
2. `EEPROM.write(address, value)`: Escribe un byte en la dirección. Siempre ejecuta la escritura, consumiendo un ciclo de vida de la celda.
3. `EEPROM.update(address, value)`: Solo escribe el valor si es diferente al contenido actual. Es fundamental para prolongar la vida útil de la memoria.
4. `EEPROM.get(address, object)`: Recupera cualquier tipo de dato o estructura personalizada. Calcula automáticamente el número de bytes necesarios según el tipo de variable.
5. `EEPROM.put(address, object)`: Almacena cualquier objeto o estructura. Utiliza internamente el método update() para minimizar ciclos de escritura.
6. `EEPROM.length()`: Retorna la capacidad total de la memoria en la placa actual, permitiendo código portátil entre dispositivos (ej. Uno vs Mega).
7. Operador de Suscrito `EEPROM[<address>]`: Permite tratar el objeto EEPROM como un arreglo, permitiendo lectura y escritura directa (ej. `EEPROM[0] = 10;`).

### Control de Integridad (CRC)

Se puede utilizar el objeto EEPROM para calcular un valor de Redundancia Cíclica (CRC). Esto funciona como una firma; cualquier cambio en los datos almacenados alterará el resultado del CRC, permitiendo detectar corrupción de datos.

### Ejemplos de Código Técnico

Inicialización y Borrado Completo

Para limpiar la memoria, se recorre cada dirección estableciendo su valor en 0. El uso de `EEPROM.length()` asegura compatibilidad universal.

```c
#include <EEPROM.h>

void setup() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

void loop() {}
```

Almacenamiento de Estructuras Complejas

Uso de put y get para manejar tipos de datos mayores a un byte:

```c
#include <EEPROM.h>

struct DatosConfig {
  float temperatura;
  int identificador;
  char nombre[10];
};

void guardarConfiguracion() {
  DatosConfig config = {25.5, 101, "Sensor1"};
  int eeAddress = 0;
  EEPROM.put(eeAddress, config); // Escribe la estructura completa
}

void leerConfiguracion() {
  DatosConfig config;
  int eeAddress = 0;
  EEPROM.get(eeAddress, config); // Recupera la estructura completa
}
```

Uso del Operador de Suscrito y Actualización

```c
// Lectura y escritura como si fuera un arreglo
uint8_t valor = EEPROM[10]; // Lee celda 10
EEPROM[10] = valor + 1;     // Escribe en celda 10 (usa update internamente)
```

### Registros de Hardware (Bajo Nivel)

Para el acceso directo mediante registros en dispositivos AVR, se utilizan los siguientes componentes:

|Registro|	Nombre|	Función|
|-|-|-|
|EEARH/EEARL|	EEPROM Address Register|	Almacena la dirección de acceso (High/Low).
|EEDR|	EEPROM Data Register|	Contiene el dato a escribir o el dato leído.
|EECR|	EEPROM Control Register|	Controla los permisos de lectura, escritura y habilitación de interrupciones.

Bits Críticos en EECR:

* EEPM1, EEPM0: Modo de programación (Erase/Write).
* EERIE: Habilitación de interrupción de EEPROM lista.
* EEMPE: Habilitación maestra de escritura.
* EEPE: Habilitación de escritura.
* EERE: Habilitación de lectura.

### Referencias

*   **Andrews, C. [pYro_65].** (2015, 1 de abril). *Official EEPROM library: support and reference* [Hilo de foro]. Arduino Forum..
*   **Arduino.** (2024, 19 de junio). *A guide to EEPROM*. Arduino Documentation..
*   **Atmel Corporation.** (2014). *ATmega48A, ATmega48PA, ATmega88A, ATmega88PA, ATmega168A, ATmega168PA, ATmega328, ATmega328P datasheet summary* (Rev. 8271IS-AVR-10/2014)..
*   **captain_morgan_1999.** (2016, 9 de diciembre). *EEPROM.write vs EEPROM.update* [Hilo de foro]. Arduino Forum..
*   **F1.** (2018, 17 de agosto). *I don't understand the part of EEPROM tutorial* [Hilo de foro]. Arduino Forum..
*   **HugoW.** (2022, 9 de enero). *How to use Clear Eeprom sktech on a Mega* [Hilo de foro]. Arduino Forum..
