from flask import Flask, render_template, jsonify, request
import json
import time

app = Flask(__name__)

# Démarre le thread série une seule fois
# start_serial_read('COM3')
start_time = time.time()

# Fonction pour lire les données du fichier JSON
def read_data_from_json():
    try:
        with open("data/card_data.json", "r") as json_file:
            data = json.load(json_file)
            return data
    except Exception as e:
        print(f"Erreur lors de la lecture du fichier JSON: {e}")
        return None

@app.route('/')
def index():
    return render_template('index.html')


@app.route('/api/temperatures')
def api_temperatures():
    data = read_data_from_json()
    if data is None:
        return jsonify({"status": "error", "message": "Erreur lors de la lecture des données"}), 500

    watertemp = data.get("watertemp", -127)
    rate = data.get("rate", -127)
    composttemp = data.get("composttemp", -127)
    composthumid = data.get("composthumid", -127)
    batterylevel = data.get("batterylevel", -127)

    elapsed = round(time.time() - start_time, 1)

    return jsonify({
        "elapsed": elapsed,
        "watertemp": float(watertemp),
        "rate": float(rate),
        "composttemp": float(composttemp),
        "composthumid": float(composthumid),
        "batterylevel": float(batterylevel)
    })


@app.route('/api/startpump', methods=['POST'])
def api_startpump():
    try:
        pass
        return jsonify({"status": "ok", "message": "Pompe démarrée"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/stoppump', methods=['POST'])
def api_stoppump():
    try:
        pass
        return jsonify({"status": "ok", "message": "Pompe arrêtée"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/command', methods=['POST'])
def api_command():
    try:
        data = request.get_json()
        value = data.get('value')
        if value is None:
            return jsonify({"status": "error", "message": "Aucune valeur fournie"}), 400

        return jsonify({"status": "ok", "message": f"Commande mise à jour à {value}"})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500



@app.route('/api/update_data', methods=['POST'])
def api_update_data():
    try:
        incoming_data = request.get_json()

        if not incoming_data:
            return jsonify({"status": "error", "message": "Aucune donnée reçue"}), 400

        # Lire les données actuelles
        data = read_data_from_json()
        if data is None:
            return jsonify({"status": "error", "message": "Impossible de lire les données JSON"}), 500

        # Mise à jour des clés existantes ou ajout de nouvelles
        data.update(incoming_data)

        # Écriture dans le fichier
        with open("data/card_data.json", "w") as json_file:
            json.dump(data, json_file, indent=4)

        return jsonify({"status": "ok", "message": "Données mises à jour", "data": data})

    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=80, debug=True)
