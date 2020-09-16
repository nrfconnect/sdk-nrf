.. _lib_at_host:

AT host library
###############

|at_host_description|
The AT host library exposes the AT command interface of the cellular modem for supported devices on an external serial interface.

Configuration
=============

The library is enabled and configured entirely in the Kconfig system. The external serial port to use, termination character and other parameters are configurable.

Specifically, if it is desirable for the client to use SMS functionality, the termination character should not be set to <CR>, because that character is used inside the AT+CMGS command.

Usage
=====

The library has no globally available members, and thus no header file. If enabled, the library will initialize with the system. Any input on the configured serial port will be forwarded to the modem upon receipt of the configured termination character(s). Only one command can be processed at any time, and no data must be sent to the serial port while the command is being processed.

.. toctree::
   :maxdepth: 1
   :glob:

   ../../lib/at_host/*
