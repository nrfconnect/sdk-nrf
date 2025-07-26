# BLE Master Test App

## Overview

The BLE Master Test App is a Bluetooth Low Energy (BLE) central application designed for testing basic Bluetooth functionality in peripheral samples. This application acts as a tester with central role to validate BLE peripheral applications in the Nordic Connect SDK (NCS).

## Features

- **BLE Central Role**: Implements a BLE central device that can discover and connect to peripheral devices
- **Automatic Device Discovery**: Scans for specific BLE peripheral devices with predefined names
- **Service Discovery**: Performs GATT service discovery on connected devices
- **Connection Management**: Handles connection establishment, maintenance, and disconnection
- **Security Support**: Supports pairing and bonding with peripheral devices
- **UART Communication**: Provides UART interface for debugging and control
- **Test Automation**: Designed for automated testing workflows

## Supported Device Types

The application is configured to discover and test the following BLE peripheral devices:

- Nordic UART Service
- Nordic Throughput
- NCS HIDS Keyboard
- NCS HIDS Mouse
- Nordic LBS (Lightweight Battery Service)
- Test Beacon
- HID devices (via UUID filtering)

## Supported Platforms

The application supports the following Nordic development kits:

- nRF52 DK (nRF52832)
- nRF52833 DK (nRF52833)
- nRF52840 DK (nRF52840)
- nRF5340 DK (nRF5340/cpuapp)
- nRF54H20 DK (nRF54H20/cpuapp)
- nRF54L15 DK (nRF54L15/cpuapp)

## Prerequisites

- Nordic Connect SDK (NCS) v5.0 or later
- Zephyr RTOS
- Supported Nordic development kit
- BLE peripheral device for testing

## Building the Application

### Prerequisites Setup

1. Install the Nordic Connect SDK
2. Set up your development environment
3. Ensure you have the required toolchain installed

### Build Commands

```bash
# Navigate to the application directory
cd nrf/tests/samples/bluetooth/samples_tests_app

# Build for a specific board (example: nRF52840 DK)
west build -b nrf52840dk_nrf52840

```

### Build Configuration

The application uses the following key configurations (defined in `prj.conf`):

- **Bluetooth Stack**: `CONFIG_BT=y`, `CONFIG_BT_CENTRAL=y`
- **GATT Client**: `CONFIG_BT_GATT_CLIENT=y`
- **Scanning**: `CONFIG_BT_SCAN=y`, `CONFIG_BT_SCAN_FILTER_ENABLE=y`
- **Service Discovery**: `CONFIG_BT_GATT_DM=y`
- **Security**: `CONFIG_BT_SMP=y`
- **Settings**: `CONFIG_BT_SETTINGS=y`, `CONFIG_SETTINGS=y`

## Usage

### Flashing the Application

```bash
# Flash the built application
west flash

```

### Running the Application

1. **Power on your development kit** with the flashed application
2. **Connect a serial terminal** to view debug output
3. **Power on a BLE peripheral device** that matches one of the supported device types
4. **Observe the console output** for connection status and test results

### Expected Behavior

The application follows this test sequence:

1. **Initialization**: Bluetooth stack initialization and module setup
2. **Scanning**: Active scanning for supported peripheral devices
3. **Connection**: Automatic connection to discovered devices
4. **Service Discovery**: GATT service discovery and logging
5. **Connection Maintenance**: Maintains connection for 5 seconds
6. **Disconnection**: Graceful disconnection
7. **Reconnection**: Waits 3 seconds, then reconnects
8. **Extended Test**: Maintains connection for 10 seconds
9. **Final Disconnection**: Completes the test cycle
10. **Success Indication**: Prints "SUCCESS" when all steps complete

### Console Output

The application provides detailed console output including:

- Bluetooth initialization status
- Device discovery information
- Connection establishment details
- Service discovery results
- Security pairing information
- Test progress and results

Example output:
```
Starting BLE test application
Bluetooth initialized
UART initialized
Scan module initialized
Scanning successfully started
Filters matched. Address: AA:BB:CC:DD:EE:FF connectable: 1
Connected
Service discovery completed
Disconnect successful
SUCCESS
```

## Testing Workflow

### Manual Testing

1. Build and flash the application to a development kit
2. Power on a BLE peripheral device (e.g., Nordic UART Service sample)
3. Monitor the console output for successful connection and service discovery
4. Verify that the application completes the full test cycle

### Automated Testing

The application is designed for integration with automated test frameworks:

- Supports CI/CD pipeline integration
- Provides structured test results
- Includes test procedure documentation
- Compatible with Nordic's test infrastructure

## Troubleshooting

### Common Issues

1. **No devices discovered**: Ensure peripheral device is advertising with correct name
2. **Connection failures**: Check device compatibility and Bluetooth stack configuration
3. **Service discovery errors**: Verify peripheral device implements expected services
4. **Build errors**: Ensure all dependencies are properly installed

### Debug Information

Enable additional debug output by modifying `prj.conf`:

```conf
# Enable additional Bluetooth debugging
CONFIG_BT_DEBUG_LOG=y
CONFIG_BT_DEBUG_HCI_CORE=y
CONFIG_BT_DEBUG_CONN=y
```

## File Structure

```
samples_tests_app/
├── src/
│   └── main.c              # Main application source code
├── CMakeLists.txt          # Build configuration
├── prj.conf               # Project configuration
├── sample.yaml            # Sample metadata and test configuration
├── Kconfig.sysbuild       # Sysbuild configuration
└── sysbuild/              # Sysbuild configuration files
    ├── CMakeLists.txt
    └── ipc_radio/
        └── prj.conf
```

## Dependencies

- **Zephyr RTOS**: Core operating system
- **Nordic Connect SDK**: Bluetooth stack and drivers
- **Bluetooth GATT Discovery Manager**: Service discovery functionality
- **Bluetooth Scan API**: Device scanning and filtering

## License

This project is licensed under the BSD-5-Clause-Nordic License. See the LICENSE file for details.

## Contributing

For contributions and bug reports, please refer to the Nordic Connect SDK contribution guidelines.

## Support

For technical support and questions:
- Nordic Developer Zone: https://devzone.nordicsemi.com/
- Nordic Connect SDK Documentation: https://developer.nordicsemi.com/nRF_Connect_SDK/
- GitHub Issues: [Nordic Connect SDK Repository](https://github.com/nrfconnect/sdk-nrf)
