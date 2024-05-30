.. _nrf52_computer_testing:

Testing with multiple nRF52 Series devices
##########################################

If you have an nRF52 Series DK with the :ref:`peripheral_uart` sample and either a dongle or second Nordic Semiconductor development kit that supports Bluetooth Low Energy, you can test the sample on your computer.
Use the Bluetooth Low Energy app in `nRF Connect for Desktop`_ for testing.

To perform the test, :ref:`connect to the nRF52 Series DK <nrf52_gs_connecting>` and complete the following steps:

1. Connect the dongle or second development kit to a USB port of your computer.
#. Open the Bluetooth Low Energy app.
#. Select the serial port that corresponds to the dongle or second development kit.
   Do not select the kit you want to test.

   .. note::
      If the dongle or second development kit has not been used with the Bluetooth Low Energy app before, you may be asked to update the J-Link firmware and connectivity firmware on the nRF SoC to continue.
      When the nRF SoC has been updated with the correct firmware, the nRF Connect Bluetooth Low Energy app finishes connecting to your device over USB.
      When the connection is established, the device appears in the main view.

#. Click :guilabel:`Start scan`.
#. Find the DK you want to test and click the corresponding :guilabel:`Connect` button.

   The default name for the Peripheral UART sample is *Nordic_UART_Service*.

#. Select the **Universal Asynchronous Receiver/Transmitter (UART)** RX characteristic value.
#. Write ``30 31 32 33 34 35 36 37 38 39`` (the hexadecimal value for the string "0123456789") and click :guilabel:`Write`.

   The data is transmitted over Bluetooth LE from the app to the DK that runs the Peripheral UART sample.
   The terminal emulator connected to the DK then displays ``"0123456789"``.

#. In the terminal emulator, enter any text, for example ``Hello``.

   The data is transmitted to the DK that runs the Peripheral UART sample.
   The **UART TX** characteristic displayed in the Bluetooth Low Energy app changes to the corresponding ASCII value.
   For example, the value for ``Hello`` is ``48 65 6C 6C 6F``.
