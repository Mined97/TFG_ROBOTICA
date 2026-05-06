#include <Servo.h>

// Motion control del brazo didactico.
// Protocolo serie compatible con la interfaz WebSerial del repositorio.

const byte NUM_CHANNELS = 7;
const byte SERVO_PINS[NUM_CHANNELS] = {2, 4, 11, 6, 8, 10, 5};

const int MIN_ANGLES[NUM_CHANNELS] = {0, 0, 0, 0, 0, 0, 95};
const int MAX_ANGLES[NUM_CHANNELS] = {180, 180, 180, 180, 180, 180, 180};
const int HOME_POSE[NUM_CHANNELS] = {90, 90, 90, 90, 90, 90, 120};

Servo servos[NUM_CHANNELS];
int currentPose[NUM_CHANNELS] = {90, 90, 90, 90, 90, 90, 120};
int targetPose[NUM_CHANNELS] = {90, 90, 90, 90, 90, 90, 120};

int globalSpeed = 60;                 // grados/seg para J4-J7
int jointSpeed[3] = {60, 60, 60};     // grados/seg para J1-J3
int acceleration = 300;               // reservado para suavizado futuro

String rxBuffer;
unsigned long lastStepMs = 0;

int clampAngle(byte index, int value) {
  if (value < MIN_ANGLES[index]) return MIN_ANGLES[index];
  if (value > MAX_ANGLES[index]) return MAX_ANGLES[index];
  return value;
}

int clampSpeed(int value) {
  if (value < 1) return 1;
  if (value > 180) return 180;
  return value;
}

int clampAcceleration(int value) {
  if (value < 10) return 10;
  if (value > 1200) return 1200;
  return value;
}

int speedForJoint(byte index) {
  if (index < 3) return jointSpeed[index];
  return globalSpeed;
}

void sendPose() {
  Serial.print(F("<T"));
  for (byte i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(',');
    Serial.print(currentPose[i]);
  }
  Serial.println(F(">"));
}

void sendSpeed() {
  Serial.print(F("<S,"));
  Serial.print(globalSpeed);
  Serial.println(F(">"));
}

void sendJointSpeeds() {
  Serial.print(F("<V,"));
  Serial.print(jointSpeed[0]);
  Serial.print(',');
  Serial.print(jointSpeed[1]);
  Serial.print(',');
  Serial.print(jointSpeed[2]);
  Serial.println(F(">"));
}

void sendAcceleration() {
  Serial.print(F("<A,"));
  Serial.print(acceleration);
  Serial.println(F(">"));
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

void sendFullState() {
  sendPose();
  sendSpeed();
  sendJointSpeeds();
  sendAcceleration();
}

void applyHomeTarget() {
  for (byte i = 0; i < NUM_CHANNELS; i++) {
    targetPose[i] = clampAngle(i, HOME_POSE[i]);
  }
}

void updateMotion() {
  unsigned long now = millis();
  if (lastStepMs == 0) {
    lastStepMs = now;
    return;
  }

  unsigned long elapsed = now - lastStepMs;
  if (elapsed < 10) return;
  lastStepMs = now;

  for (byte i = 0; i < NUM_CHANNELS; i++) {
    int diff = targetPose[i] - currentPose[i];
    if (diff == 0) continue;

    // Paso progresivo simple. La aceleracion queda validada y comunicada,
    // pero se evita una rampa compleja para mantener el firmware didactico.
    int maxStep = (long)speedForJoint(i) * elapsed / 1000;
    if (maxStep < 1) maxStep = 1;

    if (diff > 0) currentPose[i] += min(diff, maxStep);
    else currentPose[i] -= min(-diff, maxStep);

    servos[i].write(currentPose[i]);
  }
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

void handleFrame(String frame) {
  frame.trim();
  if (!frame.startsWith("<") || !frame.endsWith(">")) {
    sendErr(F("TRAMA_INVALIDA"));
    return;
  }

  frame = frame.substring(1, frame.length() - 1);
  char buffer[96];
  frame.toCharArray(buffer, sizeof(buffer));

  char *parts[10];
  byte count = splitCsv(buffer, parts, 10);
  if (count == 0) {
    sendErr(F("COMANDO_VACIO"));
    return;
  }

  char cmd = parts[0][0];

  if (cmd == 'P') {
    if (count != 1 + NUM_CHANNELS) {
      sendErr(F("P_REQUIERE_7_VALORES"));
      return;
    }
    for (byte i = 0; i < NUM_CHANNELS; i++) {
      targetPose[i] = clampAngle(i, atoi(parts[i + 1]));
    }
    sendOk(F("POSE_RECIBIDA"));
  } else if (cmd == 'S') {
    if (count != 2) {
      sendErr(F("S_REQUIERE_1_VALOR"));
      return;
    }
    globalSpeed = clampSpeed(atoi(parts[1]));
    sendSpeed();
  } else if (cmd == 'V') {
    if (count != 4) {
      sendErr(F("V_REQUIERE_3_VALORES"));
      return;
    }
    for (byte i = 0; i < 3; i++) {
      jointSpeed[i] = clampSpeed(atoi(parts[i + 1]));
    }
    sendJointSpeeds();
  } else if (cmd == 'A') {
    if (count != 2) {
      sendErr(F("A_REQUIERE_1_VALOR"));
      return;
    }
    acceleration = clampAcceleration(atoi(parts[1]));
    sendAcceleration();
  } else if (cmd == 'Q') {
    sendFullState();
  } else if (cmd == 'H') {
    applyHomeTarget();
    sendOk(F("HOME"));
  } else {
    sendErr(F("COMANDO_DESCONOCIDO"));
  }
}

void readSerialFrames() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') continue;

    if (c == '<') rxBuffer = "";
    rxBuffer += c;

    if (c == '>') {
      handleFrame(rxBuffer);
      rxBuffer = "";
    }

    if (rxBuffer.length() > 95) {
      rxBuffer = "";
      sendErr(F("TRAMA_DEMASIADO_LARGA"));
    }
  }
}

void setup() {
  Serial.begin(115200);

  for (byte i = 0; i < NUM_CHANNELS; i++) {
    servos[i].attach(SERVO_PINS[i]);
    currentPose[i] = clampAngle(i, HOME_POSE[i]);
    targetPose[i] = currentPose[i];
    servos[i].write(currentPose[i]);
  }

  delay(500);
  sendOk(F("ARDUINO_READY"));
  sendFullState();
}

void loop() {
  readSerialFrames();
  updateMotion();
}
