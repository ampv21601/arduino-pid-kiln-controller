# Arduino Thermal Ramp Controller

Sistema de control térmico automatizado basado en Arduino para la gestión de perfiles de temperatura mediante rampas y control PID.

El proyecto fue desarrollado para controlar un horno eléctrico utilizando un termopar tipo K, un módulo MAX31855 y un relé SSR, permitiendo ejecutar ciclos térmicos complejos de forma automática.

---

# Características

- Control automático de temperatura mediante PID.
- Gestión de perfiles térmicos por pasos.
- Rampas de calentamiento configurables.
- Mantenimiento de temperatura durante tiempos definidos.
- Protección contra sobretemperatura.
- Lectura de temperatura con termopar tipo K.
- Interfaz visual mediante pantalla LCD I2C 20x4.
- Control de potencia mediante SSR (Solid State Relay).
- Sistema configurable modificando el perfil térmico en el código.

---

# Hardware utilizado

## Componentes principales

- Arduino UNO / Arduino Nano
- Módulo MAX31855
- Termopar tipo K
- Pantalla LCD I2C 20x4
- SSR (Solid State Relay)
- Resistencia calefactora / horno eléctrico

---

# Esquema general de funcionamiento

El sistema funciona de la siguiente manera:

1. El termopar tipo K mide la temperatura del horno.
2. El módulo MAX31855 convierte la señal del termopar en datos digitales.
3. El Arduino lee la temperatura.
4. El controlador PID calcula cuánta potencia aplicar.
5. El SSR activa o desactiva la resistencia calefactora.
6. La pantalla LCD muestra:
   - temperatura actual,
   - temperatura objetivo,
   - paso actual,
   - tiempo restante.

---

# Conexiones utilizadas

## MAX31855 → Arduino

| MAX31855 | Arduino |
|---|---|
| VCC | 5V |
| GND | GND |
| DO | Pin 12 |
| CS | Pin 10 |
| CLK | Pin 13 |

---

## LCD I2C → Arduino

| LCD I2C | Arduino |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

*(En Arduino UNO/Nano)*

---

## SSR → Arduino

| SSR | Arduino |
|---|---|
| + | Pin 8 |
| - | GND |

El SSR controla la alimentación de la resistencia calefactora del horno.

⚠️ Importante:
La parte de potencia trabaja con tensión peligrosa. Debe realizarse con aislamiento y medidas de seguridad adecuadas.

---

# Librerías utilizadas

```cpp
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>
```

Librerías necesarias:

- Adafruit MAX31855
- LiquidCrystal_I2C
- PID_v1

---

# Control PID

El sistema utiliza un controlador PID para mantener la temperatura de forma estable y evitar sobreoscilaciones.

Parámetros utilizados actualmente:

```cpp
const double Kp = 28;
const double Ki = 0.05;
const double Kd = 10;
```

La salida PID controla el tiempo de activación del SSR dentro de una ventana temporal.

---

# Perfiles térmicos

El sistema ejecuta perfiles térmicos definidos por pasos.

Cada paso contiene:

```cpp
struct Paso {
  float objetivo;
  unsigned long duracion;
  bool rampa;
};
```

- objetivo → temperatura objetivo
- duracion → duración en segundos
- rampa → indica si la transición es progresiva

---

# Ejemplo de perfil

```cpp
Paso perfil[] = {
  {150, 900, true},
  {155, 300, false},
  {155, 3600, false},
  {730, 9000, true},
  {740, 3600, false},
  {600, 2700, true},
  {600, 17100, false}
};
```

Este perfil:
- realiza calentamientos progresivos,
- mantiene temperaturas estables,
- y ejecuta fases de enfriamiento controlado.

---

# Protección de seguridad

El sistema incluye:

- validación de errores del termopar,
- apagado automático por sobretemperatura,
- desactivación del SSR si se supera el objetivo.

Ejemplo:

```cpp
if (temperaturaActual >= temperaturaObjetivo + 1.0)
```

---

# Información mostrada en pantalla

La LCD muestra:

- Temperatura actual
- Temperatura objetivo
- Paso actual del perfil
- Velocidad de rampa
- Tiempo restante
- Tiempo total restante

---

# Posibles mejoras futuras

- Interfaz web para monitorización.
- Registro de temperaturas en SD.
- Conexión WiFi/MQTT.
- Integración IoT.
- Control remoto desde móvil.
- Gráficas en tiempo real.
- Configuración de perfiles desde interfaz.

---

# Capturas e imágenes

## Montaje del sistema

(Añadir foto)

## Pantalla en funcionamiento

(Añadir foto)

---

# Autor

Antonio Manuel Pavón Villar

GitHub:
https://github.com/ampv21601

LinkedIn:
https://linkedin.com/in/ampv21601

---

# Licencia

Proyecto desarrollado con fines educativos y de experimentación.
