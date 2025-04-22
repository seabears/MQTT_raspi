import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        client.subscribe(userdata['topic'])
        print(f"Subscribed to topic: {userdata['topic']}")
    else:
        print(f"Connection failed with code {rc}")

def on_disconnect(client, userdata, rc):
    print("Disconnected from broker, code: ", rc)

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        print(f"Message received on topic {msg.topic}: {payload}")
    except Exception as e:
        print(f"Error decoding message: {e}")

def receive_message_from_broker(broker_ip, username, password, topic, port=1883):
    userdata = {'topic': topic}
    client = mqtt.Client(userdata=userdata)

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    client.username_pw_set(username, password)
    client.connect(broker_ip, port=port)

    try:
        print("Listening for messages. Press Ctrl+C to exit.")
        client.loop_forever()
    except KeyboardInterrupt:
        print("Interrupted by user, disconnecting...")
        client.disconnect()

if __name__ == "__main__":
    broker_ip = "192.168.137.114"
    topic = "message"
    username = input("Enter username: ")
    password = input("Enter password: ")
    receive_message_from_broker(broker_ip, username, password, topic)
