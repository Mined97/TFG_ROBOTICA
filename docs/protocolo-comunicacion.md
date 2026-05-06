# Protocolo de comunicación

La comunicación usa tramas ASCII delimitadas por `<` y `>` a `115200` baudios.

## Canales del brazo

| Canal | Función | Pin Arduino Uno | Límite mínimo | Límite máximo |
| --- | --- | ---: | ---: | ---: |
| `J1` | Eje 1 | 2 | 0° | 180° |
| `J2` | Eje 2 | 4 | 0° | 180° |
| `J3` | Eje 3 | 11 | 0° | 180° |
| `J4` | Eje 4 | 6 | 0° | 180° |
| `J5` | Eje 5 | 8 | 0° | 180° |
| `J6` | Eje 6 | 10 | 0° | 180° |
| `PINZA` / `J7` | Pinza | 5 | 95° | 180° |

## Comandos de la interfaz hacia Arduino o ESP32 puente

### Mover pose completa

```text
<P,j1,j2,j3,j4,j5,j6,pinza>
```

Ejemplo:

```text
<P,90,80,100,90,90,90,120>
```

Manda una posición objetivo completa para los 7 canales.

### Velocidad global

```text
<S,velocidad>
```

Ejemplo:

```text
<S,60>
```

Define la velocidad global. En el firmware Arduino actual se aplica a `J4`, `J5`, `J6` y `PINZA`.

### Velocidades independientes de J1-J3

```text
<V,v1,v2,v3>
```

Ejemplo:

```text
<V,50,60,70>
```

Define las velocidades independientes de los tres primeros ejes.

### Aceleración

```text
<A,aceleracion>
```

Ejemplo:

```text
<A,300>
```

Configura el valor de aceleración en grados/seg². La primera versión del firmware valida y conserva este valor, aunque el movimiento progresivo implementado es simple.

### Consulta de estado

```text
<Q>
```

Solicita el estado actual del brazo, velocidad global, velocidades independientes y aceleración.

### Home

```text
<H>
```

Envía el brazo a una posición Home segura definida en el firmware.

## Respuestas esperadas del Arduino

### Posición actual

```text
<T,j1,j2,j3,j4,j5,j6,pinza>
```

Ejemplo:

```text
<T,90,90,90,90,90,90,120>
```

### Velocidad global

```text
<S,velocidad>
```

### Velocidades independientes

```text
<V,v1,v2,v3>
```

### Aceleración

```text
<A,aceleracion>
```

### Confirmación

```text
<OK,mensaje>
```

Ejemplos:

```text
<OK,ARDUINO_READY>
<OK,POSE_RECIBIDA>
<OK,HOME>
```

### Error

```text
<ERR,mensaje>
```

Ejemplos:

```text
<ERR,COMANDO_DESCONOCIDO>
<ERR,P_REQUIERE_7_VALORES>
```

## Mensajes del ESP32 puente

Al arrancar, el ESP32 envía:

```text
<ESP32,READY>
```

Cuando reenvía un comando al Arduino, informa con:

```text
<ESP32,TX_ARDUINO,[P,90,90,90,90,90,90,120]>
```

Cuando recibe una respuesta del Arduino, la reenvía al PC y además informa con:

```text
<ESP32,RX_ARDUINO,[T,90,90,90,90,90,90,120]>
```

En los mensajes de depuración se sustituyen los caracteres `<` y `>` por `[` y `]` para no crear tramas anidadas ambiguas.

## Compatibilidad con la interfaz

La interfaz analiza las tramas conocidas `T`, `S`, `V` y `A`. Las tramas `OK`, `ERR` y `ESP32` se muestran en el estado de recepción, pero no modifican sliders ni poses.
