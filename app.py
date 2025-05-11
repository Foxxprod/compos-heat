from flask import Flask, render_template, jsonify, request
from serial_conn import start_serial_read, get_card_data, startpump, stoppump, updatecommand, open_serial
import threading
import time

app = Flask(__name__)

# Démarre le thread série une seule fois
# start_serial_read('COM3')
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

# 🔁 Nouvelle route pour démarrer la pompe
@app.route('/api/startpump', methods=['POST'])
def api_startpump():
    try:
        startpump()
        return jsonify({"status": "ok", "message": "Pompe démarrée"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

# 🔁 Nouvelle route pour arrêter la pompe
@app.route('/api/stoppump', methods=['POST'])
def api_stoppump():
    try:
        stoppump()
        return jsonify({"status": "ok", "message": "Pompe arrêtée"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500

# 🔁 Nouvelle route pour mettre à jour la commande
@app.route('/api/command', methods=['POST'])
def api_command():
    try:
        data = request.get_json()
        value = data.get('value')
        if value is None:
            return jsonify({"status": "error", "message": "Aucune valeur fournie"}), 400
        updatecommand(value)
        return jsonify({"status": "ok", "message": f"Commande mise à jour à {value}"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500


if __name__ == '__main__':
    try:
        open_serial('COM3')
    except Exception as e:
        print(e)
    app.run(debug=True)
    


    
