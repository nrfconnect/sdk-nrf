#!/bin/bash -u
#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

#
# Script to stop PPP link inside CMUX channel
# using Serial LTE Modem
#

MODEM=/dev/ttyACM0
BAUD=115200
PPP_CMUX=/dev/gsmtty2
AT_CMUX=/dev/gsmtty1
CHATOPT="-vs"

if [[ ! -c $AT_CMUX ]]; then
	echo "AT CMUX channel not found: $AT_CMUX"
	pkill pppd
	pkill ldattach
	exit 1
fi

chat $CHATOPT -t30 "" "AT+CFUN=0" "#XPPP: 0,0" >$AT_CMUX <$AT_CMUX

sleep 1
test -f /var/run/ppp-nrf91.pid && kill $(head -1 </var/run/ppp-nrf91.pid)
pkill ldattach
