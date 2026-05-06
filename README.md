# Célula robótica didáctica pick and place

Repositorio base para el TFG de Automatización y Robótica Industrial de una célula robótica didáctica de tipo **pick and place**.

El sistema está pensado para evolucionar desde el control directo de un brazo robótico mediante WebSerial hasta una célula completa gobernada por un ESP32, con Arduino Uno dedicado exclusivamente al motion control del brazo.

## Contenido del repositorio

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

## Datos reales del brazo

- 6 ejes más pinza, 7 canales servo en total: `J1`, `J2`, `J3`, `J4`, `J5`, `J6` y `PINZA`.
- Pines `J1..J7`: `{2,4,11,6,8,10,5}`.
- `J7` corresponde a la pinza.
- Límites de `J1..J6`: `0°` a `180°`.
- Límites de `PINZA`: `95°` a `180°`.
- Comunicación serie: `115200` baudios.
- Protocolo base: tramas ASCII entre `<` y `>`.

## Arquitectura actual

```text
PC / navegador web -> WebSerial USB -> Arduino Uno -> servos del brazo
```

En esta arquitectura se conecta la interfaz directamente al Arduino Uno. Es el modo recomendado para probar primero el firmware del brazo.

## Arquitectura objetivo

```text
PC / navegador web -> WebSerial USB -> ESP32 -> UART/Serial -> Arduino Uno -> servos del brazo
```

En la arquitectura objetivo:

- La interfaz HTML sigue siendo la interfaz visual del usuario.
- El ESP32 actúa como controlador principal de la célula.
- El ESP32 gestionará entradas, salidas, esperas, pausas y secuencias.
- El Arduino Uno queda dedicado solamente al motion control del brazo.

## Cómo abrir la interfaz

1. Abre Chrome o Edge en un ordenador con soporte WebSerial.
2. Abre el archivo `web/index.html` directamente en el navegador, o publícalo con cualquier servidor estático simple.
3. Pulsa **Conectar**.
4. Selecciona el puerto serie del Arduino Uno o del ESP32, según el modo de prueba.

No se usa ningún framework ni dependencia externa: solo HTML, CSS y JavaScript puro.

## Cómo cargar el firmware Arduino

1. Abre Arduino IDE.
2. Abre `arduino/brazo_motion_control/brazo_motion_control.ino`.
3. Selecciona placa **Arduino Uno**.
4. Selecciona el puerto USB del Arduino.
5. Compila y carga.
6. Conecta los servos respetando exactamente los pines `{2,4,11,6,8,10,5}`.

El firmware usa solamente `Servo.h`, incluida en el entorno Arduino clásico.

## Cómo cargar el firmware ESP32

1. Instala el soporte de placas ESP32 en Arduino IDE si todavía no está instalado.
2. Abre `esp32/esp32_controller/esp32_controller.ino`.
3. Selecciona tu placa ESP32.
4. Revisa los pines UART2 definidos en el código (`RX2=16`, `TX2=17`) y ajústalos solo si tu cableado real lo requiere.
5. Compila y carga.

Esta primera versión no usa WiFi, Bluetooth, entradas, salidas ni editor de programas interno. Solo actúa como puente serie.

## Prueba 1: Arduino directo

1. Carga el firmware del Arduino Uno.
2. Conecta el Arduino al PC por USB.
3. Abre `web/index.html`.
4. Pulsa **Conectar** y selecciona el puerto del Arduino.
5. Usa **Leer estado**. Deberías recibir tramas como:
   - `<T,j1,j2,j3,j4,j5,j6,pinza>`
   - `<S,velocidad>`
   - `<V,v1,v2,v3>`
   - `<A,aceleracion>`
6. Mueve sliders, guarda poses y reproduce secuencias.

## Prueba 2: ESP32 como puente

1. Carga el firmware del Arduino Uno.
2. Carga el firmware del ESP32.
3. Cablea `TX2` del ESP32 hacia `RX` del Arduino y `RX2` del ESP32 hacia `TX` del Arduino, compartiendo `GND`.
4. Conecta el ESP32 al PC por USB.
5. Abre `web/index.html`.
6. Pulsa **Conectar** y selecciona el puerto del ESP32.
7. Al arrancar, el ESP32 envía `<ESP32,READY>`.
8. Los comandos de la interfaz se reenvían al Arduino y las respuestas vuelven al PC.

## Advertencia de niveles lógicos Arduino Uno / ESP32

El Arduino Uno trabaja normalmente a **5 V** y el ESP32 a **3,3 V**. No conectes directamente una salida de 5 V del Arduino a una entrada del ESP32 sin adaptación de nivel.

Recomendación mínima:

- Usa un divisor resistivo o conversor de nivel lógico en la línea `TX Arduino -> RX ESP32`.
- La línea `TX ESP32 -> RX Arduino` suele ser leída correctamente por el Arduino Uno como nivel alto, pero conviene validarlo en el montaje real.
- Une siempre las masas (`GND`) de Arduino y ESP32.
