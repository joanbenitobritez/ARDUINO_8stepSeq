#include <Arduino.h>

// DEFINO PINES DE PERIFERICOS IN/OUT Y VARIABLES
const int pinPot = A4;
const int pinBotones[8] = {2, 3, 4, 5, 6, 7, 8, 9};

const int pinBuzzer = A5;
const int pinLeds[8] = {10, 11, 12, 13, A0, A1, A2, A3};

// DEFINO ESTADO DE PASOS Y ESCALA DEFAULT
bool estadoPasos[8] = {true, true, true, true, true, true, true, true}; // INICIALIZO CON TODOS LOS PASOS ACTIVADOS

// ESCALA DE FRECUENCIAS (C3 A B5)
const int escala[36] = {
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294,
    311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698,
    740, 784, 831, 880, 932, 988};

// MATRIZ DE TEXTO
const char *nombresNotas[36] = {
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"};

// SECUENCIA INICIAL (ON THE RUN)
int notasPlay[8] = {4, 7, 9, 11, 12, 14, 16, 14};

// DEFINO INICIO DE SECUENCIADOR Y ANTIRREBOTE
int estadoAnterior[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; // SETEO LOS BOTONES EN HIGH PARA QUE EL FLANCO DE BAJADA SEA DETECTADO CORRECTAMENTE AL INICIO
unsigned long relojSeq = 0;                                               // INICIALIZO EL RELOJ DEL SECUENCIADOR
int pasoActual = 7;                                                       // INICIALIZO EL SECUENCIADOR EN EL PASO 8 PARA QUE AL INICIAR EL PRIMER CICLO, COMIENCE EN EL PASO 1
int botonesPresionados = 0;                                               // CONTADOR DE BOTONES ACTUALMENTE RETENIDOS (PARA CONTROL DE BPM Y MODO AFINACION)
unsigned long tiempoRebote[8] = {0, 0, 0, 0, 0, 0, 0, 0};                 // ARREGLO DE TIEMPOS DE REBOTE PARA CADA BOTON

// DEFINO VARIABLES DE TIEMPO
int tiempoNegra;
int BPM = 165;                    // BPM INICIAL ON THE RUN
unsigned long tiempoPaso = 181;   // CALCULADO A PARTIR DEL BPM INICIAL PARA CORCHEAS
int poteAnterior = 512;           // INICIALIZO LA MEMORIA DEL POTENCIOMETRO EN EL VALOR CENTRAL PARA EVITAR SALTOS INICIALES
int lecturaPoteNotaAnterior = -1; // MEMORIA PARA EVITAR REPETICIONES EN LA PANTALLA DURANTE EL MODO DE AFINACION

// DEFINO CONTROL DE ESTADOS Y COMBOS
bool enEjecucion = false;                                                             // FLAG GENERAL DE EJECUCION DEL SECUENCIADOR
bool comboPlayPauseAnt = false;                                                       // MEMORIA PARA DETECTAR EL FLANCO DE LOS COMBOS VIRTUALES PLAY/PAUSE
bool comboStopRestartAnt = false;                                                     // MEMORIA PARA DETECTAR EL FLANCO DE LOS COMBOS VIRTUALES STOP/RESTART
bool comboResetAnt = false;                                                           // MEMORIA PARA DETECTAR EL FLANCO DE LOS COMBOS VIRTUALES RESET INSTANTANEO
bool comboRandomAnt = false;                                                          // MEMORIA PARA DETECTAR EL FLANCO DE LOS COMBOS VIRTUALES RANDOMIZACION
bool ignorarLiberacion[8] = {false, false, false, false, false, false, false, false}; // FLAG DE CONTROL PARA EVITAR LA INTERFERENCIA ENTRE COMBOS VIRTUALES Y LA LOGICA DE LIBERACION DE BOTONES INDIVIDUALES

// FLAG DE BLOQUEO DE BPM
bool bpmBloqueado = false;

// DEFINO COMPORTAMIENTO INICIAL
void setup()
{
  Serial.begin(9600); // INICIALIZO SERIAL PARA MONITOREO DE DATOS Y ESTADOS
  delay(1000);        // PAUSO LA INICIALIZACION PARA QUE INICIE DESDE EL PASO 1 Y NO DESDE EL MOMENTO EN MILLIS QUE SE INICIA EL PROGRAMA

  pinMode(pinBuzzer, OUTPUT); // CONFIGURO EL PIN DEL BUZZER COMO SALIDA
  for (int i = 0; i < 8; i++) // CONFIGURO LOS PINES DE BOTONES COMO ENTRADA CON PULLUP Y LOS PINES DE LEDS COMO SALIDA
  {
    pinMode(pinBotones[i], INPUT_PULLUP);
    pinMode(pinLeds[i], OUTPUT);
  }

  // SINCRONIZACION DE ARRANQUE
  comboPlayPauseAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[7]) == LOW);   // EVALUO EL ESTADO DEL COMBO PLAY/PAUSE PARA SINCRONIZAR LA MEMORIA DE FLANCOS
  comboStopRestartAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[6]) == LOW); // EVALUO EL ESTADO DEL COMBO STOP/RESTART PARA SINCRONIZAR LA MEMORIA DE FLANCOS
  comboResetAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[4]) == LOW);       // EVALUO EL ESTADO DEL COMBO RESET PARA SINCRONIZAR LA MEMORIA DE FLANCOS
  comboRandomAnt = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[2]) == LOW);      // EVALUO EL ESTADO DEL COMBO RANDOMIZACION PARA SINCRONIZAR LA MEMORIA DE FLANCOS
}

// ------------------------------------------------------------------
// LOOP GENERAL
// ------------------------------------------------------------------
void loop()
{
  // ------------------------------------------------------------------
  // LECTURA DE COMBOS VIRTUALES
  // ------------------------------------------------------------------

  // COMBO PLAY / PAUSE (BOTONES 1 Y 8)
  bool comboPlayPauseAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[7]) == LOW); // LECTURA ACTUAL DEL COMBO PLAY/PAUSE
  if (comboPlayPauseAct == true && comboPlayPauseAnt == false)                                       // EVALUO SI LOS BOTONES 1 Y 8 ESTAN SIENDO PRESIONADOS SIMULTANEAMENTE Y SI ANTES NO LO ESTABAN
  {
    enEjecucion = !enEjecucion; // INVIERTO EL ESTADO DE EJECUCION
    ignorarLiberacion[0] = true;
    ignorarLiberacion[7] = true;

    if (enEjecucion == false) // SI PASA A PAUSA, APAGO EL SONIDO Y DEJO LEDS COMO ESTAN
    {
      noTone(pinBuzzer);
      Serial.println("--- SECUENCIADOR: PAUSE ---");
    }
    else
    {
      Serial.println("--- SECUENCIADOR: PLAY ---");
    }
  }
  comboPlayPauseAnt = comboPlayPauseAct; // ACTUALIZO LA MEMORIA DEL COMBO

  // COMBO STOP / RESTART (BOTONES 1 Y 7)
  bool comboStopRestartAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[6]) == LOW); // LECTURA ACTUAL DEL COMBO STOP/RESTART
  if (comboStopRestartAct == true && comboStopRestartAnt == false)                                     // EVALUO FLANCO DE SUBIDA DEL COMBO
  {
    enEjecucion = !enEjecucion;
    ignorarLiberacion[0] = true;
    ignorarLiberacion[6] = true;

    if (enEjecucion == false) // SI PASA A STOP, APAGO EL SONIDO Y LOS LEDS
    {
      noTone(pinBuzzer);
      digitalWrite(pinLeds[pasoActual], LOW);
      Serial.println("--- SECUENCIADOR: STOP ---");
    }
    else // SI REINICIA, VUELVO EL SECUENCIADOR AL PASO 8 PARA QUE EL MOTOR ARRANQUE EN EL PASO 1
    {
      pasoActual = 7;
      relojSeq = millis() - tiempoPaso;
      Serial.println("--- SECUENCIADOR: RESTART ---");
    }
  }
  comboStopRestartAnt = comboStopRestartAct;

  // COMBO RESET (BOTONES 1 Y 5)
  bool comboResetAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[4]) == LOW); // LECTURA ACTUAL DEL COMBO RESET
  if (comboResetAct == true && comboResetAnt == false)                                           // EVALUO FLANCO DE SUBIDA
  {
    digitalWrite(pinLeds[pasoActual], LOW); // APAGO EL PASO ACTUAL
    noTone(pinBuzzer);                      // APAGO EL SONIDO

    pasoActual = 7;                   // REINICIO EL SECUENCIADOR
    relojSeq = millis() - tiempoPaso; // REINICIO EL RELOJ

    ignorarLiberacion[0] = true; // IGNORO LA LIBERACION
    ignorarLiberacion[4] = true;

    Serial.println("--- RESET ---");
  }
  comboResetAnt = comboResetAct;

  //  COMBO RANDOMIZACION (BOTONES 1 Y 3)
  bool comboRandomAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[2]) == LOW); // LECTURA ACTUAL DEL COMBO
  if (comboRandomAct == true && comboRandomAnt == false)                                          // EVALUO FLANCO DE SUBIDA
  {
    randomSeed(millis());       // RESETEO LA SEMILLA RANDOM
    for (int i = 0; i < 8; i++) // ITERO SOBRE CADA PASO PARA RANDOMIZAR ESTADO Y NOTA
    {
      estadoPasos[i] = random(2);
      notasPlay[i] = random(36);
    }
    ignorarLiberacion[0] = true;
    ignorarLiberacion[2] = true;
    Serial.println("--- ¡¡¡ RANDOM !!! ---");
  }
  comboRandomAnt = comboRandomAct;

  // ------------------------------------------------------------------
  // LECTURAS BPM Y ESTADOS DE BOTONES
  // ------------------------------------------------------------------

  // VERIFICO SI EL BPM ESTA BLOQUEADO POR RETENCION DE BOTON PARA CONTROLAR EL ACOPLAMIENTO MECANICO
  if (bpmBloqueado == true)
  {
    int poteObjetivo = map(BPM, 80, 220, 0, 1023); // MAPEO EL VALOR DE BPM ACTUAL A UN VALOR OBJETIVO DEL POTENCIOMETRO
    int lecturaActual = analogRead(pinPot);

    if (abs(lecturaActual - poteObjetivo) < 15) // EVALUO SI LA LECTURA ACTUAL ESTA DENTRO DEL RANGO DE TOLERANCIA
    {
      bpmBloqueado = false;         // HABILITO EL CONTROL DE BPM AL DETECTAR EL PUNTO DE ENGANCHE
      poteAnterior = lecturaActual; // ACTUALIZO LA MEMORIA DEL POTENCIOMETRO
      Serial.println("--- BPM ENGANCHADO ---");
    }
  }

  // CALCULO DE BPM: SOLO SE EJECUTA SI NO HAY BOTONES RETENIDOS Y EL POTE ESTA ACOPLADO
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

    // ESTADO 1: CALCULO BAJADA
    if (estadoActual == LOW && estadoAnterior[i] == HIGH) // DETECTO EL FLANCO DE BAJADA DE CADA BOTON PARA INICIAR EL PROCESO DE RETENCION Y AFINACION
    {
      tiempoRebote[i] = millis();
      botonesPresionados++; // INCREMENTO EL CONTADOR DE BOTONES RETENIDOS EN CADA BAJADA
      lecturaPoteNotaAnterior = -1;
    }

    // ESTADO 2: RETENCION PARA AFINAR NOTA
    if (estadoActual == LOW && estadoAnterior[i] == LOW && millis() - tiempoRebote[i] > 300) // SI EL TIEMPO DE RETENCION SUPERA LOS 300MS, ENTRAMOS EN MODO NOTA PARA ESE PASO
    {
      if (ignorarLiberacion[i] == false) // SI NO IGNORA LA LIBERACION, PERMITE AFINAR LA NOTA
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

        digitalWrite(pinLeds[i], (millis() / 50) % 2); // FEEDBACK VISUAL

        // REPRODUZCO LA NOTA EN VIVO DURANTE LA AFINACION
        tone(pinBuzzer, escala[indiceElegido]);
      }
    }

    // ESTADO 3: LIBERACION DEL BOTON
    if (estadoActual == HIGH && estadoAnterior[i] == LOW)
    {
      // SILENCIO EL BUZZER SI SE SOLTO EL BOTON Y EL SECUENCIADOR ESTA PAUSADO O FRENADO
      if (enEjecucion == false)
      {
        noTone(pinBuzzer);
      }

      // TOQUE CORTO: INVERSION DE PASO
      if (millis() - tiempoRebote[i] > 50 && millis() - tiempoRebote[i] < 300)
      {
        if (ignorarLiberacion[i] == false)
        {
          estadoPasos[i] = !estadoPasos[i];
        }
      }
      // PULSACION LARGA, INGRESO EN MODO DE CONTROL DE BPM
      else if (millis() - tiempoRebote[i] >= 300)
      {
        if (ignorarLiberacion[i] == false)
        {
          bpmBloqueado = true;
          Serial.println("--- BPM CONGELADO ---");
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
      digitalWrite(pinLeds[pasoActual], LOW); // APAGO EL LED DEL PASO ANTERIOR QUE ESTA GUARDADO EN LA VARIABLE pasoActual
      noTone(pinBuzzer);                      // APAGO EL SONIDO ANTERIOR
      pasoActual++;                           // AVANZO AL SIGUIENTE PASO
      relojSeq = millis();                    // REINICIO EL RELOJ PARA EL PROXIMO PASO

      if (pasoActual == 8) // SI EL PASO ACTUAL LLEGO A 8, REINICIO EL SECUENCIADOR AL PASO 0 Y MUESTRO EN PANTALLA EL ESTADO DE LOS PASOS Y EL BPM
      {
        pasoActual = 0;

        Serial.print("--- BPM: ");
        Serial.print(BPM);
        Serial.print(" | Set: ");
        for (int x = 0; x < 8; x++)
        {
          if (estadoPasos[x] == true)
          {
            Serial.print(nombresNotas[notasPlay[x]]);
          }
          else
          {
            Serial.print("X");
            Serial.print(nombresNotas[notasPlay[x]]);
          }
          if (x < 7)
            Serial.print("-");
        }
        Serial.println(" ---");
      }

      if (estadoPasos[pasoActual] == true) // SI EL PASO ACTUAL ESTA ACTIVADO, LO ENCIENDO Y REPRODUZCO LA NOTA CORRESPONDIENTE
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