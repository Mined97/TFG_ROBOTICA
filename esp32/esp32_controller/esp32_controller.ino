// ESP32 controlador principal de la celula robotica.
// PC/navegador WebSerial -> USB Serial -> ESP32 -> Serial2/UART -> Arduino UNO.
// El ESP32 gestiona entradas, salidas y programas; el Arduino solo motion
// control.

const unsigned long BAUD_RATE = 115200;
const int ESP32_RX2_PIN = 16;
const int ESP32_TX2_PIN = 17;

const byte MAX_INPUTS = 16;
const byte MAX_OUTPUTS = 16;
const byte MAX_STEPS = 100;
const byte NUM_CHANNELS = 7;

String usbBuffer;
String arduinoBuffer;

struct DigitalInputCfg {
  bool used;
  String id;
  int pin;
  byte mode;       // INPUT / INPUT_PULLUP
  int activeState; // HIGH / LOW
  int rawValue;
  int stableValue;
  int lastRawValue;
  unsigned long rawChangedAt;
  bool initialized;
};

struct DigitalOutputCfg {
  bool used;
  String id;
  int pin;
  int value;
};

enum StepType {
  STEP_NONE,
  STEP_POSE,
  STEP_OUT,
  STEP_WAIT_IN,
  STEP_PAUSE,
  STEP_SPEED,
  STEP_HOME,
  STEP_IF_INPUTS,
  STEP_JUMP
};

struct ProgramStep {
  StepType type;
  int pose[NUM_CHANNELS];
  unsigned long delayMs;
  String id;
  int value;
  int state;
  unsigned long timeoutMs;
  unsigned long durationMs;
  int speed;
  int v1;
  int v2;
  int v3;
  int accel;
  String id2;
  int targetBoth;
  int targetOne;
  int targetElse;
};

DigitalInputCfg inputs[MAX_INPUTS];
DigitalOutputCfg outputs[MAX_OUTPUTS];
ProgramStep programSteps[MAX_STEPS];
byte programCount = 0;

bool running = false;
bool programLoop = false;
byte currentStep = 0;
bool stepActive = false;
unsigned long stepStartMs = 0;
unsigned long waitUntilMs = 0;
unsigned long lastWaitInputReportMs = 0;
const unsigned long WAIT_INPUT_REPORT_INTERVAL_MS = 300;
const unsigned long INPUT_STABLE_MS = 40;

String debugSafe(String frame) {
  frame.replace("<", "[");
  frame.replace(">", "]");
  return frame;
}

void sendOk(const String &msg) { Serial.println("<OK," + msg + ">"); }
void sendErr(const String &msg) { Serial.println("<ERR," + msg + ">"); }

String stepTypeName(StepType type) {
  switch (type) {
  case STEP_POSE:
    return "POSE";
  case STEP_OUT:
    return "OUT";
  case STEP_WAIT_IN:
    return "WAIT_IN";
  case STEP_PAUSE:
    return "PAUSE";
  case STEP_SPEED:
    return "SPEED";
  case STEP_HOME:
    return "HOME";
  case STEP_IF_INPUTS:
    return "IF_INPUTS";
  case STEP_JUMP:
    return "JUMP";
  default:
    return "NONE";
  }
}

void sendUsbDebug(const String &prefix, const String &frame) {
  Serial.print("<ESP32,");
  Serial.print(prefix);
  Serial.print(',');
  Serial.print(debugSafe(frame));
  Serial.println('>');
}

bool isArduinoMotionCommand(const String &frame) {
  return frame.startsWith("<P,") || frame.startsWith("<S,") ||
         frame.startsWith("<V,") || frame.startsWith("<A,") || frame == "<Q>" ||
         frame == "<H>";
}

void sendToArduino(const String &frame) {
  Serial2.println(frame);
  sendUsbDebug("TX_ARDUINO", frame);
}

void forwardArduinoFrame(const String &frame) {
  Serial.println(frame);
  sendUsbDebug("RX_ARDUINO", frame);
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

bool isInputOnlyPin(int pin) {
  return pin == 34 || pin == 35 || pin == 36 || pin == 39;
}

bool isReservedPin(int pin) {
  if (pin < 0 || pin > 39)
    return true;
  if (pin == 1 || pin == 3)
    return true; // Serial USB
  if (pin == ESP32_RX2_PIN || pin == ESP32_TX2_PIN)
    return true;
  if (pin >= 6 && pin <= 11)
    return true; // Flash habitual
  return false;
}

bool validatePin(int pin, bool outputPin) {
  if (isReservedPin(pin))
    return false;
  if (outputPin && isInputOnlyPin(pin))
    return false;
  return true;
}

int parseBoolState(const char *text, bool *ok = NULL) {
  String v = String(text);
  v.toUpperCase();
  if (v == "1" || v == "HIGH" || v == "ON" || v == "TRUE" || v == "ACTIVA" ||
      v == "ACTIVE") {
    if (ok)
      *ok = true;
    return HIGH;
  }
  if (v == "0" || v == "LOW" || v == "OFF" || v == "FALSE" || v == "INACTIVA" ||
      v == "INACTIVE") {
    if (ok)
      *ok = true;
    return LOW;
  }
  if (ok)
    *ok = false;
  return LOW;
}

int findInput(const String &id) {
  for (byte i = 0; i < MAX_INPUTS; i++)
    if (inputs[i].used && inputs[i].id == id)
      return i;
  return -1;
}

int findOutput(const String &id) {
  for (byte i = 0; i < MAX_OUTPUTS; i++)
    if (outputs[i].used && outputs[i].id == id)
      return i;
  return -1;
}

void sampleInput(byte idx) {
  if (!inputs[idx].used)
    return;
  int raw = digitalRead(inputs[idx].pin) == HIGH ? HIGH : LOW;
  unsigned long now = millis();
  if (!inputs[idx].initialized) {
    inputs[idx].rawValue = raw;
    inputs[idx].stableValue = raw;
    inputs[idx].lastRawValue = raw;
    inputs[idx].rawChangedAt = now;
    inputs[idx].initialized = true;
    return;
  }
  inputs[idx].rawValue = raw;
  if (raw != inputs[idx].lastRawValue) {
    inputs[idx].lastRawValue = raw;
    inputs[idx].rawChangedAt = now;
  }
  if (raw != inputs[idx].stableValue &&
      (long)(now - inputs[idx].rawChangedAt) >= (long)INPUT_STABLE_MS)
    inputs[idx].stableValue = raw;
}

void sampleInputs() {
  for (byte i = 0; i < MAX_INPUTS; i++)
    sampleInput(i);
}

int readInputValue(byte idx) {
  sampleInput(idx);
  return inputs[idx].stableValue == HIGH ? HIGH : LOW;
}

void reportInput(byte idx) {
  int value = readInputValue(idx);
  int active = (value == inputs[idx].activeState) ? 1 : 0;
  Serial.print("<IN,STATE,");
  Serial.print(inputs[idx].id);
  Serial.print(',');
  Serial.print(value == HIGH ? 1 : 0);
  Serial.print(',');
  Serial.print(active);
  Serial.println('>');
}

void reportOutput(byte idx) {
  Serial.print("<OUT,STATE,");
  Serial.print(outputs[idx].id);
  Serial.print(',');
  Serial.print(outputs[idx].value == HIGH ? 1 : 0);
  Serial.println('>');
}

void clearInputs() {
  for (byte i = 0; i < MAX_INPUTS; i++)
    inputs[i].used = false;
  sendOk("IN_CLEAR");
}

void clearOutputs() {
  for (byte i = 0; i < MAX_OUTPUTS; i++)
    outputs[i].used = false;
  sendOk("OUT_CLEAR");
}

void clearProgram() {
  running = false;
  stepActive = false;
  currentStep = 0;
  programCount = 0;
  programLoop = false;
  Serial.println("<PROG,LOADED,0>");
}

void stopProgram() {
  if (running) {
    running = false;
    stepActive = false;
    Serial.println("<RUN,STOPPED>");
  } else {
    sendOk("PROG_NO_EN_EJECUCION");
  }
}

bool appendStep(const ProgramStep &step) {
  if (programCount >= MAX_STEPS) {
    sendErr("PROG_LLENO");
    return false;
  }
  programSteps[programCount++] = step;
  Serial.print("<PROG,LOADED,");
  Serial.print(programCount);
  Serial.println('>');
  return true;
}

void sendProgramStatus() {
  Serial.print("<PROG,STATUS,");
  Serial.print(programCount);
  Serial.print(',');
  Serial.print(running ? 1 : 0);
  Serial.print(',');
  Serial.print(programLoop ? 1 : 0);
  Serial.print(',');
  Serial.print(currentStep + 1);
  Serial.println('>');
}

void listInputs() {
  byte count = 0;
  for (byte i = 0; i < MAX_INPUTS; i++) {
    if (!inputs[i].used)
      continue;
    count++;
    Serial.print("<IN,CFG,");
    Serial.print(inputs[i].id);
    Serial.print(',');
    Serial.print(inputs[i].pin);
    Serial.print(',');
    Serial.print(inputs[i].mode == INPUT_PULLUP ? "INPUT_PULLUP" : "INPUT");
    Serial.print(',');
    Serial.print(inputs[i].activeState == HIGH ? "HIGH" : "LOW");
    Serial.println('>');
  }
  Serial.print("<IN,LIST,DONE,");
  Serial.print(count);
  Serial.println('>');
}

void finishProgramCycle() {
  stepActive = false;
  if (programLoop) {
    currentStep = 0;
    Serial.println("<RUN,LOOP>");
  } else {
    running = false;
    Serial.println("<RUN,DONE>");
  }
}

void finishCurrentStep() {
  stepActive = false;
  currentStep++;
  if (currentStep >= programCount)
    finishProgramCycle();
}

bool jumpToStep(int targetOneBased) {
  if (targetOneBased < 1 || targetOneBased > programCount) {
    running = false;
    Serial.println("<RUN,ERROR,SALTO_INVALIDO>");
    return false;
  }
  currentStep = targetOneBased - 1;
  stepActive = false;
  return true;
}

int inputIsActive(const String &id, bool *ok) {
  int idx = findInput(id);
  if (idx < 0) {
    *ok = false;
    return 0;
  }
  int value = readInputValue(idx);
  reportInput(idx);
  *ok = true;
  return value == inputs[idx].activeState ? 1 : 0;
}

void startProgram() {
  if (programCount == 0) {
    Serial.println("<RUN,ERROR,PROGRAMA_VACIO>");
    return;
  }
  running = true;
  currentStep = 0;
  stepActive = false;
  Serial.print("<RUN,START,");
  Serial.print(programCount);
  Serial.println('>');
}

void beginStep(ProgramStep &step) {
  stepActive = true;
  stepStartMs = millis();
  waitUntilMs = stepStartMs;
  Serial.print("<RUN,STEP,");
  Serial.print(currentStep + 1);
  Serial.print(',');
  Serial.print(programCount);
  Serial.print(',');
  Serial.print(stepTypeName(step.type));
  Serial.println('>');

  if (step.type == STEP_POSE) {
    String cmd = "<P";
    for (byte i = 0; i < NUM_CHANNELS; i++)
      cmd += "," + String(step.pose[i]);
    cmd += ">";
    sendToArduino(cmd);
    waitUntilMs = stepStartMs + step.delayMs;
    if (step.delayMs == 0)
      finishCurrentStep();
  } else if (step.type == STEP_OUT) {
    int idx = findOutput(step.id);
    if (idx < 0) {
      running = false;
      Serial.println("<RUN,ERROR,SALIDA_NO_ENCONTRADA>");
      return;
    }
    outputs[idx].value = step.value ? HIGH : LOW;
    digitalWrite(outputs[idx].pin, outputs[idx].value);
    reportOutput(idx);
    finishCurrentStep();
  } else if (step.type == STEP_WAIT_IN) {
    int idx = findInput(step.id);
    if (idx < 0) {
      running = false;
      Serial.println("<RUN,ERROR,ENTRADA_NO_ENCONTRADA>");
      return;
    }
    waitUntilMs = stepStartMs + step.timeoutMs;
    lastWaitInputReportMs = stepStartMs;
    reportInput(idx);
  } else if (step.type == STEP_PAUSE) {
    waitUntilMs = stepStartMs + step.durationMs;
    if (step.durationMs == 0)
      finishCurrentStep();
  } else if (step.type == STEP_SPEED) {
    sendToArduino("<S," + String(step.speed) + ">");
    sendToArduino("<V," + String(step.v1) + "," + String(step.v2) + "," +
                  String(step.v3) + ">");
    sendToArduino("<A," + String(step.accel) + ">");
    finishCurrentStep();
  } else if (step.type == STEP_HOME) {
    sendToArduino("<H>");
    finishCurrentStep();
  } else if (step.type == STEP_IF_INPUTS) {
    bool ok1 = false, ok2 = false;
    int active1 = inputIsActive(step.id, &ok1);
    int active2 = inputIsActive(step.id2, &ok2);
    if (!ok1 || !ok2) {
      running = false;
      Serial.println("<RUN,ERROR,ENTRADA_NO_ENCONTRADA>");
      return;
    }
    int target = (active1 && active2) ? step.targetBoth
                 : ((active1 || active2) ? step.targetOne : step.targetElse);
    if (target <= 0)
      finishCurrentStep();
    else
      jumpToStep(target);
  } else if (step.type == STEP_JUMP) {
    jumpToStep(step.targetBoth);
  }
}

void serviceProgram() {
  if (!running)
    return;
  if (currentStep >= programCount) {
    finishProgramCycle();
    return;
  }

  ProgramStep &step = programSteps[currentStep];
  unsigned long now = millis();
  if (!stepActive) {
    beginStep(step);
    return;
  }

  if (step.type == STEP_POSE || step.type == STEP_PAUSE) {
    if ((long)(now - waitUntilMs) >= 0)
      finishCurrentStep();
  } else if (step.type == STEP_WAIT_IN) {
    int idx = findInput(step.id);
    if (idx < 0) {
      running = false;
      Serial.println("<RUN,ERROR,ENTRADA_NO_ENCONTRADA>");
      return;
    }
    int value = readInputValue(idx);
    int active = (value == inputs[idx].activeState) ? 1 : 0;
    if ((long)(now - lastWaitInputReportMs) >=
        (long)WAIT_INPUT_REPORT_INTERVAL_MS) {
      reportInput(idx);
      lastWaitInputReportMs = now;
    }
    if (active == step.state) {
      reportInput(idx);
      finishCurrentStep();
    } else if (step.timeoutMs > 0 && (long)(now - waitUntilMs) >= 0) {
      reportInput(idx);
      running = false;
      Serial.print("<RUN,ERROR,TIMEOUT_ENTRADA,step=");
      Serial.print(currentStep + 1);
      Serial.print(",id=");
      Serial.print(step.id);
      Serial.print(",esperado=");
      Serial.print(step.state ? "ACTIVE" : "INACTIVE");
      Serial.print(",valor=");
      Serial.print(value == HIGH ? 1 : 0);
      Serial.print(",activo=");
      Serial.print(active);
      Serial.println('>');
    }
  }
}

void handleEsp32Command(String frame) {
  frame.trim();
  String body = frame.substring(1, frame.length() - 1);
  char buffer[256];
  body.toCharArray(buffer, sizeof(buffer));
  char *parts[24];
  byte count = splitCsv(buffer, parts, 24);
  if (count == 0) {
    sendErr("COMANDO_VACIO");
    return;
  }

  String cmd = String(parts[0]);
  cmd.toUpperCase();

  if (cmd == "PING") {
    Serial.println("<ESP32,READY>");
  } else if (cmd == "STATUS") {
    Serial.print("<OK,STATUS,inputs=");
    Serial.print(MAX_INPUTS);
    Serial.print(",outputs=");
    Serial.print(MAX_OUTPUTS);
    Serial.print(",steps=");
    Serial.print(programCount);
    Serial.print(",running=");
    Serial.print(running ? 1 : 0);
    Serial.print(",loop=");
    Serial.print(programLoop ? 1 : 0);
    Serial.println('>');
  } else if (cmd == "IN") {
    String action = count > 1 ? String(parts[1]) : "";
    action.toUpperCase();
    if (action == "CLEAR")
      clearInputs();
    else if (action == "LIST")
      listInputs();
    else if (action == "ADD") {
      if (count != 6) {
        sendErr("IN_ADD_PARAMETROS");
        return;
      }
      String id = parts[2];
      int pin = atoi(parts[3]);
      String modeText = String(parts[4]);
      modeText.toUpperCase();
      bool activeOk = false;
      int activeState = parseBoolState(parts[5], &activeOk);
      if (!validatePin(pin, false)) {
        sendErr("PIN_INVALIDO");
        return;
      }
      if (!(modeText == "INPUT" || modeText == "INPUT_PULLUP")) {
        sendErr("MODO_ENTRADA_INVALIDO");
        return;
      }
      if (!activeOk) {
        sendErr("ACTIVO_INVALIDO");
        return;
      }
      int idx = findInput(id);
      if (idx < 0) {
        for (byte i = 0; i < MAX_INPUTS; i++)
          if (!inputs[i].used) {
            idx = i;
            break;
          }
      }
      if (idx < 0) {
        sendErr("ENTRADAS_LLENO");
        return;
      }
      inputs[idx].used = true;
      inputs[idx].id = id;
      inputs[idx].pin = pin;
      inputs[idx].mode = (modeText == "INPUT_PULLUP") ? INPUT_PULLUP : INPUT;
      inputs[idx].activeState = activeState;
      inputs[idx].initialized = false;
      pinMode(pin, inputs[idx].mode);
      sampleInput(idx);
      reportInput(idx);
      sendOk("IN_ADD");
    } else if (action == "READ") {
      if (count != 3) {
        sendErr("IN_READ_PARAMETROS");
        return;
      }
      int idx = findInput(parts[2]);
      if (idx < 0) {
        sendErr("ENTRADA_NO_ENCONTRADA");
        return;
      }
      reportInput(idx);
    } else if (action == "REMOVE") {
      if (count != 3) {
        sendErr("IN_REMOVE_PARAMETROS");
        return;
      }
      int idx = findInput(parts[2]);
      if (idx < 0) {
        sendErr("ENTRADA_NO_ENCONTRADA");
        return;
      }
      inputs[idx].used = false;
      sendOk("IN_REMOVE");
    } else
      sendErr("IN_ACCION_DESCONOCIDA");
  } else if (cmd == "OUT") {
    String action = count > 1 ? String(parts[1]) : "";
    action.toUpperCase();
    if (action == "CLEAR")
      clearOutputs();
    else if (action == "ADD") {
      if (count != 5) {
        sendErr("OUT_ADD_PARAMETROS");
        return;
      }
      String id = parts[2];
      int pin = atoi(parts[3]);
      bool stateOk = false;
      int initial = parseBoolState(parts[4], &stateOk);
      if (!validatePin(pin, true)) {
        sendErr("PIN_INVALIDO");
        return;
      }
      if (!stateOk) {
        sendErr("ESTADO_INICIAL_INVALIDO");
        return;
      }
      int idx = findOutput(id);
      if (idx < 0) {
        for (byte i = 0; i < MAX_OUTPUTS; i++)
          if (!outputs[i].used) {
            idx = i;
            break;
          }
      }
      if (idx < 0) {
        sendErr("SALIDAS_LLENO");
        return;
      }
      outputs[idx].used = true;
      outputs[idx].id = id;
      outputs[idx].pin = pin;
      outputs[idx].value = initial;
      pinMode(pin, OUTPUT);
      digitalWrite(pin, initial);
      reportOutput(idx);
      sendOk("OUT_ADD");
    } else if (action == "SET") {
      if (count != 4) {
        sendErr("OUT_SET_PARAMETROS");
        return;
      }
      int idx = findOutput(parts[2]);
      if (idx < 0) {
        sendErr("SALIDA_NO_ENCONTRADA");
        return;
      }
      bool stateOk = false;
      int value = parseBoolState(parts[3], &stateOk);
      if (!stateOk) {
        sendErr("VALOR_SALIDA_INVALIDO");
        return;
      }
      outputs[idx].value = value;
      digitalWrite(outputs[idx].pin, value);
      reportOutput(idx);
    } else if (action == "REMOVE") {
      if (count != 3) {
        sendErr("OUT_REMOVE_PARAMETROS");
        return;
      }
      int idx = findOutput(parts[2]);
      if (idx < 0) {
        sendErr("SALIDA_NO_ENCONTRADA");
        return;
      }
      outputs[idx].used = false;
      sendOk("OUT_REMOVE");
    } else
      sendErr("OUT_ACCION_DESCONOCIDA");
  } else if (cmd == "PROG") {
    String action = count > 1 ? String(parts[1]) : "";
    action.toUpperCase();
    if (action == "CLEAR")
      clearProgram();
    else if (action == "RUN")
      startProgram();
    else if (action == "STOP")
      stopProgram();
    else if (action == "LOOP") {
      if (count != 3) {
        sendErr("PROG_LOOP_PARAMETROS");
        return;
      }
      bool loopOk = false;
      programLoop = parseBoolState(parts[2], &loopOk) == HIGH;
      if (!loopOk) {
        sendErr("PROG_LOOP_VALOR_INVALIDO");
        return;
      }
      Serial.print("<PROG,LOOP,");
      Serial.print(programLoop ? 1 : 0);
      Serial.println('>');
    }
    else if (action == "STATUS")
      sendProgramStatus();
    else if (action == "ADD") {
      if (count < 3) {
        sendErr("PROG_ADD_PARAMETROS");
        return;
      }
      String type = String(parts[2]);
      type.toUpperCase();
      ProgramStep step;
      step.type = STEP_NONE;
      for (byte i = 0; i < NUM_CHANNELS; i++)
        step.pose[i] = 0;
      step.delayMs = 0;
      step.id = "";
      step.value = 0;
      step.state = 0;
      step.timeoutMs = 0;
      step.durationMs = 0;
      step.speed = 0;
      step.v1 = 0;
      step.v2 = 0;
      step.v3 = 0;
      step.accel = 0;
      step.id2 = "";
      step.targetBoth = -1;
      step.targetOne = -1;
      step.targetElse = -1;
      if (type == "POSE") {
        if (count != 11) {
          sendErr("PROG_POSE_PARAMETROS");
          return;
        }
        step.type = STEP_POSE;
        for (byte i = 0; i < NUM_CHANNELS; i++)
          step.pose[i] = atoi(parts[i + 3]);
        step.delayMs = atol(parts[10]);
      } else if (type == "OUT") {
        if (count != 5) {
          sendErr("PROG_OUT_PARAMETROS");
          return;
        }
        step.type = STEP_OUT;
        step.id = parts[3];
        if (findOutput(step.id) < 0) {
          sendErr("SALIDA_NO_ENCONTRADA");
          return;
        }
        bool ok = false;
        step.value = parseBoolState(parts[4], &ok) == HIGH ? 1 : 0;
        if (!ok) {
          sendErr("VALOR_SALIDA_INVALIDO");
          return;
        }
      } else if (type == "WAIT_IN") {
        if (count != 6) {
          sendErr("PROG_WAIT_PARAMETROS");
          return;
        }
        step.type = STEP_WAIT_IN;
        step.id = parts[3];
        if (findInput(step.id) < 0) {
          sendErr("ENTRADA_NO_ENCONTRADA");
          return;
        }
        String desired = String(parts[4]);
        desired.toUpperCase();
        if (desired == "ACTIVE" || desired == "ACTIVA" || desired == "1")
          step.state = 1;
        else if (desired == "INACTIVE" || desired == "INACTIVA" ||
                 desired == "0")
          step.state = 0;
        else {
          sendErr("ESTADO_ENTRADA_INVALIDO");
          return;
        }
        step.timeoutMs = atol(parts[5]);
      } else if (type == "PAUSE") {
        if (count != 4) {
          sendErr("PROG_PAUSE_PARAMETROS");
          return;
        }
        step.type = STEP_PAUSE;
        step.durationMs = atol(parts[3]);
      } else if (type == "SPEED") {
        if (count != 8) {
          sendErr("PROG_SPEED_PARAMETROS");
          return;
        }
        step.type = STEP_SPEED;
        step.speed = atoi(parts[3]);
        step.v1 = atoi(parts[4]);
        step.v2 = atoi(parts[5]);
        step.v3 = atoi(parts[6]);
        step.accel = atoi(parts[7]);
      } else if (type == "HOME") {
        if (count != 3) {
          sendErr("PROG_HOME_PARAMETROS");
          return;
        }
        step.type = STEP_HOME;
      } else if (type == "IF_INPUTS") {
        if (count != 8) {
          sendErr("PROG_IF_INPUTS_PARAMETROS");
          return;
        }
        step.type = STEP_IF_INPUTS;
        step.id = parts[3];
        step.id2 = parts[4];
        if (findInput(step.id) < 0 || findInput(step.id2) < 0) {
          sendErr("ENTRADA_NO_ENCONTRADA");
          return;
        }
        step.targetBoth = atoi(parts[5]);
        step.targetOne = atoi(parts[6]);
        step.targetElse = atoi(parts[7]);
      } else if (type == "JUMP") {
        if (count != 4) {
          sendErr("PROG_JUMP_PARAMETROS");
          return;
        }
        step.type = STEP_JUMP;
        step.targetBoth = atoi(parts[3]);
      } else {
        sendErr("PROG_TIPO_DESCONOCIDO");
        return;
      }
      appendStep(step);
    } else
      sendErr("PROG_ACCION_DESCONOCIDA");
  } else {
    sendErr("COMANDO_DESCONOCIDO");
  }
}

void handleUsbFrame(const String &frame) {
  if (isArduinoMotionCommand(frame))
    sendToArduino(frame);
  else
    handleEsp32Command(frame);
}

void readFrames(Stream &stream, String &buffer, bool fromUsb) {
  while (stream.available() > 0) {
    char c = stream.read();
    if (c == '\r' || c == '\n')
      continue;
    if (c == '<')
      buffer = "";
    buffer += c;
    if (c == '>') {
      if (fromUsb)
        handleUsbFrame(buffer);
      else
        forwardArduinoFrame(buffer);
      buffer = "";
    }
    if (buffer.length() > 250) {
      buffer = "";
      if (fromUsb)
        sendErr("TRAMA_USB_DEMASIADO_LARGA");
      else
        sendErr("TRAMA_ARDUINO_DEMASIADO_LARGA");
    }
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, ESP32_RX2_PIN, ESP32_TX2_PIN);
  delay(300);
  Serial.println("<ESP32,READY>");
}

void loop() {
  readFrames(Serial, usbBuffer, true);
  readFrames(Serial2, arduinoBuffer, false);
  sampleInputs();
  serviceProgram();
}
