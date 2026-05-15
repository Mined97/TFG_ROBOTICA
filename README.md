# TFG Robótica · Brazo 6 ejes + pinza

Proyecto didáctico para controlar un brazo robótico de 6 ejes y pinza con una arquitectura en dos placas:

```text
PC / navegador web -> WebSerial USB -> ESP32 -> Serial2/UART -> Arduino Uno -> servos del brazo
```

- La **interfaz web** es la interfaz visual del usuario.
- El **ESP32** es el controlador principal de la célula: entradas, salidas, esperas, pausas y secuencias.
- El **Arduino Uno** se dedica solo al motion control del brazo.

No se usa WiFi, Bluetooth, React, Vite, Node ni frameworks web.

## Estructura principal

- `web/index.html`: interfaz WebSerial completa.
- `esp32/esp32_controller/esp32_controller.ino`: firmware del ESP32 controlador.
- `arduino/brazo_motion_control/brazo_motion_control.ino`: firmware Arduino Uno para servos.
- `docs/`: arquitectura, protocolo y plan.
- `examples/programa_demo.json`: programa de ejemplo importable desde la interfaz.

## Datos reales del brazo

- Canales: `J1`, `J2`, `J3`, `J4`, `J5`, `J6` y `PINZA`.
- Pines Arduino J1..J7: `{2,4,11,6,8,10,5}`.
- J7 corresponde a la pinza.
- J1..J6: 0 a 180 grados.
- Pinza: 95 a 180 grados.
- Comunicación serie: 115200 baudios.

## Cómo abrir la interfaz

1. Abre `web/index.html` en un navegador compatible con WebSerial, por ejemplo Chrome o Edge.
2. Si el navegador bloquea WebSerial por abrir el archivo directamente, sirve la carpeta con un servidor estático simple o abre desde un origen local permitido.
3. Pulsa **Conectar** y selecciona el puerto USB del ESP32.
4. El monitor serie debe mostrar `<ESP32,READY>` al hacer `PING` o al reiniciar la placa.

## Cómo cargar Arduino Uno

1. Abre `arduino/brazo_motion_control/brazo_motion_control.ino` en Arduino IDE.
2. Selecciona placa **Arduino Uno** y su puerto USB.
3. Carga el sketch.
4. El Arduino mantiene `Servo.h`, los pines `{2,4,11,6,8,10,5}`, la pinza 95-180 y los comandos `<P>`, `<S>`, `<V>`, `<A>`, `<Q>` y `<H>`.

## Cómo cargar ESP32

1. Abre `esp32/esp32_controller/esp32_controller.ino` en Arduino IDE.
2. Selecciona tu placa ESP32.
3. Carga el sketch.
4. El ESP32 usa:
   - `Serial` USB a 115200 baudios para el PC.
   - `Serial2` a 115200 baudios hacia el Arduino.
   - TX2 = GPIO17; RX desactivado (`-1`) porque no se reciben tramas desde Arduino.

## Conexión ESP32 ↔ Arduino

- ESP32 GPIO17 / TX2 -> RX del Arduino Uno.
- GND común entre placas.

> **Arquitectura sin retorno:** no conectes TX del Arduino al ESP32 para esta versión. El Arduino solo recibe comandos de movimiento.

## Cómo probar conexión directa con Arduino

1. Conecta el Arduino Uno al PC por USB.
2. Abre un monitor serie a 115200 baudios.
3. Envía `<Q>`.
4. Debes recibir tramas como `<T,...>`, `<S,...>`, `<V,...>` y `<A,...>`.
5. Envía una pose segura, por ejemplo `<P,90,90,90,90,90,90,120>`.

## Cómo probar conexión por ESP32

1. Carga ambos firmwares.
2. Cablea solo TX ESP32 GPIO17 hacia RX Arduino y GND común.
3. Conecta el ESP32 al PC.
4. Abre `web/index.html` y pulsa **Conectar**.
5. Pulsa **Ping ESP32**.
6. En el monitor deben verse respuestas del ESP32. No deben aparecer trazas `<ESP32,RX_ARDUINO,...>`; si necesitas ver envíos al Arduino, activa `DEBUG_ARDUINO_TX` en el firmware ESP32.

## Cómo crear entradas y salidas

1. En **Configuración de entradas y salidas**, crea una entrada con nombre, GPIO, modo `INPUT` o `INPUT_PULLUP` y estado activo `HIGH` o `LOW`.
2. Crea una salida con nombre, GPIO y estado inicial.
3. Pulsa los botones de lectura o ON/OFF para probar manualmente.
4. La configuración se guarda en `localStorage` y se reenvía al ESP32 al cargar programas.

## Cómo crear una secuencia

1. Ajusta los sliders del brazo y guarda una o varias poses.
2. En **Editor de programa**, añade pasos:
   - `Pose` para enviar posiciones al Arduino.
   - `Activar salida` para controlar una salida del ESP32.
   - `Esperar entrada` para esperar un sensor activo/inactivo con timeout.
   - `Pausa` para retardos temporales.
   - `Velocidad` para enviar `<S>`, `<V>` y `<A>` al Arduino.
   - `Home` para enviar `<H>`.
   - `Condición de entrada` para leer un sensor y elegir un destino si está activa o inactiva.
   - `Condición lógica` para combinar una o dos entradas con opciones claras como “entrada A y entrada B están activas” o “entrada A está activa y entrada B no está activa”.
   - `Salto` para unir ramas o volver a un punto anterior del programa.
3. Si quieres ejecución continua, marca **Ejecutar en bucle** en el editor; el ESP32 reiniciará el programa desde el paso 1 al terminar.
4. Pulsa **Validar**.
5. Pulsa **Enviar al ESP32**.
6. Pulsa **Ejecutar**.
7. Puedes detener con **Detener**, que envía `<PROG,STOP>`.

También puedes importar `examples/programa_demo.json` desde el área JSON del editor.

## Comandos principales

El ESP32 reenvía al Arduino solo estos comandos de movimiento:

- `<P,j1,j2,j3,j4,j5,j6,pinza>`
- `<S,velocidad>`
- `<V,v1,v2,v3>`
- `<A,aceleracion>`
- `<H>`

Los comandos propios del ESP32 incluyen `<PING>`, `<STATUS>`, `<IN,...>`, `<OUT,...>` y `<PROG,...>`. Consulta `docs/protocolo-comunicacion.md` para el detalle completo.
