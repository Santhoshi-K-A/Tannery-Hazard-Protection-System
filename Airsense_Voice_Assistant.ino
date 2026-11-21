
/*
 * Airsense Voice Assistant - ESP32-S3
 * ------------------------------------
 * Hardware:
 *   INMP441   I2S MEMS Microphone (Audio Input)
 *   MAX98357A I2S Class-D Amplifier (Audio Output)
 *
 * Workflow:
 *   1. Press the button to initiate recording.
 *   2. Capture audio through the INMP441 microphone.
 *   3. Send recorded WAV audio to the backend /voice endpoint.
 *   4. Backend processes speech and generates a response.
 *   5. Receive the response as WAV audio.
 *   6. Play the response through the speaker.
 *
 * Requirements:
 *   - ESP32 Arduino Core 3.x
 *   - ESP32S3 Dev Module
 *   - ESP_I2S library
 *
 * Configure Wi-Fi credentials and backend URL
 * before deploying the firmware.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP_I2S.h>


// ---------------- Wi-Fi Configuration ----------------

// Replace these placeholders with local credentials.
// Do not commit actual credentials to a public repository.

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";


// ---------------- Backend Configuration ----------------

// Replace with the actual backend address when deploying.
// Example: http://192.168.1.100:5000/voice

const char* voiceUrl =
    "http://YOUR_BACKEND_IP:5000/voice";


// ---------------- INMP441 Microphone Pins ----------------

#define MIC_SCK  4
#define MIC_WS   5
#define MIC_SD   6


// ---------------- MAX98357A Speaker Pins ----------------

#define SPK_BCLK 15
#define SPK_LRC  16
#define SPK_DIN  7


// ---------------- Button Configuration ----------------

#define BUTTON_PIN 0


// ---------------- Audio Configuration ----------------

const uint32_t SAMPLE_RATE = 16000;
const int RECORD_SECONDS = 4;


// ---------------- I2S Instances ----------------

I2SClass i2sMic;
I2SClass i2sSpk;


// ---------------- Wi-Fi Connection ----------------

void connectWiFi() {

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected.");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}


// ---------------- Audio Initialization ----------------

void setupAudio() {

    // Initialize I2S microphone

    i2sMic.setPins(
        MIC_SCK,
        MIC_WS,
        -1,
        MIC_SD,
        -1
    );

    if (!i2sMic.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_MONO
    )) {

        Serial.println("Failed to initialize microphone!");

        while (true) {
            delay(1000);
        }
    }


    // Initialize I2S speaker

    i2sSpk.setPins(
        SPK_BCLK,
        SPK_LRC,
        SPK_DIN,
        -1,
        -1
    );

    if (!i2sSpk.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_MONO
    )) {

        Serial.println("Failed to initialize speaker!");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("I2S microphone and speaker ready.");
}


// ---------------- Audio Recording ----------------

uint8_t* recordQuery(size_t* outSize) {

    Serial.println("Listening...");

    uint8_t* wav = i2sMic.recordWAV(
        RECORD_SECONDS,
        outSize
    );

    if (wav == nullptr || *outSize == 0) {

        Serial.println("Recording failed.");

        return nullptr;
    }

    Serial.printf(
        "Recorded %u bytes of WAV.\n",
        (unsigned)*outSize
    );

    return wav;
}


// ---------------- Backend Communication ----------------

void askAndSpeak(uint8_t* wav, size_t wavSize) {

    if (WiFi.status() != WL_CONNECTED) {

        Serial.println("WiFi not connected.");

        return;
    }

    HTTPClient http;

    if (!http.begin(voiceUrl)) {

        Serial.println("Failed to initialize HTTP connection.");

        return;
    }

    http.addHeader("Content-Type", "audio/wav");

    http.setTimeout(20000);

    int code = http.POST(wav, wavSize);

    Serial.printf(
        "Backend response code: %d\n",
        code
    );


    if (code == HTTP_CODE_OK) {

        int len = http.getSize();

        WiFiClient* stream = http.getStreamPtr();

        if (len > 0) {

            uint8_t* resp = (uint8_t*)malloc(len);

            if (resp == nullptr) {

                Serial.println("Insufficient memory for audio.");

                http.end();

                return;
            }

            int bytesRead = 0;

            while (
                bytesRead < len &&
                http.connected()
            ) {

                int available = stream->available();

                if (available > 0) {

                    bytesRead += stream->readBytes(
                        resp + bytesRead,
                        min(available, len - bytesRead)
                    );
                }

                delay(1);
            }

            Serial.printf(
                "Received %d bytes of response audio.\n",
                bytesRead
            );

            if (bytesRead > 0) {

                i2sSpk.playWAV(
                    resp,
                    bytesRead
                );
            }

            free(resp);

        } else {

            Serial.println(
                "Response has no known content length."
            );
        }

    } else {

        Serial.printf(
            "Voice request failed: %s\n",
            http.errorToString(code).c_str()
        );
    }

    http.end();
}


// ---------------- Setup ----------------

void setup() {

    Serial.begin(115200);

    delay(500);

    pinMode(
        BUTTON_PIN,
        INPUT_PULLUP
    );

    connectWiFi();

    setupAudio();

    Serial.println(
        "Airsense Voice Assistant initialized."
    );

    Serial.println(
        "Press the button to ask a question."
    );
}


// ---------------- Main Loop ----------------

void loop() {

    if (digitalRead(BUTTON_PIN) == LOW) {

        delay(50);

        if (digitalRead(BUTTON_PIN) == LOW) {

            size_t wavSize = 0;

            uint8_t* wav = recordQuery(&wavSize);

            if (wav != nullptr) {

                askAndSpeak(
                    wav,
                    wavSize
                );

                free(wav);
            }

            // Wait until the button is released.

            while (digitalRead(BUTTON_PIN) == LOW) {

                delay(10);
            }
        }
    }

    delay(20);
}
