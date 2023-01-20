.. _ug_nrf52_gs:

Getting started with nRF52 Series
#################################

.. contents::
   :local:
   :depth: 2

This section gets you started with your nRF52 Series :term:`Development Kit (DK)` using the |NCS|.
It tells you how to install the :ref:`peripheral_uart` sample and perform a quick test of your DK.

If you have already set up your nRF52 Series DK and want to learn more, see the following documentation:

* :ref:`ug_nrf52` for more advanced topics related to the nRF52 Series if you are already familiar with the |NCS|.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.

.. _nrf52_gs_requirements:

Minimum requirements
********************

Make sure you have all the required hardware and that your computer and mobile device both have one of the supported operating systems.

Hardware
========

* One of the following nRF52 Series development kits:

  * nRF52840 DK
  * nRF52833 DK
  * nRF52 DK

* Micro-USB 2.0 cable

Software
========

On your computer, one of the following operating systems:

* Microsoft Windows 10
* macOS X, latest version
* Ubuntu Linux, latest Long Term Support (LTS) version

On your mobile device, one of the following operating systems:

* Android
* iOS

.. _nrf52_gs_installing_software:

Installing the required software
********************************

On your computer, install `nRF Connect for Desktop`_.
After installing and starting the application, install the Programmer app.

You must also install a terminal emulator, such as :ref:`PuTTY <putty>` or the nRF Terminal, which is part of the `nRF Connect for Visual Studio Code`_ extension.
The steps detailed in :ref:`nrf52_gs_connecting` use PuTTY, but any terminal emulator will work.

On your mobile device, install the `nRF Connect for Mobile`_ application from the corresponding application store.

.. _nrf52_gs_installing_sample:
.. _nrf52_gs_installing_application:

Installing the sample
*********************

You must program and run a precompiled version of the :ref:`peripheral_uart` sample on your development kit to test the functions.

Download the precompiled version of the sample for your DK from the corresponding download page:

* `nRF52840 DK Downloads`_
* `nRF52833 DK Downloads`_
* `nRF52 DK Downloads`_

After downloading the zip archive, extract it to a folder of your choice.
The archive contains the HEX file used to program the sample to your DK.

.. |DK| replace:: nRF52 Series DK

.. program_dk_sample_start

To program the precompiled sample to your development kit, complete the following steps:

1. Open the Programmer app.
#. Connect the |DK| to the computer with a micro-USB cable and turn on the DK.

   **LED1** starts blinking.

#. Click **SELECT DEVICE** and select the DK from the drop-down list.

   .. figure:: images/programmer_select_device1.png
      :alt: Programmer - Select Device

      Programmer - Select Device

   The drop-down text changes to the type of the selected device, with its SEGGER ID below the name.
   The **Device Memory Layout** section also changes its name to the device name, and indicates that the device is connected.
   If the **Auto read memory** option is selected in the **DEVICE** section of the side panel, the memory layout will update.
   If it is not selected and you wish to see the memory layout, click :guilabel:`Read` in the **DEVICE** section of the side panel.

#. Click :guilabel:`Add file` in the **FILE** section, and select **Browse**.
#. Navigate to where you extracted the HEX file and select it.
#. Click the :guilabel:`Erase & write` button in the **DEVICE** section to program the DK.

   Do not unplug or turn off the DK during this process.

.. note::
   If you experience any problems during the process, press ``Ctrl+R`` (``command+R`` on macOS) to restart the Programmer app, and try again.

.. program_dk_sample_end

After you have programmed the sample to the DK, you can connect to it and test the functions.
If you connect to the sample now, you can go directly to Step 2 of :ref:`nrf52_gs_connecting`.

.. _nrf52_gs_connecting:

Connecting to the sample
************************

.. uart_dk_connect_start

You can connect to the sample on the |DK| with a terminal emulator on your computer using :term:`Universal Asynchronous Receiver/Transmitter (UART)`.
This allows you to see the logging information the sample outputs as well as to enter console inputs.

You can use an external UART to USB bridge.
UART communication through the UART to USB CDC ACM bridge is referred to as CDC-UART.
This is different from communication through the Nordic UART Service (NUS) over Bluetooth® Low Energy (LE).

If you have problems connecting to the sample, restart the DK and start over.

To connect using CDC-UART, complete the following steps:

1. Connect the |DK| to the computer with a micro-USB cable, and then turn on the DK.

   **LED1** starts blinking.

#. Open a terminal emulator; this guide uses PuTTY.

   The **PuTTY Configuration** window opens and the **Session** category is chosen in the category selection tree on the left.

#. Select **Serial** as the **Connection type** under **Basic options for your PuTTY session**.

   The text fields above the selection change to **Serial line** and **Speed**.

#. Click the **Terminal** category in the category selection tree to see options controlling the terminal.
#. Enable the following options:

   * Implicit CR in every LF
   * Implicit LF in every CR
   * Local echo: Force on
   * Local line editing: Force on

   .. figure:: ../../getting_started/images/putty.svg
      :alt: PuTTY configuration for sending commands through UART

      PuTTY configuration for sending commands through UART

#. Click the **Serial** category under the **Connection** category in the category selection tree to see options controlling the local serial line.
#. Type the COM port corresponding to your DK in the **Serial line to connect to** field.

   Depending on what devices you have connected to your computer, you might have several choices.
   To find the correct port (in Windows):

   a. Right-click on the Windows Start menu, and select **Device Manager**.
   #. In the **Device Manager** window, scroll down and expand **Ports (COM & LPT)**.
   #. Find the port named *JLink CDC UART Port* and note down the number in parentheses.

      If you have more than one J-Link UART Port, unplug the one that you want to use, plug it back in, and observe which one appeared last.

      Your DK can show up as two consecutive COM ports.
      If this is the case, you need to test which COM port is the correct one.

#. Configure the following settings in the **Configure the serial line** section:

   * **Speed (baud):** 115200
   * **Data bits:** 8
   * **Stop bits:** 1
   * **Parity:** None
   * **Flow control:** RTS/CTS

#. Click :guilabel:`Open`.

   The terminal window opens.

.. uart_dk_connect_end

The connection has now been established.
If you test the sample now, you can go directly to Step 2 of :ref:`nrf52_gs_testing`.

.. _nrf52_gs_testing:

Testing the sample
******************

You can test the :ref:`peripheral_uart` sample on your DK using the `nRF Connect for Mobile`_ application.
The test requires that you have :ref:`connected to the sample <nrf52_gs_connecting>` and have the connected terminal emulator open.

.. testing_dk_start

To perform tests, complete the following steps:

.. tabs::

   .. group-tab:: Android

      1. Connect the |DK| to the computer with a micro-USB cable and turn on the DK.

         **LED1** starts blinking.

      #. Open the nRF Connect for Mobile application on your Android device.
      #. In nRF Connect for Mobile, tap :guilabel:`Scan`.
      #. Find the DK in the list, select it and tap :guilabel:`Connect`.

         The default device name for the Peripheral UART sample is **Nordic_UART_Service**.

      #. When connected, tap the three-dot menu below the device name, and select **Enable CCCDs**.

         This example communicates over Bluetooth Low Energy using the Nordic UART Service (NUS).

         .. figure:: images/nrf52_enable_cccds.png
            :alt: nRF Connect for Mobile - Enable services option

            nRF Connect for Mobile - Enable services option

      #. Tap the three-dot menu next to **Disconnect** and select **Show log**.
      #. On your computer, in the terminal emulator connected to the sample through CDC-UART, type ``hello`` and send it to the DK.

         The text is sent through the |DK| to your mobile device over a Bluetooth LE link.
         The device displays the text in the nRF Connect for Mobile log:

         .. figure:: images/nrf52_connect_log.png
            :alt: nRF Connect for Mobile - Text shown in the log

            nRF Connect for Mobile - Text shown in the log

   .. group-tab:: iOS

      1. Connect the |DK| to the computer with a micro-USB cable and turn on the DK.

         **LED1** starts blinking.

      #. Open the nRF Connect for Mobile application on your iOS device.
      #. If the application does not automatically start scanning, tap the **Play** icon in the upper right corner.
      #. Find the DK in the list and tap the corresponding :guilabel:`Connect` button.
         The default device name for the Peripheral UART sample is **Nordic_UART_Service**.

         This opens a new window with information on the device.

      #. In the new window, select the **Client** tab and scroll to the bottom so you can see the **Client Characteristic Configuration** entry.

         .. figure:: images/nrf52_connect_client_ios.png
            :alt: nRF Connect for Mobile - Client tab

            nRF Connect for Mobile - Client tab

      #. Tap the up arrow button under **Client Characteristic Configuration** to write a value to the sample.

         The **Write Value** window opens.

      #. In this window, select the **Bool** tab and set the toggle to **True**.

         This enables messages sent to the DK to show up in nRF Connect for Mobile.

         .. figure:: images/nrf52_connect_write_ios.png
            :alt: nRF Connect for Mobile - Write Value window

            nRF Connect for Mobile - Write Value window

      #. Tap **Write** to write the command to the DK.

         The **Write Value** window closes.

      #. Select the **Log** tab.
      #. On your computer, in the terminal emulator connected to the sample through CDC-UART, type ``hello`` and send it to the DK.

         The text is sent through the |DK| to your mobile device over a Bluetooth LE link.
         The device displays the text in the nRF Connect for Mobile log:

         .. figure:: images/nrf52_connect_log_ios.png
            :alt: nRF Connect for Mobile - Text shown in the log

            nRF Connect for Mobile - Text shown in the log

.. testing_dk_end

If you have a dongle or a second Nordic Semiconductor DK, you can test the sample :ref:`using a computer <nrf52_computer_testing>` instead of using this process.

Next steps
**********

You have now completed getting started with the nRF52 Series DK.
See the following links for where to go next:

* :ref:`ug_nrf52` documentation for more advanced topics related to the nRF52 Series.
* The :ref:`introductory documentation <getting_started>` for more information on the |NCS| and the development environment.
