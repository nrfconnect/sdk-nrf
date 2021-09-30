import serial

ser = serial.Serial('/dev/ttyACM1', baudrate=115200, rtscts=True)
iteration = 0

while True:
    ser.write(f'Hello target :-) #{iteration}\r'.encode('ascii'))
    iteration += 1

    while True:
        line = ser.readline()
        if b'SUCCESSFUL' in line:
            print("All done!")
            exit(0)
        elif b'Confirming' in line:
            break
