# Plan de desarrollo

## Fase actual implementada

- Repositorio ordenado con `web/`, `arduino/`, `esp32/`, `docs/` y `examples/`.
- Interfaz original conservada y ampliada con configuración de E/S, editor de programa y monitor serie.
- Arduino Uno dedicado al motion control del brazo con 7 canales y protocolo base compatible.
- ESP32 como controlador principal por USB/WebSerial y puente `Serial2` hacia Arduino.
- Gestión en memoria de hasta 16 entradas, 16 salidas y 100 pasos de programa.
- Ejecución de secuencias con máquina de estados basada en `millis()`.

## Tareas pendientes a corto plazo

- Compilar y validar firmwares en Arduino IDE con hardware real.
- Ajustar tiempos de confirmación del Arduino según velocidad real del brazo.
- Añadir estados visuales más detallados en la interfaz para entradas, salidas y ejecución.
- Mejorar validaciones de importación JSON en la interfaz.
- Añadir botón de sincronización completa desde ESP32 a interfaz si se decide conservar configuración en memoria persistente del ESP32.

## Integración futura con cinta transportadora

- Modelar la cinta como una salida digital genérica inicialmente.
- Validar etapa de potencia, relé, driver o variador usado.
- Añadir enclavamientos de seguridad y parada.
- Documentar polaridad, tensión y corriente.

## Integración futura con sensores reales

- Conectar sensores a entradas digitales del ESP32.
- Definir si trabajan activos en HIGH o LOW.
- Validar `INPUT` frente a `INPUT_PULLUP`.
- Añadir antirrebote o filtrado si el sensor lo requiere.

## Mejoras futuras

- Persistencia de configuración en memoria no volátil del ESP32.
- Editor visual más cómodo con edición directa de pasos ya creados.
- Confirmación de fin de movimiento más precisa desde Arduino.
- Pantalla de diagnóstico de pines y estado de célula.
- Integración del ciclo pick and place completo con cinta, sensores y actuadores reales.
