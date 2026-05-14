/*
  ================================================================
  Control de Horno con Perfil Térmico + PID + SSR + LCD I2C
  Hardware:
    - Arduino
    - MAX31855 (termopar tipo K)
    - SSR (control resistencia calefactora)
    - LCD I2C 20x4
    - PID_v1 library

  Funcionalidades:
    - Control PID de temperatura
    - Perfil térmico por pasos (rampas y mantenimientos)
    - Protección por sobretemperatura
    - Control SSR por ventana de tiempo
    - Visualización en LCD
    - Monitor serial de depuración

  ================================================================
*/

#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>

/* ---------------- PINES ---------------- */
const int thermoDO  = 12;
const int thermoCS  = 10;
const int thermoCLK = 13;
const int ssrPin    = 8;

/* ---------------- SENSORES Y ACTUADORES ---------------- */
Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);
LiquidCrystal_I2C lcd(0x27, 20, 4);

/* ---------------- PID ---------------- */
double temperaturaActual = 0;
double temperaturaObjetivo = 0;
double salidaPID = 0;

// Ajuste actual del sistema
const double Kp = 28, Ki = 0.05, Kd = 10;

PID miPID(&temperaturaActual, &salidaPID, &temperaturaObjetivo, Kp, Ki, Kd, DIRECT);

/* ---------------- PERFIL TÉRMICO ---------------- */
struct Paso {
  float objetivo;            // temperatura objetivo del paso
  unsigned long duracion;    // duración en segundos
  bool rampa;                // true = rampa, false = mantenimiento
};

/*
  Perfil principal (cilindros grandes)
*/
Paso perfil[] = {
  {150, 900,  true},
  {155, 300,  false},
  {155, 3600, false},
  {730, 9000, true},
  {740, 3600, false},
  {600, 2700, true},
  {600, 17100,false}
};

const int totalPasos = sizeof(perfil) / sizeof(Paso);

/* ---------------- CONTROL DE ESTADO ---------------- */
int pasoActual = 0;
unsigned long tiempoInicioPaso = 0;
unsigned long ultimoIntervalo = 0;
float temperaturaInicialPaso = 0;
float incrementoTemperatura = 0;

/* ---------------- TIEMPOS ---------------- */
const unsigned long intervaloControl = 60UL * 1000 * 5; // actualización rampa (ms)
const unsigned long ventanaPID = 2000;                  // ventana SSR (ms)
unsigned long tiempoInicioVentana = 0;
unsigned long ultimaActualizacionLCD = 0;

/* ================================================================
   SETUP
   ================================================================ */
void setup() {
  Serial.begin(9600);

  lcd.begin(20, 4);
  lcd.backlight();

  pinMode(ssrPin, OUTPUT);
  digitalWrite(ssrPin, LOW);

  miPID.SetOutputLimits(0, ventanaPID);
  miPID.SetMode(AUTOMATIC);

  // Inicialización de variables del primer paso
  tiempoInicioPaso = millis();
  tiempoInicioVentana = millis();

  temperaturaInicialPaso = thermocouple.readCelsius();

  if (perfil[pasoActual].rampa) {
    incrementoTemperatura =
      (perfil[pasoActual].objetivo - temperaturaInicialPaso) /
      (perfil[pasoActual].duracion * 1000.0 / intervaloControl);

    ultimoIntervalo = millis();
    temperaturaObjetivo = temperaturaInicialPaso + incrementoTemperatura;
  } else {
    temperaturaObjetivo = perfil[pasoActual].objetivo;
  }
}

/* ================================================================
   LOOP PRINCIPAL
   ================================================================ */
void loop() {

  /* ---------------- LECTURA SENSOR ---------------- */
  double lectura = thermocouple.readCelsius();

  if (isnan(lectura)) {
    Serial.println("Error: termopar desconectado");
    return;
  }

  temperaturaActual = lectura;
  unsigned long ahora = millis();

  /* ---------------- CONTROL DE RAMPA ---------------- */
  if (perfil[pasoActual].rampa &&
      ahora - ultimoIntervalo >= intervaloControl) {

    ultimoIntervalo = ahora;
    temperaturaObjetivo += incrementoTemperatura;

    // límite final del paso
    if ((incrementoTemperatura > 0 && temperaturaObjetivo > perfil[pasoActual].objetivo) ||
        (incrementoTemperatura < 0 && temperaturaObjetivo < perfil[pasoActual].objetivo)) {
      temperaturaObjetivo = perfil[pasoActual].objetivo;
    }
  }

  /* ---------------- SEGURIDAD TÉRMICA ---------------- */
  if (temperaturaActual >= temperaturaObjetivo + 1.0) {
    salidaPID = 0;
    miPID.SetMode(MANUAL);
    digitalWrite(ssrPin, LOW);

    Serial.println("Protección activada: sobretemperatura");
  } else {
    if (miPID.GetMode() == MANUAL) {
      miPID.SetMode(AUTOMATIC);
    }
    miPID.Compute();
  }

  /* ---------------- CONTROL SSR (VENTANA) ---------------- */
  if (ahora - tiempoInicioVentana > ventanaPID) {
    tiempoInicioVentana += ventanaPID;
  }

  digitalWrite(
    ssrPin,
    (ahora - tiempoInicioVentana) < salidaPID ? HIGH : LOW
  );

  /* ---------------- CAMBIO DE PASO ---------------- */
  if (ahora - tiempoInicioPaso >= perfil[pasoActual].duracion * 1000) {

    pasoActual++;

    if (pasoActual < totalPasos) {
      tiempoInicioPaso = ahora;
      temperaturaInicialPaso = temperaturaActual;
      ultimoIntervalo = ahora;

      if (perfil[pasoActual].rampa) {
        incrementoTemperatura =
          (perfil[pasoActual].objetivo - temperaturaInicialPaso) /
          (perfil[pasoActual].duracion * 1000.0 / intervaloControl);

        temperaturaObjetivo = temperaturaInicialPaso + incrementoTemperatura;
      } else {
        temperaturaObjetivo = perfil[pasoActual].objetivo;
      }

    } else {
      digitalWrite(ssrPin, LOW);
      lcd.clear();
      lcd.print("Perfil terminado");

      while (true); // stop final
    }
  }

  /* ---------------- LCD (1s) ---------------- */
  if (ahora - ultimaActualizacionLCD >= 1000) {
    ultimaActualizacionLCD = ahora;

    unsigned long tiempoRestante =
      (tiempoInicioPaso + perfil[pasoActual].duracion * 1000 - ahora) / 1000;

    unsigned long tiempoRestanteTotal = tiempoRestante;

    for (int i = pasoActual + 1; i < totalPasos; i++) {
      tiempoRestanteTotal += perfil[i].duracion;
    }

    // ajuste manual final (reposo)
    if (tiempoRestanteTotal > 14400) {
      tiempoRestanteTotal -= 14400;
    } else {
      tiempoRestanteTotal = 0;
    }

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperaturaActual, 1);
    lcd.print("C O:");
    lcd.print(temperaturaObjetivo, 1);

    lcd.setCursor(0, 1);
    lcd.print("Paso ");
    lcd.print(pasoActual + 1);

    lcd.setCursor(0, 2);
    lcd.print("Vel ");
    lcd.print(incrementoTemperatura, 1);

    lcd.setCursor(0, 3);

    if (pasoActual == totalPasos - 1) {
      unsigned long t = (ahora - tiempoInicioPaso) / 1000;
      lcd.print("t ");
      lcd.print(t / 60);
      lcd.print("m ");
      lcd.print(t % 60);
      lcd.print("s");
    } else {
      lcd.print("R ");
      lcd.print(tiempoRestante / 60);
      lcd.print("m ");
      lcd.print(tiempoRestante % 60);
      lcd.print("s");
    }

    /* ---------------- DEBUG SERIAL ---------------- */
    Serial.print("Temp: ");
    Serial.print(temperaturaActual);
    Serial.print(" Obj: ");
    Serial.print(temperaturaObjetivo);
    Serial.print(" Out: ");
    Serial.println(salidaPID);
  }
}
