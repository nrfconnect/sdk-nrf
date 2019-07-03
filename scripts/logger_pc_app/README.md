# ZephyrLogParser

Small application written in Python 3, which purpose is to collect data bytes from target device via backends (e.g GPIO parallel or UART) and parse them.
It allows faster logging rate, due to fact that there will be no need for usage of costly string-parsing functions on target device side. All human-readable strings are constructed on PC host.

## Getting started

This section tells about how to prepare your system in order to run application.

### Prerequisites

Application is written in pure Python 3, nevertheless it uses some external packages. These can be obtained using following command:

```
pip3 install saleae pyelftools pyserial pynrfjprog
```

Clone this repository into chosen directory on your local machine.

### Launching

First of all, enter directory containg cloned repository and open terminal.

Check application available settings by typing:

```
python3 zephyrlogparser.py -h
```

List of application arguments should appear, together with their default values. It is possible to change them, my modifying "logparser_config.json" file.
These settings can be also changed by launching script with argument relating to setting to change. Moreover, some of them can be altered during runtime.

In order to run application, enter directory containg cloned repository, open terminal and type:

```
python3 zephyrlogparser.py --elf [ELF_FILE_LOCATION] --gpio-on --uart-on --rtt-on
```

Where ELF_FILE_LOCATION should be a path to valid .elf file - the same used to program target device.

A following output should appear on the terminal:

```
[INFO]  <UART>  UART Backend Initialized
[INFO]  <GPIO>  GPIO Backend Initialized
[INFO]  <RTT>   RTT Backend Initialized
[INFO]  <UART>  Waiting for TCP connection on port 1338
[INFO]  <GPIO>  Waiting for TCP connection on port 1337
[INFO]  <RTT>   Waiting for TCP connection on port 1339
>>
```

Application is up-and-running. Read next sections to know how to use backend of your interest.

### Usage

Application behaves similarly to console terminal. Backends are controlled using text commands, confirmed by "Enter" click. Each command has to be preceded
with backend name, e.g. to change Saleae acquisition time to two seconds:

```
>> gpio time 2.0
```

Moreover, it is possible to control application using TCP terminal, e.g. PuTTY. Connection port can be provided during initialization or in config file.
There is a different TCP socket available for each backend, so all I/O done using this method is connected directly to specific backend process.
It means it can be used to filter target device output by backends. Note that commands dispatcher over TCP does not have to be preceded by backend name
(as they are already connected to relevant backend).

Must-have PuTTY settings:
 - Session:
    - Connection type:                     Raw
    - Port:                                Same as set in config file or during startup
 - Terminal:
    - Local echo:                          Force off
    - Local line edit:                     Force off
    - Keyboard:
        - The function keys and keypad:     Linux

List of available commands is printed after typing "help" command.

#### GPIO backend

In order to use GPIO parallel backend, Saleae Logic Analyzer has to be used. It is recommended to use Saleae Pro, due to higher sampling rates.

9 GPIO pins are needed - 8 for data and 1 for clock signal. These are selected using KConfig file and can be changed by typing "ninja menuconfig"
in target device source location. Pins should be connected in ascending order - INPUT1 to LSB, ... , INPUT8 to MSB and
clock signal to INPUT9, where INPUTx are consecutive Saleae inputs. Do not forget about connecting Saleae GROUND pin to target device ground.

If not already done, download and install Saleae Logic software from offical site. Launch it, enter "Options" (in right upper corner) and then "Preferences".
Under "Developer" tab tick "Enable scripting socket server", with default listening port.

Launch ZephyrLogParser:

```
python3 zephyrlogparser.py --elf [ELF_FILE_LOCATION] --gpio-on
```

Type "init" command to connect to Saleae Logic software, and then "start" to launch data acquisition. Type "parse" to start parsing data dump.

Remember about target device reset after starting data collection, due to fact that synchronization frame is used to prevent log corruption and it is send
during logger system initialization.

#### UART backend

In order to use UART backend connect target device with PC host via USB and launch application:

```
python3 zephyrlogparser.py --elf [ELF_FILE_LOCATION] --uart-on --baud [BAUDRATE] --serial [SERIAL DEVICE PATH]
```

Remember about target device reset after starting data collection, due to fact that synchronization frame is used to prevent log corruption and it is send
during logger system initialization.

#### RTT backend

Before using RTT make sure JLink software is installed. Then connect target device with PC host via USB and launch application:

```
python3 zephyrlogparser.py --elf [ELF_FILE_LOCATION] --rtt-on
```

Remember about target device reset after starting data collection, due to fact that synchronization frame is used to prevent log corruption and it is send
during logger system initialization.
