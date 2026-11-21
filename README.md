# TANNERY HAZARD PROTECTION SYSTEM

### National Children's Science Congress 2026

**A Technology-Based Approach to Environmental Monitoring and Workplace Safety**

---

## 1. Project Overview

The Tannery Hazard Protection System is a student-developed technological prototype designed to address environmental hazards and workplace safety concerns in tannery environments.

The project explores sensor-based environmental monitoring, real-time data communication, automated alerts and voice-assisted interaction. It combines hardware development, embedded programming, backend communication and computational simulation.

The system aims to support early identification of potentially hazardous environmental conditions and improve access to environmental information.

## 2. Problem Addressed

Tannery workplaces can present environmental and occupational health challenges. Delayed awareness of changing environmental conditions can increase workers' exposure to potential hazards.

The project focuses on:
- Monitoring environmental conditions.
- Identifying potentially hazardous changes.
- Improving access to pollution-related information.
- Supporting timely alerts and protective responses.

## 3. Project Objectives

- Develop a sensor-based monitoring prototype.
- Enable real-time communication of sensor readings.
- Provide visual and audible alerts.
- Integrate voice-based interaction.
- Explore computational approaches to environmental hazard monitoring.

## 4. Hardware Architecture

The project incorporates the following hardware components:

| Component | Function |
|---|---|
| ESP32-S3 | Microcontroller and wireless connectivity |
| Gas / air-quality sensor | Provides analogue environmental readings |
| 16×2 I2C LCD | Displays readings and system status |
| Red LED | Alert indication |
| Green LED | Normal-status indication |
| Buzzer | Audible alert |
| INMP441 | Digital voice input |
| MAX98357A | Audio amplification |
| Speaker | Voice response output |
| Push button | Initiates voice recording |

*Component specifications and final hardware configurations are documented separately.*

## 5. Software Architecture

The software architecture combines embedded firmware and a Python-based backend.

### Embedded Firmware

**Language:** C++  
**Platform:** Arduino IDE  
**Controller:** ESP32-S3

Responsibilities:
- Sensor and hardware interfacing.
- Audio recording through the I2S microphone.
- Wi-Fi connectivity.
- HTTP communication with the backend.
- Audio playback through the I2S amplifier.

### Backend System

**Language:** Python  
**Framework:** Flask  
**Communication:** HTTP and Flask-SocketIO

Responsibilities:
- Receiving sensor readings.
- Managing system-state information.
- Providing sensor-data endpoints.
- Handling voice requests.
- Returning generated audio responses.

### Voice Assistant

The voice interaction pipeline uses:

- Groq Whisper for speech recognition.
- Groq language models for response generation.
- Piper for offline text-to-speech synthesis.

The backend uses available sensor and system-state information as context when generating responses.

## 6. Working Principle

The proposed system follows these stages:

1. Environmental readings are acquired through the monitoring hardware.
2. The microcontroller processes and communicates the readings.
3. The backend receives and provides access to the transmitted data.
4. The monitoring interface displays the available information.
5. Configured alerts indicate changes in monitored conditions.
6. The voice interface accepts spoken questions and returns spoken responses.

## 7. Computational Simulation and Validation

Computational simulation and prototype observations are documented separately in this repository.

The supporting materials include:
- Simulation files.
- Computational performance graphs.
- System visualisations.
- Functional observations.
- Supporting validation documentation.

Computational results represent model-based findings and must not be interpreted as direct field measurements or experimentally established industrial performance.

## 8. Digital Documentation

This repository provides access to the project's technical materials:

| Directory / File | Description |
|---|---|
| Source Code | Embedded firmware and backend programming |
| Hardware Components | Component details and specifications |
| Computational Simulation | Simulation files and results |
| Validation | Supporting performance and functional evidence |
| Demonstration | Prototype photographs and videos |

## 9. Development Technologies

- C++ and Arduino
- ESP32-S3
- Python
- Flask
- Flask-SocketIO
- Groq Whisper
- Groq LLM
- Piper TTS
- HTML, CSS and JavaScript
- Computational modelling and visualisation

## 10. Limitations and Future Development

The current work is a student research prototype.

Further development should include:
- Testing under actual tannery environmental conditions.
- Calibration and verification of sensor readings.
- Extended hardware reliability testing.
- Evaluation of communication reliability.
- Further validation of computational models.
- Assessment of practical deployment requirements.

The prototype is not a certified industrial life-safety system.

## 11. Project Significance

The project explores how embedded systems, environmental monitoring, computational tools and accessible communication interfaces can contribute to workplace safety.

It supports the broader objectives of:

- **SDG 3:** Good Health and Well-being.
- **SDG 8:** Decent Work and Economic Growth.
- **SDG 9:** Industry, Innovation and Infrastructure.

---

**Project:** Tannery Hazard Protection System  
**Presented at:** National Children's Science Congress 2026  
**Developer:** K. A. Santhoshi

*This repository is maintained for academic, research and educational documentation.*
