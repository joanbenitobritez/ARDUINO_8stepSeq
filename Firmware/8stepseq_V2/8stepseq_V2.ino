#include <Arduino.h>
#include <U8x8lib.h> 
#include <Wire.h>    
#include <Bounce2.h> 
#include <Encoder.h> 
#include <MIDI.h>    

// =================================================================
// 1. INSTANCIAS DE HARDWARE Y PROTOCOLO
// =================================================================
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
Encoder miEncoder(2, 3);          
Bounce botones[8];                
Bounce btnEncSW = Bounce();
Bounce btnStepUp = Bounce();
MIDI_CREATE_DEFAULT_INSTANCE();

// =================================================================
// 2. CONFIGURACIÓN TÉCNICA Y ASIGNACIÓN DE PINES
// =================================================================
const int RESOLUCION_ENCODER = 4; 
const int FIGURA_MUSICAL = 4;     

const uint8_t PIN_BOTONES[8] = {5, 6, 7, 8, 9, 10, 11, 12};
const uint8_t PIN_ENCODER_SW = 4;
const uint8_t PIN_STEP_UP = A1;
const uint8_t PIN_STEP_DN = A6; 
const uint8_t PIN_BUZZER = A0;
const uint8_t PIN_LED_G = A2;
const uint8_t PIN_LED_R = A3;

// =================================================================
// 3. VARIABLES DE ESTADO Y FLAGS
// =================================================================
bool buzzerActivado = true;

// ---- Variables Modo Teclado ----
bool modoTeclado = false; 
int tipoEscala = 0; // 0 = Mayor, 1 = Menor
int raizTeclado = 12; // C4 (Índice 12)

const int intervalosMayor[8] = {0, 2, 4, 5, 7, 9, 11, 12}; 
const int intervalosMenor[8] = {0, 2, 3, 5, 7, 8, 10, 12}; 
int notasSonandoTeclado[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; 

// ---- Variables Secuenciador ----
bool estadoPasos[8] = {true, true, true, true, true, true, true, true};
int notasPlay[8] = {4, 7, 9, 11, 12, 14, 16, 14};

bool cambioPasos[8] = {true, true, true, true, true, true, true, true};
bool cambioTransporte = true; 
bool cambioBPM = true;
bool cambioCabezal = true;
bool refrescarTextosNotas = true; 

bool encoderUsadoEnPaso[8] = {false};

int notaMostradaEdit = -1;
int pasoEditadoActual = -1; 
int notaMostradaPlay = -1;

const int escala[36] = {131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988};
const char *nombresCortos[12] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};
const char *nombresNotas[36] = {"C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"};

// =================================================================
// 4. MOTOR DE TIEMPO
// =================================================================
uint8_t estadoTransporte = 0; 
int BPM = 165;
unsigned long tiempoPaso = 181; 
unsigned long relojSeq = 0;
int pasoActual = 0;
int pasoAnterior = 0;

int notaMidiActual = -1;        
unsigned long tiempoInicioNota = 0; 

unsigned long tiempoUltimoClic = 0; 
long posicionEncoderAnt = 0;

bool estadoAntStepDn = HIGH;
unsigned long ultimoMuestreoADC = 0; 

void setup() {
  delay(200); 

  MIDI.begin(1);
  MIDI.turnThruOff(); 

  Wire.begin();
  // Se remueve setWireTimeout para evitar el corte por ráfaga inicial I2C
  
  u8x8.begin();
  u8x8.setPowerSave(0); 
  u8x8.setFont(u8x8_font_chroma48medium8_r); 
  u8x8.clearDisplay(); // Limpieza inicial obligatoria de la memoria de video
  
  pinMode(PIN_BUZZER, OUTPUT); 
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT); 

  for (int i = 0; i < 8; i++) {
    botones[i].attach(PIN_BOTONES[i], INPUT_PULLUP);
    botones[i].interval(10);
  }
  
  btnEncSW.attach(PIN_ENCODER_SW, INPUT_PULLUP);
  btnEncSW.interval(10);
  btnStepUp.attach(PIN_STEP_UP, INPUT_PULLUP); 
  btnStepUp.interval(10);
  
  tiempoPaso = 60000UL / BPM / FIGURA_MUSICAL;
}

void loop() {
  // ---------------------------------------------------------------
  // A. ESCANEO DE ENTRADAS
  // ---------------------------------------------------------------
  for (int i = 0; i < 8; i++) botones[i].update();
  btnEncSW.update();
  btnStepUp.update();  

  bool stepDnPresionado = false;
  if (millis() - ultimoMuestreoADC >= 10) {
    bool estadoActualStepDn = (analogRead(PIN_STEP_DN) < 500) ? LOW : HIGH;
    if (estadoActualStepDn != estadoAntStepDn) {
      if (estadoActualStepDn == LOW) stepDnPresionado = true;
      estadoAntStepDn = estadoActualStepDn;
    }
    ultimoMuestreoADC = millis();
  }

  bool upActivo = (btnStepUp.read() == LOW);
  bool dnActivo = (analogRead(PIN_STEP_DN) < 500);

  // ---------------------------------------------------------------
  // B. INTERCEPTORES DE SISTEMA (COMBINACIONES)
  // ---------------------------------------------------------------
  if ((btnStepUp.fell() && dnActivo) || (stepDnPresionado && upActivo)) {
    modoTeclado = !modoTeclado;
    u8x8.clearDisplay(); 
    
    cambioTransporte = true;
    cambioBPM = true;
    cambioCabezal = true;
    refrescarTextosNotas = true;
    for(int i=0; i<8; i++) cambioPasos[i] = true;
    
    noTone(PIN_BUZZER);
    for (int i=0; i<127; i++) MIDI.sendNoteOff(i, 0, 1);
    stepDnPresionado = false; 
    return; 
  }

  // ---------------------------------------------------------------
  // C. GATE MIDI
  // ---------------------------------------------------------------
  if (!modoTeclado && notaMidiActual != -1) {
    if (millis() - tiempoInicioNota >= (tiempoPaso / 2)) {
      MIDI.sendNoteOff(notaMidiActual, 0, 1);
      notaMidiActual = -1;
      noTone(PIN_BUZZER);
    }
  }

  // ---------------------------------------------------------------
  // D. LÓGICA DIVIDIDA (SECUENCIADOR)
  // ---------------------------------------------------------------
  if (!modoTeclado) {
    
    if (btnStepUp.fell() && !dnActivo) { 
      buzzerActivado = true;
      if (estadoTransporte != 1) { 
        pasoActual = (pasoActual + 1) % 8;
        if (estadoPasos[pasoActual]) {
          if (buzzerActivado) tone(PIN_BUZZER, escala[notasPlay[pasoActual]], 100);
          notaMostradaPlay = notasPlay[pasoActual];
        }
        cambioCabezal = true; 
      }
      refrescarTextosNotas = true;
    }

    if (stepDnPresionado && !upActivo) {
      buzzerActivado = false;
      noTone(PIN_BUZZER);
      if (estadoTransporte != 1) { 
        pasoActual = (pasoActual - 1 + 8) % 8;
        if (estadoPasos[pasoActual]) {
          notaMostradaPlay = notasPlay[pasoActual];
        }
        cambioCabezal = true;
      }
      refrescarTextosNotas = true;
    }

    if (btnEncSW.fell()) { 
      if (millis() - tiempoUltimoClic < 300) { 
        estadoTransporte = 0; 
        pasoActual = 0;       
        cambioCabezal = true; 
        notaMostradaPlay = -1; 
        refrescarTextosNotas = true;
        noTone(PIN_BUZZER);
        if (notaMidiActual != -1) { 
          MIDI.sendNoteOff(notaMidiActual, 0, 1);
          notaMidiActual = -1;
        }
      } else {
        estadoTransporte = (estadoTransporte == 1) ? 2 : 1;
        if (estadoTransporte == 2) {
          noTone(PIN_BUZZER);
          if (notaMidiActual != -1) { 
            MIDI.sendNoteOff(notaMidiActual, 0, 1);
            notaMidiActual = -1;
          }
        }
      }
      tiempoUltimoClic = millis();
      cambioTransporte = true;
    }

    if (estadoTransporte == 1 && (millis() - relojSeq >= tiempoPaso)) {
      pasoActual = (pasoActual + 1) % 8;
      relojSeq = millis(); 
      
      if (notaMidiActual != -1) { 
        MIDI.sendNoteOff(notaMidiActual, 0, 1); 
        notaMidiActual = -1;
        noTone(PIN_BUZZER);
      }
      
      if (estadoPasos[pasoActual]) { 
        if (buzzerActivado) tone(PIN_BUZZER, escala[notasPlay[pasoActual]]);
        
        notaMidiActual = 48 + notasPlay[pasoActual];
        MIDI.sendNoteOn(notaMidiActual, 127, 1);
        tiempoInicioNota = millis(); 
        
        notaMostradaPlay = notasPlay[pasoActual];
        refrescarTextosNotas = true;
      } 
      
      cambioPasos[pasoActual] = true; 
      cambioCabezal = true;
    }

  } else {
    // ---- LÓGICA DIVIDIDA (TECLADO) ----
    if (btnStepUp.fell() && !dnActivo) {
      tipoEscala = 0; 
      refrescarTextosNotas = true;
    }
    if (stepDnPresionado && !upActivo) {
      tipoEscala = 1; 
      refrescarTextosNotas = true;
    }
  }

  // ---------------------------------------------------------------
  // ENRUTAMIENTO DEL ENCODER PRINCIPAL
  // ---------------------------------------------------------------
  long posHw = miEncoder.read() / RESOLUCION_ENCODER;
  int delta = posHw - posicionEncoderAnt;
  
  if (delta != 0) { 
    posicionEncoderAnt = posHw;
    
    if (!modoTeclado) {
      bool afinandoPaso = false;
      for (int i = 0; i < 8; i++) { 
        if (botones[i].read() == LOW) { 
          notasPlay[i] += delta;
          if (notasPlay[i] < 0) notasPlay[i] = 0; 
          if (notasPlay[i] > 35) notasPlay[i] = 35; 
          
          encoderUsadoEnPaso[i] = true;
          afinandoPaso = true;
          cambioPasos[i] = true;
          
          notaMostradaEdit = notasPlay[i];
          pasoEditadoActual = i + 1; 
          refrescarTextosNotas = true;
          
          if (estadoTransporte != 1 && buzzerActivado) tone(PIN_BUZZER, escala[notasPlay[i]]);
          break; 
        }
      }
      
      if (!afinandoPaso) { 
        BPM += (delta * 2);
        if (BPM < 60) BPM = 60;
        if (BPM > 240) BPM = 240;
        tiempoPaso = 60000UL / BPM / FIGURA_MUSICAL; 
        cambioBPM = true; 
      }
      
    } else {
      raizTeclado += delta;
      if (raizTeclado < 0) raizTeclado = 0; 
      if (raizTeclado > 35) raizTeclado = 35; 
      refrescarTextosNotas = true;
    }
  }

  // ---------------------------------------------------------------
  // E. ESCANEO DE MATRIZ DE BOTONES
  // ---------------------------------------------------------------
  for (int i = 0; i < 8; i++) {
    if (botones[i].fell()) {
      encoderUsadoEnPaso[i] = false;
      
      if (modoTeclado) {
        int intervalo = (tipoEscala == 0) ? intervalosMayor[i] : intervalosMenor[i];
        int indiceBuzzer = raizTeclado + intervalo;
        int notaMidi = 48 + indiceBuzzer;
        
        notasSonandoTeclado[i] = notaMidi;
        MIDI.sendNoteOn(notaMidi, 127, 1);
        
        if (indiceBuzzer >= 0 && indiceBuzzer <= 35) {
          if (buzzerActivado) tone(PIN_BUZZER, escala[indiceBuzzer]);
          notaMostradaPlay = indiceBuzzer;
        } else {
          notaMostradaPlay = -1;
        }
        refrescarTextosNotas = true;
      }
    }
    
    if (botones[i].rose()) { 
      if (!modoTeclado) {
        if (!encoderUsadoEnPaso[i]) { 
          estadoPasos[i] = !estadoPasos[i];
          cambioPasos[i] = true;
        } else {
          noTone(PIN_BUZZER); 
        }
      } else {
        if (notasSonandoTeclado[i] != -1) {
          MIDI.sendNoteOff(notasSonandoTeclado[i], 0, 1);
          noTone(PIN_BUZZER);
          notasSonandoTeclado[i] = -1;
          notaMostradaPlay = -1; 
          refrescarTextosNotas = true; 
        }
      }
    }
  }

  // ---------------------------------------------------------------
  // F. RENDER PANTALLA 
  // ---------------------------------------------------------------
  if (!modoTeclado) {
    if (cambioTransporte) { 
      u8x8.setCursor(0,0);
      if(estadoTransporte==1) u8x8.print("SEQ PLAY");
      else if(estadoTransporte==2) u8x8.print("SEQ PAUS"); 
      else u8x8.print("SEQ STOP"); 
      cambioTransporte = false;
    }
    
    if (cambioBPM) { 
      u8x8.setCursor(9,0);
      char bufBPM[16]; // Aumentado para mayor seguridad
      snprintf(bufBPM, sizeof(bufBPM), "BPM:%d  ", BPM); 
      u8x8.print(bufBPM); 
      cambioBPM = false;
    }
    
    for(int i=0; i<8; i++){
      if(cambioPasos[i]){
        u8x8.setCursor(i*2, 2);
        u8x8.print(estadoPasos[i] ? "[]" : "  "); 
        
        u8x8.setCursor(i*2, 3);
        if (estadoPasos[i]) {
          u8x8.print(nombresCortos[notasPlay[i] % 12]);
        } else {
          u8x8.print("--"); 
        }
        cambioPasos[i] = false;
      }
    }

    if (cambioCabezal) {
      u8x8.setCursor(pasoAnterior * 2, 4); 
      u8x8.print("  ");
      u8x8.setCursor(pasoActual * 2, 4);
      u8x8.print("^ "); 
      pasoAnterior = pasoActual;
      cambioCabezal = false;
    }

    if (refrescarTextosNotas) {
      u8x8.setCursor(0, 6);
      char bufEdit[24]; // Aumentado para evitar truncamiento
      if (pasoEditadoActual != -1 && notaMostradaEdit != -1) {
        snprintf(bufEdit, sizeof(bufEdit), "EDIT P%d: %s   ", pasoEditadoActual, nombresNotas[notaMostradaEdit]);
      } else {
        snprintf(bufEdit, sizeof(bufEdit), "EDIT P-: --     ");
      }
      u8x8.print(bufEdit);

      u8x8.setCursor(0, 7);
      char bufPlay[24]; // Aumentado
      if (notaMostradaPlay != -1) {
        snprintf(bufPlay, sizeof(bufPlay), "PLAY: %s        ", nombresNotas[notaMostradaPlay]);
      } else {
        snprintf(bufPlay, sizeof(bufPlay), "PLAY: --        ");
      }
      u8x8.print(bufPlay);
      
      refrescarTextosNotas = false;
    }
    
    digitalWrite(PIN_LED_R, (estadoTransporte == 0) ? LOW : HIGH); 
    digitalWrite(PIN_LED_G, (estadoTransporte == 1) ? LOW : HIGH); 

  } else {
    if (refrescarTextosNotas) {
      u8x8.setCursor(0, 0);
      u8x8.print("  MODO TECLADO  ");
      
      u8x8.setCursor(0, 3);
      char bufEscala[24];
      snprintf(bufEscala, sizeof(bufEscala), "MODO: %s     ", (tipoEscala == 0) ? "MAYOR" : "MENOR");
      u8x8.print(bufEscala);

      u8x8.setCursor(0, 6);
      char bufRaiz[24];
      snprintf(bufRaiz, sizeof(bufRaiz), "RAIZ: %s        ", nombresNotas[raizTeclado]);
      u8x8.print(bufRaiz);

      u8x8.setCursor(0, 7);
      char bufPlay[24];
      if (notaMostradaPlay != -1) {
        snprintf(bufPlay, sizeof(bufPlay), "PLAY: %s        ", nombresNotas[notaMostradaPlay]);
      } else {
        snprintf(bufPlay, sizeof(bufPlay), "PLAY: --        ");
      }
      u8x8.print(bufPlay);
      
      refrescarTextosNotas = false;
    }
    
    digitalWrite(PIN_LED_R, HIGH); 
    digitalWrite(PIN_LED_G, LOW); 
  }
}