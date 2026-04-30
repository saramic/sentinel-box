#include <Arduino.h>

// UART spy / fake-sensor for MAX32630 fingerprint reader debugging.
//
// Wiring (all 3.3V — no level shifting needed):
//   MAX32630 P3.1 (TX)  →  ESP32 GPIO16 (RX2)
//   MAX32630 P3.0 (RX)  ←  ESP32 GPIO17 (TX2)
//   GND                 —  GND
//
// Modes (uncomment one):
//   MEASURE_BAUD  — times LOW pulses via pulseIn() to detect the actual baud
//                   rate without any baud configuration; reveals CPU clock:
//                   ~104 µs → 9600 baud → CPU at 96 MHz ✓
//                   ~208 µs → 4800 baud → CPU at 48 MHz (clock switch broken)
//   SPY           — decode bytes at TARGET_BAUD and fake ACK on verify_password
// #define MEASURE_BAUD
#define SPY

// #define TARGET_BAUD 57600
#define TARGET_BAUD 9600
// #define TARGET_BAUD 4800
#define RX2_PIN     16
#define TX2_PIN     17
#define ADC_PIN     34

// verify_password command packet (16 bytes)
const uint8_t CMD_VERIFY_PASSWORD[] = {
    0xEF, 0x01,                         // header
    0xFF, 0xFF, 0xFF, 0xFF,             // address
    0x01,                               // PID: command
    0x00, 0x07,                         // length: 7
    0x13, 0x00, 0x00, 0x00, 0x00,       // CMD_VERIFY_PASSWORD + password 0x00000000
    0x00, 0x1B                          // checksum
};

// ACK OK response (12 bytes)
const uint8_t ACK_OK[] = {
    0xEF, 0x01,                         // header
    0xFF, 0xFF, 0xFF, 0xFF,             // address
    0x07,                               // PID: ack
    0x00, 0x03,                         // length: 3
    0x00,                               // confirmation: OK
    0x00, 0x0A                          // checksum
};

static uint8_t buf[64];
static uint8_t buf_len = 0;
static uint32_t total_bytes = 0;
static uint32_t last_heartbeat_ms = 0;
static uint32_t nlast_heartbeat_ms = 0;

void setup() {
    Serial.begin(115200);
    pinMode(RX2_PIN, INPUT);
#ifdef SPY
    Serial.printf("bench_tools: spy at %d baud on GPIO%d/GPIO%d — init Serial2...\r\n",
                  TARGET_BAUD, RX2_PIN, TX2_PIN);
    Serial.flush();
    Serial2.begin(TARGET_BAUD, SERIAL_8N1, RX2_PIN, TX2_PIN);
    Serial.println("bench_tools: Serial2 ready, waiting for bytes");
#else
    Serial.printf("bench_tools: measuring bit width on GPIO%d — power cycle MAX32630\n",
                  RX2_PIN);
#endif
}

void loop() {
    // // Measure pulse widths via ADC transition detection (works with 1.8V logic
    // // that is below ESP32 digital VIH of 2.475V, so digitalRead/pulseIn unreliable).
    // int val = analogRead(ADC_PIN);
    // bool is_high = val > 1000;

    // static bool prev_high = false;
    // static unsigned long t0 = 0;

    // if (is_high != prev_high) {
    //     unsigned long now = micros();
    //     Serial.printf("%s for %lu µs  (ADC=%d)\r\n",
    //                   prev_high ? "HIGH" : "LOW ", now - t0, val);
    //     t0 = now;
    //     prev_high = is_high;
    // }
#ifdef MEASURE_BAUD
    // Measure the first LOW pulse (a UART start bit or zero-bit).
    // pulseIn returns duration in µs; 1 bit period = 1e6 / baud.
    unsigned long width = pulseIn(RX2_PIN, LOW, 5000000UL); // 5 s timeout
    if (width == 0) {
        Serial.println("timeout — no signal on RX pin");
        return;
    }
    // Filter noise: allow 1200–115200 baud = 8–833 µs.
    // Extended upper bound to catch 2400 baud (416 µs) and 1200 baud (833 µs).
    if (width < 8 || width > 833) {
        Serial.printf("noise/glitch: %lu µs (ignored)\n", width);
        return;
    }
    unsigned long estimated_baud = 1000000UL / width;
    // Square-wave formula: CPU MHz = N / width_µs, where N is the delay_cycles count.
    // Firmware step 1 uses N=4800. Steps double: 4800, 9600, 19200, 38400, 76800.
    // Read the first group of pulses (smallest width) and use N=4800.
    Serial.printf("LOW pulse: %lu µs  |  sq-wave asm::delay(4800)~9600 cycles: CPU ~%lu MHz\n",
                  width,
                  9600UL / width); // asm::delay(N) ≈ 2N cycles → CPU MHz = 9600 / width_µs
#endif

#ifdef SPY
    uint32_t now = millis();
    if (now - last_heartbeat_ms >= 2000) {
        Serial.printf("[%lu ms] listening at %d baud — %lu bytes received so far\r\n",
                      now, TARGET_BAUD, total_bytes);
        last_heartbeat_ms = now;
    }

    while (Serial2.available()) {
        uint8_t b = Serial2.read();
        total_bytes++;
        Serial.printf("%02X ", b);

        if (buf_len < sizeof(buf)) buf[buf_len++] = b;

        // Slide window: once we have 16 bytes check for verify_password.
        // If the first byte is not 0xEF (header) discard it and shift the buffer.
        if (buf_len > 0 && buf[0] != 0xEF) {
            memmove(buf, buf + 1, buf_len - 1);
            buf_len--;
        }
        if (buf_len == sizeof(CMD_VERIFY_PASSWORD)) {
            if (memcmp(buf, CMD_VERIFY_PASSWORD, sizeof(CMD_VERIFY_PASSWORD)) == 0) {
                Serial.printf("\r\n→ verify_password detected (total %lu bytes), sending ACK OK\r\n", total_bytes);
                delay(2);
                Serial2.write(ACK_OK, sizeof(ACK_OK));
            } else {
                Serial.printf("\r\n→ 16 bytes with 0xEF header but no match — wrong baud?\r\n");
            }
            buf_len = 0;
        }
    }
#endif
}
