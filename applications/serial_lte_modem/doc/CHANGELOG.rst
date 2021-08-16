.. _serial_lte_modem_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All notable changes to this project after NCSv1.7.0 are documented in this file.

SLM 1.7.99
**********

* Adapted new AT interface for AT commands by libmodem, remove at_cmd and at_notify.

Limitations
###########

SLM 1.7.99
**********

* No <CR><LF> before AT command final result due to no support by modem.
