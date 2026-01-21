.. _defining_custom_board:

Defining custom board
#####################

.. contents::
   :local:
   :depth: 2

Defining your own board is a very common step in application development, because applications are typically designed to run on boards that are not directly supported by the |NCS| and these boards are often custom designs not available publicly.

This page lists resources about defining custom board files in the |NCS|.
To read about how to program a custom board, see :ref:`programming_custom_board`.

.. note::
   If you want to go through a dedicated training related to some of the topics covered here, enroll in the `nRF Connect SDK Intermediate course`_ in the `Nordic Developer Academy`_. The `Lesson 3 - Adding custom board support`_ shows how to add custom board support using |nRFVSC|.

Guidelines for custom boards
****************************

To define your own board, you can use the following Zephyr guides as reference, since boards are defined in the |NCS| just as they are in Zephyr:

* :ref:`custom_board_definition` is a guide to adding your own custom board to the Zephyr build system.
* :ref:`board_porting_guide` is a complete guide to porting Zephyr to your own board.

Adding a custom board in |nRFVSC|
*********************************

|nRFVSC| lets you add your own boards to your |NCS| project.
Read the `How to work with boards and devices`_ page in the extension documentation for detailed steps.

Application porting guides
**************************

Some :ref:`applications` in the |NCS| provide detailed guides for adapting the application to custom boards:

* :ref:`nrf_desktop` - See :ref:`nrf_desktop_porting_guide` in the application documentation.
* :ref:`nrf53_audio_app` - See :ref:`nrf53_audio_app_adapting` in the application documentation.
* :ref:`nrf_machine_learning_app` - See :ref:`nrf_machine_learning_app_porting_guide` in the application documentation.
