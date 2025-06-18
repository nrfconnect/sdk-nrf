# The PSEMI front-end module support
This project implements the MPSL FEM interface.
This project implements the power model for the MPSL power model interface.
It controls the gain of the front-end module using a provided power map table and power limits.
The power map table and power limit ID, which indicates the active region of the power limit table, are stored in non-volatile memory.
The partitions for power map table and power limits are defined in `pm.yml.power_map` when the Partition Manager is enabled.
Otherwise, they are specified in the DTS overlay file.

Additionally, this project extends the MPSL FEM simple_gpio module glue layer by configuring attenuation GPIO pins to control the gain of the front-end module.

This project is designed to be part of nRF Connect SDK, and is highly dependent on its toolchain.

## Usage
To enable FEM PSEMI support, add hot_potato snippet to **west build** command:
```console
west build -b nrf52840dk/nrf52840 -S hot_potato nrf/samples/openthread/cli/
```

## Configuration
- `boards/[board]_hot_potato.overlay` - GPIO pin settings
- `models/power_map/Kconfig` - power map model related settings
- `Kconfig` - project general settings

## Openthread CLI vendor extension
This project includes Openthread CLI vendor extension that can be used for testing purposes.
The Openthread module is connected to the extension through the configuration option `CONFIG_OPENTHREAD_CLI_VENDOR_EXTENSION`.
The option points to a CMake file that includes the extension's source code during Openthread build.
For vendor commands description, look at the [Vendor Cmds README](openthread_vendor/README_VENDOR_CMDS.md).

## Utilities
To merge the application hex file with an exemplary power map table, use the following script:

```console
./scripts/merge_power_map.sh build/merged.hex
```

The script assigns the power map table to a specific address where the partition for the power map will be created.
Ensure that the script settings align with your power map partition configuration.

After merging the hex files, you can flash the device.
Skip rebuild to avoid overwriting the app.
Use the following command:

```console
west flash --skip-rebuild
``` 

## Unit tests
This project contains unit tests covering power map model and power limits implementation.
For more information, look at the [unit tests README](test/unit_tests/README.md).