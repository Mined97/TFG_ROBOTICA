# Célula robótica didáctica pick and place

Repositorio para el TFG de Automatización y Robótica Industrial de una célula robótica didáctica tipo **pick and place**. El objetivo es separar claramente el control de proceso de la célula y el motion control del brazo.

## Arquitectura actual y objetivo

Arquitectura de pruebas directas del brazo:

```text
PC / navegador web -> WebSerial USB -> Arduino Uno -> servos del brazo
```

Arquitectura objetivo de célula:

```text
PC / navegador web -> WebSerial USB -> ESP32 -> UART/Serial2 -> Arduino Uno -> servos del brazo
```

- La interfaz `web/index.html` es la interfaz visual del usuario.
- El ESP32 es el controlador principal de la célula: entradas digitales, salidas digitales, pausas, esperas y secuencias.
- El Arduino Uno queda reservado únicamente al motion control del brazo.
- El ESP32 envía al Arduino solo comandos de movimiento compatibles con el protocolo base.

## Estructura del repositorio

```text
/
├── README.md
├── docs/
│   ├── arquitectura.md
│   ├── protocolo-comunicacion.md
│   └── plan-desarrollo.md
├── web/
│   └── index.html
├── arduino/
│   └── brazo_motion_control/
│       └── brazo_motion_control.ino
├── esp32/
│   └── esp32_controller/
│       └── esp32_controller.ino
└── examples/
    └── programa_demo.json
```

## Datos reales del brazo que se mantienen

- 7 canales: `J1`, `J2`, `J3`, `J4`, `J5`, `J6` y `PINZA`.
- Pines Arduino `J1..J7`: `{2,4,11,6,8,10,5}`.
- `J7` corresponde a la pinza.
- Límites de `J1..J6`: `0°` a `180°`.
- Límites de `PINZA`: `95°` a `180°`.
- Comunicación serie: `115200` baudios.

## Cómo abrir la interfaz

1. Usa Chrome o Edge con soporte WebSerial.
2. Abre `web/index.html` directamente o mediante un servidor estático simple.
3. Pulsa **Conectar**.
4. Selecciona el puerto del Arduino Uno para pruebas directas o el puerto del ESP32 para la arquitectura objetivo.

La interfaz conserva el control original del brazo: sliders, Home, lectura de estado, velocidades, aceleración, poses, reproducción de poses e importación/exportación JSON. Además añade configuración de E/S, editor de programa y monitor serie.

## Cómo cargar el firmware Arduino

1. Abre `arduino/brazo_motion_control/brazo_motion_control.ino` en Arduino IDE.
2. Selecciona placa **Arduino Uno**.
3. Selecciona el puerto USB del Arduino.
4. Compila y carga.
5. Conecta los servos respetando los pines `{2,4,11,6,8,10,5}`.

El firmware usa únicamente `Servo.h`.

## Cómo cargar el firmware ESP32

1. Instala el soporte ESP32 en Arduino IDE si aún no lo tienes.
2. Abre `esp32/esp32_controller/esp32_controller.ino`.
3. Selecciona tu placa ESP32.
4. Compila y carga.
5. Por defecto se usa `Serial` USB a `115200` baudios y `Serial2` a `115200` baudios con `RX2=GPIO16` y `TX2=GPIO17`.

No se usa WiFi ni Bluetooth en esta fase.

## Cómo probar primero Arduino directo

1. Carga el firmware Arduino.
2. Conecta el Arduino Uno al PC por USB.
3. Abre `web/index.html`.
4. Pulsa **Conectar** y selecciona el puerto del Arduino.
5. Pulsa **Leer estado** y comprueba respuestas `<T,...>`, `<S,...>`, `<V,...>` y `<A,...>`.
6. Prueba sliders, Home, guardar poses y reproducir poses.

## Cómo probar ESP32 como puente/controlador

1. Carga el firmware Arduino.
2. Carga el firmware ESP32.
3. Cablea ESP32 `TX2=GPIO17` hacia RX del Arduino y ESP32 `RX2=GPIO16` hacia TX del Arduino.
4. Une `GND` de ESP32 y Arduino.
5. Conecta el ESP32 al PC por USB.
6. Abre `web/index.html`, pulsa **Conectar** y selecciona el puerto del ESP32.
7. El monitor debe mostrar `<ESP32,READY>` y los mensajes de depuración `TX_ARDUINO` / `RX_ARDUINO`.

## Configurar entradas y salidas

En la zona **Configuración de entradas y salidas**:

- Entradas: define `ID`, nombre, GPIO, modo (`INPUT` o `INPUT_PULLUP`) y estado activo (`HIGH` o `LOW`). Puedes leer manualmente el estado de cada entrada.
- Salidas: define `ID`, nombre, GPIO y estado inicial. Puedes activar/desactivar manualmente cada salida.

Restricciones de GPIO en el ESP32:

- No usar GPIO `1` y `3`: Serial USB.
- No usar GPIO `16` y `17`: Serial2 hacia Arduino.
- No usar GPIO `6`, `7`, `8`, `9`, `10`, `11`: memoria flash.
- No usar GPIO `34`, `35`, `36`, `39` como salidas porque son solo entrada.

## Crear y ejecutar una secuencia

En **Editor de programa** puedes añadir pasos de tipo:

- `POSE`: pose desde sliders actuales o una pose guardada, con delay posterior.
- `OUT`: activar/desactivar una salida configurada.
- `WAIT_IN`: esperar entrada activa/inactiva con timeout.
- `PAUSE`: pausa en milisegundos.
- `SPEED`: velocidad global, `V1`, `V2`, `V3` y aceleración.
- `HOME`: enviar el brazo a Home.

Flujo recomendado:

1. Configura entradas y salidas.
2. Guarda poses del brazo o usa sliders actuales.
3. Añade pasos al programa.
4. Pulsa **Validar**.
5. Pulsa **Enviar al ESP32**.
6. Pulsa **Ejecutar**.
7. Usa **Detener** para enviar `<PROG,STOP>`.

## Advertencia de niveles lógicos Arduino Uno / ESP32

El Arduino Uno trabaja a **5 V** y el ESP32 a **3,3 V**. No conectes una salida de 5 V del Arduino a una entrada del ESP32 sin adaptación de nivel.

Recomendación:

- Usa divisor resistivo o conversor de nivel en `TX Arduino -> RX ESP32`.
- `TX ESP32 -> RX Arduino` suele ser reconocido como nivel alto, pero debe validarse en el montaje real.
- Une siempre las masas (`GND`).
