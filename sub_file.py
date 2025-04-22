import paho.mqtt.client as mqtt
import os
import base64

topic = "topic"
name_topic = "updates/name"
file_topic = "updates/file"
broker_ip = "192.168.137.104"

tmp_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tmp')
os.makedirs(tmp_dir, exist_ok=True)

file_name = None
file_data = None

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
    elif rc == 5:
        print("Connection refused : not authorized")
    else:
        print(f"Connection failed with code: {rc}")

def on_disconnect(client, userdata, rc):
    if rc == 0:
        print(f"Disconnected from broker, code: {rc}")
    elif rc != 0:
        print(f'Unexpected disconnection: {rc}')

def on_message(client, userdata, msg):
    global file_name, file_data
    try:
        payload = msg.payload.decode('utf-8')
        topic = msg.topic

        if topic == name_topic:
            file_name =  payload
        elif topic == file_topic:
            file_data = base64.b64decode(payload)


        print(f"MSG RECEIVED ON TOPIC; {msg.topic}: {msg.payload}")
    except Exception as e:
        print(f"ERROR: DECODING MSG {e}")

    if file_name and file_data:
        file_path = os.path.join(tmp_dir, file_name)
        with open(file_path, 'wb') as f:
            f.write(file_data)
        print(f"FILE RECEIVED as {file_name}")
        file_name = None
        file_data = None

def receive_message_to_broker(broker_ip, username, password, port=1883):
    client = mqtt.Client()

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    client.username_pw_set(username, password)

    client.connect(broker_ip, port=port)
    
    client.subscribe(name_topic)
    client.subscribe(file_topic)
    print(f"SUBSCRIBED TOPIC : {name_topic}")
    print(f"SUBSCRIBED TOPIC : {file_topic}")
    client.loop_forever()

if __name__ == "__main__":

    broker_ip = '192.168.137.104'
    username = 'woong'
    password = 'qkrgodnd'

    receive_message_to_broker(broker_ip, username, password)