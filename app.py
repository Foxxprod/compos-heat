# app.py

from flask import Flask, render_template, jsonify
from serial_conn import start_serial_read, get_card_data
import threading
import time

app = Flask(__name__)

# Démarre le thread série une seule fois
#start_serial_read('COM3')
start_time = time.time()


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/api/temperatures')
def api_temperatures():
    watertemp, rate, composttemp, composthumid, batterylevel = get_card_data()
    elapsed = round(time.time() - start_time, 1)

    return jsonify({
        "elapsed": elapsed,
        "watertemp": float(watertemp),
        "rate": float(rate),
        "composttemp": float(composttemp),
        "composthumid": float(composthumid),
        "batterylevel": float(batterylevel)
    })


if __name__ == '__main__':
    app.run(debug=True)
