//DEFINO PINES 
const int pinRndm = A1;
const int pinPot = A4;
const int pinBuzzer = A5;

const int pinBotones[8] = {2,3,4,5,6,7,8,9};
const int pinLeds[8] = {10,11,12,13,A0,A1,A2,A3};

//DEFINO ESTADO DE PASOS Y ESCALA DEFAULT
bool estadoPasos[8] = {true, true, true, true, true, true, true, true};
const int escala[8] = {261, 294, 329, 349, 392, 440, 493, 523};

//DEFINO COMPORTAMIENTO INICIAL
void setup() {
  delay (1000); //PARO  EL DISPOSITIVO PARA QUE NO TENGA UN FALSO INICIO
  

}

void loop() {
  // put your main code here, to run repeatedly:

}
