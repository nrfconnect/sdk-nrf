#!/bin/bash

set -e

SCRIPTDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
NCS_BASE="$SCRIPTDIR/../../.."

SOC=${1}

declare -a NCS_VERSIONS=(
	"2.0.0"
	"2.1.0"
	"2.2.0"
	"2.3.0"
	"2.4.0"
	"2.5.0"
	"2.6.0"
	"2.7.0"
	"2.8.0"
	"2.8.99"
)

declare -a SOCS=(
	"nrf52805 caaa 2.0.0"
	"nrf52810 qfaa 2.0.0"
	"nrf52811 qfaa 2.0.0"
	"nrf52820 qdaa 2.0.0"
	"nrf52832 ciaa 2.0.0"
	"nrf52832 qfaa 2.0.0"
	"nrf52832 qfab 2.0.0"
	"nrf52833 qdaa 2.5.0"
	"nrf52833 qiaa 2.0.0"
	"nrf52840 qfaa 2.5.0"
	"nrf52840 qiaa 2.0.0"
	"nrf5340 qkaa 2.0.0"
	"nrf54l15 qfaa 2.7.0"
	"nrf9131 laca 2.5.0"
	"nrf9151 laca 2.6.0"
	"nrf9160 sica 2.0.0"
	"nrf9161 laca 2.5.0"
)

HELLO_WORLD="$NCS_BASE/../zephyr/samples/hello_world"

# https://stackoverflow.com/questions/4023830
verlte() {
	printf '%s\n' "$1" "$2" | sort -C -V
}

verlt() {
	! verlte "$2" "$1"
}

rm -rf $NCS_BASE/boards/arm/testvnd $NCS_BASE/boards/testvnd
BRANCH=$(git rev-parse --abbrev-ref HEAD)

for soc in "${SOCS[@]}"; do
	read -a socarr <<< "$soc"

	if [ ! -z "$SOC" ] && [ $SOC != ${socarr[0]} ]; then
		echo "Skipping ${socarr[0]} (not requested)"
		continue
	fi

	for ncs_version in "${NCS_VERSIONS[@]}"; do
		ncs_version_esc="${ncs_version//./_}"

		if verlt $ncs_version ${socarr[2]}; then
			echo "Skipping ${socarr[0]}-${socarr[1]} for NCS $ncs_version (unsupported)"
			continue;
		fi

		board=brd_${socarr[0]}_${socarr[1]}_$ncs_version_esc

		echo "Generating board: $board"

		west ncs-genboard \
			-o $NCS_BASE/boards \
			-e "testvnd" \
			-b $board \
			-d "Test Board" \
			-s ${socarr[0]} \
			-v ${socarr[1]} \
			-n "$ncs_version"
	done
done

for ncs_version in "${NCS_VERSIONS[@]}"
do
	ncs_version_esc="${ncs_version//./_}"

	echo "Switching to NCS $ncs_version"

	LATEST=".*99"
	if [[ ${ncs_version} =~ $LATEST ]]; then
		git checkout main
	else
		git checkout v$ncs_version
	fi
	west update

	for soc in "${SOCS[@]}"; do
		read -a socarr <<< "$soc"

		if [ ! -z "$SOC" ] && [ $SOC != ${socarr[0]} ]; then
			echo "Skipping ${socarr[0]} (not requested)"
			continue
		fi

		if verlt $ncs_version ${socarr[2]}; then
			continue;
		fi

		board=brd_${socarr[0]}_${socarr[1]}_$ncs_version_esc

		echo "Building for: $board"

		if [[ ${socarr[0]} == nrf52* ]]; then
			west build -p -b $board $HELLO_WORLD
		elif [[ ${socarr[0]} == nrf53* ]]; then
			if verlt $ncs_version "2.7.0"; then
				west build -p -b ${board}_cpuapp $HELLO_WORLD
				if verlte "2.6.0" $ncs_version; then
					west build -p -b ${board}_cpuapp_ns $HELLO_WORLD
				fi
				west build -p -b ${board}_cpunet $HELLO_WORLD
			else
				west build -p -b $board/${socarr[0]}/cpuapp $HELLO_WORLD
				west build -p -b $board/${socarr[0]}/cpuapp/ns $HELLO_WORLD
				west build -p -b $board/${socarr[0]}/cpunet $HELLO_WORLD
			fi
		elif [[ ${socarr[0]} == nrf54l* ]]; then
			west build -p -b $board/${socarr[0]}/cpuapp $HELLO_WORLD
			# west build -p -b $board/${socarr[0]}/cpuflpr $HELLO_WORLD
			# west build -p -b $board/${socarr[0]}/cpuflpr/xip $HELLO_WORLD
		elif [[ ${socarr[0]} == nrf91* ]]; then
			if verlt $ncs_version "2.7.0"; then
				west build -p -b $board $HELLO_WORLD
				if verlte "2.6.0" $ncs_version; then
					west build -p -b ${board}_ns $HELLO_WORLD
				fi
			else
				west build -p -b $board/${socarr[0]} $HELLO_WORLD
				west build -p -b $board/${socarr[0]}/ns $HELLO_WORLD
			fi
		fi
	done
done

rm -rf $NCS_BASE/boards/arm/brd_* $NCS_BASE/boards/testvnd
git checkout $BRANCH
west update
