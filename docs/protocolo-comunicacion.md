# Protocolo de comunicación

Todas las tramas son texto ASCII delimitado por `<` y `>` a `115200` baudios.

## Comandos de movimiento hacia Arduino

El ESP32 mantiene el protocolo base del brazo y lo reenvía al Arduino:

| Comando | Uso |
| --- | --- |
| `<P,j1,j2,j3,j4,j5,j6,pinza>` | Pose completa del brazo |
| `<S,velocidad>` | Velocidad global para `J4..J7` |
| `<V,v1,v2,v3>` | Velocidades independientes de `J1..J3` |
| `<A,aceleracion>` | Aceleración |
| `<Q>` | Consulta de estado |
| `<H>` | Home |

Canales y límites:

| Canal | Pin Arduino | Límite |
| --- | ---: | --- |
| `J1` | 2 | `0..180` |
| `J2` | 4 | `0..180` |
| `J3` | 11 | `0..180` |
| `J4` | 6 | `0..180` |
| `J5` | 8 | `0..180` |
| `J6` | 10 | `0..180` |
| `PINZA/J7` | 5 | `95..180` |

## Comandos propios del ESP32

### Estado

```text
<PING>
<STATUS>
```

Respuestas típicas:

```text
<OK,PONG>
<OK,STATUS,IN=1,OUT=1,PROG=6,RUN=0>
```

### Entradas digitales

```text
<IN,CLEAR>
<IN,ADD,id,pin,mode,activeState>
<IN,READ,id>
<IN,REMOVE,id>
```

Parámetros:

- `id`: identificador corto sin espacios.
- `pin`: GPIO del ESP32.
- `mode`: `INPUT` o `INPUT_PULLUP`.
- `activeState`: `HIGH` o `LOW`.

Ejemplo:

```text
<IN,ADD,sensor1,32,INPUT_PULLUP,LOW>
<IN,READ,sensor1>
```

Respuesta:

```text
<IN,STATE,sensor1,0,1>
```

El último valor indica si la entrada se considera activa.

### Salidas digitales

```text
<OUT,CLEAR>
<OUT,ADD,id,pin,initialState>
<OUT,SET,id,value>
<OUT,REMOVE,id>
```

Parámetros:

- `initialState`: `0` u `1`.
- `value`: `0` u `1`.

Ejemplo:

```text
<OUT,ADD,actuador1,25,0>
<OUT,SET,actuador1,1>
```

Respuesta:

```text
<OUT,STATE,actuador1,1>
```

### Programa

```text
<PROG,CLEAR>
<PROG,ADD,POSE,j1,j2,j3,j4,j5,j6,pinza,delayMs>
<PROG,ADD,OUT,id,value>
<PROG,ADD,WAIT_IN,id,state,timeoutMs>
<PROG,ADD,PAUSE,durationMs>
<PROG,ADD,SPEED,s,v1,v2,v3,a>
<PROG,ADD,HOME>
<PROG,RUN>
<PROG,STOP>
<PROG,STATUS>
```

Ejemplo completo:

```text
<IN,CLEAR>
<IN,ADD,in_pieza,32,INPUT_PULLUP,LOW>
<OUT,CLEAR>
<OUT,ADD,out_cinta,25,0>
<PROG,CLEAR>
<PROG,ADD,SPEED,60,60,60,60,300>
<PROG,ADD,POSE,90,90,90,90,90,90,120,800>
<PROG,ADD,OUT,out_cinta,1>
<PROG,ADD,WAIT_IN,in_pieza,1,10000>
<PROG,ADD,PAUSE,500>
<PROG,ADD,HOME>
<PROG,RUN>
```

## Respuestas del ESP32

```text
<ESP32,READY>
<OK,mensaje>
<ERR,mensaje>
<IN,STATE,id,value,active>
<OUT,STATE,id,value>
<PROG,LOADED,totalSteps>
<RUN,START,totalSteps>
<RUN,STEP,index,total,type>
<RUN,DONE>
<RUN,STOPPED>
<RUN,ERROR,mensaje>
<ESP32,TX_ARDUINO,...>
<ESP32,RX_ARDUINO,...>
```

Los mensajes `TX_ARDUINO` y `RX_ARDUINO` usan `[` y `]` dentro del payload para evitar tramas anidadas ambiguas.

## Respuestas del Arduino

```text
<T,j1,j2,j3,j4,j5,j6,pinza>
<S,velocidad>
<V,v1,v2,v3>
<A,aceleracion>
<OK,P>
<OK,H>
<OK,S>
<OK,V>
<OK,A>
<ERR,mensaje>
```

## Restricciones de pines ESP32

- GPIO `1` y `3`: no permitidos por Serial USB.
- GPIO `16` y `17`: no permitidos porque se usan para `Serial2` hacia Arduino.
- GPIO `6..11`: no permitidos porque suelen estar asociados a memoria flash.
- GPIO `34`, `35`, `36`, `39`: permitidos como entrada, no como salida.
