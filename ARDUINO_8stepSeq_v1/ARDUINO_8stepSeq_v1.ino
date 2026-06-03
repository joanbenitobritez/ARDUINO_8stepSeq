//DEFINO PINES Y VARIABLES
const int pinRndm = 1;
const int pinPot = A4;
const int pinBuzzer = A5;


const int pinBotones[8] = {2,3,4,5,6,7,8,9};
const int pinLeds[8] = {10,11,12,13,A0,A1,A2,A3};

//DEFINO ESTADO DE PASOS Y ESCALA DEFAULT
bool estadoPasos[8] = {true, true, true, true, true, true, true, true};
const int escala[8] = {261, 294, 329, 349, 392, 440, 493, 523}; //DEFINO UNA ESCALA FIJA Y ARBITRARIA DE UNA OCTAVA: C4 A C5
int escalaRndm[8] = {261, 294, 329, 349, 392, 440, 493, 523}; //DEFINO ESCALA PARA RANDOM

//DEFINO INICIO DE SECUENCIADOR
bool estadoAnterior[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long relojSeq = 0;
int pasoActual = 0;
bool estadoRndmAnt = HIGH;


//DEFINO COMPORTAMIENTO INICIAL
void setup() {
  //PARO  EL DISPOSITIVO PARA QUE NO TENGA UN FALSO INICIO
  delay (1000);
  //DEFINO PINES INDIVIDUALES
  pinMode(pinRndm, INPUT_PULLUP); 
  pinMode(pinBuzzer, OUTPUT);
  //DEFINO PINES DE ARRAYS
  for (int i=0; i<8; i++) {
    pinMode(pinBotones[i], INPUT_PULLUP);
    pinMode(pinLeds[i], OUTPUT);
  }
  //GENERO SEED PARA BOTON RANDOM, LEO VOLTAJE DE PIN PARA GENERAR ALEATOREIDAD
  randomSeed(analogRead(A1));
}

void loop() {
 // DEFINO LOOP
 //CALCULO DE BPM min 60000ms/40 BPM=1500ms | CALCULO DE BPM MAX 60000ms/240 BPM = 250ms
 int tiempoPaso = map(analogRead(pinPot),0,1023,1500,250);
 //LEO BOTONES DE PASOS
 for (int i=0; i<8; i++){
  bool estadoActual = digitalRead(pinBotones[i]);
  if (estadoActual == LOW && estadoAnterior[i] == HIGH){
    estadoPasos[i] = !estadoPasos[i];
      }
    estadoAnterior[i] = estadoActual;
 }
 //LEO BOTON RANDOM Y COMPARO CAMBIOS
 bool estadoRndmAct = digitalRead(pinRndm);
 if (estadoRndmAct == LOW && estadoRndmAnt == HIGH){
  estadoRndmAnt = estadoRndmAct;
  for (int i=0; i<8; i++){
    estadoPasos[i] = random(0, 2);

  }
 }

 //LOOP SEQ
 if (millis()-relojSeq >= tiempoPaso) {// VERIFICO SI EL TIEMPO TRANSCURRIDO (DELTA) SUPERA EL INTERVALO DEL BPM
    digitalWrite(pinLeds[pasoActual], LOW); //APAGO EL LED DEL PASO ANTERIOR
    noTone(pinBuzzer); //APAGO EL TONO DEL PASO ANTERIOR
    pasoActual++; //SI EL TIEMPO ES MAYOR, INCREMENTO EL PASO
    relojSeq=millis();


  if  (pasoActual == 8){ //VERIFICO EL PASO, SI COMPLETO LOS 8 PASOS RESETEA LA POSICION
    pasoActual = 0;
  }
  if(estadoPasos[pasoActual] == true){ //SI EL PASO ESTA HABILITADO ENCIENDO EL ACTUAL
      digitalWrite(pinLeds[pasoActual], HIGH);
      tone(pinBuzzer, escala[pasoActual]);
  }
 

  
  
  
 }


}
