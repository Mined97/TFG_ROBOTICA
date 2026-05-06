# Plan de desarrollo

## Fase 1: ordenar repositorio

- Mover la interfaz real a `web/index.html`.
- Crear estructura `docs/`, `arduino/`, `esp32/` y `examples/`.
- Documentar arquitectura, protocolo y plan de evolución.

## Fase 2: firmware Arduino compatible con interfaz

- Implementar firmware Arduino Uno con `Servo.h`.
- Controlar los 7 canales en los pines reales `{2,4,11,6,8,10,5}`.
- Aplicar límites `0°..180°` en `J1..J6` y `95°..180°` en `PINZA`.
- Interpretar comandos `<P>`, `<S>`, `<V>`, `<A>`, `<Q>` y `<H>`.
- Responder con tramas compatibles con la interfaz.
- Implementar Home y movimiento progresivo simple.

## Fase 3: ESP32 como puente serie

- Comunicar PC/interfaz con ESP32 por USB a `115200` baudios.
- Comunicar ESP32 con Arduino Uno por `Serial2` a `115200` baudios.
- Reenviar comandos de movimiento al Arduino.
- Reenviar respuestas del Arduino al PC.
- Añadir mensajes de depuración claros.

## Fase 4: salidas digitales desde ESP32

- Añadir abstracción de salidas digitales.
- Preparar comandos futuros para activar/desactivar actuadores.
- Mantener el Arduino aislado de esta lógica.
- No mover todavía la cinta transportadora hasta validar cableado y seguridad.

## Fase 5: entradas digitales desde ESP32

- Añadir lectura de sensores de detección.
- Crear estados de entrada claros.
- Preparar esperas condicionadas por sensores.
- Documentar polaridad, antirrebote y seguridad.

## Fase 6: editor visual de programa

- Ampliar la interfaz HTML con bloques de programa.
- Permitir pasos de pose, salida digital, espera de entrada y pausa.
- Mantener importación/exportación JSON.
- Evitar frameworks para conservar simplicidad y portabilidad.

## Fase 7: ejecución de secuencias completas desde ESP32

- Transferir programas desde la interfaz al ESP32.
- Ejecutar secuencias de célula en el ESP32.
- Gestionar pausa, parada, errores y estados.
- Enviar al Arduino únicamente órdenes de motion control.

## Fase 8: integración con cinta transportadora y sensores

- Integrar cinta transportadora como actuador controlado por ESP32.
- Integrar sensores de presencia o posición.
- Validar ciclo pick and place completo.
- Documentar pruebas, riesgos y mejoras para el TFG.
