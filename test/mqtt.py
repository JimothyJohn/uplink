import json

import paho.mqtt.client as mqtt

HOST = "broker.hivemq.com"
topic = "/jimothyjohn/current-monitor/test"

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    assert rc == 0


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    data = json.loads(payload)
    assert data["device_name"] == "Olimex"
    assert data["device_id"] == 0 
    assert data["measurement"] == "current" 
    assert data["sample_rate"] == 16 
    assert data["units"] == "A" 
    assert data["peak_hz"] > 0
    assert data["energy"] >= 0
    assert data["a_peak"] >= 0
    assert data["a_rms"] >= 0
    assert data["freq_peak"] >= 0
    assert data["freq_rms"] >= 0
    print(f"Message: {payload}")

def on_subscribe(mqttc, obj, mid, granted_qos):
    assert mid == 1

client = mqtt.Client()
client.on_connect = on_connect
client.on_subscribe = on_subscribe
client.on_message = on_message
client.connect(f"{HOST}", 1883, 60)
client.subscribe(topic)

client.loop_forever()
