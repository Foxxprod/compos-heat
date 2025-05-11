import serial, re, threading, random

serial_read_on = False

#definition des données de la carte arduino
watertemp = -127
rate = -127
composttemp = -127
composthumid = -127
batterylevel = 100

def open_serial(port):
    global ser
    ser = serial.Serial(port, 9600, timeout=1)
    print("Port serial ouvert")
    


def serial_read():
    
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


def start_serial_read():
    global serial_read_on
    serial_read_on = True
    serial_thread = threading.Thread(target=serial_read)
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
            if ":" in part:  # Vérifier si ':' est présent avant de diviser
                key, value = part.split(":", 1)  # Diviser en clé et valeur
                data_dict[key] = value  # Ajouter dans le dictionnaire avec la clé comme nom

        
        try:
            batterylevel = data_dict.get("batterylevel", None)
            watertemp = data_dict.get("watertemp", None)
            rate = data_dict.get("rate", None)
            composttemp = data_dict.get("composttemp", None)
            composthumid = data_dict.get("composthumid", None)
        except ValueError:
            batterylevel = -127
            watertemp = -127
            rate = -127
            composttemp = -127
            composthumid = -127
            print("Impossible de lire les données recues depuis la carte du compost, affectation des valeurs par defaut")
        
    except Exception as e:
        print("Erreur lors de la recuperation des données depuis le module compost:", e)


def get_card_data():
    global watertemp, rate, composttemp, composthumid, batterylevel

    return watertemp, rate, composttemp, composthumid, batterylevel


def get_card_data_test():
    global watertemp, pooltemp, composttemp, composthumid, batterylevel

    # Générer des données aléatoires dans des plages réalistes
    watertemp = round(random.uniform(15, 30), 2)  # Température de l'eau entre 15°C et 30°C
    pooltemp = round(random.uniform(18, 28), 2)   # Température de la piscine entre 18°C et 28°C
    composttemp = round(random.uniform(20, 60), 2) # Température du compost entre 20°C et 60°C
    composthumid = round(random.uniform(30, 80), 2) # Humidité du compost entre 30% et 80%
    batterylevel = round(random.uniform(20, 100), 2) # Niveau de la batterie entre 20% et 100%

    return watertemp, pooltemp, composttemp, composthumid, batterylevel

def startpump():
    global rate, ser
    if rate == 0:
        command = "startpump"
        ser.write(command.encode('ascii'))
        

def stoppump():
    global rate, ser
    if rate > 0:
        command = "stoppump"
        ser.write(command.encode('ascii'))

def updatecommand(value):
    global rate, ser
    command = f"updatecommand:{value}"
    ser.write(command.encode('ascii'))



if __name__ == "__main__":
    """
    start_serial_read('COM3')
    import time
    while True:
        print(get_card_data())

        time.sleep(1)

    """
