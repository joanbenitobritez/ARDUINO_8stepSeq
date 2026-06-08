#include <Arduino.h>
// DEFINO PINES Y VARIABLES
const int pinPot = A4;
const int pinBuzzer = A5;

const int pinBotones[8] = {2, 3, 4, 5, 6, 7, 8, 9};
const int pinLeds[8] = {10, 11, 12, 13, A0, A1, A2, A3};

// DEFINO ESTADO DE PASOS Y ESCALA DEFAULT
bool estadoPasos[8] = {true, true, true, true, true, true, true, true};                                                                                                                                      // POR DEFAULT TODOS LOS PASOS ESTAN ACTIVADOS
const int escala[36] = {131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988}; // DEFINO UNA ESCALA FIJA Y ARBITRARIA DE UNA OCTAVA: C4 A C5
int notasPlay[8] = {261, 294, 329, 349, 392, 440, 493, 523};                                                                                                                                                 // DEFINO ESCALA PARA RANDOM

// DEFINO INICIO DE SECUENCIADOR Y ANTIRREBOTE
bool estadoAnterior[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; // INDICO QUE TODOS LOS BOTONES ESTAN SIN PULSAR (INPUT_PULLUP)
unsigned long relojSeq = 0;                                                // INICIO EL CONTADOR DEL RELOJ EN EL PASO 0
int pasoActual = 7;                                                        // LO INICIALIZO EN 7 PARA QUE EL CONTADOR LO RESETEE A CERO EN EL LOOP E INICIALICE EN EL PRIMER PASO
bool comboAnt = false;
bool potSecuestrado = false;                              // INICIO EL SWITCH DE COMBO(RANDOM AL PRESIONAR BOTONES 1 Y 3)
unsigned long tiempoRebote[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // CONTADOR EN 0 PARA ANTIRREBOTE
// DEFINO VARIABLES DE TIEMPO PARA CALCULO DE BPM Y PASOS
int tiempoNegra;
int BPM;
int tiempoPaso = 120;

// DEFINO COMPORTAMIENTO INICIAL
void setup()
{
  // INICIALIZO EL SERIAL
  Serial.begin(9600);
  // PARO  EL DISPOSITIVO PARA QUE NO TENGA UN FALSO INICIO
  delay(1000);
  // DEFINO PINES INDIVIDUALES
  pinMode(pinBuzzer, OUTPUT);
  // DEFINO PINES CON FOR EN ARRAYS
  for (int i = 0; i < 8; i++)
  {
    pinMode(pinBotones[i], INPUT_PULLUP);
    pinMode(pinLeds[i], OUTPUT);
  }
}
// LOOP GENERAL

void loop()
{
  // CALCULO SECUESTRO DEL POT
  if (potSecuestrado == false)
  { // VERIFICO SI EL POT NO ESTA SIENDO SECUESTRADO POR UN BOTON, SI NO LO ESTA, CALCULO EL BPM Y LOS PASOS EN CORCHEAS CON EL VALOR DEL POT
    // CALCULO DE BPM min 60000ms/80 BPM=750ms | CALCULO DE BPM MAX 60000ms/220 BPM = 272ms
    tiempoNegra = map(analogRead(pinPot), 0, 1023, 750, 272);
    BPM = 60000 / tiempoNegra;
    // CALCULO DE PASOS EN CORCHEAS
    tiempoPaso = tiempoNegra / 2;
  }

  for (int i = 0; i < 8; i++)
  {
    bool estadoActual = digitalRead(pinBotones[i]);

    // guardo el tiempo del ultimo cambio de estado para cada boton
    if (estadoActual == LOW && estadoAnterior[i] == HIGH)
    {
      tiempoRebote[i] = millis(); // GUARDO EL TIEMPO DEL ULTIMO CAMBIO DE ESTADO PARA CADA BOTON
    }
    if (estadoActual == LOW && estadoAnterior[i] == LOW && millis() - tiempoRebote[i] > 300)
    {                                                                 // VERIFICO SI EL BOTON ESTA SIENDO PRESIONADO Y SI EL TIEMPO TRANSCURRIDO DESDE EL ULTIMO CAMBIO DE ESTADO ES MAYOR A 200ms (TIEMPO DE REBOTE)
      notasPlay[i] = escala[map(analogRead(pinPot), 0, 1023, 0, 35)]; // SI EL BOTON ESTA SIENDO PRESIONADO Y EL TIEMPO DE REBOTE HA PASADO, MAPEO EL VALOR DEL POT A LA ESCALA Y LO ASIGNO AL PASO CORRESPONDIENTE
      potSecuestrado = true;                                          // INDICO QUE EL POT ESTA SIENDO SECUESTRADO POR UN BOTON
                                                                      // Escribe un 1 (HIGH) o un 0 (LOW) alternando cada 50 milisegundos
      digitalWrite(pinLeds[i], (millis() / 50) % 2);
    }
    // ESTADO 3 (LA LIBERACIÓN): Verifico si el boton fue soltado
    if (estadoActual == HIGH && estadoAnterior[i] == LOW)
    {
      // Verifico si fue un toque corto válido
      if (millis() - tiempoRebote[i] > 50 && millis() - tiempoRebote[i] < 300)
      {
        estadoPasos[i] = !estadoPasos[i]; // Invierto el paso
      }
      potSecuestrado = false; // Siempre libero el potenciómetro al soltar
      digitalWrite(pinLeds[i], LOW); //FUERZO APAGADO DE LED AL SOLTAR EL BOTON PARA EVITAR QUE QUEDE PRENDIDO POR EL SECUESTRO DEL POT
    }

    estadoAnterior[i] = estadoActual; // ACTUALIZO LA MEMORIA DEL BOTON
  } // <-- ESTA LLAVE CIERRA EL BUCLE FOR DE LOS BOTONES

  // LEO COMBO VIRTUAL Y COMPARO CAMBIOS

  // DEFINO LOOP

  // LEO COMBO VIRTUAL Y COMPARO CAMBIOS
  bool comboAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[2]) == LOW); // AND ENTRE BOTONES 1 y 3

  if (comboAct == true && comboAnt == false)
  {                       // COMPARO ESTADOS, SI HUBO CAMBIO ENTRA EN EL IF
    randomSeed(millis()); // // GENERO SEED PARA BOTON RANDOM, LEO millis() PARA QUE SEA UNA SEMILLA CAMBIANTE CADA VEZ QUE SE PRESIONA EL COMBO
    for (int i = 0; i < 8; i++)
    {

      estadoPasos[i] = random(2); // RANDOMIZA PASOS ACTIVADOS
      notasPlay[i] = escala[random(36)];
    }
  }
  comboAnt = comboAct;

  // LOOP SEQ

  // CORRO EL CONTADOR DE PASOS
  if (millis() - relojSeq >= tiempoPaso)
  {                                         // VERIFICO SI EL TIEMPO TRANSCURRIDO SUPERA EL INTERVALO DEL BPM
    digitalWrite(pinLeds[pasoActual], LOW); // APAGO EL LED DEL PASO ANTERIOR
    noTone(pinBuzzer);                      // APAGO EL TONO DEL PASO ANTERIOR
    pasoActual++;                           // SI EL TIEMPO TRANSCURRIDO ES MAYOR A LA DURACION DEL PASO, INCREMENTO EL PASO
    relojSeq = millis();                    // TOMO VALOR DE millis() PARA COMPARAR EL TIEMPO TRANSCURRIDO

    // ESCRIBO BPM EN EL SERIAL CON FORMATO DE TEXTO
    Serial.print("BPM: ");
    Serial.println(BPM);

    if (pasoActual == 8)
    { // VERIFICO EL PASO, SI COMPLETO LOS 8 PASOS RESETEA LA POSICION
      pasoActual = 0;
    }
    if (estadoPasos[pasoActual] == true)
    { // SI EL PASO ESTA HABILITADO ENCIENDO EL ACTUAL
      digitalWrite(pinLeds[pasoActual], HIGH);
      tone(pinBuzzer, notasPlay[pasoActual]);
    }
  }
}