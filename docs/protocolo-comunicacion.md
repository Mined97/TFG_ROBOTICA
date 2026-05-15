# Protocolo de comunicación

Todas las tramas usan el formato `<...>` y se transmiten a **115200 baudios**.

## PC / WebSerial -> ESP32

### Estado

- `<PING>`: comprueba que el ESP32 responde.
- `<STATUS>`: solicita estado general del controlador.

### Entradas digitales

- `<IN,CLEAR>`: elimina todas las entradas configuradas.
- `<IN,ADD,id,pin,mode,activeState>`: configura una entrada.
  - `mode`: `INPUT` o `INPUT_PULLUP`.
  - `activeState`: `HIGH`, `LOW`, `1` o `0`.
- `<IN,READ,id>`: lee una entrada.
- `<IN,LIST>`: lista las entradas realmente configuradas en el ESP32 mediante `<IN,CFG,...>` y finaliza con `<IN,LIST,DONE,count>`.
- `<IN,REMOVE,id>`: elimina una entrada.

### Salidas digitales

- `<OUT,CLEAR>`: elimina todas las salidas configuradas.
- `<OUT,ADD,id,pin,initialState>`: configura una salida y aplica su estado inicial.
- `<OUT,SET,id,value>`: cambia una salida a `1/ON/HIGH` o `0/OFF/LOW`.
- `<OUT,REMOVE,id>`: elimina una salida.

### Programa del ESP32

- `<PROG,CLEAR>`
- `<PROG,LOOP,value>`: activa (`1`) o desactiva (`0`) que el programa vuelva al paso 1 al finalizar.
- `<PROG,ADD,POSE,j1,j2,j3,j4,j5,j6,pinza,delayMs>`
- `<PROG,ADD,OUT,id,value>`
- `<PROG,ADD,WAIT_IN,id,state,timeoutMs>`
- `<PROG,ADD,PAUSE,durationMs>`
- `<PROG,ADD,SPEED,s,v1,v2,v3,a>`
- `<PROG,ADD,HOME>`
- `<PROG,ADD,IF_INPUTS,id1,id2,targetBoth,targetOne,targetElse>`: evalúa dos entradas; si las dos están activas salta a `targetBoth`, si solo una está activa salta a `targetOne`, y si ninguna está activa salta a `targetElse`. Los destinos son pasos empezando en 1; `0` significa continuar con el siguiente paso.
- `<PROG,ADD,JUMP,target>`: salta incondicionalmente al paso `target`, empezando en 1.
- `<PROG,RUN>`
- `<PROG,STOP>`
- `<PROG,STATUS>`: devuelve `<PROG,STATUS,count,running,loop,step>` para verificar la carga antes de ejecutar.

## Comandos reenviados al Arduino

El ESP32 distingue sus propios comandos y reenvía al Arduino solo los comandos de movimiento:

- `<P,j1,j2,j3,j4,j5,j6,pinza>`
- `<S,velocidad>`
- `<V,v1,v2,v3>`
- `<A,aceleracion>`
- `<Q>`
- `<H>`

## Respuestas del ESP32

- `<ESP32,READY>`
- `<OK,mensaje>`
- `<ERR,mensaje>`
- `<IN,STATE,id,value,active>`
- `<IN,CFG,id,pin,mode,activeState>`
- `<IN,LIST,DONE,count>`
- `<OUT,STATE,id,value>`
- `<PROG,LOADED,totalSteps>`
- `<PROG,STATUS,count,running,loop,step>`
- `<PROG,LOOP,value>`
- `<RUN,START,totalSteps>`
- `<RUN,STEP,index,total,type>`
- `<RUN,DONE>`
- `<RUN,LOOP>`
- `<RUN,STOPPED>`
- `<RUN,ERROR,mensaje>`
- `<ESP32,TX_ARDUINO,...>`
- `<ESP32,RX_ARDUINO,...>`

## Respuestas compatibles del Arduino

- `<T,j1,j2,j3,j4,j5,j6,pinza>`
- `<S,velocidad>`
- `<V,v1,v2,v3>`
- `<A,aceleracion>`
- `<OK,mensaje>`
- `<ERR,mensaje>`

El ESP32 reenvía a la interfaz la respuesta original del Arduino y además genera una línea de monitorización `<ESP32,RX_ARDUINO,...>`.
