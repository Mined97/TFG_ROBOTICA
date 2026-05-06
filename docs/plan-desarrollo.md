# Plan de desarrollo

## Fase completada: ESP32 como controlador principal

- Interfaz web ampliada para conectarse al ESP32 por WebSerial.
- Configuración persistente en `localStorage` de poses, entradas, salidas y programa.
- Editor de programa con pasos de tipo pose, salida, espera de entrada, pausa, velocidad y Home.
- Firmware ESP32 con `Serial` USB y `Serial2` hacia Arduino.
- Gestión de hasta 16 entradas digitales, 16 salidas digitales y 100 pasos.
- Máquina de estados con `millis()` para no bloquear completamente la ejecución.
- Arduino Uno mantenido como motion control del brazo.

## Próximas fases propuestas

1. Confirmación de movimiento real: esperar `<OK,...>` o comprobar `<T,...>` antes de avanzar pasos críticos.
2. Guardado persistente en ESP32 mediante NVS/Preferences para entradas, salidas y programas.
3. Pantalla de diagnóstico de pines y cableado.
4. Modo simulación visual de la secuencia antes de enviarla a hardware.
5. Interlocks de seguridad: seta de emergencia, puerta, límites externos y rearme.
6. Calibración fina de home y límites mecánicos reales por eje.

## Criterios de seguridad

- No conectar señales de 5 V del Arduino directamente a entradas del ESP32 sin adaptación de nivel.
- Verificar sentido de giro y límites antes de ejecutar secuencias automáticas.
- Probar primero con servos sin carga y velocidades bajas.
