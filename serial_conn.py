import serial, re, threading

serial_read_on = False

#definition des données de la carte arduino
watertemp = -127
pooltemp = -127
composttemp = -127
composthumid = -127
batterylevel = 100



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
    global watertemp, pooltemp, composttemp, composthumid, batterylevel

    try:

        if data.startswith("TX") and data.endswith("$TX"):
            message = data[2:-3]  # Enlever 'TX' au début et '$TX' à la fin
        
        data_parts = message.split(";")
        data_dict = {}

        for part in data_parts:
            if ":" in part:  # Vérifier si ':' est présent avant de diviser
                key, value = part.split(":", 1)  # Diviser en clé et valeur
                data_dict[key] = value  # Ajouter dans le dictionnaire avec la clé comme nom

        
        try:
            batterylevel = data_dict.get("batterylevel", None)
            watertemp = data_dict.get("watertemp", None)
            pooltemp = data_dict.get("pooltemp", None)
            composttemp = data_dict.get("composttemp", None)
            composthumid = data_dict.get("composthumid", None)
        except ValueError:
            batterylevel = -127
            watertemp = -127
            pooltemp = -127
            composttemp = -127
            composthumid = -127
            print("Impossible de lire les données recues depuis la carte du compost, affectation des valeurs par defaut")
        
    except Exception as e:
        print("Erreur lors de la recuperation des données depuis le module compost:", e)


def get_card_data():
    global watertemp, pooltemp, composttemp, composthumid, batterylevel

    return watertemp, pooltemp, composttemp, composthumid, batterylevel


if __name__ == "__main__":
    start_serial_read('COM3')
    import time
    while True:
        print(get_card_data())

        time.sleep(1)
