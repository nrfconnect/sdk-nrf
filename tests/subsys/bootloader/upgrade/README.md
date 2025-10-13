# Bootloader upgrade tests

The test suite includes various scenarios for testing bootloader functionality.
The tests use pytest framework with Twister harness for automated execution on hardware.
These tests applications are based on the SMP server from `zephyr/samples/subsys/mgmt/mcumgr/smp_svr`.

## Requirements

The tests require the `mcumgr` command-line tool to be installed and available in your system PATH.

Python packages listed in `requirements.txt` are also required. Install them with:

```console
pip install -r requirements.txt
```

Before running the tests, ensure that the `ZEPHYR_BASE` environment variable is set:

```console
export ZEPHYR_BASE='<PATH_TO_NCS>/ncs/zephyr'
```

## Running with Twister

To run a specific test scenario using Twister:

```console
$ZEPHYR_BASE/scripts/twister -vv -ll debug --enable-slow \
--west-flash="--recover" --device-testing -p nrf54l15dk/nrf54l15/cpuapp --device-serial /dev/ttyACM1 \
-T $ZEPHYR_BASE/../nrf/tests/subsys/bootloader/upgrade \
-s mcuboot.upgrade.basic --pytest-args="-k test_upgrade_with_revert"
```

## Running without Twister

You can also run tests manually without Twister:

1. Build the test application:

   ```console
   cd tests/subsys/bootloader/upgrade
   west build -p -b nrf54l15dk/nrf54l15/cpuapp ref_smp_svr -T mcuboot.upgrade.basic --build-dir build-54l
   ```

2. Run the specific test:

   ```console
   pytest --twister-harness -s -v --log-cli-level=DEBUG --device-type=hardware \
   --device-serial=/dev/ttyACM1 --west-flash-extra-args=--recover --build-dir=build-54l \
   -k test_upgrade_with_revert
   ```

**Note:** The exact pytest command is displayed in the console when running Twister.
Once the `twister-out` directory is generated, you can find the build directory in it.

## Manual testing workflow

For manual testing and debugging, you can prepare a second image and test the upgrade process step by step
(it depends on the test scenario), e.g.:

1. Build a second version of the application:

   ```console
   west build -p -b nrf54l15dk/nrf54l15/cpuapp ref_smp_svr -T mcuboot.upgrade.basic \
   --build-dir build-54l-v2 -- '-DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION="2.0.0+0"'
   ```

2. Upload the image using mcumgr:

   ```console
   mcumgr --conntype serial --connstring=/dev/ttyACM1 image upload \
   build-54l-v2/ref_smp_svr/zephyr/zephyr.signed.bin
   ```

3. List images to get the hash:

   ```console
   mcumgr --conntype serial --connstring=/dev/ttyACM1 image list
   ```

4. Test the new image:

   ```console
   mcumgr --conntype serial --connstring=/dev/ttyACM1 image test <hash_of_second_image>
   ```

5. Reset the device and check the console logs:

   ```console
   I: Image index: 0, Swap type: test
   I: Starting swap using offset algorithm.
   I: Bootloader chainload address offset: 0x11000
   I: Image version: v2.0.0
   ```