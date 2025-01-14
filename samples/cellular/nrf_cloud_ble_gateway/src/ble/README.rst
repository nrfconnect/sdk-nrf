.. _nrfcloud_ble_gateway:

nRF9160: nRF Cloud BLE Gateway
##############################

The nRF Cloud BLE Gateway uses the `lib_nrf_cloud`_ to connect an nRF9160-based board to `nRF Cloud`_ via LTE, connnect to multiple Bluetooth LE peripherals, and transmit their data to the cloud.
Therefore, this application acts as a gateway between Bluetooth LE and the LTE connection to nRF Cloud.

Overview
********

The application uses the LTE link control driver to establish a network connection.
It is then able to connect to multiple Bluetooth LE peripherals, and can then transmit the peripheral data to Nordic Semiconductor's cloud solution, `nRF Cloud`_.
The data is visualized in nRF Cloud's web interface.

Programs such as PuTTY_, `Tera Term`_, or the `LTE Link Monitor`_ application, implemented as part of `nRF Connect for Desktop`_, can be used to interact with the included shell.
You can also send AT commands from the **Terminal** card on nRF Cloud when the device is connected.

By default, the gateway supports firmware updates through `lib_nrf_cloud_fota`_.

.. _nrfcloud_ble_gateway_requirements:

Requirements
************

* One of the following boards:

  * Apricity Gateway
  * `nRF9160 DK <ug_nrf9160>`_

* For the Apricity Gateway nRF9160, `nrfcloud_gateway_controller`_ must be programmed in the nRF52 board controller.
* For the nRF9160 DK, `hci_lpuart`_ must instead be programmed in the nRF52 board controller.
* The sample is configured to compile and run as a non-secure application on nRF91's Cortex-M33.
  Therefore, it automatically includes the `secure_partition_manager`_ that prepares the required peripherals to be available for the application.


.. _nrfcloud_ble_gateway_user_interface:

Key Features
************
Hardware
--------
- Runs on either the Apricity Gateway hardware or the nRF9160DK
- 700-960 MHz + 1710-2200 MHz LTE band support.
  The following bands, based on geographic regions, are used:
   - USA 2, 4, 12, and 13
   - EU 3, 8, 20, and 28
- Certifications: CE, FCC <TBD>
- LTE-M/NB-IoT and Bluetooth LE antennas
- Nano/4FF Subscriber Identity Module (SIM) card slot
- 64 megabit SPI serial Flash memory
- PC connection through USB
- Normal operating temperature range: 5C ~ 35C <TBD>

Apricity Gateway hardware distinguishing features:

- Single button with multiple uses power, reset, and update enable
- Dual RGB LEDs for status indication of LTE and BLE connections
- Larger LTE and BLE antennas
- Rechargeable 3.7V Li-Po battery with 2000 mAh capacity
- Charging through Universal Serial Bus (USB) or external power supply with barrel jack connector
- Rigid, wall mountable enclosure

Firmware
--------
- Supports up to 8 [#]_ BLE Devices
- Supports FOTA (firmware over the air) updates of the nRF9160 modem, bootloader, and application
- Supports FOTA updates of select BLE devices
- Supports USB updates of nRF52840 processor
- Offers shell interface with secure login for USB serial management of gateway

.. [#] design improvements are planned to enable more.

User interface
**************

One can locally manage the gateway with a USB connection and a terminal program.
See :ref:`Shell` for details.

The Apricity Gateway button has the following functions:

    * Power off device when held for more than one second and released before reset.
    * Short press again will power on the device.
    * Reset device when held for > 7.5 seconds.
    * Enter USB MCUboot update mode for nrf52840 when held for > 11.5 seconds.

The application state is indicated by the LEDs.

.. list-table::
   :header-rows: 1
   :align: center

   * - LTE LED 1 color
     - State
   * - Off
     - Not connected to LTE carrier
   * - Slow White Pulse
     - Connecting to LTE carrier
   * - Slow Yellow Pulse
     - Associating with nRF Cloud
   * - Slow Cyan Pulse
     - Connecting to nRF Cloud
   * - Solid Blue
     - Connected, ready for BLE connections
   * - Red Pulse
     - Error

.. list-table::
   :header-rows: 1
   :align: center

   * - BLE LED 2 color
     - State
   * - Slow Purple Pulse
     - Button being held; continue to hold to enter nRF52840 USB MCUboot update mode
   * - Rapid Purple Pulse
     - in nRF52840 USB MCUboot update mode
   * - Slow Yellow Pulse
     - Waiting for Bluetooth LE device to connect
   * - Solid White
     - Bluetooth LE connection established

Building and running
********************

In order to Flash the first firmware image to the Apricity Gateway, you will need one of the following connections:

   - An nRF9160 DK with VDDIO set to 3V, a 10 pin ribbon connected to Debug out, and an adapter from that to a 6 pin Tag Connect connector.
   - A Segger J-Link with an adapter to a 6 pin Tag Connect.
   - For either method, connect the Tag Connect to ``NRF91:J1`` on the PCB.

For programming to run on the nRF9160 DK, set ``PROG/DEBUG`` to ``nRF91``.

Program nRF9160 Application Processor
-------------------------------------

1. Checkout this repository.
#. Execute the following to pull down all other required repositories::

      west update

#. Execute the following to build for the Apricity Gateway hardware::

      west build -d build_ag -b apricity_gateway_nrf9160ns

#. Or execute this, to build for the nRF9160 DK::

      west build -d build_dk -b nrf9160dk_nrf9160_ns

#. Flash to either board, replacing <build dir> with the value above after the ``-d`` option::

      west flash -d <build dir> --erase --force

Program nRF52840 Board Controller
---------------------------------

`nrfcloud_gateway_controller`_

For the Apricity Gateway hardware, follow the same instructions as above in the folder for its repository, except use ``apricity_gateway_nrf52840`` instead of ``apricity_gateway_nrf9160ns``, and connect the Tag Connect to ``NRF52:J1``.

For the nRF9160 DK, `hci_lpuart`_ must instead be programmed in the nRF52 board controller.
This should be done from the root of the lte-gateway repo so that the required device tree overlays in the `boards <./boards>`_ folder are utilized.

Program The nRF9160 Modem Processor
-----------------------------------

`Modem Firmware v1.3.0`_

For either the Apricity Gateway or the nRF9160 DK, you must also flash the modem firmware.
Version ``mfw_1.3.0`` or higher is required.
Program this using `nRF Connect Programmer`_ application.


Generating Certificates
***********************

An nRF Cloud BLE Gateway must have proper security certificates in order to connect to nRF Cloud.

Create Self-Signed CA Certs
---------------------------
This step is done using the `Create CA Cert`_ Python 3 script.

Check out this repository, install the specified prerequisite Python 3 packages, and then follow the instructions to create a CA cert.
This only needs to be done once per customer.

Install Device Certificates
---------------------------
This step is done using the `Device Credentials Installer`_ Python 3 script.

Here is an example of running this script (replace the CA0x522... values with the file names for your CA certs)::

   $  python3 device_credentials_installer.py -g --ca CA0x522400c80ef6d95ea65ef4860d12adc1b031aa9_ca.pem --ca_key CA0x522400c80ef6d95ea65ef4860d12adc1b031aa9_prv.pem --csv provision.csv -d -F "APP|MODEM|BOOT"

Using the generated ``provision.csv`` file, go on to the next step.

NOTES:

- The ``-A`` (all ports) option will be necessary if using the nRF9160DK, in order to find the board.
  If you have more than one board powered on and connected to your PC via USB, you will need to select which board to use.
  Otherwise, it will use the first one detected.
- The ``-g`` (gateway) option forces the program to assume this device has a shell which uses the expected Gateway commands in order to send and receive modem ``AT`` commands.
  Usually the program will detect this automatically.
- The ``-a`` (append) option allows you to build up a CSV file for multiple devices, and then add them all at once to your account with a single curl command.
- The ``-d`` (delete) option will remove any previous certificates from the ``SECTAG`` being used.
  A ``SECTAG`` is a programming slot for certificates in the modem.
  The ``SECTAG`` here must match the ``CONFIG_NRF_CLOUD_SEC_TAG`` Kconfig option in the prj.conf file.

The provision.csv file lists the device ID (UUID) in the first column.
It is also displayed by the Device Credentials Installer script.

Provisioning and Associating with nRF Cloud
*******************************************

Once you are signed in, perform the following steps to add the gateway to your nRF Cloud account.

There are two ways to provision and associate using the provision.csv file you generated::

1. Via the nRF Cloud website: `nRF Cloud Provision Devices`_
#. Programmatically using `nRF Cloud ProvisionDevices REST API`_

On the `nRF Cloud Provision Devices`_ page, you can drag and drop the CSV file, or click the button to browse for and select it.
Click the Upload and Provision button to begin the process.
The status will be displayed in the table below.

Instead of using the website, you can instead use curl or Postman to submit the csv file to the `nRF Cloud ProvisionDevices REST API`_ directly.
You will need to find your nRF Cloud account API Key on your account settings page, and use it in place of $API_KEY below.

e.g.::

   $ curl --location --request POST 'https://api.nrfcloud.com/v1/devices' --header 'Authorization: Bearer $API_KEY' --header 'Content-Type: text/csv' --data-binary '@provision.csv'

*returns*::

   {"bulkOpsRequestId":"01FE6M2552H7YZQ4XAGWJPR2TW"}
   $

You can then determine if it succeeded by passing the bulkOpsRequestId returned to the `nRF Cloud FetchBulkOpsRequest REST API`_.

e.g.::

   $ curl --location --request GET 'https://api.nrfcloud.com/v1/bulk-ops-requests/01FE6M2552H7YZQ4XAGWJPR2TW' --header 'Authorization: Bearer $API_KEY'

*returns*::

   {"bulkOpsRequestId":"01FE6M2552H7YZQ4XAGWJPR2TW","status":"SUCCEEDED","endpoint":"PROVISION_DEVICES","requestedAt":"2021-10-08T19:42:45.992Z","completedAt":"2021-10-08T19:42:49.069Z","uploadedDataUrl":"https://bulk-ops-requests.nrfcloud.com/f08f15c3-b523-7841-ec5a-b277610ade88/provision_devices/01FE6M2552H7YZQ4XAGWJPR2TW.csv"}
   $

Testing
*******

After programming the application and all prerequisites to your board, test the Apricity Gateway application by performing the following steps:

1. Connect the board to the computer using a USB cable. The board is assigned a COM port (Windows) or ttyACM or ttyS device (Linux).

#. Connect to the board with a terminal emulator, for example, PuTTY, Tera Term, or LTE Link Monitor.
   Turn off local echo. Output CR and LF when either is received.
   The shell uses VT100-compatible escape sequences for coloration.
#. Reset the board if it was already on.
#. Observe in the terminal window that the board starts up in the Secure Partition Manager and that the application starts.
   This is indicated by output similar to the following lines::

      *** Booting Zephyr OS build v2.6.99-ncs1-rc2-5-ga64e96d17cc7  ***
      ...
      SPM: prepare to jump to Non-Secure image.

      login:

#. For PuTTY_ or `LTE Link Monitor`_, you will need to reconnect terminal.
   (Bluetooth LE HCI control resets the terminal output and needs to be reconnected).
   `Tera Term`_ automatically reconnects.
#. Log in with the default password::

      nordic

#. If you wish to see logging messages other than ERROR, such as INFO, execute::

      log enable inf

#. Open a web browser and navigate to https://nrfcloud.com/.
   Click on Device Management then Gateways.
   Click on your device's Device ID (UUID), which takes you to the detailed view of your gateway.
#. The first time you start the application, the device will be added to your account automatically.

   a. Observe that the LED(s) indicate that the device is connected.
   #. If the LED(s) indicate an error, check the details of the error in the terminal window.

#. Add BLE devices by clicking on the + sign.
   Read, write, and enable notifications on connected peripheral and observe data being received on the nRF Cloud.
#. Optionally send AT commands from the terminal, and observe that the response is received.


.. _Shell:

Using the Management Interface (Shell)
**************************************
The shell is available via a USB serial port, through one of the two provided serial connections.
Using a 3rd party terminal program such as PuTTY_ or `Tera Term`_, you can log in and administer the gateway directly.

Once logged in at the login: prompt, you can get help using the tab key or by typing help. ::

   login: ******
   nRF Cloud Gateway
   Hit tab for help.
   gateway:#
      at ble clear cpu_load date exit fota
      help history info kernel log login logout
      passwd reboot resize session shell shutdown
   gateway:#

Many commands have sub commands.
The shell offers command completion.
Type the start of a command and hit tab, and it will offer available choices. ::

   at - <enable | AT<cmd> | exit> Execute an AT command. Use <at enable>
      first to remain in AT command mode until 'exit'.

This command, plus the parameter 'enable' places the shell into AT command mode.
In this mode, the only available commands are nrf9160
modem AT commands or 'exit' to return to normal shell mode.
See the `nRF91 AT Command Reference`_ for more information about AT commands. ::

   ble - Bluetooth commands
   Subcommands:
      scan: Scan for BLE devices.
      save: Save desired connections to shadow on nRF Cloud.
      conn: <all | name | MAC> Connect to BLE device(s).
      disc: <all | name | MAC> Disconnect BLE device(s).
      en:   <all | MAC [all | handle]> Enable notifications on
            BLE device(s).
      dis:  <all | MAC [all | handle]> Disable notifications on
            BLE device(s).
      fota: <addr> <host> <path> <size> <final> [ver] [crc] [sec_tag]
            [frag_size] [apn] start BLE firmware over-the-air update.
      test: Set BLE FOTA download test mode.

   clear - Clear the terminal.
   cpu_load - debug command to see how busy the nrf9160 is.
   date -     <get | set [Y-m-d] <H:M:S>> - display or change the current
              date and time.
   exit -     leave AT command mode.
   fota -     <host> <path> [sec_tag] [frag_size] [apn] start firmware
              over-the-air update.
   history -  display previous commands entered in the shell.  Up arrow
              will move backward one by one, making it easy to repeat
              previous commands.

   info - Informational commands
   Subcommands:
      cloud:   Cloud information.
      ctlr:    BLE controller information.
      conn:    Connected Bluetooth devices information.
      gateway: Gateway information.
      irq:     Dump IRQ table.
      list:    List known BLE MAC addresses.
      modem:   Modem information.
      param:   List parameters.
      scan:    Bluetooth scan results.

   kernel - Kernel commands
   Subcommands:
      cycles:  Kernel cycles.
      reboot:  Reboot.
      stacks:  List threads stack usage.
      threads: List kernel threads.
      uptime:  Kernel uptime.
      version: Kernel version.

   log - Commands for controlling logger
   Subcommands:
      backend:       Logger backends commands.
      disable:       'log disable <module_0> .. <module_n>' disables
                     logs in specified modules (all if no modules
                     specified).
      enable:        'log enable <level> <module_0> ... <module_n>'
                     enables logs up to given level in specified modules
                     (all if no modules specified).
      go:            Resume logging
      halt:          Halt logging
      list_backends: Lists logger backends.
      status:        Logger status

   login - the default first command after reboot and after logout.
   logout - lock shell until user logs in again with correct password.
   passwd - <oldpw> <newpw> change the password.
   reboot - disconnect from nRF Cloud and the LTE network, then restart.
   resize - update the shell's information about the current dimensions
            of the terminal window.
   session - display or change the persistent sessions setting.

   shell - Useful, not Unix-like shell commands.
   Subcommands:
      backspace_mode: Toggle backspace key mode.
                      Some terminals are not sending separate escape code
                      for backspace and delete button. This command forces
                      shell to interpret delete key as backspace.
      colors:         Toggle colored syntax.
      echo:           Toggle shell echo.
      stats:          Shell statistics.

   shutdown - Disconnect from nRF Cloud and the LTE network, then
              power down the gateway.  Press the button to restart.


Updating Firmware with FOTA
***************************

The nRF9160 modem, application firmware, and bootloader, can all be updated over the air by nRF Cloud.
This can be done when the gateway is connected to the cloud via https://nrfcloud.com/#/updates-dashboard.

Modem
-----
Incremental modem updates are supported.
Full modem updates using external flash memory on board the Apricity gateway hardware is possible but not enabled yet.

nRF9160
-------
Application and MCUboot firmware updates are supported.

nRF52840
--------
The nRF52840 cannot be updated over the air yet; it must be updated over USB on the Apricity Gateway.
See the Button Behavior and LED Indicator Behavior sections for the process to enter MCUboot mode.
Once in that mode, nRF Connect Desktop Programmer can update the firmware using the MCUBoot mode, by clicking the Enable MCUBoot checkbox.

The nRF52840 can be updated on an nRF9160DK using the built-in Segger J-Link programming hardware and the already established methods for flashing (nRF Connect for Desktop Programmer, west, nrfjprog, etc.).

BLE Devices
-----------
Bluetooth devices running the nRF5SDK and its buttonless secure DFU bootloader can be updated as well.
Create a device group in nRF Cloud for one or more indentical Bluetooth peripherals that are connected to your gateway.
Then in the Updates Dashboard, upload the firmware bundle using the Bundles ... menu, then click Create Update.
Select the device group and the firmware bundle, and click Save.
Then click Apply Update.

Known Issues and Limitations
****************************

Hardware
--------
1. Battery unplugged from PCB for safe shipping
#. Power button and control is implemented in software; no hard on/off provided
#. SIM card is not accessible from outside the enclosure, though it can be replaced by opening the enclosure on the antenna side
#. JTAG programming of onboard nRF9160 and nRF52840 require special connectors and adapters
#. While there is an U.FL RF connector provided for GPS, there is no mounting hole in the enclosure for an SMA connector and external antenna
#. The cables for the BLE and LTE antennas are easily damaged by the edge of the PCB if they are not routed properly prior to screwing either end cap back on
#. It would be better to have a bigger, more professional-looking waterproof enclosure

Firmware
--------
1. Modification and storage of login password by user not yet implemented
#. LTE-M / TLS / MQTT is the only tested configuration (NBIoT and LWM2M not tested and/or implemented)
#. Maximum number of BLE device characteristic notifications per second limited to <TBD> (approximately 10)
#. BLE Beacons are not supported
#. BLE devices which frequently disconnect and reconnect to save power will not work well if at all
#. Only BLE devices which implement Nordic nRF5 SDK-style Secure DFU can be updated using FOTA
#. FOTA updates of the on-board nRF52840 is not implemented (must be done over USB)
#. Bonding is not yet implemented, so devices which require it will not work well if at all
#. External Flash is not utilized yet to implement full modem updates
#. GPS not supported (single and/or multi cell location services could be supported in the future)

Cloud
-----
1. Frontend facilities for helping easily provision a gateway using device certificates are not yet released.
#. Frontend graphics are not appropriate for BLE devices  or for nRF Cloud BLE Gateway vs. Phone Gateway
#. Bandwidth is used inefficiently by requiring frequent complete rediscovery of BLE device services and characteristics
#. JSON format is overly verbose which wastes gateway SRAM and bandwidth, limiting the number of connected BLE devices and rate of notifications

Dependencies
************

This application uses the following nRF Connect SDK libraries and drivers:

* `lib_nrf_cloud`_
* `modem_info_readme`_
* `at_cmd_parser_readme`_
* ``lib/modem_lib``
* `dk_buttons_and_leds_readme`_
* ``drivers/lte_link_control``
* ``drivers/flash``
* ``bluetooth/gatt_dm``
* ``bluetooth/scan``

From Zephyr:
  * `zephyr:bluetooth_api`_

In addition, it uses the Secure Partition Manager sample:

* `secure_partition_manager`_

For nrf52840:

* `nrfcloud_gateway_controller`_
* `hci_lpuart`_

History
*******

The Apricity Gateway application was created using the following nRF Connect SDK sample applications:

  * `lte_ble_gateway`_
  * `asset_tracker`_

From Zephyr:
  * `zephyr:bluetooth-hci-uart-sample`_


.. ### These are links used in gateway docs.

.. _PuTTY: https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html

.. _`Tera Term`: https://ttssh2.osdn.jp/index.html.en

.. _`LTE Link Monitor`: https://infocenter.nordicsemi.com/topic/ug_link_monitor/UG/link_monitor/lm_intro.html

.. _`nRF Connect Programmer`: https://infocenter.nordicsemi.com/topic/ug_nc_programmer/UG/nrf_connect_programmer/ncp_introduction.html

.. _`nRF Connect for Desktop`: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-desktop

.. _`nRF Cloud`: https://nrfcloud.com/
.. _`nRF Cloud Provision Devices`: https://nrfcloud.com/#/provision-devices
.. _`nRF Cloud ProvisionDevices REST API`: https://api.nrfcloud.com/v1#operation/ProvisionDevices
.. _`nRF Cloud FetchBulkOpsRequest REST API`: https://api.nrfcloud.com/v1#operation/FetchBulkOpsRequest

.. _`nrfcloud_gateway_controller`: https://github.com/nRFCloud/lte-gateway-ble

.. _`Modem Firmware v1.3.0`: https://www.nordicsemi.com/-/media/Software-and-other-downloads/Dev-Kits/nRF9160-DK/nRF9160-modem-FW/mfw_nrf9160_1.3.0.zip

.. _`nRF91 AT Command Reference`: https://infocenter.nordicsemi.com/topic/ref_at_commands/REF/at_commands/intro.html

.. _`Create CA Cert`: https://github.com/nRFCloud/utils/tree/master/python/modem-firmware-1.3%2B#create-ca-cert

.. _`Device Credentials Installer`: https://github.com/nRFCloud/utils/tree/master/python/modem-firmware-1.3%2B#device-credentials-installer

.. _`asset_tracker`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker/README.html
.. _`lte_ble_gateway`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/nrf9160/lte_ble_gateway/README.html
.. _`lib_nrf_cloud_fota`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/networking/nrf_cloud.html#firmware-over-the-air-fota-updates
.. _`nRF9160 DK <ug_nrf9160>`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_nrf9160.html#ug-nrf9160
.. _`hci_lpuart`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/bluetooth/hci_lpuart/README.html
.. _`secure_partition_manager`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/spm/README.html#secure-partition-manager
.. _`lib_nrf_cloud`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/networking/nrf_cloud.html
.. _`modem_info_readme`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/modem/modem_info.html
.. _`at_cmd_parser_readme`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/modem/at_cmd_parser.html
.. _`dk_buttons_and_leds_readme`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/dk_buttons_and_leds.html

.. _`zephyr:bluetooth_api`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/reference/bluetooth/index.html
.. _`zephyr:bluetooth-hci-uart-sample`: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/samples/bluetooth/hci_uart/README.html
