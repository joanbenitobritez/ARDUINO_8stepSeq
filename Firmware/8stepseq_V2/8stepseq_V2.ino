#include <Arduino.h>   // Librería base del microcontrolador. Contiene las definiciones de hardware.
#include <U8x8lib.h>   // Driver pre-ensamblado para controlar pantallas monocromáticas I2C.
#include <Wire.h>      // Controlador del protocolo de comunicación por hardware I2C (pines SDA y SCL).
#include <Bounce2.h>   // Filtro digital (antirrebote) para los switches mecánicos.
#include <Encoder.h>   // Decodificador de cuadratura (fases A y B) para la perilla rotativa.
#include <MIDI.h>      // Estructura y empaquetador del protocolo de comunicación musical serie.

// =================================================================
// 1. INSTANCIAS DE HARDWARE Y PROTOCOLO
// =================================================================
// Creamos los "objetos" de software que manejarán el hardware conectado.
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE); // Instancia la pantalla OLED asignando el driver SH1106.
Encoder miEncoder(2, 3);                              // Instancia el encoder leyendo las interrupciones en los pines 2 y 3.
Bounce botones[8];                                    // Crea un array (vector) de 8 filtros antirrebote, uno para cada botón de paso.
Bounce btnEncSW = Bounce();                           // Instancia el filtro para el botón central del encoder (Switch).
Bounce btnStepUp = Bounce();                          // Instancia el filtro para el botón '+' (Avance).
MIDI_CREATE_DEFAULT_INSTANCE();                       // Abre el puerto Serial de hardware para enviar los datos MIDI.

// =================================================================
// 2. CONFIGURACIÓN TÉCNICA Y ASIGNACIÓN DE PINES
// =================================================================
// Las constantes ('const') se guardan en la memoria ROM. Son valores fijos que no cambian en ejecución.
const int RESOLUCION_ENCODER = 4; // Escala matemática: 4 pulsos de hardware del encoder equivalen a 1 movimiento lógico.
const int FIGURA_MUSICAL = 4;     // Divisor de tempo. 4 representa notas Negras. Se deja como variable para futuras modificaciones (ej. 8 para corcheas).

// 'uint8_t' es un entero de 8 bits (1 byte, valor máximo 255). Se usa para ahorrar RAM al direccionar pines.
const uint8_t PIN_BOTONES[8] = {5, 6, 7, 8, 9, 10, 11, 12}; // Matriz que asocia cada paso (0 al 7) con su pin físico.
const uint8_t PIN_ENCODER_SW = 4; // Asignación del pin digital 4 al botón del encoder.
const uint8_t PIN_STEP_UP = A1;   // Asignación del pin analógico 1 (usado como digital) al botón '+'.
const uint8_t PIN_STEP_DN = A6;   // Asignación del pin analógico 6 al botón '-'. (Solo lectura analógica).
const uint8_t PIN_BUZZER = A0;    // Pin de salida PWM para el zumbador de audio local.
const uint8_t PIN_LED_G = A2;     // Pin de salida para el LED Verde (Estado Play).
const uint8_t PIN_LED_R = A3;     // Pin de salida para el LED Rojo (Estado Stop).

// =================================================================
// 3. VARIABLES DE ESTADO Y FLAGS
// =================================================================
// Variables globales guardadas en RAM dinámica. Mantienen el estado del sistema.
bool buzzerActivado = true; // Flip-flop maestro del audio local. 'true' (1) permite sonido, 'false' (0) lo silencia.

// ---- Variables Modo Teclado ----
bool modoTeclado = false; // Compuerta principal: 0 = Modo Secuenciador, 1 = Modo Teclado.
int tipoEscala = 0;       // Selector de matriz musical: 0 = Modo Mayor, 1 = Modo Menor.
int raizTeclado = 12;     // Nota base (raíz). 12 corresponde a C4 (Do central) en nuestro diccionario de notas.

// Arrays fijos (ROM) con las distancias en semitonos para formar las escalas musicales.
const int intervalosMayor[8] = {0, 2, 4, 5, 7, 9, 11, 12}; 
const int intervalosMenor[8] = {0, 2, 3, 5, 7, 8, 10, 12}; 
int notasSonandoTeclado[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // Memoria de qué nota MIDI disparó cada botón. -1 indica circuito abierto/silencio.

// ---- Variables Secuenciador ----
bool estadoPasos[8] = {true, true, true, true, true, true, true, true}; // Flip-flops de encendido/apagado para los 8 pasos de la secuencia.
int notasPlay[8] = {4, 7, 9, 11, 12, 14, 16, 14}; // Altura musical (índice) pre-programada de inicio para cada paso.

// Flags (banderas) de redibujado. Si están en 'true', el bus I2C transmite datos a la pantalla. Si están en 'false', descansa.
bool cambioPasos[8] = {true, true, true, true, true, true, true, true}; 
bool cambioTransporte = true; 
bool cambioBPM = true; 
bool cambioCabezal = true; 
bool refrescarTextosNotas = true; 

bool encoderUsadoEnPaso[8] = {false}; // Memoria temporal para evitar que un paso se apague accidentalmente tras afinarlo con la perilla.

int notaMostradaEdit = -1; // Almacena qué nota mostrar en la sección "EDIT" de la pantalla. -1 dibuja guiones "--".
int pasoEditadoActual = -1; // Almacena qué número de paso se está girando. -1 oculta el texto.
int notaMostradaPlay = -1; // Almacena qué nota está sonando en tiempo real en la sección "PLAY".

// Diccionarios de frecuencias (Hz) y etiquetas de texto. Estructuras fijas en memoria para traducción visual y sonora.
const int escala[36] = {131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988};
const char *nombresCortos[12] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};
const char *nombresNotas[36] = {"C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"};

// =================================================================
// 4. MOTOR DE TIEMPO
// =================================================================
uint8_t estadoTransporte = 0; // Máquina de estados: 0 = STOP, 1 = PLAY, 2 = PAUSA.
int BPM = 165; // Velocidad base del reloj.
unsigned long tiempoPaso = 181; // 'unsigned long' evita el desbordamiento aritmético al contar milisegundos.
unsigned long relojSeq = 0; // Acumulador de tiempo que actúa como el cristal oscilador del secuenciador.
int pasoActual = 0; // Puntero (cabezal) que indica qué posición del array se está leyendo.
int pasoAnterior = 0; // Puntero de memoria para saber qué paso previo hay que apagar visualmente.

int notaMidiActual = -1; // Registro de la última nota MIDI encendida. Indispensable para saber cuál apagar con NoteOff.
unsigned long tiempoInicioNota = 0; // Marca temporal de inicio de nota para controlar la duración del Gate.

unsigned long tiempoUltimoClic = 0; // Evita el múltiple disparo accidental si presionás el botón central muy lento.
long posicionEncoderAnt = 0; // Referencia de posición del motor paso a paso del encoder para calcular deltas de movimiento.

bool estadoAntStepDn = HIGH; // Memoria del flanco del botón '-'. HIGH porque la lógica es Pull-Up (Normalmente alto).
unsigned long ultimoTiempoBotonMenos = 0; // Temporizador para limitar la frecuencia de lectura del conversor ADC.

void setup() {
  delay(200); // Retardo térmico/eléctrico para estabilización de capacitores de la pantalla.

  MIDI.begin(1); // Abre el puerto UART en baud rate 31250 y escucha/transmite en el Canal MIDI 1.
  MIDI.turnThruOff(); // Corta el "eco" MIDI. Evita procesar datos entrantes innecesarios.

  Wire.begin(); // Inicializa el hardware del protocolo I2C.
  
  u8x8.begin(); // Envía la trama de inicialización a la pantalla OLED.
  u8x8.setPowerSave(0); // Comando I2C para encender la matriz de píxeles (0 = Desactivar ahorro de energía).
  u8x8.setFont(u8x8_font_chroma48medium8_r); // Carga el mapa de bits de esta fuente en la memoria de la pantalla.
  u8x8.clearDisplay(); // Borra toda la basura (estática) residual en la memoria RAM de la pantalla al arrancar.
  
  pinMode(PIN_BUZZER, OUTPUT); // Configura el silicio del pin A0 como salida de voltaje (emisor).
  pinMode(PIN_LED_R, OUTPUT);  
  pinMode(PIN_LED_G, OUTPUT);  

  // Bucle que configura masivamente los 8 pines de la botonera como entradas (receptores) con resistencia pull-up interna activa (5V por defecto).
  for (int i = 0; i < 8; i++) {
    botones[i].attach(PIN_BOTONES[i], INPUT_PULLUP);
    botones[i].interval(10); // Establece la ventana del filtro RC de software en 10 milisegundos.
  }
  
  btnEncSW.attach(PIN_ENCODER_SW, INPUT_PULLUP); // Conecta la lógica del objeto Bounce al pin físico.
  btnEncSW.interval(10); 
  btnStepUp.attach(PIN_STEP_UP, INPUT_PULLUP); 
  btnStepUp.interval(10);
  
  // Fórmula inicial de BPM a milisegundos. 60000ms (1 min) dividido los BPM, dividido la figura.
  tiempoPaso = 60000UL / BPM / FIGURA_MUSICAL; 
}

void loop() {
  // ===============================================================
  // A. ESCANEO DE ENTRADAS (Lectura de Hardware en cada ciclo de reloj)
  // ===============================================================
  for (int i = 0; i < 8; i++) botones[i].update(); // Escanea el estado eléctrico actual de los 8 pines de la matriz.
  btnEncSW.update(); // Muestrea el pin del encoder central.
  btnStepUp.update();  // Muestrea el pin del botón '+'.

  bool stepDnPresionado = false; // Variable local. Se resetea en cada vuelta del loop para capturar solo pulsos, no estados sostenidos.
  if (millis() - ultimoTiempoBotonMenos >= 10) { // Compuerta temporal: lee el pin analógico solo cada 10ms para no saturar al ADC.
    // Condicional Ternario: Si el voltaje ADC es menor a 500 (caída de tensión por presionar el switch), lo considera LOW; de lo contrario, HIGH.
    bool estadoActualStepDn = (analogRead(PIN_STEP_DN) < 500) ? LOW : HIGH; 
    
    if (estadoActualStepDn != estadoAntStepDn) { // Detector de cambio de estado (Flanco).
      if (estadoActualStepDn == LOW) stepDnPresionado = true; // Si el cambio fue hacia abajo (se presionó), activa el pulso lógico.
      estadoAntStepDn = estadoActualStepDn; // Actualiza la memoria para el próximo ciclo.
    }
    ultimoTiempoBotonMenos = millis(); // Resetea el cronómetro del ADC.
  }

  // Variables booleanas para evaluar si los botones están mantenidos apretados estáticamente en este instante.
  bool upActivo = (btnStepUp.read() == LOW); 
  bool dnActivo = (analogRead(PIN_STEP_DN) < 500);

  // ===============================================================
  // B. INTERCEPTORES DE SISTEMA (Prioridad Absoluta)
  // ===============================================================
  // Compuerta lógica AND/OR: Verifica si se presionó '+' mientras se mantenía '-', O viceversa.
  if ((btnStepUp.fell() && dnActivo) || (stepDnPresionado && upActivo)) {
    modoTeclado = !modoTeclado; // Invierte el estado del modo. (Si era 0, pasa a 1).
    u8x8.clearDisplay(); // Borra la pantalla completa para dar lugar a la nueva interfaz gráfica.
    
    // Enciende todas las banderas para forzar un redibujado total de la nueva vista en el próximo renderizado.
    cambioTransporte = true; 
    cambioBPM = true;
    cambioCabezal = true; 
    refrescarTextosNotas = true; 
    for(int i=0; i<8; i++) cambioPasos[i] = true;
    
    noTone(PIN_BUZZER); // Apaga cualquier señal cuadrada que el hardware estuviera emitiendo en A0.
    for (int i=0; i<127; i++) MIDI.sendNoteOff(i, 0, 1); // Barrido de seguridad: apaga todas las frecuencias MIDI posibles por si la polifonía quedó trabada.
    
    stepDnPresionado = false; // Limpia el pulso para evitar que el sistema interprete un comando residual.
    return; // Aborta la ejecución de todo lo que sigue abajo y vuelve al inicio del loop() para una lectura limpia.
  }

  // ===============================================================
  // C. GATE MIDI (Control de duración de nota Asíncrono)
  // ===============================================================
  if (!modoTeclado && notaMidiActual != -1) { // Solo opera si estamos en secuenciador y hay una nota registrada en memoria activa.
    // Compara el reloj actual con el momento de disparo. Si pasó el 50% de la duración del paso, corta el sonido (Staccato).
    if (millis() - tiempoInicioNota >= (tiempoPaso / 2)) {
      MIDI.sendNoteOff(notaMidiActual, 0, 1); // Comando serial para silenciar el sintetizador externo.
      notaMidiActual = -1; // Libera la memoria indicando que el canal está limpio.
      noTone(PIN_BUZZER); // Silencia la síntesis de audio local.
    }
  }

  // ===============================================================
  // D. LÓGICA DIVIDIDA (Multiplexor Lógico de Modos)
  // ===============================================================
  if (!modoTeclado) { // RUTA A: SECUENCIADOR ------------------------
    
    // Control '+' manual
    if (btnStepUp.fell() && !dnActivo) { // Si solo se presionó el '+'.
      buzzerActivado = true; // Habilita la variable local de audio.
      if (estadoTransporte != 1) { // Si NO está en PLAY (está en Stop o Pausa), permite el avance manual del paso.
        pasoActual = (pasoActual + 1) % 8; // Avanza el cabezal. El '% 8' asegura que si llega a 8, vuelva a 0 (Bucle).
        if (estadoPasos[pasoActual]) { // Si ese paso está activado por el usuario:
          if (buzzerActivado) tone(PIN_BUZZER, escala[notasPlay[pasoActual]], 100); // Suena nota breve local.
          notaMostradaPlay = notasPlay[pasoActual]; // Actualiza el texto de la pantalla.
        }
        cambioCabezal = true; // Solicita mover la flecha visual en pantalla.
      }
      refrescarTextosNotas = true;
    }

    // Control '-' manual
    if (stepDnPresionado && !upActivo) { // Si solo se presionó el '-'.
      buzzerActivado = false; // Apaga (Mutea) el audio local.
      noTone(PIN_BUZZER); // Aplica el silencio instantáneo al pin.
      if (estadoTransporte != 1) { 
        pasoActual = (pasoActual - 1 + 8) % 8; // Retrocede un paso matemáticamente seguro.
        if (estadoPasos[pasoActual]) {
          notaMostradaPlay = notasPlay[pasoActual];
        }
        cambioCabezal = true;
      }
      refrescarTextosNotas = true;
    }

    // Control Transporte (Botón del Encoder)
    if (btnEncSW.fell()) { 
      if (millis() - tiempoUltimoClic < 300) { // Lógica de Doble Clic (Stop Total). Si pasaron menos de 300ms desde la última pulsación.
        estadoTransporte = 0; // Cambia estado a STOP.
        pasoActual = 0;       // Resetea el cabezal al inicio.
        cambioCabezal = true; 
        notaMostradaPlay = -1; // Limpia textos.
        refrescarTextosNotas = true;
        noTone(PIN_BUZZER);
        if (notaMidiActual != -1) {  // Silencia sintetizador.
          MIDI.sendNoteOff(notaMidiActual, 0, 1);
          notaMidiActual = -1;
        }
      } else { // Clic Simple (Alternar Play/Pausa).
        estadoTransporte = (estadoTransporte == 1) ? 2 : 1; // Ternario: Si está en Play (1), pasa a Pausa (2). Si no, pasa a Play (1).
        if (estadoTransporte == 2) { // Si entró en Pausa:
          noTone(PIN_BUZZER);
          if (notaMidiActual != -1) { 
            MIDI.sendNoteOff(notaMidiActual, 0, 1); // Apaga la nota colgada externamente para evitar saturación de mezcla.
            notaMidiActual = -1;
          }
        }
      }
      tiempoUltimoClic = millis(); // Guarda el reloj para el cálculo del próximo doble clic.
      cambioTransporte = true; // Llama a redibujar el texto SEQ PLAY/PAUS/STOP.
    }

    // Motor de Secuencia Continua (Solo corre en PLAY).
    if (estadoTransporte == 1 && (millis() - relojSeq >= tiempoPaso)) { // Si es PLAY y se cumplió el tiempo calculado en base al BPM.
      pasoActual = (pasoActual + 1) % 8; // Avanza el cabezal.
      relojSeq = millis(); // Resetea el cronómetro del secuenciador.
      
      if (notaMidiActual != -1) { // Corta la nota anterior si quedó prendida por error.
        MIDI.sendNoteOff(notaMidiActual, 0, 1); 
        notaMidiActual = -1;
        noTone(PIN_BUZZER);
      }
      
      if (estadoPasos[pasoActual]) { // Verifica si el usuario prendió (habilitó) este casillero de la secuencia.
        if (buzzerActivado) tone(PIN_BUZZER, escala[notasPlay[pasoActual]]); // Extrae la frecuencia Hz de la tabla ROM y activa pin PWM.
        
        notaMidiActual = 48 + notasPlay[pasoActual]; // Offset matemático para alinear la matriz local con la tabla de protocolo MIDI universal (48 = C3).
        MIDI.sendNoteOn(notaMidiActual, 127, 1); // Transmite el pulso (Nota, Velocity Máxima, Canal 1).
        tiempoInicioNota = millis(); // Guarda el milisegundo de inicio para que el Gate la corte luego.
        
        notaMostradaPlay = notasPlay[pasoActual]; // Actualiza texto en pantalla.
        refrescarTextosNotas = true;
      } 
      
      cambioPasos[pasoActual] = true; // Avisa que el estado gráfico de ese cuadradito debe actualizarse.
      cambioCabezal = true; // Avisa que la flecha se movió.
    }

  } else { // RUTA B: TECLADO MANUAL ------------------------
    if (btnStepUp.fell() && !dnActivo) { // El botón '+' selecciona la escala Mayor.
      tipoEscala = 0; 
      refrescarTextosNotas = true;
    }
    if (stepDnPresionado && !upActivo) { // El botón '-' selecciona la escala Menor.
      tipoEscala = 1; 
      refrescarTextosNotas = true;
    }
  }

  // ===============================================================
  // ENRUTAMIENTO DEL ENCODER PRINCIPAL (Afinación y BPM)
  // ===============================================================
  long posHw = miEncoder.read() / RESOLUCION_ENCODER; // Lee el motor paso a paso y lo divide por 4 para generar 1 salto matemático entero por cada 'clic' táctil.
  int delta = posHw - posicionEncoderAnt; // Calcula la dirección. Positivo si giró a la derecha, Negativo a la izquierda, 0 si no se movió.
  
  if (delta != 0) { // Si hubo rotación comprobada:
    posicionEncoderAnt = posHw; // Memoriza la nueva posición para calcular el próximo giro.
    
    if (!modoTeclado) { // Si estamos en el secuenciador:
      bool afinandoPaso = false;
      for (int i = 0; i < 8; i++) { // Revisa cada botón de la botonera.
        if (botones[i].read() == LOW) { // Si hay un botón apretado en este instante, el encoder afina esa nota, no el BPM.
          notasPlay[i] += delta; // Suma o resta un semitono.
          if (notasPlay[i] < 0) notasPlay[i] = 0; // Traba matemática para no desbordar el array por debajo.
          if (notasPlay[i] > 35) notasPlay[i] = 35; // Traba matemática para no desbordar por encima.
          
          encoderUsadoEnPaso[i] = true; // Setea una bandera para avisar que este botón se usó como tecla 'Shift'.
          afinandoPaso = true;
          cambioPasos[i] = true;
          
          notaMostradaEdit = notasPlay[i]; // Guarda la nota para mostrar en pantalla la vista 'EDIT'.
          pasoEditadoActual = i + 1; // +1 porque para el humano los pasos son 1-8, no 0-7.
          refrescarTextosNotas = true;
          
          if (estadoTransporte != 1 && buzzerActivado) tone(PIN_BUZZER, escala[notasPlay[i]]); // Audición previa local si está en stop.
          break; // Rompe el bucle 'for' para afinar solo 1 nota a la vez.
        }
      }
      
      if (!afinandoPaso) { // Si no había botones presionados, el giro altera el BPM global.
        BPM += (delta * 2); // Salta de a 2 BPM para mayor rapidez.
        if (BPM < 60) BPM = 60; // Límite inferior.
        if (BPM > 240) BPM = 240; // Límite superior.
        tiempoPaso = 60000UL / BPM / FIGURA_MUSICAL; // Recalcula los milisegundos de la fase en tiempo real.
        cambioBPM = true; 
      }
      
    } else { // Si estamos en Modo Teclado:
      raizTeclado += delta; // El encoder transpone la nota base para todo el teclado.
      if (raizTeclado < 0) raizTeclado = 0; 
      if (raizTeclado > 35) raizTeclado = 35; 
      refrescarTextosNotas = true;
    }
  }

  // ===============================================================
  // E. ESCANEO MATRICIAL (Acción de tocar pads)
  // ===============================================================
  for (int i = 0; i < 8; i++) { // Bucle de evaluación de las 8 entradas físicas.
    if (botones[i].fell()) { // FLANCO DE BAJADA (Presión inicial).
      encoderUsadoEnPaso[i] = false; // Limpia la bandera de "shift".
      
      if (modoTeclado) { // Comportamiento de instrumento en vivo:
        // Operador Ternario: Elige qué array leer (Mayor o Menor) según el 'tipoEscala', busca el intervalo de esa tecla [i] y lo suma a la Raíz.
        int intervalo = (tipoEscala == 0) ? intervalosMayor[i] : intervalosMenor[i];
        int indiceBuzzer = raizTeclado + intervalo;
        int notaMidi = 48 + indiceBuzzer; // Transposición al protocolo universal.
        
        notasSonandoTeclado[i] = notaMidi; // Guarda en memoria que este pad disparó esta nota exacta, para apagarla al soltar.
        MIDI.sendNoteOn(notaMidi, 127, 1); // Dispara el sintetizador vía hardware serial.
        
        if (indiceBuzzer >= 0 && indiceBuzzer <= 35) { // Filtro de seguridad RAM.
          if (buzzerActivado) tone(PIN_BUZZER, escala[indiceBuzzer]); // Toca el zumbador.
          notaMostradaPlay = indiceBuzzer; // Graba texto para pantalla.
        } else {
          notaMostradaPlay = -1;
        }
        refrescarTextosNotas = true;
      }
    }
    
    if (botones[i].rose()) {  // FLANCO DE SUBIDA (Pad soltado).
      if (!modoTeclado) { // En el secuenciador:
        if (!encoderUsadoEnPaso[i]) { // Si lo soltás rápido sin girar el encoder, significa que querés apagar o prender ese paso (Mute local del paso).
          estadoPasos[i] = !estadoPasos[i]; // Invierte el flip-flop lógico de encendido/apagado.
          cambioPasos[i] = true; // Avisa a la pantalla que dibuje/borre los corchetes "[]".
        } else {
          noTone(PIN_BUZZER); // Si se soltó después de afinar, solo apaga el sonido de preview.
        }
      } else { // En el teclado en vivo:
        if (notasSonandoTeclado[i] != -1) { // Revisa si hay una nota real vinculada a este botón que sigue sonando.
          MIDI.sendNoteOff(notasSonandoTeclado[i], 0, 1); // Ordena el apagado al NTS-1.
          noTone(PIN_BUZZER); // Apaga el zumbador.
          notasSonandoTeclado[i] = -1; // Libera el canal de memoria.
          notaMostradaPlay = -1;  // Resetea el texto en pantalla a "--".
          refrescarTextosNotas = true; 
        }
      }
    }
  }

  // ===============================================================
  // F. RENDER PANTALLA (Empaquetado y envíos por bus I2C)
  // ===============================================================
  if (!modoTeclado) { // DIBUJO DEL SECUENCIADOR
    
    if (cambioTransporte) { // Solo transmite la palabra si realmente hubo un cambio de estado.
      u8x8.setCursor(0,0); // Mueve el puntero de video a Columna 0, Fila 0.
      if(estadoTransporte==1) u8x8.print("SEQ PLAY");
      else if(estadoTransporte==2) u8x8.print("SEQ PAUS"); 
      else u8x8.print("SEQ STOP"); 
      cambioTransporte = false; // Apaga la bandera para no saturar el I2C.
    }
    
    if (cambioBPM) { 
      u8x8.setCursor(9,0);
      char bufBPM[16]; // 'char[]' crea una cadena de caracteres vacía (Buffer) de 16 bytes de tamaño.
      snprintf(bufBPM, sizeof(bufBPM), "BPM:%d  ", BPM); // La función de C empaqueta la palabra "BPM:", la variable de velocidad (%d) y espacios en blanco de limpieza, de forma blindada contra basura de memoria RAM.
      u8x8.print(bufBPM); // Dispara el paquete formateado limpio a la pantalla.
      cambioBPM = false;
    }
    
    for(int i=0; i<8; i++){ // Dibuja la fila de 8 corchetes e indicadores de nota.
      if(cambioPasos[i]){
        u8x8.setCursor(i*2, 2); // Calcula matemáticamente la posición X dejando espacio entre notas. Fila 2.
        u8x8.print(estadoPasos[i] ? "[]" : "  "); // Ternario: Dibuja "[]" si está ON, o dos espacios "  " si está OFF para borrar remanentes.
        
        u8x8.setCursor(i*2, 3); // Fila 3.
        if (estadoPasos[i]) {
          u8x8.print(nombresCortos[notasPlay[i] % 12]); // Busca el texto del acorde truncado al módulo 12.
        } else {
          u8x8.print("--"); // Guiones si el paso está deshabilitado.
        }
        cambioPasos[i] = false;
      }
    }

    if (cambioCabezal) { // Dibuja el movimiento de la flecha indicadora del paso actual.
      u8x8.setCursor(pasoAnterior * 2, 4);  // Va a la ubicación del paso viejo...
      u8x8.print("  ");                     // ...y lo borra con espacios vacíos.
      u8x8.setCursor(pasoActual * 2, 4);    // Va a la ubicación del paso nuevo...
      u8x8.print("^ ");                     // ...y dibuja la flecha.
      pasoAnterior = pasoActual;            // Actualiza la memoria de borrado para el próximo avance.
      cambioCabezal = false;
    }

    if (refrescarTextosNotas) { // Dibuja la sección inferior.
      u8x8.setCursor(0, 6);
      char bufEdit[24]; // Buffer sobredimensionado por seguridad ante desbordamientos RP2040.
      if (pasoEditadoActual != -1 && notaMostradaEdit != -1) {
        snprintf(bufEdit, sizeof(bufEdit), "EDIT P%d: %s   ", pasoEditadoActual, nombresNotas[notaMostradaEdit]);
      } else {
        snprintf(bufEdit, sizeof(bufEdit), "EDIT P-: --     ");
      }
      u8x8.print(bufEdit);

      u8x8.setCursor(0, 7);
      char bufPlay[24]; 
      if (notaMostradaPlay != -1) {
        snprintf(bufPlay, sizeof(bufPlay), "PLAY: %s        ", nombresNotas[notaMostradaPlay]); // %s inyecta un String (cadena de texto) del diccionario.
      } else {
        snprintf(bufPlay, sizeof(bufPlay), "PLAY: --        ");
      }
      u8x8.print(bufPlay);
      
      refrescarTextosNotas = false; // Apaga bandera de render.
    }
    
    // Asignación directa de voltajes altos/bajos a los pines físicos LED leyendo la máquina de estado.
    digitalWrite(PIN_LED_R, (estadoTransporte == 0) ? LOW : HIGH); 
    digitalWrite(PIN_LED_G, (estadoTransporte == 1) ? LOW : HIGH); 

  } else { // DIBUJO DEL MODO TECLADO
    if (refrescarTextosNotas) { // Este bloque unificado evita el parpadeo de pantalla y cuello de botella I2C.
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