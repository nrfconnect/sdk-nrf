#!/bin/bash

if [ "$1" == "--help" ] || [ "$1" == "" ]; then
	echo "Test CONFIG_DISABLE_FLASH_PATCH."
	echo "Usage: $0 <board>"
	echo "  e.g. $0 nrf52840dk_nrf52840"
	echo ""
	echo "Will print SUCCESS or FAIL before exiting when running the test."
	echo "Will return an error code unless SUCCESS."
	exit -1
fi

do_test() {
	if [ -d "build" ]; then rm -r build; fi
	mkdir build
	pushd build
	cmake -GNinja $1
	nrfjprog --recover
	ninja flash
	popd

	sleep 1

	# Check NRF_UICR->DEBUGCTRL
	if nrfjprog --memrd 0x10001210 | grep "0x10001210: FFFF00FF"; then
		echo
		echo "SUCCESS"
		echo
		return
	fi
	echo
	echo "FAIL"
	echo
	exit $?
}

do_test "-DBOARD=$1 ${ZEPHYR_BASE}/samples/hello_world -DCONFIG_SECURE_BOOT=y \
	-Db0_CONFIG_DISABLE_FLASH_PATCH=y -Db0_CONFIG_REBOOT=y"

do_test "-DBOARD=$1 ${ZEPHYR_BASE}/samples/hello_world \
	-DCONFIG_BOOTLOADER_MCUBOOT=y -Dmcuboot_CONFIG_DISABLE_FLASH_PATCH=y \
	-Dmcuboot_CONFIG_REBOOT=y"

exit 0
