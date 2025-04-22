import paho.mqtt.client as mqtt
import os
import base64

name_topic = 'updates/name'
file_topic = 'updates/file'
broker_ip = '192.168.137.104'

def make_message(file_path):
    try:
        with open(file_path, 'rb') as file:
            message = base64.b64encode(file.read())
        return message
    except FileNotFoundError as e:
        print("Error: ", e)
        raise

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
    else:
        print(f"Connection failed with code {rc}")

def on_disconnect(client, userdata, rc):
    print("Disconnected from broker, code: ", rc)

def on_publish(client, userdata, mid):
    print(f"Message {mid} published successfully")

def send_file_to_broker(file_path, brocker_ip, username, password, port=1883):
    client = mqtt.Client()

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish

    client.username_pw_set(username, password)

    client.connect(brocker_ip, port=port)

    message  = make_message(file_path)
    file_name = os.path.basename(file_path)

    print(file_name)

    client.loop_start()

    client.publish(name_topic, file_name, qos=2)
    client.publish(file_topic, message, qos=2)

    client.loop_stop()

    print(f"success sending file(updates/name): {file_name}")
    client.disconnect()

if __name__ == "__main__":

    username = input("Enter username: ")
    pw = input("Enter pw: ")
    file_path = input("Enter file path to publish: ")
    send_file_to_broker(file_path, broker_ip, username, pw)

