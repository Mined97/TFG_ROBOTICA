# Arquitectura del sistema

## Arquitectura actual: PC -> Arduino

```text
PC / navegador web -> WebSerial USB -> Arduino Uno -> servos del brazo
```

La interfaz `web/index.html` se comunica por WebSerial a `115200` baudios con el Arduino Uno. El Arduino interpreta comandos de movimiento y controla los 7 canales del brazo: `J1`, `J2`, `J3`, `J4`, `J5`, `J6` y `PINZA`.

Este modo permite probar de forma directa:

- Sliders de articulaciones.
- Velocidad global.
- Velocidades independientes de `J1`, `J2` y `J3`.
- Aceleración configurada.
- Home.
- Consulta de estado.
- Guardado, borrado, exportación, importación y reproducción de poses desde la interfaz.

## Arquitectura futura: PC -> ESP32 -> Arduino

```text
PC / navegador web -> WebSerial USB -> ESP32 -> UART/Serial -> Arduino Uno -> servos del brazo
```

En la arquitectura futura, la interfaz se conecta al ESP32 por USB. El ESP32 reenvía los comandos de movimiento al Arduino Uno mediante UART y recibe sus respuestas.

## Función de cada parte

### Interfaz HTML

- Es la interfaz visual del usuario.
- Permite mover articulaciones y pinza.
- Permite modificar velocidades y aceleración.
- Permite guardar, borrar, reproducir, exportar e importar poses.
- En fases futuras podrá editar programas de célula completos.

### ESP32

- Será el controlador principal de la célula.
- Gestionará entradas digitales, salidas digitales, esperas, pausas y secuencias.
- En esta primera versión solo funciona como puente serie compatible con la interfaz actual.
- No implementa todavía WiFi, Bluetooth, control real de cinta ni entradas/salidas reales.

### Arduino Uno

- Se dedica exclusivamente al motion control del brazo robótico.
- Recibe órdenes de movimiento.
- Controla los servos en los pines reales `{2,4,11,6,8,10,5}`.
- Aplica límites de seguridad, incluyendo la pinza entre `95°` y `180°`.
- No debe gestionar sensores, cinta transportadora, actuadores externos ni lógica de proceso.

## Motivo de separar control de proceso y motion control

La separación mejora la claridad didáctica y técnica del sistema:

- El Arduino Uno mantiene una tarea concreta y repetitiva: mover servos de forma progresiva.
- El ESP32 puede crecer como controlador de célula sin sobrecargar al Arduino.
- La lógica de proceso queda separada de la generación de movimiento.
- Será más fácil añadir sensores, cinta transportadora, pausas, condiciones y secuencias.
- El protocolo serie mantiene una frontera clara entre órdenes de proceso y movimientos del brazo.
