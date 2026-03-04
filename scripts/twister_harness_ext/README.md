# Shared Python's libraries for the `pytest-twister-harness`

The package contains plugins for pytest that can be used in twister tests
and other shared Python modules for testing framework.
It is required to export ``PYTHONPATH`` with the path to `nrf/scripts`
so this shared library is available for pytest tests.

For example:
```sh
cd ncs/nrf
PYTHONPATH=$PYTHONPATH:scripts west twister -vv -ll debug --enable-slow \
--west-flash="--recover" --device-testing -p nrf54l15dk/nrf54l15/cpuapp --device-serial /dev/ttyACM1 \
-T tests/subsys/bootloader/upgrade \
-s mcuboot.upgrade.basic --pytest-args="-k test_upgrade_with_revert"
```
