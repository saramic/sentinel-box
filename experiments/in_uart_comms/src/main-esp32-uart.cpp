#include <Arduino.h>

// UART communicator using ESP32
//
// Wiring (all 3.3V — no level shifting needed):
//   ESP32 GPIO17 (TX2)  →  MAX32630 P3.0 (RX)
//   ESP32 GPIO16 (RX2)  ←  MAX32630 P3.1 (TX)
//   GND                 —  GND
//

// #define UART_BAUD   9600
#define UART_BAUD  57600
#define UART_TX_PIN   17
#define UART_RX_PIN   16

// Message framing
#define MSG_START 0x02  // ASCII STX - start of message
#define MSG_END   0x03  // ASCII ETX - end of message
#define BUF_SIZE  128

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 1000; // ms

uint8_t rxBuf[BUF_SIZE];
int rxBufPos = 0;
bool inMessage = false;

void setup()
{
  // Serial for debug output to computer
  Serial.begin(115200); // for debug output
  Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("[DEBUG] UART bridge ready");
}

void sendMessage(const char* msg) {
  Serial1.write(MSG_START);
  Serial1.print(msg);
  Serial1.write(MSG_END);
  Serial.print("[TX] --> ");
  Serial.println(msg);
}

void handleCompleteMessage(uint8_t* buf, int len) {
  buf[len] = '\0'; // null terminate
  Serial.print("[RX] <-- ");
  Serial.println((char*)buf);
  // Put your application logic here
}

void readIncoming() {
  while (Serial1.available()) {
    uint8_t b = Serial1.read();

    if (b == MSG_START) {
      // Start of a new message - reset buffer
      rxBufPos = 0;
      inMessage = true;

    } else if (b == MSG_END) {
      // Complete message received
      if (inMessage && rxBufPos > 0) {
        handleCompleteMessage(rxBuf, rxBufPos);
      }
      rxBufPos = 0;
      inMessage = false;

    } else if (inMessage) {
      // Middle of a message - accumulate bytes
      if (rxBufPos < BUF_SIZE - 1) {
        rxBuf[rxBufPos++] = b;
      } else {
        // Buffer overflow - discard and reset
        Serial.println("[WARN] RX buffer overflow, discarding message");
        rxBufPos = 0;
        inMessage = false;
      }
    }
    // bytes outside MSG_START/END are silently ignored
    // (handles garbage on startup, noise, partial frames)
  }
}

void loop() {
  // Always check for incoming - non-blocking
  readIncoming();

  // Send a heartbeat every SEND_INTERVAL ms
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    char msg[32];
    snprintf(msg, sizeof(msg), "PING %lu", now);
    sendMessage(msg);
  }

  // Your other application code runs here uninterrupted
  // readIncoming() will catch bytes whenever loop() cycles
}
