#!/bin/bash

if [ "$1" == "--help" ] || [ "$1" == "" ]; then
	echo "Test fprotect driver."
	echo "Usage: $0 <board>"
	echo "  e.g. $0 nrf9160dk_nrf9160"
	echo ""
	echo "Will print SUCCESS or FAIL before exiting when running the test."
	echo "Will return an error code unless SUCCESS."
	exit -1
fi

if [ -d "build" ]; then rm -r build; fi
mkdir build
pushd build
cmake -GNinja -DBOARD=$1 ..
nrfjprog --recover
stty -F /dev/ttyACM0 115200 raw noflsh icanon
ninja flash

while read -t 1 l< /dev/ttyACM0; do
	echo $l >> uart.log
done
cat uart.log

popd

if grep -F "NOTE: A BUS FAULT (BFAR addr 0x7000)" build/uart.log && \
	grep -F "Precise data bus error" build/uart.log && \
	grep -F "BFAR Address: 0x7000" build/uart.log
then
	echo
	echo SUCCESS
	echo
	exit 0
else
	echo
	echo FAIL
	echo
	exit 1
fi
