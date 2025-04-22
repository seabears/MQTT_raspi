import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
    else:
        print(f"Connection failed with code {rc}")

def on_disconnect(client, userdata, rc):
    print("Disconnected from broker, code: ", rc)

def on_publish(client, userdata, mid):
    print(f"Message {mid} published successfully")

def send_message_to_broker(message, brocker_ip, username, password, port=1883):
    client = mqtt.Client()

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish

    client.username_pw_set(username, password)

    client.connect(brocker_ip, port=port)

    client.loop_start()

    client.publish(topic, message)

    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    topic = "message"
    broker_ip = "192.168.137.114"
    username = input("Enter username: ")
    pw = input("Enter pw: ")
    msg = input("Enter msg: ")
    send_message_to_broker(msg, broker_ip, username, pw)

