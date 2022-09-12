.. _slm_extending:

Extending the application
#########################

.. contents::
   :local:
   :depth: 2

You can extend the serial LTE modem application by adding your own AT commands and custom error handling.

Adding AT commands
******************

If you want to implement custom AT commands and add them to the serial LTE modem application, see the parser implementation of the provided proprietary AT commands for reference.
The source files can be found in the :file:`applications/serial_lte_modem/src/` folder.

Complete the following steps to add a parser for your own AT commands:

1. Create a header file in :file:`applications/serial_lte_modem/src/`.
   The file must expose the following functions:

   * ``*_init()`` - Initialize the parser.
   * ``*_uninit()`` - Uninitialize the parser.

   See the files for existing AT command parsers for reference.
#. Implement your AT strings and handlers in a corresponding :file:`.c` file.
   See the files for existing AT command parsers for reference.

   Pay attention to the following requirements:

   * The names of new AT commands should start with ``AT#X``.
   * Before entering idle state, the serial LTE modem application will call the uninit function.
     Make sure that the uninit function exits successfully.
     Otherwise, the application cannot enter idle state.
#. Edit :file:`applications/serial_lte_modem/src/slm_at_commands.c` and add calls to your functions:

   a. In ``slm_at_init()``, add a call to your init function.
   #. In ``slm_at_uninit()``, add a call to your uninit function.
   #. Declare your command handler like those of other service modules.
   #. In ``slm_at_cmd_list``, add the mapping of your AT command and its handler.

If you discover any bugs in the :file:`main.c`, :file:`slm_at_host.h`, or :file:`slm_at_host.c` files, report them on the `DevZone`_.

Error handling
**************

The Serial LTE modem application returns error codes that are defined in :file:`zephyr/lib/libc/minimal/include/errno.h`.
See the comments in that file for information about a specific error.

If an error occurs during TCP/IP processing, the socket is usually closed.
See the `POSIX Programmer's Manual`_ for information about the errors returned by each API.
