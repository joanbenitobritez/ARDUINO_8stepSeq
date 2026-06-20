# ARDUINO_8stepSeq
# 8-Step MIDI Sequencer / Secuenciador MIDI de 8 Pasos

*(Scroll down for the Spanish version / Desplácese hacia abajo para la versión en español)*

---

## 🇬🇧 English Version

### About the Project
This is an open-source, 8-step MIDI sequencer based on the Arduino Nano. Designed for modularity and ease of assembly, it features a visual interface via a 1.3" OLED screen, step input via tactile switches, and menu navigation using a rotary encoder. 

### Features
* **8-Step Sequencing:** Independent tactile switches for each step.
* **OLED Display:** Visual feedback for steps, BPM, and notes using a 1.3" I2C display.
* **Rotary Encoder:** Seamless menu navigation and parameter adjustment with hardware interrupts.
* **MIDI Out:** Standard TRS 3.5mm jack (Type A) with hardware insertion detection.

### Bill of Materials (BOM)

| Component | Qty | Specifications / Notes |
| :--- | :--- | :--- |
| Arduino Nano V3.0 | 1 | ATmega328P, CH340G USB Chip |
| OLED Display 1.3" | 1 | SH1106 Controller, I2C (4-pin) |
| Rotary Encoder | 1 | EC11 or KY-040 (with push button) |
| Tactile Switches | 11 | 6x6mm standard |
| TRS 3.5mm Jack | 1 | Stereo with switch (e.g., PJ-324M) |
| Resistors | 2 | 220 Ω (1/4W) for MIDI line |
| Wire | 1 | AWG 22 solid tinned copper |

### How to Flash the Firmware
1. Open the `.ino` file located in the `/firmware` folder using the Arduino IDE.
2. Install the required libraries via the Library Manager: `U8g2` (for the OLED) and `Encoder` by Paul Stoffregen.
3. Select **Arduino Nano** as the board. If upload fails, try changing the processor to **ATmega328P (Old Bootloader)**.
4. Compile and upload.

---

## 🇪🇸 Versión en Español

### Sobre el Proyecto
Este es un secuenciador MIDI de 8 pasos de código abierto basado en el Arduino Nano. Diseñado para ser modular y fácil de ensamblar, cuenta con una interfaz visual mediante una pantalla OLED de 1.3", entrada de pasos por pulsadores táctiles y navegación de menús utilizando un encoder rotativo.

### Características
* **Secuencia de 8 Pasos:** Pulsadores independientes para cada paso.
* **Pantalla OLED:** Retroalimentación visual para pasos, BPM y notas usando un display I2C de 1.3".
* **Encoder Rotativo:** Navegación fluida de parámetros mediante interrupciones de hardware.
* **Salida MIDI:** Jack estándar TRS de 3.5mm con detección física de inserción de cable.

### Lista de Materiales (BOM)

| Componente | Cant. | Especificaciones / Notas |
| :--- | :--- | :--- |
| Arduino Nano V3.0 | 1 | ATmega328P, Chip USB CH340G |
| Pantalla OLED 1.3" | 1 | Controlador SH1106, I2C (4 pines) |
| Encoder Rotativo | 1 | EC11 o KY-040 (con pulsador) |
| Micro-switch Tactile | 11 | Estándar de 6x6mm |
| Jack TRS 3.5mm | 1 | Estéreo con corte (ej. PJ-324M) |
| Resistencias | 2 | 220 Ω (1/4W) para línea MIDI |
| Alambre | 1 | Cobre rígido estañado AWG 22 |

### Cómo flashear el Firmware
1. Abrí el archivo `.ino` ubicado en la carpeta `/firmware` utilizando el Arduino IDE.
2. Instaló las librerías necesarias desde el Gestor de Librerías: `U8g2` (para la OLED) y `Encoder` de Paul Stoffregen.
3. Seleccioná **Arduino Nano** en el menú de placas. Si falla la subida, probá cambiando el procesador a **ATmega328P (Old Bootloader)**.
4. Compilá y subí el código.