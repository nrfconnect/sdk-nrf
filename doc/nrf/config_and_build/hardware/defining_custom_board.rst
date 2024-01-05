.. _defining_custom_board:

Defining custom board
#####################

Defining your own board is a very common step in application development, because applications are typically designed to run on boards that are not directly supported by the |NCS|, and are often custom designs not available publicly.
To define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

One of the |NCS| applications that lets you add custom boards is :ref:`nrf_desktop`.
See :ref:`nrf_desktop_porting_guide` in the application documentation for details.
