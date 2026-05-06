// ESP32 controlador principal - version inicial como puente serie.
// PC/navegador WebSerial <-> ESP32 USB Serial <-> Serial2 <-> Arduino UNO.

const unsigned long BAUD_RATE = 115200;

// Ajustar si el cableado real usa otros pines UART2 del ESP32.
// RX2 recibe desde TX del Arduino. TX2 envia hacia RX del Arduino.
const int ESP32_RX2_PIN = 16;
const int ESP32_TX2_PIN = 17;

String usbBuffer;
String arduinoBuffer;

bool isAllowedCommand(const String &frame) {
  return frame.startsWith("<P,") ||
         frame.startsWith("<S,") ||
         frame.startsWith("<V,") ||
         frame.startsWith("<A,") ||
         frame == "<Q>" ||
         frame == "<H>";
}

String debugSafe(String frame) {
  frame.replace("<", "[");
  frame.replace(">", "]");
  return frame;
}

void sendUsbDebug(const String &prefix, const String &frame) {
  Serial.print("<ESP32,");
  Serial.print(prefix);
  Serial.print(',');
  Serial.print(debugSafe(frame));
  Serial.println('>');
}

void forwardUsbFrame(const String &frame) {
  if (isAllowedCommand(frame)) {
    Serial2.println(frame);
    sendUsbDebug("TX_ARDUINO", frame);
  } else {
    Serial.print(F("<ERR,ESP32_COMANDO_NO_REENVIADO,"));
    Serial.print(debugSafe(frame));
    Serial.println('>');
  }
}

void forwardArduinoFrame(const String &frame) {
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
      if (fromUsb) forwardUsbFrame(buffer);
      else forwardArduinoFrame(buffer);
      buffer = "";
    }

    if (buffer.length() > 160) {
      buffer = "";
      if (fromUsb) Serial.println(F("<ERR,ESP32_TRAMA_USB_DEMASIADO_LARGA>"));
      else Serial.println(F("<ERR,ESP32_TRAMA_ARDUINO_DEMASIADO_LARGA>"));
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
}
