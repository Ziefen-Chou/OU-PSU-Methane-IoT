import paho.mqtt.client as mqtt
from datetime import datetime

# MQTT server details (placeholders)
MQTT_BROKER = 'gust.caps.ou.edu'
MQTT_PORT = 8883  # MQTT over TLS (MQTTS)

MQTT_TOPIC = 'aimnet/psu/log/response'  #   Replace with your MQTT topic
MQTT_USERNAME = 'msense_device'
MQTT_PASSWORD = 'wgzs7igsw56s'

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully.")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Connection failed with code {rc}")

# Callback when receiving a message from the MQTT broker
def on_message(client, userdata, msg):
    message = msg.payload.decode()
    FILE_NAME = 'RX_mqtt_2.txt'
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print(f"{current_time} - Received message on topic '{msg.topic}': '{message}' ")

    with open(FILE_NAME, 'a') as file:
        file.write(f"{current_time} - {message}\n")

# Set up the MQTT client
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Set the username and password for MQTT broker
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

# Connect to the MQTT broker over TLS
client.tls_set()
client.connect(MQTT_BROKER, MQTT_PORT, 60)

# Start the loop to process callbacks and manage network traffic
client.loop_forever()
