# Arquitectura del sistema

## Arquitectura de célula

```text
PC / navegador web -> WebSerial USB -> ESP32 -> UART/Serial2 -> Arduino Uno -> servos del brazo
```

La interfaz web se conecta al ESP32 por WebSerial. El ESP32 actúa como controlador principal y comunica con el Arduino Uno por `Serial2` a `115200` baudios. El Arduino Uno controla únicamente los servos del brazo.

## Por qué el ESP32 controla el proceso

El ESP32 dispone de más recursos para gestionar lógica de célula:

- Entradas digitales configurables.
- Salidas digitales configurables.
- Esperas condicionadas por sensores.
- Pausas temporizadas.
- Ejecución de secuencias paso a paso.
- Comunicación con la interfaz y con el Arduino al mismo tiempo.

En esta fase no se usa WiFi ni Bluetooth. La comunicación con el PC sigue siendo USB/WebSerial.

## Por qué el Arduino solo controla movimiento

El Arduino Uno se reserva para motion control porque debe ejecutar una tarea concreta y estable:

- Recibir `<P,...>`, `<S,...>`, `<V,...>`, `<A,...>`, `<Q>` y `<H>`.
- Aplicar límites articulares.
- Generar movimiento progresivo de los servos.
- Responder estado y confirmaciones.

No gestiona sensores, cinta transportadora, actuadores externos ni programas completos. Esa separación evita mezclar lógica de proceso con generación de movimiento.

## Flujo de comunicación

1. La interfaz envía comandos al ESP32 mediante tramas `<...>`.
2. Si el comando es de movimiento, el ESP32 lo reenvía al Arduino.
3. Si el comando es propio del ESP32, lo gestiona localmente.
4. El Arduino devuelve estado o confirmaciones.
5. El ESP32 reenvía esas respuestas a la interfaz y añade mensajes de depuración.
6. La interfaz muestra todo en el monitor serie.

## Gestión de entradas y salidas

El ESP32 mantiene en memoria hasta 16 entradas y 16 salidas:

- Entradas: `id`, GPIO, modo (`INPUT` o `INPUT_PULLUP`) y estado activo (`HIGH` o `LOW`).
- Salidas: `id`, GPIO y valor actual.

Se validan pines reservados: USB (`1`, `3`), Serial2 (`16`, `17`), flash (`6` a `11`) y salidas no permitidas (`34`, `35`, `36`, `39`).

## Ejecución de secuencias

El ESP32 almacena hasta 100 pasos de programa. La ejecución usa una máquina de estados con `millis()` para no bloquear completamente la recepción de comandos.

Tipos de paso:

- `POSE`: reenvía `<P,...>` al Arduino y espera una respuesta o timeout.
- `OUT`: actualiza una salida digital configurada.
- `WAIT_IN`: espera una entrada activa/inactiva hasta timeout.
- `PAUSE`: espera un tiempo en milisegundos.
- `SPEED`: envía `<S,...>`, `<V,...>` y `<A,...>` al Arduino.
- `HOME`: envía `<H>` al Arduino.
