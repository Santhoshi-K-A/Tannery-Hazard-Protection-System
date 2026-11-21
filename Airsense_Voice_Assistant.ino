/*
 * Airsense Voice Assistant - ESP32-S3
 * ------------------------------------
 * Hardware:
 *   INMP441  I2S MEMS microphone   (audio IN  / I2S RX)
 *   MAX98357A I2S class-D amplifier (audio OUT / I2S TX)
 *
 * Flow:
 *   1. Press the button (or wake) -> record a few seconds from the mic.
 *   2. POST the recorded WAV to the backend /voice endpoint.
 *   3. Backend does speech-to-text + answers + text-to-speech.
 *   4. Backend returns a WAV, which we play back through the speaker.
 *
 * Requires: ESP32 Arduino core 3.x (uses the ESP_I2S library).
 *   Board: "ESP32S3 Dev Module"
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP_I2S.h>

// ---------------- WiFi / backend ----------------
const char* ssid     = "Airsense";
const char* password = "1234567890";

// Point this at your backend's voice route. The main.py backend is
// served over HTTP on your LAN; update host/port to match.
// Example local: "http://192.168.1.50:5000/voice"
const char* voiceUrl = "http://192.168.1.50:5000/voice";

// ---------------- Pins: INMP441 (microphone, I2S RX) ----------------
#define MIC_SCK   4   // INMP441 SCK  (bit clock)
#define MIC_WS    5   // INMP441 WS   (word select / LRCL)
#define MIC_SD    6   // INMP441 SD   (serial data out -> ESP32 in)

// ---------------- Pins: MAX98357A (speaker, I2S TX) ----------------
#define SPK_BCLK  15  // MAX98357A BCLK
#define SPK_LRC   16  // MAX98357A LRC  (word select)
#define SPK_DIN   7   // MAX98357A DIN  (ESP32 out -> amp)

// ---------------- Button to start a query ----------------
#define BUTTON_PIN 0  // BOOT button by default; use a dedicated push button for production

// ---------------- Audio settings ----------------
const uint32_t SAMPLE_RATE   = 16000;  // 16 kHz is plenty for speech
const int      RECORD_SECONDS = 4;     // how long to listen per query

// Two independent I2S peripherals: one for the mic, one for the speaker.
I2SClass i2sMic;
I2SClass i2sSpk;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void setupAudio() {
  // Microphone (RX): dout = -1, din = MIC_SD, no master clock.
  i2sMic.setPins(MIC_SCK, MIC_WS, -1, MIC_SD, -1);
  if (!i2sMic.begin(I2S_MODE_STD, SAMPLE_RATE,
                    I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("Failed to init I2S microphone!");
    while (true) delay(1000);
  }

  // Speaker (TX): dout = SPK_DIN, din = -1.
  i2sSpk.setPins(SPK_BCLK, SPK_LRC, SPK_DIN, -1, -1);
  if (!i2sSpk.begin(I2S_MODE_STD, SAMPLE_RATE,
                    I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("Failed to init I2S speaker!");
    while (true) delay(1000);
  }
  Serial.println("I2S mic + speaker ready.");
}

// Records RECORD_SECONDS of audio into a freshly malloc'd WAV buffer.
// Caller must free() the returned pointer. Returns nullptr on failure.
uint8_t* recordQuery(size_t* outSize) {
  Serial.println("Listening...");
  uint8_t* wav = i2sMic.recordWAV(RECORD_SECONDS, outSize);
  if (wav == nullptr || *outSize == 0) {
    Serial.println("Recording failed.");
    return nullptr;
  }
  Serial.printf("Recorded %u bytes of WAV.\n", (unsigned)*outSize);
  return wav;
}

// Sends the recorded WAV to the backend and plays back the WAV response.
void askAndSpeak(uint8_t* wav, size_t wavSize) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return;
  }

  HTTPClient http;
  http.begin(voiceUrl);
  http.addHeader("Content-Type", "audio/wav");
  http.setTimeout(20000);  // STT + answer + TTS can take a few seconds

  int code = http.POST(wav, wavSize);
  Serial.printf("Backend responded: %d\n", code);

  if (code == HTTP_CODE_OK) {
    int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    if (len > 0) {
      // Known length: buffer the whole WAV, then play it.
      uint8_t* resp = (uint8_t*)malloc(len);
      if (resp == nullptr) {
        Serial.println("Out of memory for response audio.");
        http.end();
        return;
      }
      int read = 0;
      while (read < len && http.connected()) {
        int avail = stream->available();
        if (avail > 0) {
          read += stream->readBytes(resp + read, min(avail, len - read));
        }
        delay(1);
      }
      Serial.printf("Playing %d bytes of response audio.\n", read);
      i2sSpk.playWAV(resp, read);
      free(resp);
    } else {
      Serial.println("Response had no content length; skipping playback.");
    }
  } else {
    Serial.printf("Voice request failed: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  connectWiFi();
  setupAudio();
  Serial.println("Press the button to ask a question.");
}

void loop() {
  // Trigger a query on button press (active low).
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);  // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      size_t wavSize = 0;
      uint8_t* wav = recordQuery(&wavSize);
      if (wav != nullptr) {
        askAndSpeak(wav, wavSize);
        free(wav);
      }
      // wait for release so we don't re-trigger
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }
  delay(20);
}
