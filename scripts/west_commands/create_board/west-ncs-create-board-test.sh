#!/bin/bash

set -e

SCRIPTDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
NCS_BASE="$SCRIPTDIR/../../.."

SOC=${1}

declare -a SOCS=(
	"nRF52805-CAAA"
	"nRF52810-QFAA"
	"nRF52811-QFAA"
	"nRF52820-QDAA"
	"nRF52832-CIAA"
	"nRF52832-QFAA"
	"nRF52832-QFAB"
	"nRF52833-QDAA"
	"nRF52833-QIAA"
	"nRF52840-QFAA"
	"nRF52840-QIAA"
	"nRF5340-QKAA"
	"nRF54L05-QFAA"
	"nRF54L10-QFAA"
	"nRF54L15-QFAA"
	"nRF9131-LACA"
	"nRF9151-LACA"
	"nRF9160-SICA"
	"nRF9161-LACA"
)

HELLO_WORLD="$NCS_BASE/../zephyr/samples/hello_world"

rm -rf $NCS_BASE/boards/testvnd

for soc in "${SOCS[@]}"; do
	soc_parts=(${soc//-/ })
	soc_name=$(echo ${soc_parts[0]} | tr "[:upper:]" "[:lower:]")
	soc_variant=$(echo ${soc_parts[1]} | tr "[:upper:]" "[:lower:]")

	if [ ! -z "$SOC" ] && [ $SOC != ${soc} ]; then
		echo "Skipping $soc (not requested)"
		continue
	fi

	board=brd_${soc_name}_${soc_variant}

	echo "Creating board: $board"

	west ncs-create-board --json-schema-response \
		"{\"board\": \"$board\", \"description\": \"Test Board\", \"vendor\": \"testvnd\", \"soc\": \"$soc\", \"root\": \"$NCS_BASE\"}"

	echo "Building hello_world for: $board"

	if [[ $soc == nRF52* ]]; then
		west build -p -b $board $HELLO_WORLD
	elif [[ $soc == nRF53* ]]; then
		west build -p -b $board/$soc_name/cpuapp $HELLO_WORLD
		west build -p -b $board/$soc_name/cpuapp/ns $HELLO_WORLD
		west build -p -b $board/$soc_name/cpunet $HELLO_WORLD
	elif [[ $soc == nRF54L* ]]; then
		west build -p -b $board/$soc_name/cpuapp $HELLO_WORLD
		if [[ $soc == nRF54L15* ]]; then
			west build -p -b $board/$soc_name/cpuflpr $HELLO_WORLD
			west build -p -b $board/$soc_name/cpuflpr/xip $HELLO_WORLD
		fi
	elif [[ $soc == nRF91* ]]; then
		west build -p -b $board/$soc_name $HELLO_WORLD
		west build -p -b $board/$soc_name/ns $HELLO_WORLD
	fi

	rm -rf $NCS_BASE/boards/testvnd
done
