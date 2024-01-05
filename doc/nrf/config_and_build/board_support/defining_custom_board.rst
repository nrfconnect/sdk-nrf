.. _defining_custom_board:

Defining custom board
#####################

Defining your own board is a very common step in application development, because applications are typically designed to run on boards that are not directly supported by the |NCS|, and are often custom designs not available publicly.

Guidelines for custom boards
****************************

To define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

Adding a custom board in the |nRFVSC|
*************************************

The |nRFVSC| lets you add your own boards to your |NCS| project.
Read the `How to work with boards and devices`_ page in the extension documentation for detailed steps.

Application porting guides
**************************

Some :ref:`applications` in the |NCS| provide detailed guides for adapting the application to custom boards:

* :ref:`nrf_desktop` - See :ref:`nrf_desktop_porting_guide` in the application documentation.
* :ref:`nrf53_audio_app` - See :ref:`nrf53_audio_app_adapting` in the application documentation.
* :ref:`nrf_machine_learning_app` - See :ref:`nrf_machine_learning_app_porting_guide` in the application documentation.
