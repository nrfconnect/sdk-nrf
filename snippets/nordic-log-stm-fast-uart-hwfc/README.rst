.. _nordic-log-stm-fast-uart-hwfc:

Nordic Standalone STM logging snippet (_nordic-log-stm-fast-uart-hwfc)
######################################################################

Overview
********

This snippet allows users to build Zephyr with the logging to the Coresight STM
stimulus ports. Data is collected in ETR buffer. Data from ETR buffer is
decoded and human-readable data is output on the UART.
UART speed is set to 1000000 bauds, with hardware flow control enabled,
this prevents ETR buffer overflows at high log rates.
