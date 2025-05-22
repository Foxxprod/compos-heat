import serial
import re
import threading
import random
import json
import time
import requests  # Pour envoyer les données à l'API

serial_read_on = False

# Définition des données de la carte Arduino
watertemp = -127
rate = -127
composttemp = -127
composthumid = -127
batterylevel = 100

API_URL = "http://composheat.cloud/api/update_data"  # URL de l'API

def serial_read(port):
    ser = serial.Serial(port, 9600, timeout=1)
    
    pattern = re.compile(r'TX;.*?\$TX')
    while serial_read_on:
        try:
            line = ser.readline().decode('utf-8').strip()
            if not line:
                continue

            matches = pattern.findall(line)
            for match in matches:
                print(match)
                receive_data_from_card(match)

        except Exception as e:
            print("Erreur :", e)


def start_serial_read(port):
    global serial_read_on
    serial_read_on = True
    serial_thread = threading.Thread(target=serial_read, args=(port,))
    serial_thread.start()

def stop_serial_read():
    global serial_read_on
    serial_read_on = False

def receive_data_from_card(data):
    global watertemp, rate, composttemp, composthumid, batterylevel

    try:
        if data.startswith("TX") and data.endswith("$TX"):
            message = data[2:-3]  # Enlever 'TX' au début et '$TX' à la fin
        
        data_parts = message.split(";")
        data_dict = {}

        for part in data_parts:
            if ":" in part:
                key, value = part.split(":", 1)
                data_dict[key] = value

        try:
            batterylevel = float(data_dict.get("batterylevel", -127))
            watertemp = float(data_dict.get("watertemp", -127))
            rate = float(data_dict.get("rate", -127))
            composttemp = float(data_dict.get("composttemp", -127))
            composthumid = float(data_dict.get("composthumid", -127))

            # Envoi des données à l'API
            send_data_to_api()

        except ValueError:
            batterylevel = watertemp = rate = composttemp = composthumid = -127
            print("Valeurs invalides, valeurs par défaut utilisées.")
        
    except Exception as e:
        print("Erreur lors de la récupération des données:", e)

def get_card_data():
    return watertemp, rate, composttemp, composthumid, batterylevel

def get_card_data_test():
    global watertemp, composttemp, composthumid, batterylevel

    watertemp = round(random.uniform(15, 30), 2)
    composttemp = round(random.uniform(20, 60), 2)
    composthumid = round(random.uniform(30, 80), 2)
    batterylevel = round(random.uniform(20, 100), 2)
    rate = round(random.uniform(0, 1), 2)

    return watertemp, composttemp, composthumid, batterylevel, rate

def startpump():
    global rate, ser
    if rate == 0:
        ser.write("startpump".encode('ascii'))

def stoppump():
    global rate, ser
    if rate > 0:
        ser.write("stoppump".encode('ascii'))

def updatecommand(value):
    global ser
    command = f"updatecommand:{value}"
    ser.write(command.encode('ascii'))

def send_data_to_api():
    data = {
        "watertemp": watertemp,
        "rate": rate,
        "composttemp": composttemp,
        "composthumid": composthumid,
        "batterylevel": batterylevel
    }

    try:
        response = requests.post(API_URL, json=data)
        if response.status_code == 200:
            print("Données envoyées à l'API avec succès")
        else:
            print(f"Échec de l'envoi. Code {response.status_code} : {response.text}")
    except Exception as e:
        print("Erreur lors de l'envoi des données à l'API:", e)

if __name__ == "__main__":
    #start_serial_read('/dev/tty.usbmodem143201')
    while True:
        get_card_data() #recuperer les données de la carte via les variables
        send_data_to_api() #envoyer les données a l'api composheat.cloud/api/update_data
        print("Données mise a jour depuis la carte vers le serveur web")
        time.sleep(2) # Attendre 1 seconde avant de lire les données de nouveau
