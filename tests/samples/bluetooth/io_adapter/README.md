# Overview

The  IO Adapter application provides a shell interface for controlling buttons and monitoring LEDs on DevKits using the nRF52840-DK board as IO Adapter controller. It serves as a bridge between test automation systems and the DevKits by providing a standardized way to interact with the board's GPIO pins.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Building and Running](#building-and-running)
- [Usage](#usage)
  - [Button Control](#button-control)
    - [Button Push](#button-push)
    - [Button Hold](#button-hold)
    - [Button Release](#button-release)
  - [LED State Reading](#led-state-reading)
  - [LED Blink Monitoring](#led-blink-monitoring)
  - [IO Mapping](#io-mapping)
- [IO Mapping Reference](#io-mapping-reference)
  - [Channel 0](#channel-0)
  - [Channel 1](#channel-1)
  - [Channel 2](#channel-2)
- [Response Format](#response-format)
  - [Success Response](#success-response)
  - [Error Response](#error-response)

# Features

- Button control through GPIO outputs
- LED state monitoring through GPIO inputs
- Shell command interface
- Support for multiple channels (up to 3 devices)
- JSON-formatted command responses
- Configurable button press duration and repeat count
- LED state change monitoring

# Requirements

- nRF52840-DK board
- nRF Connect SDK (NCS)
- Serial terminal for shell access

# Building and Running

1. Build the application:
   ```bash
   west build -p -b nrf52840dk/nrf52840
   ```

2. Flash the application:
   ```bash
   west flash --erase
   ```

3. Connect to the serial terminal:
   ```bash
   picocom /dev/ttyACM0 -b 115200
   ```

# Usage

The application provides several shell commands for interacting with the IO adapter.

## Button Control

### Button Push

```bash
button push <ioadapter_channel> <button_number> [time_ms] [repeats]
```

Presses a button on the specified IO adapter channel.

**Parameters:**
- `ioadapter_channel`: Channel number (0-2)
- `button_number`: Button number (0-3)
- `time_ms`: Press duration in milliseconds (optional, defaults to 500ms, must be > 5ms)
- `repeats`: Number of times to push the button (optional, defaults to 1, must be >= 1)

**Example:**
```bash
button push 0 1 100 3  # Press button 1 on channel 0 for 100ms, 3 times
```

### Button Hold

```bash
button hold <ioadapter_channel> <button_number>
```

Holds a button in the pressed state on the specified IO adapter channel.

**Parameters:**
- `ioadapter_channel`: Channel number (0-2)
- `button_number`: Button number (0-3)

**Example:**
```bash
button hold 0 1  # Hold button 1 on channel 0
```
### Button Release

```bash
button release <ioadapter_channel> <button_number>
```

Releases a previously held button on the specified IO adapter channel.

**Parameters:**
- `ioadapter_channel`: Channel number (0-2)
- `button_number`: Button number (0-3)

**Example:**
```bash
button release 0 1  # Release button 1 on channel 0
```

## LED State Reading

```bash
led <ioadapter_channel> <led_number>
```

Reads the state of an LED on the specified IO adapter channel.

**Parameters:**
- `ioadapter_channel`: Channel number (0-2)
- `led_number`: LED number (0-3)

**Example:**
```bash
led 0 1  # Read state of LED 1 on channel 0
```

## LED Blink Monitoring

```bash
blink <ioadapter_channel> <led_number> <time_ms>
```

Monitors LED state changes over a specified time period.

**Parameters:**
- `ioadapter_channel`: Channel number (0-2)
- `led_number`: LED number (0-3)
- `time_ms`: Monitoring duration in milliseconds (must be > 5)

**Example:**
```bash
blink 0 1 1000  # Monitor LED 1 on channel 0 for 1 second
```

## IO Mapping

```bash
mapping
```

Displays the complete mapping of buttons and LEDs for all IO adapter channels.

## IO Mapping Reference

## Channel 0
### Buttons (Outputs)
- Button 0: Port 0, Pin 11
- Button 1: Port 0, Pin 12
- Button 2: Port 0, Pin 24
- Button 3: Port 0, Pin 25

### LEDs (Inputs)
- LED 0: Port 0, Pin 31
- LED 1: Port 0, Pin 4
- LED 2: Port 0, Pin 30
- LED 3: Port 0, Pin 28

## Channel 1
### Buttons (Outputs)
- Button 0: Port 1, Pin 3
- Button 1: Port 1, Pin 4
- Button 2: Port 1, Pin 8
- Button 3: Port 0, Pin 3

### LEDs (Inputs)
- LED 0: Port 1, Pin 2
- LED 1: Port 1, Pin 5
- LED 2: Port 1, Pin 1
- LED 3: Port 1, Pin 6

## Channel 2
### Buttons (Outputs)
- Button 0: Port 1, Pin 13
- Button 1: Port 1, Pin 14
- Button 2: Port 0, Pin 27
- Button 3: Port 1, Pin 10

### LEDs (Inputs)
- LED 0: Port 1, Pin 12
- LED 1: Port 1, Pin 15
- LED 2: Port 1, Pin 11
- LED 3: Port 0, Pin 2

## Response Format

All commands return JSON-formatted responses:

## Success Response
```json
{
    "status": "success",
    "result": "<value>",
    "msg": "<detailed message>"
}
```

## Error Response
```json
{
    "status": "error",
    "msg": "<error message>"
}
```
