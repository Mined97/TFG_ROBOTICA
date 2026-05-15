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
- `<PROG,ADD,IF_INPUT,id,expectedState,targetTrue,targetFalse>`: evalúa una entrada. `expectedState` admite `ACTIVE` o `INACTIVE`; si coincide salta a `targetTrue`, si no coincide salta a `targetFalse`. Los destinos son pasos empezando en 1; `0` significa continuar con el siguiente paso.
- `<PROG,ADD,IF_LOGIC,idA,operator,idB,targetTrue,targetFalse>`: evalúa una condición lógica. `operator` admite `A_ACTIVE`, `A_INACTIVE`, `NOT_A`, `A_AND_B`, `A_OR_B`, `A_AND_NOT_B`, `A_OR_NOT_B`, `NOT_A_AND_B` y `NOT_A_OR_B`. Para operadores que no necesitan entrada B, `idB` puede ir vacío o como `NONE`.
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
- `<H>`

## Respuestas del ESP32

- `<ESP32,READY>`
- `<OK,mensaje>`
- `<ERR,mensaje>`
- `<IN,STATE,id,value,active>`
- `<IN,CFG,id,pin,mode,activeState,raw,stable,active>`
- `<IN,LIST,DONE,count>`
- `<OUT,STATE,id,value>`
- `<PROG,LOADED,totalSteps>`
- `<PROG,STATUS,count,running,loop,step>`
- `<PROG,LOOP,value>`
- `<RUN,START,totalSteps>`
- `<RUN,STEP,index,total,type>`
- `<RUN,BRANCH,index,result,target>`: informa del resultado de una condición (`true`/`false`) y del destino elegido; `target=0` indica continuar con el siguiente paso.
- `<RUN,DONE>`
- `<RUN,LOOP>`
- `<RUN,STOPPED>`
- `<RUN,ERROR,mensaje>`
- `<ESP32,TX_ARDUINO,...>` si `DEBUG_ARDUINO_TX` está activado en el firmware ESP32.

Errores específicos de condiciones en ejecución:

- `<RUN,ERROR,ENTRADA_NO_ENCONTRADA>`: la entrada configurada para una espera o condición no existe en el ESP32.
- `<RUN,ERROR,OPERADOR_LOGICO_INVALIDO>`: el operador de `IF_LOGIC` no está soportado.
- `<RUN,ERROR,SALTO_INVALIDO>`: el destino elegido no apunta a un paso cargado.

No existe retorno Arduino → ESP32: el ESP32 no lee `Serial2`, no reenvía respuestas del Arduino y no emite `<ESP32,RX_ARDUINO,...>`.

## Respuestas del Arduino

El ESP32 no consume ni reenvía respuestas del Arduino. Para diagnosticar el Arduino, conéctalo directamente por USB a un monitor serie.
