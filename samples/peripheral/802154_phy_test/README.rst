.. _802154_phy_test:

IEEE 802.15.4 PHY Test Tool
###########################

.. contents::
   :local:
   :depth: 2

IEEE 802.15.4 PHY Test Tool provides a solution for performing Zigbee RF Performance and PHY Certification tests, as well as general evaluation of the integrated radio with IEEE 802.15.4 standard. 

Overview
********

You can perform testing by connecting to the development kit through the serial port and sending supported commands.

See :ref:`802154_phy_test_ui` for a list of available commands.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpunet, nrf52840dk_nrf52840

You can use any one of the development kits listed above.

Conducting tests using the sample also requires a testing device, such as:

  * Another development kit with the same sample, set into DUT mode.
    See :ref:`802154_phy_test_testing_board`.

.. note::
   You can perform the testing using other equipment, such as a spectrum analyzer, oscilloscope, RF power meter, etc.
   This method of testing is not covered by this documentation.

.. _802154_phy_test_ui:

User interface
**************
.. list-table:: IEEE 802.15.4 PHY Test Tool Serial Commands
   :header-rows: 1

   * - Functionality
     - Command
     - Argument(s)
     - Description
     - Example
   * - Change the device mode
     - custom changemode <mode>
     - <mode> - 0 for DUT, 1 for CMD
     - Changes the device mode if BOTH modes are available
     - custom changemode 1
   * - LED indication
     - custom lindication <value>
     - <value> - 1 for LED packet reception indication, 0 for none
     - CMD will control LED indicating packet reception
     - cutom lindication 1
   * - Ping the DUT
     - custom rping
     - 
     - CMD will send PING to the DUT and wait for the reply
     - custom rping
   * - Set the ping timeout
     - custom lpingtimeout <timeout:1> <timeout:0>
     - <timeout:1> - higher byte of the timeout; <timeout:0> - lower byte of the timeout
     - CMD sets the timeout in ms for the DUT responses
     - custom lpingtimeout 0 255
   * - Set the common radio channel for the DUT and CMD
     - custom setchannel <channel:3> <channel:2> <channel:1> <channel:0>
     - <channel:3-0> 4 octets defining the channel page and number 
     - Sets the DUT and CMD channel, sends  PING and waits for a response
     - custom setchannel 0 0 8 0
   * - Set the CMD radio channel
     - custom lsetchannel <channel:3> <channel:2> <channel:1> <channel:0>
     - <channel:3-0> - 4 bytes defining channel page and number
     - Sets the CMD channel
     - custom lsetchannel 0 0 16 0
   * - Set the DUT radio channel
     - custom rsetchannel <channel>
     - <channel> - selected channel's number
     - Sets the DUT channel
     - custom rsetchannel 13
   * - Get the CMD radio channel
     - custom lgetchannel
     - 
     - Gets the current CMD device configured channel
     - custom lgetchannel
   * - Set the CMD radio power
     - custom lsetpower <mode:1> <mode:0> <power>
     - <mode:1-0> - mode parameter is currently unsupported, use 0 for both; <power> - TX power as a signed integer in dBm
     - Sets the CMD device TX power
     - custom lsetpower 0 0 -17
   * - Set the DUT radio power
     - custom rsetpower <mode:1> <mode:0> <power>
     - <mode:1-0> - mode parameter is currently unsupported, use 0 for both; <power> - TX power as a signed integer in dBm
     - Sets the DUT device TX power
     - custom rsetpower 0 0 -17
   * - Get the CMD radio power
     - custom lgetpower
     - 
     - Gets the current CMD configured power
     - custom lgetpower
   * - Get the DUT radio power
     - custom rgetpower
     - 
     - Gets the current DUT configured power
     - custom rgetpower
   * - DUT Modulated Waveform Transmission
     - custom rstream <duration:1> <duration:0>
     - <duration:1> - higher byte of the duration value; <duration:0> - lower byte of the duration value
     - Commands the DUT to start a Modulated Waveform Transmission with a certain duration in ms
     - custom rstream 255 255
   * - Start the RX Test
     - custom rstart
     - 
     - Command the DUT to start the RX test routine, clearing previous statistics
     - custom rstart
   * - End the RX Test
     - custom rend
     - 
     - Command the DUT to terminate the RX test routine. The DUT will send the test results to the CMD device
     - custom rend
   * - Find the DUT
     - custom find
     - 
     - CMD device cycles all channels (11-26) trying to PING the DUT, stops upon receiving a reply
     - custom find
   * - Clear Channel Assessment
     - custom lgetcca <mode>
     - <mode> - IEEE 802.15.4 CCA mode (1-3)
     - CMD will perform CCA with requested mode and print the result
     - custom lgetcca 1
   * - Perform Energy Detection
     - custom lgeted
     - 
     - Perform ED and report result as 2 hex bytes
     - custom lgeted
   * - Get Link Quality Indicator
     - custom lgetlqi
     - 
     - Put the device in receive mode and wait for a packet. Output the result as 2 hex bytes
     - custom lgetlqi
   * - Measure RSSI
     - custom lgetrssi
     - 
     - Get the RSSI in dBm
     - custom lgetrssi
   * - Set short address
     - custom lsetshort 0x<short_address>
     - <short_address> - IEEE 802.15.4 short, two-byte address
     - Set the device short address. Used for frame filtering and Acknowledgement transmission
     - custom lsetshort 0x00FF
   * - Set extended address
     - custom lsetextended 0x<extended_address>
     - <extended_address> - IEEE 802.15.4 long, 8-byte address
     - Set the device extended address. Used for frame filtering and Acknowledgement transmission
     - custom lsetextended 0x000000000000FFFF
   * - Set PAN id
     - custom lsetpanid 0x<panid>
     - <panid> - Two-byte IEEE 802.15.4 PAN ID
     - Set the PAN id. Used for frame filtering and Acknowledgement transmission
     - custom lsetpanid 0x000A
   * - Set payload for burst transmission
     - custom lsetpayload <length> <payload>
     - <length> - length of payload in bytes; <payload> - packet payload bytes
     - Set an arbitrary payload of a raw IEEE 802.15.4 packet
     - custom lsetpayload 5 FFFFFFFFFF
   * - Burst transmission of packets
     - custom ltx <number> <delay>
     - <number> - number of packets to be sent, 0 for infinite transmission; <delay> - delay in ms between the transmissions
     - Starts transmission of packets with random (or previously defined) payload.
     - custom ltx 10 1000
   * - Stop burst transmission of packets
     - custom ltxend
     - 
     - CMD will stop current burst transmission. 
     - custom ltxend
   * - Start continuous receive mode
     - custom lstart
     - 
     - Enter continuous receive mode and print received packet information over serial connection. No commmands are accepted until custom lend is received
     - custom lstart
   * - End continuous receive mode
     - custom lend
     - 
     - Leave continuous receive mode and print statistics in format: [total]0x%x%x%x%x [protocol]0x%x%x%x%x [totalLqi]0x%x%x%x%x [totalRssiMgnitude]0x%x%x%x%x
     - custom lend
   * - Set CMD antenna number
     - custom lsetantenna <antenna>
     - <antenna> - antenna number in range 0-255
     - Sets the antenna used by the CMD device
     - custom lsetantenna 1
   * - Get CMD antenna number
     - custom lgetantenna
     - 
     - Gets the antenna used by the CMD device
     - custom lgetantenna
   * - Set DUT antenna number
     - custom rsetantenna <antenna>
     - <antenna> - antenna number in range 0-255
     - Sets the antenna used by the DUT device
     - custom rsetantenna 1
   * - Get DUT antenna number
     - custom rgetantenna
     - 
     - Gets the antenna used by the DUT device
     - custom rgetantenna
   * - Unmodulated waveform (carrier) transmission
     - custom lcarrier <pulse_duration> <interval> <transmission_duration>
     - <pulse_duration> - the signal is transmitted continuously during this period of time, 1-32,767ms; <interval> - time between pulses, 0-32,767ms; <transmission_duration> - upper limit for the command's execution, 0-32,767ms, 0 for infinite transmission
     - Starts transmission of unmodulated carrier
     - custom lcarrier 10 200 2000
   * - Modulated waveform transmission
     - custom lstream <pulse_duration> <interval> <transmission_duration>
     - <pulse_duration> - packets are transmitted continuously during this period of time, 1-32,767ms; <interval> - time between pulses, 0-32,767ms; <transmission_duration> - upper limit for the command's execution, 0-32,767ms, 0 for infinite transmission
     - Starts modulated waveform transmission
     - custom lstream 100 200 20000
   * - Get the DUT hardware version
     - custom rhardwareversion
     - 
     - Gets the DUT hardware version
     - custom rhardwareversion
   * - Get the DUT software version
     - custom rsoftwareversion
     - 
     - Gets the DUT software version
     - custom rsoftwareversion
   * - High frequency clock output on a selected pin
     - custom lclk <pin> <value>
     - <pin> - GPIO pin number, 0-N, where N depends on used SoC; <value> - enabled when 1, disabled when 0
     - CMD will disable/enable high frequency clock output on a selected pin. Actual clock frequency depends on SoC and is the highest as it’s possible, taking in account GPIO and CLOCK modules possibilities
     - custom lclk 10 1
   * - Set GPIO pin value
     - custom lsetgpio <pin> <value>
     - <pin> - GPIO pin number, 0-N, where N depends on used SoC; <value> - high when 1, low when 0
     - CMD will set value of the selected GPIO pin if it’s an out pin
     - custom lsetgpio 29 0
   * - Get GPIO pin value
     - custom lgetgpio <pin>
     - <pin> - GPIO pin number, 0-N, where N depends on used SoC
     - CMD will reconfigure selected GPIO pin to INPUT mode and read its value
     - custom lgetgpio 29
   * - Set DC/DC mode
     - custom lsetdcdc <value>
     - <value> - enabled when 1, disabled when 0
     - CMD will disable/enable DC/DC mode (no effect on unsupported boards)
     - custom lsetdcdc 1
   * - Get DC/DC mode
     - custom lgetdcdc
     - 
     - Gets the DC/DC mode of CMD device, always 0 for unsupported boards
     - custom lgetdcdc
   * - Set ICACHE configuration
     - custom lseticache <value>
     - <value> - enabled when 1, disabled when 0
     - CMD will disable/enable ICACHE
     - custom lseticache 1
   * - Read SoC temperature
     - custom lgettemp
     - 
     - CMD will print SoC temperature in the following format: <.%02>
     - custom lgettemp
   * - Transition radio to sleep mode
     - custom lsleep
     - 
     - CMD will put radio into sleep mode
     - custom lsleep
   * - Transition radio to receive mode
     - custom lreceive
     - 
     - CMD will put radio in receive mode
     - custom lreceive
   * - Reboot the device
     - custom lreboot
     - 
     - Reboots the device
     - custom lreboot


   

Building and running
********************
.. |sample path| replace:: :file:`samples/peripheral/802154_phy_test`

.. include:: /includes/build_and_run.txt

.. note::
   On the |nRF5340DKnoref|, the IEEE 802.15.4 PHY Test Tool is a standalone network sample that does not require any counterpart application sample.
   However, you must still program the application core to boot up the network core (and forward UART pins to the network core of the CMD device).
   The :ref:`nrf5340_empty_app_core`, which accomplishes both, is built and programmed automatically by default.
   If you want to program another sample for the application core, unset the :option:'CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE' option.

.. _802154_phy_test_testing:

Using the tool
==============

After programming the supported development kit of your choice with the sample, you can verify that it functions as expected and begin performing tests.

.. _802154_phy_test_testing_check:

Checking if the sample works as expected
----------------------------------------

1. Connect the development kit to the computer using a USB cable. Use the DK's programmer USB port (J2).
   The kits are assigned a COM port (Windows) or ttyACM device (Linux), which should be now visible in the Device Manager or /dev directory.
#. Connect to the DK using a terminal emulator (for example, PuTTY or minicom), with the following settings:

    - Baudrate: 115200
    - Parity bits: none
    - Stop bits: 1

#. If the sample is configured to support BOTH modes (the default setting), switch the DK into CMD mode by sending the following command:

    - ``custom changemode 1``

#. On the bottom side of your DK, locate a table describing GPIO pin assignment to LEDs. Read the number of GPIO pin assigned to LED 1, 2, 3 or 4. For example, on nRF52840DK, the LEDs are controlled by pins from P0.13 through P0.16.
#. The LEDs on nRF5340DK and nRF52840DK are in ``sink`` configuration. In order to turn them on, you need to set the respective pin's state to low in order to let the current flow through the LED. Send the following command to the device:
    
    - ``custom lsetgpio <pin> 0`` where ``<pin>`` is the number of the pin assigned for selected LED.
    - ``custom lsetgpio 28 0`` would be the proper command to light up LED 1 on nRF5340DK. 

#. If the selected LED lights up, the sample works as expected and is redy for use. 

.. _802154_phy_test_testing_board:

Performing radio tests without serial interface
-----------------------------------------------

1. Make sure that at least one of the development kits can be set into CMD mode, and the other one to the DUT mode. DUT device will not initialize serial interface. The easiest way to achieve this is to flash both devices with the cample configured to support BOTH modes (default setting). 
#. Connect both development kits to the computer using a USB cable.
   The kits are assigned a COM port (Windows) or ttyACM device (Linux), which should be now visible in the Device Manager or /dev directory.
#. Connect to the DKs using a terminal emulator (for example, PuTTY or minicom), with the following settings:
    
    - Baudrate: 115200
    - Parity bits: none
    - Stop bits: 1

#. If the samples are configured to support BOTH modes (the default setting), switch one of the DKs into CMD mode by sending the following command:
    
    - ``custom changemode 1``

#. Run the following command on the CMD kit:
    
    - ``custom find``

#. The CMD kit should respond with
    
    - ``channel <ch> find <ack>`` if the CMD device successfully communicates with the DUT device
    - ``DUT NOT FOUND`` if it couldn't exchange packets with the DUT device

#. Please refer to :ref:`802154_phy_test_ui` for the complete list of available commands.
