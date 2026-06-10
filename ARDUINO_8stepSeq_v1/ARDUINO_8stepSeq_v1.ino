#include <Arduino.h>

// DEFINO PINES Y VARIABLES
const int pinPot = A4;
const int pinBuzzer = A5;

const int pinBotones[8] = {2, 3, 4, 5, 6, 7, 8, 9};
const int pinLeds[8] = {10, 11, 12, 13, A0, A1, A2, A3};

// DEFINO ESTADO DE PASOS Y ESCALA DEFAULT
bool estadoPasos[8] = {true, true, true, true, true, true, true, true};

const int escala[36] = {
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294,
    311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698,
    740, 784, 831, 880, 932, 988};

// MATRIZ DE TEXTO ESTÁTICO (Evita el colapso de RAM en el Uno)
const char *nombresNotas[36] = {
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"};

int notasPlay[8] = {12, 14, 16, 17, 19, 21, 23, 24};

// DEFINO INICIO DE SECUENCIADOR Y ANTIRREBOTE
int estadoAnterior[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long relojSeq = 0;
int pasoActual = 7;
int botonesPresionados = 0;
unsigned long tiempoRebote[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// DEFINO VARIABLES DE TIEMPO
int tiempoNegra;
int BPM = 120;
unsigned long tiempoPaso = 250;
int poteAnterior = 512;
int lecturaPoteNotaAnterior = -1;

// DEFINO CONTROL DE ESTADOS Y COMBOS
bool enEjecucion = false;
bool comboPlayPauseAnt = false;
bool comboStopRestartAnt = false;
bool comboResetAnt = false;
bool comboRandomAnt = false;
bool ignorarLiberacion[8] = {false, false, false, false, false, false, false, false};

// ALGORITMO DE ENGANCHE: Bandera de bloqueo para el potenciómetro de BPM
bool bpmBloqueado = false;

// DEFINO COMPORTAMIENTO INICIAL
void setup()
{
  Serial.begin(9600);
  delay(1000);

  pinMode(pinBuzzer, OUTPUT);
  for (int i = 0; i < 8; i++)
  {
    pinMode(pinBotones[i], INPUT_PULLUP);
    pinMode(pinLeds[i], OUTPUT);
  }

  // SINCRONIZACIÓN DE ARRANQUE
  comboPlayPauseAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[7]) == LOW);
  comboStopRestartAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[6]) == LOW);
  comboResetAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[4]) == LOW);
  comboRandomAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[2]) == LOW);
}

// LOOP GENERAL
void loop()
{
  // ------------------------------------------------------------------
  // LECTURA DE COMBOS VIRTUALES
  // ------------------------------------------------------------------

  // 1. COMBO PLAY / PAUSE (BOTONES 1 Y 8)
  bool comboPlayPauseAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[7]) == LOW);
  if (comboPlayPauseAct == true && comboPlayPauseAnt == false)
  {
    enEjecucion = !enEjecucion;
    ignorarLiberacion[0] = true;
    ignorarLiberacion[7] = true;

    if (enEjecucion == false)
    {
      noTone(pinBuzzer);
      Serial.println("--- SECUENCIADOR: PAUSE ---");
    }
    else
    {
      Serial.println("--- SECUENCIADOR: PLAY ---");
    }
  }
  comboPlayPauseAnt = comboPlayPauseAct;

  // 2. COMBO STOP / RESTART (BOTONES 1 Y 7)
  bool comboStopRestartAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[6]) == LOW);
  if (comboStopRestartAct == true && comboStopRestartAnt == false)
  {
    enEjecucion = !enEjecucion;
    ignorarLiberacion[0] = true;
    ignorarLiberacion[6] = true;

    if (enEjecucion == false)
    {
      noTone(pinBuzzer);
      digitalWrite(pinLeds[pasoActual], LOW);
      Serial.println("--- SECUENCIADOR: STOP ---");
    }
    else
    {
      pasoActual = 7;
      relojSeq = millis() - tiempoPaso;
      Serial.println("--- SECUENCIADOR: RESTART ---");
    }
  }
  comboStopRestartAnt = comboStopRestartAct;

  // 3. COMBO RESET INSTANTÁNEO (BOTONES 1 Y 5)
  bool comboResetAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[4]) == LOW);
  if (comboResetAct == true && comboResetAnt == false)
  {
    digitalWrite(pinLeds[pasoActual], LOW);
    noTone(pinBuzzer);

    pasoActual = 7;
    relojSeq = millis() - tiempoPaso;

    ignorarLiberacion[0] = true;
    ignorarLiberacion[4] = true;

    Serial.println("--- RESET INSTANTANEO ---");
  }
  comboResetAnt = comboResetAct;

  // 4. COMBO RANDOMIZACIÓN (BOTONES 1 Y 3)
  bool comboRandomAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[2]) == LOW);
  if (comboRandomAct == true && comboRandomAnt == false)
  {
    randomSeed(millis());
    for (int i = 0; i < 8; i++)
    {
      estadoPasos[i] = random(2);
      notasPlay[i] = random(36);
    }
    ignorarLiberacion[0] = true;
    ignorarLiberacion[2] = true;
    Serial.println("--- RANDOMIZACION APLICADA ---");
  }
  comboRandomAnt = comboRandomAct;

  // ------------------------------------------------------------------
  // LECTURAS ANALÓGICAS Y MÁQUINA DE ESTADOS DE BOTONES
  // ------------------------------------------------------------------

  // ADUANA DE CONTROL: Verifica si la perilla cruzó el valor teórico asignado al BPM actual
  if (bpmBloqueado == true)
  {
    int poteObjetivo = map(BPM, 80, 220, 0, 1023); // Mapeo inverso: Calculo la posición teórica de la perilla
    int lecturaActual = analogRead(pinPot);

    if (abs(lecturaActual - poteObjetivo) < 15) // Ventana de coincidencia de 15 puntos del ADC
    {
      bpmBloqueado = false;         // Destrabo el motor de BPM de forma definitiva
      poteAnterior = lecturaActual; // Sincronizo la memoria para asimilar el movimiento fluido
      Serial.println("--- BPM ENGANCHADO ---");
    }
  }

  // CALCULO DE BPM: Solo se ejecuta si no hay botones retenidos Y el potenciómetro está acoplado
  if (botonesPresionados == 0 && bpmBloqueado == false)
  {
    int lecturaActual = analogRead(pinPot);
    if (abs(lecturaActual - poteAnterior) > 10)
    {
      BPM = map(lecturaActual, 0, 1023, 80, 220);
      tiempoPaso = 60000UL / BPM / 2;
      poteAnterior = lecturaActual;
    }
  }

  // ESCANER FISICO DE BOTONES INDIVIDUALES
  for (int i = 0; i < 8; i++)
  {
    int estadoActual = digitalRead(pinBotones[i]);

    // ESTADO 1: IMPACTO DE BAJADA
    if (estadoActual == LOW && estadoAnterior[i] == HIGH)
    {
      tiempoRebote[i] = millis();
      botonesPresionados++;
      lecturaPoteNotaAnterior = -1;
    }

    // ESTADO 2: RETENCIÓN PARA AFINAR NOTA
    if (estadoActual == LOW && estadoAnterior[i] == LOW && millis() - tiempoRebote[i] > 300)
    {
      if (ignorarLiberacion[i] == false)
      {
        int lecturaAfinacion = analogRead(pinPot);
        if (abs(lecturaAfinacion - poteAnterior) > 10)
        {
          poteAnterior = lecturaAfinacion;
        }

        int indiceElegido = map(poteAnterior, 0, 1023, 0, 35);
        notasPlay[i] = indiceElegido;

        if (indiceElegido != lecturaPoteNotaAnterior)
        {
          Serial.print("Afinando Paso ");
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.println(nombresNotas[indiceElegido]);
          lecturaPoteNotaAnterior = indiceElegido;
        }

        digitalWrite(pinLeds[i], (millis() / 50) % 2);
      }
    }

    // ESTADO 3: LIBERACIÓN DEL BOTÓN
    if (estadoActual == HIGH && estadoAnterior[i] == LOW)
    {
      // Toque corto: Inversión de paso
      if (millis() - tiempoRebote[i] > 50 && millis() - tiempoRebote[i] < 300)
      {
        if (ignorarLiberacion[i] == false)
        {
          estadoPasos[i] = !estadoPasos[i];
        }
      }
      // Pulsación larga: Al salir del modo nota, activo la traba por acoplamiento mecánico
      else if (millis() - tiempoRebote[i] >= 300)
      {
        if (ignorarLiberacion[i] == false)
        {
          bpmBloqueado = true; // Forzado de enganche obligatorio
          Serial.println("--- BPM CONGELADO: Busque el punto de enganche con la perilla ---");
        }
      }

      ignorarLiberacion[i] = false;
      botonesPresionados--;
      if (botonesPresionados < 0)
      {
        botonesPresionados = 0;
      }

      poteAnterior = analogRead(pinPot);
      digitalWrite(pinLeds[i], LOW);
    }

    estadoAnterior[i] = estadoActual;
  }

  // ------------------------------------------------------------------
  // LOOP SEQ PRINCIPAL (MOTOR DE AUDIO)
  // ------------------------------------------------------------------
  if (enEjecucion == true)
  {
    if (millis() - relojSeq >= tiempoPaso)
    {
      digitalWrite(pinLeds[pasoActual], LOW);
      noTone(pinBuzzer);
      pasoActual++;
      relojSeq = millis();

      if (pasoActual == 8)
      {
        pasoActual = 0;

        Serial.print("--- BPM: ");
        Serial.print(BPM);
        Serial.print(" | Set: ");
        for (int x = 0; x < 8; x++)
        {
          if (estadoPasos[x] == true)
          {
            Serial.print("[");
            Serial.print(nombresNotas[notasPlay[x]]);
            Serial.print("]");
          }
          else
          {
            Serial.print("x-");
            Serial.print(nombresNotas[notasPlay[x]]);
          }
          if (x < 7)
            Serial.print("-");
        }
        Serial.println(" ---");
      }

      if (estadoPasos[pasoActual] == true)
      {
        if (estadoAnterior[pasoActual] == HIGH || ignorarLiberacion[pasoActual] == true)
        {
          digitalWrite(pinLeds[pasoActual], HIGH);
        }
        tone(pinBuzzer, escala[notasPlay[pasoActual]]);
      }
    }
  }
}