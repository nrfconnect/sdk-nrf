#!/bin/bash -eu
#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

#
# Script to start PPP link inside CMUX channel
# using Serial LTE Modem
#

MODEM=/dev/ttyACM0
BAUD=115200
PPP_CMUX=/dev/gsmtty2
AT_CMUX=/dev/gsmtty1
CHATOPT="-vs"

cleanup() {
	set +eu
	pkill pppd
	pkill ldattach
	echo "Failed to start..."
	exit 1
}
trap cleanup ERR

if [[ ! -c $MODEM ]]; then
	echo "Serial port not found: $MODEM"
	exit 1
fi

stty -F $MODEM $BAUD pass8 raw crtscts clocal

echo "Wait modem to boot"
chat -t5 "" "AT" "OK" <$MODEM >$MODEM || true

echo "Attach CMUX channel to modem..."
ldattach -c $'AT#XCMUX=1\r' GSM0710 $MODEM

sleep 1
stty -F $AT_CMUX clocal

echo "Connect and wait for PPP link..."
test -c $AT_CMUX
chat $CHATOPT -t60 "" "AT+CFUN=1" "OK" "\c" "#XPPP: 1,0" >$AT_CMUX <$AT_CMUX

pppd $PPP_CMUX noauth novj nodeflate nobsdcomp debug noipdefault passive +ipv6 \
		noremoteip local linkname nrf91 defaultroute defaultroute-metric -1
