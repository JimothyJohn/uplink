from time import sleep
import serial

PACKET_SIZE = 4096 
ser = serial.Serial('/dev/ttyUSB0',115200, timeout=1)  # open serial port

message = ser.read(PACKET_SIZE)
while message == b"":
    message = ser.read(PACKET_SIZE)
    sleep(1)

ser.close() 

def extract_values(payload: bytes, header: str):
    message = payload.decode() 
    valueChars = message.split(f"{header},")[1].split("\r\n")[0].split(",")[:-1]
    return [float(i) for i in valueChars]

def test_Time():
    timestamps = extract_values(message, "Time (ms)")
    assert len(timestamps) == 128

def test_Reading():
    readings = extract_values(message, "Reading")
    assert len(readings) == 128

def test_Frequency():
    frequencies = extract_values(message, "Frequency (Hz)")
    assert len(frequencies) == 64

def test_Magnitudes():
    magnitudes = extract_values(message, "Magnitudes")
    assert len(magnitudes) == 64 
