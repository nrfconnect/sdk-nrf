#!/bin/bash

set -e

SCRIPTDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
NCS_BASE="$SCRIPTDIR/../../.."

SOC=${1}

declare -a SOCS=(
	"nrf52805 caaa"
	"nrf52810 qfaa"
	"nrf52811 qfaa"
	"nrf52820 qdaa"
	"nrf52832 ciaa"
	"nrf52832 qfaa"
	"nrf52832 qfab"
	"nrf52833 qdaa"
	"nrf52833 qiaa"
	"nrf52840 qfaa"
	"nrf52840 qiaa"
	"nrf5340 qkaa"
	"nrf54l15 qfaa"
	"nrf9131 laca"
	"nrf9151 laca"
	"nrf9160 sica"
	"nrf9161 laca"
)

HELLO_WORLD="$NCS_BASE/../zephyr/samples/hello_world"

rm -rf $NCS_BASE/boards/testvnd

for soc in "${SOCS[@]}"; do
	read -a socarr <<< "$soc"

	if [ ! -z "$SOC" ] && [ $SOC != ${socarr[0]} ]; then
		echo "Skipping ${socarr[0]} (not requested)"
		continue
	fi

	board=brd_${socarr[0]}_${socarr[1]}

	echo "Generating board: $board"

	west ncs-genboard \
		-o $NCS_BASE/boards \
		-e "testvnd" \
		-b $board \
		-d "Test Board" \
		-s ${socarr[0]} \
		-v ${socarr[1]} \

	echo "Building hello_world for: $board"

	if [[ ${socarr[0]} == nrf52* ]]; then
		west build -p -b $board $HELLO_WORLD
	elif [[ ${socarr[0]} == nrf53* ]]; then
		west build -p -b $board/${socarr[0]}/cpuapp $HELLO_WORLD
		west build -p -b $board/${socarr[0]}/cpuapp/ns $HELLO_WORLD
		west build -p -b $board/${socarr[0]}/cpunet $HELLO_WORLD
	elif [[ ${socarr[0]} == nrf54l* ]]; then
		west build -p -b $board/${socarr[0]}/cpuapp $HELLO_WORLD
		# west build -p -b $board/${socarr[0]}/cpuflpr $HELLO_WORLD
		# west build -p -b $board/${socarr[0]}/cpuflpr/xip $HELLO_WORLD
	elif [[ ${socarr[0]} == nrf91* ]]; then
		west build -p -b $board/${socarr[0]} $HELLO_WORLD
		west build -p -b $board/${socarr[0]}/ns $HELLO_WORLD
	fi

	rm -rf $NCS_BASE/boards/testvnd
done
