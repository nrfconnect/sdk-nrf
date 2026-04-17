.. bt_gap_shell:

Bluetooth: GAP Shell
####################

The GAP shell is the "main" shell of Bluetooth and handles connection management, scanning, advertising, and more.

Identities
**********

Identities are a Zephyr host concept, allowing a single physical device to behave like multiple logical Bluetooth devices.

The shell allows to create multiple identities, to a maximum that is set by the :kconfig:option:`CONFIG_BT_ID_MAX` Kconfig option.
To create a new identity, use the :code:`bt id-create` command.
You can then use it by selecting it with its ID (:code:`bt id-select <id>`).
To list all the available identities, use :code:`id-show`.

Scan for devices
****************

To start scanning, use the :code:`bt scan on` command.
Depending on the environment, you might see a lot of lines printed on the shell.
To stop the scan, run :code:`bt scan off`, the scrolling should stop.

Here is an example of what you can expect:

.. code-block:: console
    uart:~$ bt scan on
    Bluetooth active scan enabled
    [DEVICE]: CB:01:1A:2D:6E:AE (random), AD evt type 0, RSSI -78  C:1 S:1 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: 20:C2:EE:59:85:5B (random), AD evt type 3, RSSI -62  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: E3:72:76:87:2F:E8 (random), AD evt type 3, RSSI -74  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: 1E:19:25:8A:CB:84 (random), AD evt type 3, RSSI -67  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: 26:42:F3:D5:A0:86 (random), AD evt type 3, RSSI -73  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: 0C:61:D1:B9:5D:9E (random), AD evt type 3, RSSI -87  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: 20:C2:EE:59:85:5B (random), AD evt type 3, RSSI -66  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    [DEVICE]: 25:3F:7A:EE:0F:55 (random), AD evt type 3, RSSI -83  C:0 S:0 D:0 SR:0 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 us), SID: 0xff
    uart:~$ bt scan off
    Scan successfully stopped

This can lead to a high number of results.
To reduce the number of results and find a specific device, you can enable scan filters.
There are four types of filters:

* Name
* RSSI
* Address
* Periodic advertising interval

To apply a filter, use the :code:`bt scan-set-filter` command followed by the type of filters.
You can add multiple filters using the commands again.

For example, if you want to look only for devices with the name *test shell*:

.. code-block:: console

   uart:~$ bt scan-filter-set name "test shell"

To look for devices at a very close range:

.. code-block:: console

   uart:~$ bt scan-filter-set rssi -40
   RSSI cutoff set at -40 dB

To remove all filters:

.. code-block:: console

   uart:~$ bt scan-filter-clear all

Use the command :code:`bt scan on` to create an *active* scanner, meaning that the scanner will ask the advertisers for more information by sending a *scan request* packet.
Alternatively, you can create a *passive scanner* using the :code:`bt scan passive` command, so that the scanner will not ask the advertiser for more information.

Connecting to a device
**********************

To connect to a device, you need to know its address and type of address and use the :code:`bt connect` command with the address and the type as arguments.

Here is an example:

.. code-block:: console
    uart:~$ bt connect 52:84:F6:BD:CE:48 random
    Connection pending
    Connected: 52:84:F6:BD:CE:48 (random)
    Remote LMP version 5.3 (0x0c) subversion 0xffff manufacturer 0x05f1
    LE Features: 0x000000000001412f
    LE PHY updated: TX PHY LE 2M, RX PHY LE 2M
    LE conn  param req: int (0x0018, 0x0028) lat 0 to 42
    LE conn param updated: int 0x0028 lat 0 to 42

You can list the active connections of the shell using the :code:`bt connections` command.
The shell maximum number of connections is defined by the :kconfig:option:`CONFIG_BT_MAX_CONN` Kconfig option.
To disconnect, use the :code:`bt disconnect <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>` command.

.. note::
   If you were scanning just before, you can connect to the last scanned device using the :code:`bt connect` command.
   Alternatively, you can use the :code:`bt connect-name <name>` command to automatically enable scanning with a name filter and connect to the first match.

Advertising
***********

Begin advertising using the :code:`bt advertise on` command.
This will use the default parameters and advertise a resolvable private address with the name of the device.
You can choose to use the identity address instead using the :code:`bt advertise on identity` command.
To stop advertising, use the :code:`bt advertise off` command.

To enable more advanced features of advertising, create an advertiser using the :code:`bt adv-create` command.
You can pass the parameters for the advertiser either at the creation or using the :code:`bt adv-param` command.
To begin advertising with this newly created advertiser, use the :code:`bt adv-start` command, and the :code:`bt adv-stop` command to stop advertising.

When using custom advertisers, you can choose between connectable or scannable.
You have the following four options:

* :code:`conn-scan`
* :code:`conn-nscan`
* :code:`nconn-scan`
* :code:`nconn-nscan`

These parameters are mandatory when creating an advertiser or updating its parameters.

For example, if you want to create a connectable and scannable advertiser and start it:

.. code-block:: console
   uart:~$ bt adv-create conn-scan
   Created adv id: 0, adv: 0x200022f0
   uart:~$ bt adv-start
   Advertiser[0] 0x200022f0 set started

The custom advertiser does not advertise the device name and you need to add it.
Continuing from the previous example:

.. code-block:: console
   uart:~$ bt adv-stop
   Advertiser set stopped
   uart:~$ bt adv-data dev-name
   uart:~$ bt adv-start
   Advertiser[0] 0x200022f0 set started

You should now see the name of the device in the advertising data.
You can set a custom name using the :code:`name <custom name>` command instead of :code:`dev-name`.
You can also set the advertising data manually using the :code:`bt adv-data` command.
The following example shows how to set the advertiser name with it using raw advertising data:

.. code-block:: console
   uart:~$ bt adv-create conn-scan
   Created adv id: 0, adv: 0x20002348
   uart:~$ bt adv-data 1009426C7565746F6F74682D5368656C6C
   uart:~$ bt adv-start
   Advertiser[0] 0x20002348 set started

The data must be formatted according to the Bluetooth Core Specification (see version 5.3, vol. 3, part C, 11).
In this example, the raw value ``1009426C7565746F6F74682D5368656C6C`` starts with the size octet
``0x10`` and then the type octet ``0x09``.
The value ``0x09`` is the Complete Local Name type, and the remaining bytes are the name in ASCII.
So, on the other device you should see the name *Bluetooth-Shell*.

When advertising, if other devices use an *active* scanner, you may receive *scan request* packets.
To visualize those packets, you can add :code:`scan-reports` to the parameters of your advertiser.

Directed Advertising
====================

You can use directed advertising on the shell if you want to reconnect to a device.
The following example demonstrates how to create a directed advertiser with the address specified right after the parameter :code:`directed`.
The :code:`low` parameter indicates that you want to use the low duty cycle mode, and the :code:`dir-rpa` parameter is required if the remote device is privacy-enabled and supports address resolution of the target address in directed advertisement.

.. code-block:: console

   uart:~$ bt adv-create conn-scan directed D7:54:03:CE:F3:B4 random low dir-rpa
   Created adv id: 0, adv: 0x20002348

After that, you can start the advertiser and the target device can reconnect.

Extended Advertising
====================

To enable extended advertising, use the ``ext-adv`` parameter.

.. code-block:: console

   uart:~$ bt adv-create conn-nscan ext-adv
   Created adv id: 0, adv: 0x200022f0
   uart:~$ bt adv-start
   Advertiser[0] 0x200022f0 set started

This will create an extended advertiser, that is connectable and non-scannable.

Encrypted Advertising Data
==========================

Zephyr supports the Encrypted Advertising Data feature.
The :code:`bt encrypted-ad` sub-commands allow managing the advertising data of a given advertiser.

To encrypt the advertising data, you need to provide key materials using :code:`bt encrypted-ad set-keys <session key> <init vector>`.
The session key is 16 bytes long and the initialisation vector is 8 bytes long.

To add advertising data, use :code:`bt encrypted-ad add-ad` and :code:`bt encrypted-ad add-ead`.
The former will take add one advertising data structure (as defined in the Core Specification), whereas the later will read the given data, encrypt them, and add the generated encrypted advertising data structure.
You can mix encrypted and non-encrypted data, when done adding advertising data.
Use :code:`bt encrypted-ad commit-ad` to apply the change to the data of the selected advertiser.
After that, you can start the advertiser as described previously.
To clear the advertising data, use :code:`bt encrypted-ad clear-ad`.

On the Central side, you can decrypt the received encrypted advertising data.
Set the correct keys material as described earlier and enable the decrypting of the data with :code:`bt encrypted-ad decrypt-scan on`.

.. note::
   To see the advertising data in the scan report, you need to enable :code:`bt scan-verbose-output`.

   You can increase the length of the advertising data by increasing the value of the Kconfig options :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_LEN_MAX` and :kconfig:option:`CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX`.

Here is a simple example demonstrating the usage of EAD:

.. tabs::

   .. group-tab:: Peripheral

      .. code-block:: console

         uart:~$ bt init
        ...
        uart:~$ bt adv-create conn-nscan ext-adv
        Created adv id: 0, adv: 0x81769a0
        uart:~$ bt encrypted-ad set-keys 9ba22d3824efc70feb800c80294cba38 2e83f3d4d47695b6
        session key set to:
        00000000: 9b a2 2d 38 24 ef c7 0f  eb 80 0c 80 29 4c ba 38 |..-8$... ....)L.8|
        initialisation vector set to:
        00000000: 2e 83 f3 d4 d4 76 95 b6                          |.....v..         |
        uart:~$ bt encrypted-ad add-ad 06097368656C6C
        uart:~$ bt encrypted-ad add-ead 03ffdead03ffbeef
        uart:~$ bt encrypted-ad commit-ad
        Advertising data for Advertiser[0] 0x81769a0 updated.
        uart:~$ bt adv-start
        Advertiser[0] 0x81769a0 set started

   .. group-tab:: Central

      .. code-block:: console

         uart:~$ bt init
         ...
         uart:~$ bt scan-verbose-output on
         uart:~$ bt encrypted-ad set-keys 9ba22d3824efc70feb800c80294cba38 2e83f3d4d47695b6
         session key set to:
         00000000: 9b a2 2d 38 24 ef c7 0f  eb 80 0c 80 29 4c ba 38 |..-8$... ....)L.8|
         initialisation vector set to:
         00000000: 2e 83 f3 d4 d4 76 95 b6                          |.....v..         |
         uart:~$ bt encrypted-ad decrypt-scan on
         Received encrypted advertising data will now be decrypted using provided key materials.
         uart:~$ bt scan on
         Bluetooth active scan enabled
         [DEVICE]: 68:49:30:68:49:30 (random), AD evt type 5, RSSI -59   shell C:1 S:0 D:0 SR:0 E:1 Prim: LE 1M, Secn: LE 2M, Interval: 0x0000 (0 us), SID: 0x0
                 [SCAN DATA START - EXT_ADV]
                 Type 0x09:    shell
                 Type 0x31: Encrypted Advertising Data: 0xe2, 0x17, 0xed, 0x04, 0xe7, 0x02, 0x1d, 0xc9, 0x40, 0x07, uart:~0x18, 0x90, 0x6c, 0x4b, 0xfe, 0x34, 0xad
                 [START DECRYPTED DATA]
                 Type 0xff: 0xde, 0xad
                 Type 0xff: 0xbe, 0xef
                 [END DECRYPTED DATA]
                 [SCAN DATA END]
                 ...

Filter Accept List
******************

You can create a list of allowed addresses that can be used to connect to them automatically as follows:

.. code-block:: console
   uart:~$ bt fal-add 47:38:76:EA:29:36 random
   uart:~$ bt fal-add 66:C8:80:2A:05:73 random
   uart:~$ bt fal-connect on

The shell will connect to the first available device.
If both devices are advertising at the same time, connection is established to the first address added to the list.

You can also use the Filter Accept List for scanning or advertising using the :code:`fal` command.
For example, if you want to scan for a bunch of selected addresses, you can set up a Filter Accept List like this:

.. code-block:: console
   uart:~$ bt fal-add 65:4B:9E:83:AF:73 random
   uart:~$ bt fal-add 73:72:82:B4:8F:B9 random
   uart:~$ bt fal-add 5D:85:50:1C:72:64 random
   uart:~$ bt scan on fal

You should see only those three addresses reported by the scanner.

Enabling security
*****************

When connected to a device, you can enable multiple levels of security.
Here is the list for Bluetooth LE:

* **1** - No encryption and no authentication
* **2** - Encryption and no authentication
* **3** - Encryption and authentication
* **4** - Bluetooth LE Secure Connection

To enable security, use the :code:`bt security <level>` command.
For levels requiring authentication (level 3 and above), you must first set the authentication method using the :code:`bt auth all` command.
When you set the security level, you are asked to confirm the passkey on both devices.
On the shell side, use the :code:`bt auth-passkey-confirm` command.

Pairing
=======

Enabling authentication requires the devices to be bondable.
By default, the shell is bondable.
To make the shell not bondable, use the :code:`bt bondable off` command.
To list all the devices you are paired with, use the :code:`bt bonds` command.

To set the maximum number of paired devices, use the :kconfig:option:`CONFIG_BT_MAX_PAIRED` Kconfig option.
To remove a paired device, use the :code:`bt clear <address: XX:XX:XX:XX:XX:XX> <type: (public|random)>` command or remove all paired
devices with the :code:`bt clear all` command.
