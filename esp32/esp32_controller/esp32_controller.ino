// ESP32 controlador principal de celula - fase con E/S digitales y programas.
// PC/navegador WebSerial <-> ESP32 USB Serial <-> Serial2 <-> Arduino UNO.
// No usa WiFi, Bluetooth ni librerias externas.

const unsigned long BAUD_RATE = 115200;
const int ESP32_RX2_PIN = 16;
const int ESP32_TX2_PIN = 17;

const byte MAX_INPUTS = 16;
const byte MAX_OUTPUTS = 16;
const byte MAX_STEPS = 100;
const unsigned long ARM_ACK_TIMEOUT_MS = 2500;

struct DigitalInputConfig {
  bool used;
  String id;
  byte pin;
  byte mode;
  bool activeHigh;
};

struct DigitalOutputConfig {
  bool used;
  String id;
  byte pin;
  byte value;
};

enum StepType { STEP_POSE, STEP_OUT, STEP_WAIT_IN, STEP_PAUSE, STEP_SPEED, STEP_HOME };
enum RunState { RUN_IDLE, RUN_START_STEP, RUN_WAIT_ARM, RUN_DELAY, RUN_WAIT_INPUT };

struct ProgramStep {
  StepType type;
  int pose[7];
  unsigned long delayMs;
  String id;
  byte value;
  unsigned long timeoutMs;
  int speed;
  int v1;
  int v2;
  int v3;
  int accel;
};

DigitalInputConfig inputs[MAX_INPUTS];
DigitalOutputConfig outputs[MAX_OUTPUTS];
ProgramStep programSteps[MAX_STEPS];
byte inputCount = 0;
byte outputCount = 0;
byte programCount = 0;

String usbBuffer;
String arduinoBuffer;
bool armResponseSeen = false;

RunState runState = RUN_IDLE;
int currentStep = -1;
unsigned long stateStartedMs = 0;
unsigned long delayUntilMs = 0;

String debugSafe(String frame) {
  frame.replace("<", "[");
  frame.replace(">", "]");
  return frame;
}

void sendOk(const __FlashStringHelper *message) {
  Serial.print(F("<OK,"));
  Serial.print(message);
  Serial.println(F(">"));
}

void sendErr(const __FlashStringHelper *message) {
  Serial.print(F("<ERR,"));
  Serial.print(message);
  Serial.println(F(">"));
}

bool isMovementCommand(const String &frame) {
  return frame.startsWith("<P,") || frame.startsWith("<S,") || frame.startsWith("<V,") ||
         frame.startsWith("<A,") || frame == "<Q>" || frame == "<H>";
}

void sendUsbDebug(const String &prefix, const String &frame) {
  Serial.print(F("<ESP32,"));
  Serial.print(prefix);
  Serial.print(',');
  Serial.print(debugSafe(frame));
  Serial.println('>');
}

void sendToArduino(const String &frame) {
  armResponseSeen = false;
  Serial2.println(frame);
  sendUsbDebug("TX_ARDUINO", frame);
}

bool isReservedPin(byte pin, bool output) {
  if (pin == 1 || pin == 3) return true;                 // Serial USB
  if (pin == ESP32_RX2_PIN || pin == ESP32_TX2_PIN) return true; // Serial2 Arduino
  if (pin >= 6 && pin <= 11) return true;                 // memoria flash
  if (output && (pin == 34 || pin == 35 || pin == 36 || pin == 39)) return true;
  return false;
}

bool validatePin(int pin, bool output) {
  return pin >= 0 && pin <= 39 && !isReservedPin((byte)pin, output);
}

int findInput(const String &id) {
  for (byte i = 0; i < inputCount; i++) if (inputs[i].used && inputs[i].id == id) return i;
  return -1;
}

int findOutput(const String &id) {
  for (byte i = 0; i < outputCount; i++) if (outputs[i].used && outputs[i].id == id) return i;
  return -1;
}

bool readInputActive(byte index, int *rawValue = nullptr) {
  int value = digitalRead(inputs[index].pin);
  if (rawValue) *rawValue = value;
  return inputs[index].activeHigh ? (value == HIGH) : (value == LOW);
}

void sendInputState(byte index) {
  int raw = 0;
  bool active = readInputActive(index, &raw);
  Serial.print(F("<IN,STATE,"));
  Serial.print(inputs[index].id);
  Serial.print(',');
  Serial.print(raw ? 1 : 0);
  Serial.print(',');
  Serial.print(active ? 1 : 0);
  Serial.println(F(">"));
}

void sendOutputState(byte index) {
  Serial.print(F("<OUT,STATE,"));
  Serial.print(outputs[index].id);
  Serial.print(',');
  Serial.print(outputs[index].value ? 1 : 0);
  Serial.println(F(">"));
}

byte splitCsv(char *text, char *parts[], byte maxParts) {
  byte count = 0;
  char *token = strtok(text, ",");
  while (token != NULL && count < maxParts) {
    parts[count++] = token;
    token = strtok(NULL, ",");
  }
  return count;
}

String stripFrame(String frame) {
  frame.trim();
  if (frame.startsWith("<") && frame.endsWith(">")) return frame.substring(1, frame.length() - 1);
  return frame;
}

void handleInputCommand(char *parts[], byte count) {
  if (count < 2) { sendErr(F("IN_COMANDO_INVALIDO")); return; }
  String action = parts[1];

  if (action == "CLEAR") {
    inputCount = 0;
    sendOk(F("IN_CLEAR"));
  } else if (action == "ADD") {
    if (count != 6) { sendErr(F("IN_ADD_PARAMETROS")); return; }
    if (inputCount >= MAX_INPUTS) { sendErr(F("IN_MAX")); return; }
    String id = parts[2];
    int pin = atoi(parts[3]);
    String modeText = parts[4];
    String activeText = parts[5];
    if (id.length() == 0 || findInput(id) >= 0) { sendErr(F("IN_ID_INVALIDO")); return; }
    if (!validatePin(pin, false)) { sendErr(F("IN_PIN_NO_PERMITIDO")); return; }
    if (modeText != "INPUT" && modeText != "INPUT_PULLUP") { sendErr(F("IN_MODE_INVALIDO")); return; }
    if (activeText != "HIGH" && activeText != "LOW") { sendErr(F("IN_ACTIVE_INVALIDO")); return; }

    inputs[inputCount].used = true;
    inputs[inputCount].id = id;
    inputs[inputCount].pin = (byte)pin;
    inputs[inputCount].mode = (modeText == "INPUT_PULLUP") ? INPUT_PULLUP : INPUT;
    inputs[inputCount].activeHigh = (activeText == "HIGH");
    pinMode(inputs[inputCount].pin, inputs[inputCount].mode);
    inputCount++;
    sendOk(F("IN_ADD"));
  } else if (action == "READ") {
    if (count != 3) { sendErr(F("IN_READ_PARAMETROS")); return; }
    int idx = findInput(parts[2]);
    if (idx < 0) { sendErr(F("IN_NO_EXISTE")); return; }
    sendInputState(idx);
  } else if (action == "REMOVE") {
    if (count != 3) { sendErr(F("IN_REMOVE_PARAMETROS")); return; }
    int idx = findInput(parts[2]);
    if (idx < 0) { sendErr(F("IN_NO_EXISTE")); return; }
    for (byte i = idx; i + 1 < inputCount; i++) inputs[i] = inputs[i + 1];
    inputCount--;
    sendOk(F("IN_REMOVE"));
  } else {
    sendErr(F("IN_ACCION_DESCONOCIDA"));
  }
}

void handleOutputCommand(char *parts[], byte count) {
  if (count < 2) { sendErr(F("OUT_COMANDO_INVALIDO")); return; }
  String action = parts[1];

  if (action == "CLEAR") {
    outputCount = 0;
    sendOk(F("OUT_CLEAR"));
  } else if (action == "ADD") {
    if (count != 5) { sendErr(F("OUT_ADD_PARAMETROS")); return; }
    if (outputCount >= MAX_OUTPUTS) { sendErr(F("OUT_MAX")); return; }
    String id = parts[2];
    int pin = atoi(parts[3]);
    byte value = atoi(parts[4]) ? 1 : 0;
    if (id.length() == 0 || findOutput(id) >= 0) { sendErr(F("OUT_ID_INVALIDO")); return; }
    if (!validatePin(pin, true)) { sendErr(F("OUT_PIN_NO_PERMITIDO")); return; }

    outputs[outputCount].used = true;
    outputs[outputCount].id = id;
    outputs[outputCount].pin = (byte)pin;
    outputs[outputCount].value = value;
    pinMode(outputs[outputCount].pin, OUTPUT);
    digitalWrite(outputs[outputCount].pin, value ? HIGH : LOW);
    sendOutputState(outputCount);
    outputCount++;
    sendOk(F("OUT_ADD"));
  } else if (action == "SET") {
    if (count != 4) { sendErr(F("OUT_SET_PARAMETROS")); return; }
    int idx = findOutput(parts[2]);
    if (idx < 0) { sendErr(F("OUT_NO_EXISTE")); return; }
    outputs[idx].value = atoi(parts[3]) ? 1 : 0;
    digitalWrite(outputs[idx].pin, outputs[idx].value ? HIGH : LOW);
    sendOutputState(idx);
  } else if (action == "REMOVE") {
    if (count != 3) { sendErr(F("OUT_REMOVE_PARAMETROS")); return; }
    int idx = findOutput(parts[2]);
    if (idx < 0) { sendErr(F("OUT_NO_EXISTE")); return; }
    digitalWrite(outputs[idx].pin, LOW);
    for (byte i = idx; i + 1 < outputCount; i++) outputs[i] = outputs[i + 1];
    outputCount--;
    sendOk(F("OUT_REMOVE"));
  } else {
    sendErr(F("OUT_ACCION_DESCONOCIDA"));
  }
}

void sendProgramLoaded() {
  Serial.print(F("<PROG,LOADED,"));
  Serial.print(programCount);
  Serial.println(F(">"));
}

void handleProgramAdd(char *parts[], byte count) {
  if (programCount >= MAX_STEPS) { sendErr(F("PROG_MAX")); return; }
  if (count < 3) { sendErr(F("PROG_ADD_PARAMETROS")); return; }
  String type = parts[2];
  ProgramStep step;
  step.delayMs = 0;
  step.timeoutMs = 0;
  step.value = 0;
  step.id = "";

  if (type == "POSE") {
    if (count != 11) { sendErr(F("PROG_POSE_PARAMETROS")); return; }
    step.type = STEP_POSE;
    for (byte i = 0; i < 7; i++) step.pose[i] = atoi(parts[3 + i]);
    step.delayMs = atol(parts[10]);
  } else if (type == "OUT") {
    if (count != 5) { sendErr(F("PROG_OUT_PARAMETROS")); return; }
    if (findOutput(parts[3]) < 0) { sendErr(F("PROG_OUT_NO_EXISTE")); return; }
    step.type = STEP_OUT;
    step.id = parts[3];
    step.value = atoi(parts[4]) ? 1 : 0;
  } else if (type == "WAIT_IN") {
    if (count != 6) { sendErr(F("PROG_WAIT_PARAMETROS")); return; }
    if (findInput(parts[3]) < 0) { sendErr(F("PROG_IN_NO_EXISTE")); return; }
    step.type = STEP_WAIT_IN;
    step.id = parts[3];
    step.value = atoi(parts[4]) ? 1 : 0;
    step.timeoutMs = atol(parts[5]);
  } else if (type == "PAUSE") {
    if (count != 4) { sendErr(F("PROG_PAUSE_PARAMETROS")); return; }
    step.type = STEP_PAUSE;
    step.delayMs = atol(parts[3]);
  } else if (type == "SPEED") {
    if (count != 8) { sendErr(F("PROG_SPEED_PARAMETROS")); return; }
    step.type = STEP_SPEED;
    step.speed = atoi(parts[3]);
    step.v1 = atoi(parts[4]);
    step.v2 = atoi(parts[5]);
    step.v3 = atoi(parts[6]);
    step.accel = atoi(parts[7]);
  } else if (type == "HOME") {
    if (count != 3) { sendErr(F("PROG_HOME_PARAMETROS")); return; }
    step.type = STEP_HOME;
  } else {
    sendErr(F("PROG_TIPO_DESCONOCIDO"));
    return;
  }

  programSteps[programCount++] = step;
  sendProgramLoaded();
}

const __FlashStringHelper *stepTypeName(StepType type) {
  switch (type) {
    case STEP_POSE: return F("POSE");
    case STEP_OUT: return F("OUT");
    case STEP_WAIT_IN: return F("WAIT_IN");
    case STEP_PAUSE: return F("PAUSE");
    case STEP_SPEED: return F("SPEED");
    case STEP_HOME: return F("HOME");
  }
  return F("UNKNOWN");
}

void stopRun(const __FlashStringHelper *reason, bool error) {
  runState = RUN_IDLE;
  currentStep = -1;
  if (error) {
    Serial.print(F("<RUN,ERROR,"));
    Serial.print(reason);
    Serial.println(F(">"));
  } else {
    Serial.print(F("<RUN,"));
    Serial.print(reason);
    Serial.println(F(">"));
  }
}

void startRun() {
  if (programCount == 0) { sendErr(F("PROG_VACIO")); return; }
  currentStep = 0;
  runState = RUN_START_STEP;
  stateStartedMs = millis();
  Serial.print(F("<RUN,START,"));
  Serial.print(programCount);
  Serial.println(F(">"));
}

String poseCommand(const ProgramStep &step) {
  String cmd = "<P";
  for (byte i = 0; i < 7; i++) {
    cmd += ",";
    cmd += String(step.pose[i]);
  }
  cmd += ">";
  return cmd;
}

void reportRunStep(const ProgramStep &step) {
  Serial.print(F("<RUN,STEP,"));
  Serial.print(currentStep + 1);
  Serial.print(',');
  Serial.print(programCount);
  Serial.print(',');
  Serial.print(stepTypeName(step.type));
  Serial.println(F(">"));
}

void updateProgramRunner() {
  if (runState == RUN_IDLE) return;
  if (currentStep < 0 || currentStep >= programCount) {
    stopRun(F("DONE"), false);
    return;
  }

  unsigned long now = millis();
  ProgramStep &step = programSteps[currentStep];

  if (runState == RUN_START_STEP) {
    reportRunStep(step);
    stateStartedMs = now;

    if (step.type == STEP_POSE) {
      sendToArduino(poseCommand(step));
      delayUntilMs = now + step.delayMs;
      runState = RUN_WAIT_ARM;
    } else if (step.type == STEP_HOME) {
      sendToArduino("<H>");
      delayUntilMs = now;
      runState = RUN_WAIT_ARM;
    } else if (step.type == STEP_SPEED) {
      sendToArduino(String("<S,") + String(step.speed) + ">");
      sendToArduino(String("<V,") + String(step.v1) + "," + String(step.v2) + "," + String(step.v3) + ">");
      sendToArduino(String("<A,") + String(step.accel) + ">");
      delayUntilMs = now + 200;
      runState = RUN_DELAY;
    } else if (step.type == STEP_OUT) {
      int idx = findOutput(step.id);
      if (idx < 0) { stopRun(F("OUT_NO_EXISTE"), true); return; }
      outputs[idx].value = step.value ? 1 : 0;
      digitalWrite(outputs[idx].pin, outputs[idx].value ? HIGH : LOW);
      sendOutputState(idx);
      currentStep++;
    } else if (step.type == STEP_WAIT_IN) {
      if (findInput(step.id) < 0) { stopRun(F("IN_NO_EXISTE"), true); return; }
      runState = RUN_WAIT_INPUT;
    } else if (step.type == STEP_PAUSE) {
      delayUntilMs = now + step.delayMs;
      runState = RUN_DELAY;
    }
  } else if (runState == RUN_WAIT_ARM) {
    if (armResponseSeen || now - stateStartedMs >= ARM_ACK_TIMEOUT_MS) {
      if (!armResponseSeen) Serial.println(F("<RUN,ERROR,ARM_ACK_TIMEOUT_CONTINUE>"));
      runState = RUN_DELAY;
      if (delayUntilMs < now) delayUntilMs = now;
    }
  } else if (runState == RUN_DELAY) {
    if ((long)(now - delayUntilMs) >= 0) {
      currentStep++;
      if (currentStep >= programCount) stopRun(F("DONE"), false);
      else runState = RUN_START_STEP;
    }
  } else if (runState == RUN_WAIT_INPUT) {
    int idx = findInput(step.id);
    if (idx < 0) { stopRun(F("IN_NO_EXISTE"), true); return; }
    bool active = readInputActive(idx);
    bool expected = step.value ? true : false;
    if (active == expected) {
      sendInputState(idx);
      currentStep++;
      if (currentStep >= programCount) stopRun(F("DONE"), false);
      else runState = RUN_START_STEP;
    } else if (now - stateStartedMs >= step.timeoutMs) {
      stopRun(F("TIMEOUT_INPUT"), true);
    }
  }
}

void handleProgramCommand(char *parts[], byte count) {
  if (count < 2) { sendErr(F("PROG_COMANDO_INVALIDO")); return; }
  String action = parts[1];
  if (action == "CLEAR") {
    programCount = 0;
    runState = RUN_IDLE;
    currentStep = -1;
    sendProgramLoaded();
  } else if (action == "ADD") {
    handleProgramAdd(parts, count);
  } else if (action == "RUN") {
    startRun();
  } else if (action == "STOP") {
    stopRun(F("STOPPED"), false);
  } else if (action == "STATUS") {
    sendProgramLoaded();
  } else {
    sendErr(F("PROG_ACCION_DESCONOCIDA"));
  }
}

void handleUsbFrame(String frame) {
  frame.trim();
  if (!frame.startsWith("<") || !frame.endsWith(">")) { sendErr(F("TRAMA_INVALIDA")); return; }

  if (isMovementCommand(frame)) {
    sendToArduino(frame);
    return;
  }

  String inner = stripFrame(frame);
  char buffer[180];
  inner.toCharArray(buffer, sizeof(buffer));
  char *parts[14];
  byte count = splitCsv(buffer, parts, 14);
  if (count == 0) { sendErr(F("COMANDO_VACIO")); return; }

  String cmd = parts[0];
  if (cmd == "PING") {
    sendOk(F("PONG"));
  } else if (cmd == "STATUS") {
    Serial.print(F("<OK,STATUS,IN="));
    Serial.print(inputCount);
    Serial.print(F(",OUT="));
    Serial.print(outputCount);
    Serial.print(F(",PROG="));
    Serial.print(programCount);
    Serial.print(F(",RUN="));
    Serial.print(runState == RUN_IDLE ? 0 : 1);
    Serial.println(F(">"));
  } else if (cmd == "IN") {
    handleInputCommand(parts, count);
  } else if (cmd == "OUT") {
    handleOutputCommand(parts, count);
  } else if (cmd == "PROG") {
    handleProgramCommand(parts, count);
  } else {
    sendErr(F("ESP32_COMANDO_DESCONOCIDO"));
  }
}

void forwardArduinoFrame(const String &frame) {
  armResponseSeen = true;
  Serial.println(frame);
  sendUsbDebug("RX_ARDUINO", frame);
}

void readFrames(Stream &stream, String &buffer, bool fromUsb) {
  while (stream.available() > 0) {
    char c = stream.read();
    if (c == '\r' || c == '\n') continue;
    if (c == '<') buffer = "";
    buffer += c;

    if (c == '>') {
      if (fromUsb) handleUsbFrame(buffer);
      else forwardArduinoFrame(buffer);
      buffer = "";
    }

    if (buffer.length() > 179) {
      buffer = "";
      if (fromUsb) sendErr(F("TRAMA_USB_DEMASIADO_LARGA"));
      else sendErr(F("TRAMA_ARDUINO_DEMASIADO_LARGA"));
    }
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, ESP32_RX2_PIN, ESP32_TX2_PIN);
  delay(300);
  Serial.println(F("<ESP32,READY>"));
}

void loop() {
  readFrames(Serial, usbBuffer, true);
  readFrames(Serial2, arduinoBuffer, false);
  updateProgramRunner();
}
