#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Bounce2.h>
#include <Encoder.h>

// =================================================================
// 1. INSTANCIAS DE COMPONENTES (HARDWARE VIRTUAL)
// =================================================================
// Se utiliza el modo Page Buffer (_1_) para proteger la memoria RAM del Arduino Nano.
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); 
Encoder miEncoder(2, 3);          // Lector del codificador rotativo (Pines D2 y D3)
Bounce botones[8];                // Filtros de rebote para los 8 botones de pasos musicales
Bounce btnEncSW = Bounce();       // Filtro de rebote para el pulsador del encoder
Bounce btnStepMas = Bounce();     // Filtro de rebote para el botón de avance manual

// =================================================================
// 2. ASIGNACIÓN DE PINES (MAPA FÍSICO)
// =================================================================
const uint8_t PIN_BOTONES[8] = {5, 6, 7, 8, 9, 10, 11, 12};
const uint8_t PIN_ENCODER_SW = 4;
const uint8_t PIN_STEP_PLUS = A1;
const uint8_t PIN_STEP_MINUS = A6; // Entrada exclusiva del conversor analógico (ADC)
const uint8_t PIN_BUZZER = A0;
const uint8_t PIN_LED_G = A2;      // Cátodo Verde del LED RGB (Ánodo común)
const uint8_t PIN_LED_R = A3;      // Cátodo Rojo del LED RGB (Ánodo común)

// =================================================================
// 3. MATRICES DE DATOS MUSICALES
// =================================================================
bool estadoPasos[8] = {true, true, true, true, true, true, true, true}; 
int notasPlay[8] = {4, 7, 9, 11, 12, 14, 16, 14}; // Secuencia inicial "On the run"

// Tabla de frecuencias en Hertz para el Buzzer local
const int escala[36] = {
  131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294,
  311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698,
  740, 784, 831, 880, 932, 988
};

// Matriz de caracteres para la representación visual en la pantalla OLED
const char *nombresNotas[36] = {
  "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
  "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
  "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"
};

// =================================================================
// 4. VARIABLES DE CONTROL GLOBAL
// =================================================================
uint8_t estadoTransporte = 0;   // Máquina de estados: 0 = STOP, 1 = PLAY, 2 = PAUSE
int BPM = 165;                
unsigned long tiempoPaso = 181; // Duración de cada paso en milisegundos
unsigned long relojSeq = 0;     // Registro de tiempo para el avance del secuenciador
int pasoActual = 0;             // Cabezal de lectura (0 a 7)

int notaMidiActual = -1;        // Almacena la última nota enviada para evitar notas colgadas
long posicionEncoderAnt = 0;    
bool encoderUsadoEnPaso[8] = {false}; // Bandera para diferenciar una afinación de un muteo
bool estadoAntA6 = HIGH;        // Registro previo del pin analógico A6
unsigned long tiempoDebounceA6 = 0;
bool actualizarPantalla = true; // Refresco coalescido para evitar sobrecarga del OLED
const unsigned long intervaloRefrescoOLED = 100; // Refresco máximo más lento para priorizar audio
unsigned long ultimoRefrescoOLED = 0;
int ultimoPasoDibujado = -1;
uint8_t ultimoEstadoTransporte = 255;

unsigned long tiempoUltimoClic = 0; // Temporizador para la detección de doble clic
bool esperandoSegundoClic = false;

// Prototipos de funciones
void dibujarOLED();
void controlarLeds();
void enviarMIDI(byte comando, byte nota, byte velocidad);

// =================================================================
// 5. ETAPA DE CONFIGURACIÓN (SETUP)
// =================================================================
void setup() {
  // Comunicación serial configurada a alta velocidad para MIDI por USB
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  
  // Apagado inicial de los LEDs (Lógica invertida por ser Ánodo Común)
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);

  // Inicialización de la librería Bounce2 para entradas digitales fijas
  for (int i = 0; i < 8; i++) {
    botones[i].attach(PIN_BOTONES[i], INPUT_PULLUP);
    botones[i].interval(10); 
  }
  btnEncSW.attach(PIN_ENCODER_SW, INPUT_PULLUP);
  btnEncSW.interval(10);
  btnStepMas.attach(PIN_STEP_PLUS, INPUT_PULLUP);
  btnStepMas.interval(10);

  // CONFIGURACIÓN CORRECTA: Se aplica la dirección 0x3C modificada para U8g2
  u8g2.setI2CAddress(0x3C * 2); 
  u8g2.begin();

  tiempoPaso = 60000UL / BPM / 2; // Conversión matemática de BPM a milisegundos por paso
}

// =================================================================
// 6. BUCLE DE EJECUCIÓN CONTINUA (LOOP)
// =================================================================
void loop() {
  // Refresco del estado de los filtros de entrada
  for (int i = 0; i < 8; i++) botones[i].update();
  btnEncSW.update();
  btnStepMas.update();

  // MÁQUINA DE ESTADOS: Manejo del pulsador del encoder
  if (btnEncSW.fell()) {
    if (esperandoSegundoClic && (millis() - tiempoUltimoClic < 300)) {
      // DOBLE CLIC CONFIRMADO -> Modo STOP
      estadoTransporte = 0;
      pasoActual = 0;
      noTone(PIN_BUZZER);
      if (notaMidiActual != -1) enviarMIDI(0x80, notaMidiActual, 0);
      notaMidiActual = -1;
      esperandoSegundoClic = false;
    } else {
      if (estadoTransporte == 1) {
        estadoTransporte = 2; // PAUSE inmediato
        noTone(PIN_BUZZER);
        if (notaMidiActual != -1) enviarMIDI(0x80, notaMidiActual, 0);
        notaMidiActual = -1;
      } else {
        estadoTransporte = 1; // PLAY inmediato
        relojSeq = millis() - tiempoPaso;
      }
      esperandoSegundoClic = true;
      tiempoUltimoClic = millis();
    }
    actualizarPantalla = true;
  }

  if (esperandoSegundoClic && (millis() - tiempoUltimoClic >= 300)) {
    esperandoSegundoClic = false;
  }

  // CONTROLES MANUALES DE CABEZAL (Habilitados únicamente en modo PAUSE)
  if (estadoTransporte == 2) {
    // Botón Avanzar Paso (Digital)
    if (btnStepMas.fell()) { 
      pasoActual = (pasoActual + 1) % 8;
      if (estadoPasos[pasoActual]) tone(PIN_BUZZER, escala[notasPlay[pasoActual]], 100);
      actualizarPantalla = true;
    }
    
    // Botón Retroceder Paso (Lectura del pin analógico A6 con Pull-Up externa)
    bool estadoActualA6 = (analogRead(PIN_STEP_MINUS) < 500) ? LOW : HIGH;
    if (estadoActualA6 != estadoAntA6 && millis() - tiempoDebounceA6 > 15) {
      if (estadoActualA6 == LOW) {
        pasoActual = (pasoActual - 1 + 8) % 8; 
        if (estadoPasos[pasoActual]) tone(PIN_BUZZER, escala[notasPlay[pasoActual]], 100);
        actualizarPantalla = true;
      }
      estadoAntA6 = estadoActualA6;
      tiempoDebounceA6 = millis();
    }
  }

  // INTERPRETACIÓN DEL ENCODER ROTATIVO
  long posActual = miEncoder.read() / 4; 
  int delta = posActual - posicionEncoderAnt;
  
  if (delta != 0) { 
    bool afinandoPaso = false;
    
    // Verificación de co-procesamiento: botón presionado + giro de encoder = Afinación
    for (int i = 0; i < 8; i++) {
      if (botones[i].read() == LOW) { 
        notasPlay[i] += delta; 
        if (notasPlay[i] < 0) notasPlay[i] = 0;
        if (notasPlay[i] > 35) notasPlay[i] = 35;
        encoderUsadoEnPaso[i] = true; 
        afinandoPaso = true;
        if (estadoTransporte != 1) tone(PIN_BUZZER, escala[notasPlay[i]], 100); 
      }
    }
    
    // Si no se altera ningún paso individual, el giro modifica los BPM globales
    if (!afinandoPaso) {
      BPM += (delta * 2); 
      if (BPM < 60) BPM = 60;   
      if (BPM > 240) BPM = 240; 
      tiempoPaso = 60000UL / BPM / 2; 
    }
    
    posicionEncoderAnt = posActual;
    actualizarPantalla = true;
  }

  // LÓGICA DE ACTIVACIÓN / MUTE DE PASOS
  for (int i = 0; i < 8; i++) {
    if (botones[i].fell()) {
      encoderUsadoEnPaso[i] = false; 
    }
    if (botones[i].rose()) { 
      // Se conmuta el paso solo si el botón no fue retenido para realizar una afinación
      if (!encoderUsadoEnPaso[i]) {
        estadoPasos[i] = !estadoPasos[i];
        actualizarPantalla = true;
      }
    }
  }

  // MOTOR SECUENCIADOR DE TIEMPO REAL (Gatillado en modo PLAY)
  if (estadoTransporte == 1) { 
    if (millis() - relojSeq >= tiempoPaso) {
      pasoActual = (pasoActual + 1) % 8; 
      relojSeq = millis(); 
      
      // Corte inmediato de la nota MIDI anterior para limpiar el canal del sintetizador
      if (notaMidiActual != -1) {
        enviarMIDI(0x80, notaMidiActual, 0); 
        notaMidiActual = -1;
      }

      // Si el paso actual está encendido, se dispara el audio local y el comando MIDI externo
      if (estadoPasos[pasoActual]) {
        tone(PIN_BUZZER, escala[notasPlay[pasoActual]]);
        
        // Mapeo a estándar MIDI: Sumamos 48 para que el índice 0 corresponda a C3
        notaMidiActual = 48 + sizeof(notasPlay) ? (48 + notasPlay[pasoActual]) : 48;
        enviarMIDI(0x90, notaMidiActual, 127); // 0x90 = Note ON, 127 = Velocidad máxima
      } else {
        noTone(PIN_BUZZER); 
      }
      actualizarPantalla = true; 
    }
  }

  // Salidas físicas secundarias (Luces e imágenes)
  controlarLeds();
  
  bool debeRefrescar = actualizarPantalla && (millis() - ultimoRefrescoOLED >= intervaloRefrescoOLED);
  if (debeRefrescar) {
    if (estadoTransporte == 1) {
      if (pasoActual != ultimoPasoDibujado || estadoTransporte != ultimoEstadoTransporte) {
        dibujarOLED();
        actualizarPantalla = false;
        ultimoRefrescoOLED = millis();
      }
    } else {
      dibujarOLED();
      actualizarPantalla = false;
      ultimoRefrescoOLED = millis();
    }
  }

  ultimoPasoDibujado = pasoActual;
  ultimoEstadoTransporte = estadoTransporte;
}

// =================================================================
// 7. SUBRUTINA: TRANSMISIÓN DE COMANDOS MIDI SERIALES
// =================================================================
void enviarMIDI(byte comando, byte nota, byte velocidad) {
  Serial.write(comando);   
  Serial.write(nota);      
  Serial.write(velocidad); 
}

// =================================================================
// 8. SUBRUTINA: MÁQUINA DE CONTROL LED RGB (LÓGICA INVERTIDA)
// =================================================================
void controlarLeds() {
  if (estadoTransporte == 0) { 
    // MODO STOP -> Rojo Fijo
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, HIGH);
  } 
  else if (estadoTransporte == 2) { 
    // MODO PAUSA -> Amarillo Intermitente (Mezcla de Rojo y Verde)
    bool titilar = (millis() / 500) % 2; 
    digitalWrite(PIN_LED_R, titilar ? LOW : HIGH);
    digitalWrite(PIN_LED_G, titilar ? LOW : HIGH);
  } 
  else if (estadoTransporte == 1) { 
    // MODO PLAY -> Verde Pulsante parpadeando en sincronía con los pasos activos
    digitalWrite(PIN_LED_R, HIGH); 
    if (estadoPasos[pasoActual] && (millis() - relojSeq < 100)) {
      digitalWrite(PIN_LED_G, LOW); 
    } else {
      digitalWrite(PIN_LED_G, HIGH);
    }
  }
}

// =================================================================
// 9. SUBRUTINA: PROCESAMIENTO GRÁFICO (DIBUJO POR PÁGINAS)
// =================================================================
void dibujarOLED() {
  u8g2.firstPage();
  do {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_ncenB08_tr);
    const char* estadoTexto = (estadoTransporte == 1) ? "PLAY" :
                              (estadoTransporte == 2) ? "PAUSE" : "STOP";
    u8g2.drawStr(0, 10, estadoTexto);

    u8g2.setCursor(70, 10);
    u8g2.print(F("BPM: "));
    u8g2.print(BPM);

    u8g2.setFont(u8g2_font_4x6_tr);
    for (int i = 0; i < 8; i++) {
      int posX = (i * 15) + 4;
      int posY = 35;

      if (estadoPasos[i]) {
        if (i == pasoActual && estadoTransporte != 0) {
          u8g2.drawBox(posX, posY, 14, 20);
          u8g2.setDrawColor(0);
          u8g2.setCursor(posX + 2, posY + 12);
          u8g2.print(nombresNotas[notasPlay[i]]);
          u8g2.setDrawColor(1);
        } else {
          u8g2.drawFrame(posX, posY, 14, 20);
          u8g2.setCursor(posX + 2, posY + 12);
          u8g2.print(nombresNotas[notasPlay[i]]);
        }
      } else {
        u8g2.drawFrame(posX, posY, 14, 20);
        if (i == pasoActual && estadoTransporte != 0) {
          u8g2.drawBox(posX + 5, posY + 8, 4, 4);
        }
      }
    }
  } while (u8g2.nextPage());
}