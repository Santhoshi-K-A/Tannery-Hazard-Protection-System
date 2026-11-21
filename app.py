
import io
import os
import wave

from flask import Flask, request, jsonify, send_file
from flask_socketio import SocketIO, emit

from Api.modules.constants import (
    latest_sensor_value,
    normalairValue,
    system_state_val
)
from Api.modules.system import Debug, Host, Port

from groq import Groq
from piper import PiperVoice


# API configuration
# Set GROQ_API_KEY as an environment variable before running.
groq_client = Groq(
    api_key=os.environ.get("GROQ_API_KEY")
)

STT_MODEL = "whisper-large-v3-turbo"
LLM_MODEL = "llama-3.3-70b-versatile"

# Offline text-to-speech configuration
PIPER_VOICE_PATH = os.environ.get(
    "PIPER_VOICE",
    "voices/en_US-lessac-medium.onnx"
)

_piper_voice = PiperVoice.load(PIPER_VOICE_PATH)

DIDNT_CATCH_MSG = "Sorry, I didn't catch that. Please try again."


# Flask application
app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")


# Receive sensor data
@app.route('/', methods=['POST'])
def receive_data():
    global latest_sensor_value

    try:
        data = request.get_json()

        if 'sensorValue' in data:
            latest_sensor_value = data['sensorValue']

            print(f"Received sensor value: {latest_sensor_value}")

            socketio.emit(
                'sensor_update',
                {'sensorValue': latest_sensor_value}
            )

            return jsonify({
                "status": "success",
                "receivedValue": latest_sensor_value
            }), 200

        return jsonify({
            "status": "error",
            "message": "Invalid payload"
        }), 400

    except Exception as e:
        print(f"Error: {e}")

        return jsonify({
            "status": "error",
            "message": "Invalid request"
        }), 400


# Get latest sensor reading
@app.route('/gasdata', methods=['GET'])
def get_sensor_value():

    if latest_sensor_value is not None:
        return jsonify({
            "status": "success",
            "sensorValue": latest_sensor_value
        }), 200

    return jsonify({
        "status": "error",
        "message": "No data available"
    }), 404


# Receive system state
@app.route('/system', methods=['POST'])
def system_state():
    global system_state_val

    try:
        data = request.get_json()
        print(data)

        if 'systemState' in data:
            system_state_val = data['systemState']

            print(f"Received System State: {system_state_val}")

            return jsonify({
                "status": "success",
                "receivedValue": system_state_val,
                "normalairValue": normalairValue
            }), 200

        return jsonify({
            "status": "error",
            "message": "Invalid payload"
        }), 400

    except Exception as e:
        print(f"Error: {e}")

        return jsonify({
            "status": "error",
            "message": "Invalid request"
        }), 400


# Get system state
@app.route('/state', methods=['GET'])
def get_state():

    if system_state_val is not None:
        return jsonify({
            "status": "success",
            "systemstate": system_state_val,
            "normalairValue": normalairValue
        }), 200

    return jsonify({
        "status": "error",
        "message": "No data available"
    }), 404


# Update normal air threshold
@app.route('/normalvalue', methods=['POST'])
def normal_val():
    global normalairValue

    try:
        data = request.get_json()
        print(data)

        if 'normalairValue' in data:
            normalairValue = data['normalairValue']

            print(f"Received System State: {normalairValue}")

            return jsonify({
                "status": "success",
                "receivedValue": normalairValue
            }), 200

        return jsonify({
            "status": "error",
            "message": "Invalid payload"
        }), 400

    except Exception as e:
        print(f"Error: {e}")

        return jsonify({
            "status": "error",
            "message": "Invalid request"
        }), 400


# Get normal air threshold
@app.route('/normalstate', methods=['GET'])
def get_nstate():

    if normalairValue is not None:
        return jsonify({
            "status": "success",
            "normalairValue": normalairValue
        }), 200

    return jsonify({
        "status": "error",
        "message": "No data available"
    }), 404


# AI voice assistant response
def answer_question(text):
    """Generate an answer using live Airsense sensor data."""

    gas_status = "unknown"

    if latest_sensor_value is not None:
        gas_status = (
            "high"
            if latest_sensor_value > normalairValue
            else "normal"
        )

    context = (
        "You are Airsense, a friendly voice assistant for an "
        "air-quality monitoring device. Answer in one or two "
        "short spoken sentences. Here is the current live data:\n"
        f"- Gas sensor value: {latest_sensor_value} "
        f"(status: {gas_status})\n"
        f"- Normal air threshold: {normalairValue}\n"
        f"- System state: "
        f"{'on' if system_state_val == 1 else 'off'}\n"
        "If the question is unrelated to air quality or the "
        "device, answer briefly and helpfully anyway."
    )

    completion = groq_client.chat.completions.create(
        model=LLM_MODEL,
        messages=[
            {"role": "system", "content": context},
            {"role": "user", "content": text or "Hello"},
        ],
        temperature=0.5,
        max_tokens=120,
    )

    return completion.choices[0].message.content.strip()


# Offline text-to-speech
def synthesize(text):
    """Convert text into a WAV audio buffer using Piper."""

    wav_buf = io.BytesIO()

    with wave.open(wav_buf, "wb") as wav_file:
        _piper_voice.synthesize_wav(text, wav_file)

    wav_buf.seek(0)

    return wav_buf


# Voice assistant endpoint
@app.route('/voice', methods=['POST'])
def voice_assistant():
    """Receive audio, transcribe speech, generate a response,
    and return spoken WAV audio."""

    try:
        audio_bytes = request.get_data()

        if not audio_bytes:
            return jsonify({
                "status": "error",
                "message": "No audio received"
            }), 400

        # Speech-to-text using Groq Whisper
        transcription = groq_client.audio.transcriptions.create(
            file=("query.wav", audio_bytes),
            model=STT_MODEL,
            response_format="text",
        )

        question = (transcription or "").strip()

        print(f"Heard: {question!r}")

        if not question:
            print("No speech detected; sending fallback prompt.")

            return send_file(
                synthesize(DIDNT_CATCH_MSG),
                mimetype="audio/wav"
            )

        # Generate answer using live sensor data
        reply = answer_question(question)

        print(f"Replying: {reply!r}")

        # Convert answer into speech
        return send_file(
            synthesize(reply),
            mimetype="audio/wav"
        )

    except Exception as e:
        print(f"Voice error: {e}")

        return jsonify({
            "status": "error",
            "message": "Voice processing failed"
        }), 500


# SocketIO connection handler
@socketio.on('connect')
def handle_connect():

    print("Client connected!")

    if latest_sensor_value is not None:
        emit(
            'sensor_update',
            {'sensorValue': latest_sensor_value}
        )


# Run application
if __name__ == '__main__':
    socketio.run(
        app,
        debug=Debug,
        host=Host,
        port=Port
    )
