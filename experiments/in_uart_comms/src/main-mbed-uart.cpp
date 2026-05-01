// MAX32630FTHR - Mbed OS 6 UART2 communicator
// Pairs with main-esp32-uart.cpp (ESP32 side)
//
// Wiring:
//   This board P3.1 (TX) --> ESP32 GPIO16 (RX2)
//   This board P3.0 (RX) <-- ESP32 GPIO17 (TX2)
//   GND <--> GND
//
// Debug printf goes to USB serial (stdout via Mbed OS 6 console)

#include "mbed.h"
#include <cstdio>
#include <cstring>
using namespace std::chrono_literals;

// Device-to-device UART2 on P3.1(TX) / P3.0(RX)
BufferedSerial uart2(P3_1, P3_0, 57600);

// Message framing
#define MSG_START   0x02    // STX
#define MSG_END     0x03    // ETX
#define BUF_SIZE    128

// RX state
uint8_t rxBuf[BUF_SIZE];
int     rxBufPos  = 0;
bool    inMessage = false;

// Ticker for periodic send
Ticker  sendTicker;
bool    doSend = false;  // flag set by ticker ISR, consumed in main loop

// -----------------------------------------------------------
// Called from Ticker ISR - keep it minimal, just set a flag
// -----------------------------------------------------------
void onSendTick() {
    doSend = true;
}

// -----------------------------------------------------------
// Assemble and send a framed message over uart2
// -----------------------------------------------------------
void sendMessage(const char* msg) {
    char start = MSG_START;
    char end   = MSG_END;
    uart2.write(&start, 1);
    uart2.write(msg, strlen(msg));
    uart2.write(&end, 1);
    printf("[TX] --> %s\r\n", msg);
}

// -----------------------------------------------------------
// Handle a fully received, framed message
// -----------------------------------------------------------
void handleCompleteMessage(uint8_t* buf, int len) {
    buf[len] = '\0';
    printf("[RX] <-- %s\r\n", (char*)buf);
}

// -----------------------------------------------------------
// Drain the uart2 RX buffer - call every loop iteration
// Non-blocking: returns immediately if nothing waiting
// -----------------------------------------------------------
void readIncoming() {
    while (uart2.readable()) {
        uint8_t b;
        if (uart2.read(&b, 1) != 1) break;

        if (b == MSG_START) {
            rxBufPos  = 0;
            inMessage = true;

        } else if (b == MSG_END) {
            if (inMessage && rxBufPos > 0) {
                handleCompleteMessage(rxBuf, rxBufPos);
            }
            rxBufPos  = 0;
            inMessage = false;

        } else if (inMessage) {
            if (rxBufPos < BUF_SIZE - 1) {
                rxBuf[rxBufPos++] = b;
            } else {
                printf("[WARN] RX buffer overflow, discarding\r\n");
                rxBufPos  = 0;
                inMessage = false;
            }
        }
    }
}

// -----------------------------------------------------------
int main() {
    printf("[DEBUG] MAX32630FTHR UART2 bridge ready\r\n");

    // Attach ticker - fires every 1 second
    sendTicker.attach(&onSendTick, 1s);

    uint32_t count = 0;

    while (true) {
        readIncoming();

        if (doSend) {
            doSend = false;
            char msg[32];
            snprintf(msg, sizeof(msg), "PING %lu", count++);
            sendMessage(msg);
        }
    }
}
