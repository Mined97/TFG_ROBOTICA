# Arquitectura del sistema

La célula se organiza en cuatro niveles conectados por serie:

```text
PC / navegador web -> WebSerial USB -> ESP32 -> Serial2/UART -> Arduino Uno -> servos del brazo
```

## Reparto de responsabilidades

- **Interfaz web (`web/index.html`)**: interfaz visual en HTML, CSS y JavaScript puro. Permite mover el brazo, guardar poses, configurar entradas/salidas, editar programas, enviarlos al ESP32 y monitorizar el bus serie.
- **ESP32 (`esp32/esp32_controller/esp32_controller.ino`)**: controlador principal de la célula. Atiende WebSerial por USB, gestiona hasta 16 entradas, hasta 16 salidas y hasta 100 pasos de programa. Reenvía al Arduino únicamente comandos de movimiento.
- **Arduino Uno (`arduino/brazo_motion_control/brazo_motion_control.ino`)**: controlador de movimiento. Mantiene `Servo.h`, los pines `{2,4,11,6,8,10,5}`, los límites de articulaciones y el protocolo del brazo.

## Comunicación

- PC a ESP32: `Serial` USB a **115200 baudios**.
- ESP32 a Arduino: `Serial2` a **115200 baudios**, con **RX2 GPIO16** y **TX2 GPIO17**.
- No se usa WiFi ni Bluetooth.

## Cableado serie recomendado

- ESP32 GPIO17 / TX2 -> RX del Arduino Uno.
- TX del Arduino Uno -> ESP32 GPIO16 / RX2.
- GND común entre ESP32 y Arduino.

> Advertencia: el Arduino Uno trabaja normalmente a 5 V y el ESP32 a 3,3 V. La señal TX del Arduino hacia RX del ESP32 debe adaptarse con divisor resistivo o conversor de nivel lógico.

## Control de entradas y salidas

Las entradas y salidas pertenecen al ESP32, no al Arduino. La interfaz envía la configuración al ESP32 mediante comandos `<IN,...>` y `<OUT,...>`. El firmware valida pines reservados: GPIO 1, 3, 6-11, 16 y 17 no se permiten; GPIO 34, 35, 36 y 39 no se permiten como salidas.
