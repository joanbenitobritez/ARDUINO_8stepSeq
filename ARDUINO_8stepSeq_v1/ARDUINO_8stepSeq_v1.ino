//DEFINO PINES Y VARIABLES
const int pinPot = A4;
const int pinBuzzer = A5;

const int pinBotones[8] = {2,3,4,5,6,7,8,9};
const int pinLeds[8] = {10,11,12,13,A0,A1,A2,A3};

//DEFINO ESTADO DE PASOS Y ESCALA DEFAULT
bool estadoPasos[8] = {true, true, true, true, true, true, true, true}; //POR DEFAULT TODOS LOS PASOS ESTAN ACTIVADOS
const int escala[8] = {261, 294, 329, 349, 392, 440, 493, 523}; //DEFINO UNA ESCALA FIJA Y ARBITRARIA DE UNA OCTAVA: C4 A C5
int notasPlay[8] = {261, 294, 329, 349, 392, 440, 493, 523}; //DEFINO ESCALA PARA RANDOM

//DEFINO INICIO DE SECUENCIADOR Y ANTIRREBOTE
bool estadoAnterior[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; //INDICO QUE TODOS LOS BOTONES ESTAN SIN PULSAR (INPUT_PULLUP)
unsigned long relojSeq = 0; //INICIO EL CONTADOR DEL RELOJ EN EL PASO 0
int pasoActual = 7; //LO INICIALIZO EN 7 PARA QUE EL CONTADOR LO RESETEE A CERO EN EL LOOP E INICIALICE EN EL PRIMER PASO
bool comboAnt = false;//INICIO EL SWITCH DE COMBO(RANDOM AL PRESIONAR BOTONES 1 Y 3)
unsigned long tiempoRebote[8] = {0,0,0,0,0,0,0,0};//CONTADOR EN 0 PARA ANTIRREBOTE

//DEFINO COMPORTAMIENTO INICIAL
void setup() {
  //INICIALIZO EL SERIAL
  Serial.begin(9600);
  //PARO  EL DISPOSITIVO PARA QUE NO TENGA UN FALSO INICIO
  delay (1000);
  //DEFINO PINES INDIVIDUALES
  pinMode(pinBuzzer, OUTPUT);
  //DEFINO PINES CON FOR EN ARRAYS
  for (int i=0; i<8; i++) {
    pinMode(pinBotones[i], INPUT_PULLUP);
    pinMode(pinLeds[i], OUTPUT);
  }
  
}


//LOOP GENERAL

void loop() {
 //DEFINO LOOP
 //CALCULO DE BPM min 60000ms/80 BPM=750ms | CALCULO DE BPM MAX 60000ms/220 BPM = 272ms
 int tiempoNegra = map(analogRead(pinPot),0,1023,750,272);
 int BPM = 60000/tiempoNegra;
 //CALCULO DE PASOS EN CORCHEAS
 int tiempoPaso = tiempoNegra/2;
 
 //LEO BOTONES DE PASOS Y DETECTO ANTIRREBOTE
 for (int i=0; i<8; i++){
  bool estadoActual = digitalRead(pinBotones[i]);
  
  if (estadoActual == LOW && estadoAnterior[i] == HIGH && (millis() - tiempoRebote[i] > 50)){
    estadoPasos[i] = !estadoPasos[i];
    tiempoRebote[i] = millis(); // Sello el tiempo exacto del impacto válido
  }
  
  estadoAnterior[i] = estadoActual;
 }
 
 //LEO COMBO VIRTUAL Y COMPARO CAMBIOS
 bool comboAct = (digitalRead(pinBotones[0]) == LOW && digitalRead(pinBotones[2]) == LOW); //AND ENTRE BOTONES 1 y 3
 if (comboAct == true && comboAnt == false){ //COMPARO ESTADOS, SI HUBO CAMBIO ENTRA EN EL IF
     for (int i=0; i<8; i++){
      //GENERO SEED PARA BOTON RANDOM, LEO VOLTAJE DE PIN PARA GENERAR ALEATOREIDAD
      randomSeed(millis); //LEO EL VALOR DEL POTENCIOMETRO DE BPM PARA TENER UN MEJOR RANDOM
      estadoPasos[i] = random(2); //RANDOMIZA PASOS ACTIVADOS
      notasPlay[i] = escala[random(0, 8)];
    }
   }
 comboAnt = comboAct;

 //LOOP SEQ


 //CORRO EL CONTADOR DE PASOS
 if (millis()-relojSeq >= tiempoPaso) {// VERIFICO SI EL TIEMPO TRANSCURRIDO SUPERA EL INTERVALO DEL BPM
    digitalWrite(pinLeds[pasoActual], LOW); //APAGO EL LED DEL PASO ANTERIOR
    noTone(pinBuzzer); //APAGO EL TONO DEL PASO ANTERIOR
    pasoActual++; //SI EL TIEMPO TRANSCURRIDO ES MAYOR A LA DURACION DEL PASO, INCREMENTO EL PASO
    relojSeq=millis(); //TOMO VALOR DE millis() PARA COMPARAR EL TIEMPO TRANSCURRIDO
    
    //ESCRIBO BPM EN EL SERIAL CON FORMATO DE TEXTO
    Serial.print("BPM: ");
    Serial.println(BPM);

  if  (pasoActual == 8){ //VERIFICO EL PASO, SI COMPLETO LOS 8 PASOS RESETEA LA POSICION
    pasoActual = 0;
  }
  if(estadoPasos[pasoActual] == true){ //SI EL PASO ESTA HABILITADO ENCIENDO EL ACTUAL
      digitalWrite(pinLeds[pasoActual], HIGH);
      tone(pinBuzzer, notasPlay[pasoActual]);
  }
 }
}