import network
import time
from umqtt.simple import MQTTClient
from machine import UART, Pin
import socket

CLIENT_NAME = "pico_client"
SSID = ""
PASS = ""
MQTT_PORT = 1883
UDP_PORT = 4444
BROADCAST_PORT = 5555
BROADCAST_IP = "192.168.1.255"
BROADCAST_MESSAGE = "NetworkDiscovery;Ver=1;"
BOUT = 9600

uart = UART(0, baudrate=BOUT, tx=Pin(0), rx=Pin(1))

serverIP = None

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(SSID, PASS)
while not wlan.isconnected():
    print(".")
    time.sleep(1)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.bind(('0.0.0.0', BROADCAST_PORT))
sock.settimeout(2)


while not serverIP:
    sock.sendto(BROADCAST_MESSAGE.encode(), (BROADCAST_IP, UDP_PORT))
    try:
        data, addr = sock.recvfrom(64)
        print(addr)
        serverIP = addr[0]
    except OSError as e:
        print("No response yet:", e)
        time.sleep(2)

def mqtt_callback(topic, msg):
    print(topic+":"+msg)
    uart.write(topic+":"+msg+"\n")

client = MQTTClient(CLIENT_NAME, serverIP, MQTT_PORT)
client.set_callback(mqtt_callback)
client.connect()

buffer = ""
def handleMessage():
    global buffer
    line = uart.read().decode()
    buffer += line
    while "\n" in buffer:
        line, buffer = buffer.split("\n", 1)
        line = line.strip()
        line = line.split(":", 1)
        print(line)
        if len(line) == 2:
            if line[0] != "log":
                client.publish(line[0], line[1], qos=1)
                uart.write("ACK\n")

registering = True
uart.write("cmd:reg\n")
print("registering")
while registering:
    if not uart.any():
        time.sleep(0.1)
        continue
    line = uart.read().decode()
    buffer += line
    while "\n" in buffer:
        line, buffer = buffer.split("\n", 1)
        line = line.strip()
        line = line.split(":", 1)
        print(line)
        if line[0] == "DONE":
            uart.write("ACK\n")
            registering = False
            break
        elif line[0] == "RDY":
            uart.write("cmd:reg\n")
        elif line[0] == "register":
            client.publish(line[0], line[1], qos=1)
            uart.write("ACK\n")
        elif len(line) == 1:
            client.subscribe(line[0])

while True:
    client.check_msg()
    if uart.any():
        handleMessage()
    time.sleep(0.1)
