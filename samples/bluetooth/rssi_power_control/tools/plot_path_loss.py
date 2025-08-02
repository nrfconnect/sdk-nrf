import serial
import matplotlib

import matplotlib.pyplot as plt
import matplotlib.animation as animation

#Input which COM-port is used
port = input("Enter the serial port where the data is printed(for example, /dev/ttyACM1 or /dev/ttyACM3): ")
print(f"You selected: {port}")

#Configure serial port
ser = serial.Serial(port, 115200, timeout=1)

#Live data buffers
loss_data = []
tx_data = []

#Create figure and axes BEFORE update() uses them
fig, (ax1, ax2) = plt.subplots(2, 1)

def update(frame):
    line = ser.readline().decode('utf-8').strip()

    if line:
        print(f"Raw line: {line}")

        if line.startswith("I: "):
            line = line[2:].strip()

        if line.startswith("Tx:") and "RSSI:" in line:
            try:
                #Extract value
                parts = line.split(',')
                tx_string = parts[0].split(':')[1]
                loss_string = parts[1].split(':')[1]
                tx = int(tx_string[1:-4])
                loss = int(loss_string[1:-4])

                #Store values
                tx_data.append(tx)
                loss_data.append(loss)

                #Keep last 100 entries
                if len(tx_data) > 100:
                    tx_data.pop(0)
                    loss_data.pop(0)

                #Update path loss plot
                ax1.clear()
                ax1.plot(loss_data, label='RSSI [dBm]', color='blue')
                ax1.set_ylabel("RSSI [dBm]")
                ax1.set_title("Live power control & RSSI monitoring")
                ax1.set_ylim(-100, 0)
                ax1.set_xlim(0, 100)
                ax1.legend()
                ax1.grid(True)

                #Update tx plot
                ax2.clear()
                ax2.plot(tx_data, label='Tx [dBm]', color='orange')
                ax2.set_ylabel("Tx [dBm]")
                ax2.set_xlabel("Sample")
                ax2.set_ylim(-50, 10)
                ax2.set_xlim(0, 100)
                ax2.legend()
                ax2.grid(True)

            except (IndexError, ValueError) as e:
                print(f"Failed to parse line: {line}, Error: {e}")
        else:
            print("Ignored line:", line)

#Start live plotting
ani = animation.FuncAnimation(fig, update, interval=200)
plt.tight_layout
plt.show()