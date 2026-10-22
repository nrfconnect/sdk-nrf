.. _ug_nrf93m1_updating_modem_fw:

Updating the modem firmware
###########################

.. contents::
   :local:
   :depth: 2

Modem firmware for an nRF93M1 module is built, signed, and released by Nordic Semiconductor, so you never compile it yourself.
Your task is to deliver it, either over the air or locally through the host serial link.

Modem firmware updates are separate from host application updates.
See :ref:`ug_nrf93m1_building_fota`.

.. _ug_nrf93m1_updating_modem_fw_versions:

Checking the current version
****************************

Read the modem firmware version with a standard 3GPP command:

.. code-block:: console

   AT+CGMR
   mfw_nrf93m1_1.5.1
   OK

The version string identifies the module and the firmware revision.
Modem firmware versions 1.x.x are for nRF93M1-LABA, and versions 2.x.x are for nRF93M1-LACA.
Use a delta package that matches the variant of your module, as described in :ref:`ug_nrf93m1_variants`.

Always include this version when you report an issue, and record it in device telemetry so you can correlate field behavior with a specific firmware release.

.. _ug_nrf93m1_updating_modem_fw_delta:

Update mechanism
****************

Updates are delivered as differential, or delta, images.
A delta image contains only the difference between the running firmware and the target firmware, which keeps the transfer small enough to be practical over a cellular link.

Delta images are signed by Nordic.
The module verifies the signature before applying an update, and secure boot prevents unsigned or unauthorized firmware from executing.
Firmware rollback protection prevents an attacker from downgrading a device to an older firmware version with known weaknesses.

.. note::
   Because a delta image is relative to a specific starting version, a device several releases behind might need to apply updates in sequence rather than jumping directly to the newest release.
   Confirm the upgrade path for your starting version before you plan a fleet update.

.. _ug_nrf93m1_updating_modem_fw_ota:

Updating over the air
*********************

The nRF Cloud client in the module firmware handles updates over the air.
The device must be provisioned and connected. See :ref:`ug_93m1_cloud_connecting`.

Updates over the air use ``AT%NRFCLOUDFOTA``, which serves both the module and the host application through five modes.

.. list-table::
   :header-rows: 1

   * - ``<mode>``
     - Action
   * - 0
     - Check for a host application OTA update and cache the download URL
   * - 1
     - Download one host OTA chunk from the cached URL
   * - 2
     - Check for a modem firmware update
   * - 3
     - Check for and apply a modem firmware update
   * - 4
     - Clear the cached host OTA download URL

The following sections describe how to update the modem firmware with nRF Cloud.

The delta package for modem firmware is named after the versions it connects, in the format ``<base_mfw>_to_<new_mfw>``.
For example, ``mfw_nrf93m1_update_from_1.5.1_to_1.5.1-FOTA-TEST.bin`` updates a device running version 1.5.1 to the 1.5.1-FOTA-TEST test image.
Nordic Semiconductor also provides test images in both directions, so you can verify the update flow on a device and return it to the release firmware.

.. _ug_nrf93m1_updating_modem_fw_ota_prereq:

Requirements
============

* A device that is provisioned and shows up in your nRF Cloud device list.
  See :ref:`ug_93m1_cloud_connecting_provisioning`.
* Modem firmware enabled with ``AT+CFUN=1`` and a working network connection.
* The delta package for the version your device is running.
  Use ``AT+CGMR`` to read the running version, as described in :ref:`ug_nrf93m1_updating_modem_fw_versions`.

.. _ug_nrf93m1_updating_modem_fw_ota_release:

Creating the OTA release in nRF Cloud
=====================================

An OTA release tells nRF Cloud which devices get which firmware.

1. In the nRF Cloud left sidebar, click :guilabel:`OTA Releases`, then click :guilabel:`Create Release`.

#. Enter the from and to versions of the delta package.
   For example:

   * :guilabel:`From Version`: ``mfw_nrf93m1_1.5.1``
   * :guilabel:`To Version`: ``mfw_nrf93m1_z1.5.1_fotatest``

   .. note::
      nRF Cloud offers an update only to a version that sorts higher than the running one, using alphabetical order.
      A test image with the same version number as the running firmware therefore needs a letter in front of the version, as the ``z`` in the preceding example.

#. Click :guilabel:`Create release`.

#. In the release tab, click :guilabel:`Add OTA Payload` and fill in the following fields:

   * :guilabel:`Hardware Version`: The ``<model>`` part of the ``AT+CGMM`` response of your device, without the silicon revision in parentheses.
     For example, ``nRF93M1-LABA-AFA`` for the response ``nRF93M1-LABA-AFA (R3)``.
   * :guilabel:`Software Type`: ``mfw``
   * :guilabel:`Payload`: The delta package, for example ``mfw_nrf93m1_update_from_1.5.1_to_1.5.1-FOTA-TEST.bin``.

#. Click :guilabel:`+ Create Activation`, select the default cohort, and click :guilabel:`Activate`.

#. Open :guilabel:`Project Settings` and copy the :guilabel:`Project Key`.
   The AT commands in the next section need it.

.. _ug_nrf93m1_updating_modem_fw_ota_apply:

Checking for and applying the update
====================================

Run the following commands over the serial terminal, replacing ``<projectKey>`` with the key that you copied:

1. Enable full functionality:

   .. code-block:: console

      AT+CFUN=1
      OK

#. Check whether an update is available for the device:

   .. code-block:: console

      AT%NRFCLOUDFOTA=2,"<projectKey>"
      %FOTA: CHECKING
      %FOTA: UPDATE_AVAILABLE
      OK

   ``%FOTA: NO_UPDATE`` means that no active release matches the device.
   In that case, check the release activation, the :guilabel:`Hardware Version` of the payload, and the running version.

#. Download and apply the update:

   .. code-block:: console

      AT%NRFCLOUDFOTA=3,"<projectKey>"
      %FOTA: CHECKING
      %COAP: RESPONSE,2.05
      %COAP: DATA,153,{"data": {"url": "https://ota-cdn.memfault.com/11227/ota-payloads/..."}}
      %FOTA: ZONE,1,462848
      %FOTA: ERASING
      %FOTA: DOWNLOADING
      %FOTA: DOWNLOADING,16384
      %FOTA: DOWNLOADING,32768
      ...
      %FOTA: DOWNLOAD_DONE,213657
      %FOTA: VERIFYING
      %FOTA: VERIFIED
      %FOTA: UPGRADING
      OK

   The module downloads the delta package, verifies it, and then resets to apply it.

#. Wait for the bootloader to apply the update.
   The bootloader reports the progress and the module restarts:

   .. code-block:: console

      %FOTA: "APPLY"
      %FOTA: "UPDATING",0
      ...
      %FOTA: "UPDATING",100
      %FOTA: "DONE",0
      RDY

#. Confirm that the device runs the new firmware:

   .. code-block:: console

      AT+CGMR
      mfw_nrf93m1_<new_version>
      OK

   The returned version must match the firmware that the delta package updates to.

To return a device to the release firmware after a test, create a new release with the following versions:

* :guilabel:`From Version`: ``mfw_nrf93m1_1.5.1_fotatest``
* :guilabel:`To Version`: ``mfw_nrf93m1_z1.5.1``

The ``z`` in front of the release version is needed for the same reason as in the test release, because nRF Cloud offers only a version that sorts higher than the running one.
Upload the matching delta package, for example :file:`mfw_nrf93m1_update_from_1.5.1-FOTA-TEST_to_1.5.1.bin`, in the :guilabel:`Add OTA Payload` step, and repeat the commands in this section.

If a command reports ``%FOTA: ERROR,<reason>``, see the ``%FOTA`` notifications in the `nRF93M1 AT Commands Reference Guide`_ for the meaning of each reason.

Plan for the following in your host application:

* The module is unavailable for normal traffic while it applies the update and restarts.
  Your host must tolerate this rather than treating it as a fault.
* An update consumes cellular data. Account for it in your data plan, particularly across a fleet.
* Power must remain stable through the update.
  Do not start an update on a device that is low on battery.

.. _ug_nrf93m1_updating_modem_fw_host:

The module also updates your host application
*********************************************

This is easy to miss, and it changes how you design host updates.

``%NRFCLOUDFOTA`` modes 0, 1, and 4 let the module act as the delivery channel for *host application* firmware, not just its own.
The host declares what it is running, the module checks nRF Cloud, and the host then pulls the image through the module one chunk at a time.

.. code-block:: console

   AT%NRFCLOUDFOTA=0,"<projectKey>","<hwVer>","<swType>","<swVer>"
   %FOTA: CHECKING
   %COAP: RESPONSE,2.05

``<hwVer>``, ``<swType>``, and ``<swVer>`` describe the host application, not the module.
If an update exists, the module caches the download URL, and the host then requests chunks by index:

.. code-block:: console

   AT%NRFCLOUDFOTA=1,<chunk_idx>
   %FOTA: <base64>,<crc16>,<has_more>

Each chunk arrives Base64 encoded with a CRC-16/CCITT-FALSE checksum of the raw payload, and ``<has_more>`` is ``1`` until the final chunk.
Verify the CRC per chunk rather than only at the end, so a corrupted transfer fails on the chunk rather than after the whole image.
Clear the cached URL with mode 4 when you are done or when you abandon an update.

.. note::
   This means an nRF93 Series design does not need a second connectivity path for host firmware updates and the module is on the critical path for host recovery.
   If the module cannot reach the cloud, your host cannot be updated.
   Keep a local update path as well.
   See :ref:`ug_nrf93m1_updating_modem_fw_serial`.

.. _ug_nrf93m1_updating_modem_fw_serial:

Updating over serial
********************

A local update over the host serial link is useful during development, in production test, and for recovery when a device cannot reach the network.

Use the ``93-series`` command of `nRF Util`_ to upload and apply a delta package over the serial link:

1. Flash the :ref:`nrf93m1dk_modem_bypass` sample on the host MCU, to expose the module AT interface over UART.
   On the nRF93M1 DK, the AT interface is on ``vcom1``.

#. Install the ``93-series`` command, which is required once:

   .. code-block:: console

      nrfutil install 93-series

#. Identify the serial port of the AT interface:

   .. code-block:: console

      nrfutil device list

#. Upload and apply the delta package:

   .. code-block:: console

      nrfutil 93-series modem-firmware-upgrade --serial-port <SERIAL_PORT> --firmware <FIRMWARE>

   For example, to update from 1.5.1 to the 1.5.1-FOTA-TEST test image:

   .. code-block:: console

      nrfutil 93-series modem-firmware-upgrade --serial-port /dev/ttyACM1 --firmware mfw_nrf93m1_update_from_1.5.1_to_1.5.1-FOTA-TEST.bin
      [00:00:21] ###### 100% [Modem firmware upgrade] Modem firmware upgrade

#. Confirm the new version:

   .. code-block:: console

      nrfutil 93-series version-query --serial-port <SERIAL_PORT>

The command drives the on-device DFU engine with ``AT%FWUPD``.
By default, it uploads the package directly over UART within a timeout.
Add ``--packet-mode`` on a slow or unstable connection.
In packet mode, the package is uploaded in numbered packets that are each verified with a hash and retried if they fail, which is slower.
The command waits for the module to apply the update before it exits.
Add ``--skip-wait-for-apply`` if you do not want it to wait.

To return the device to the release firmware, run the same command with the delta package for the opposite direction, for example ``mfw_nrf93m1_update_from_1.5.1-FOTA-TEST_to_1.5.1.bin``.

Updating over HTTP or HTTPS
***************************

The ``AT%HTTPFOTADL`` command downloads a delta update package directly from a web server and applies it, without nRF Cloud and without the host having to carry the image.
Use it when you host firmware yourself, for example, on your own CDN or an internal server during development.

Nordic Semiconductor also hosts the delta packages at ``firmware.nrfcloud.com``, so you can use this method without hosting anything.
The package file names have the following format, where the base version is the version that the device currently runs:

.. code-block:: none

   mfw_nrf93m1_update_from_<base_version>_to_<new_version>.bin

For example, the following URLs are the packages that update a device to version 1.5.1:

* ``https://firmware.nrfcloud.com/93m1/mfw_nrf93m1_update_from_1.4.1_to_1.5.1.bin``
* ``https://firmware.nrfcloud.com/93m1/mfw_nrf93m1_update_from_1.5.0_to_1.5.1.bin``

The following URLs are the packages that update a device from version 1.5.1 to the FOTA test image and back:

* ``https://firmware.nrfcloud.com/93m1/mfw_nrf93m1_update_from_1.5.1_to_1.5.1-FOTA-TEST.bin``
* ``https://firmware.nrfcloud.com/93m1/mfw_nrf93m1_update_from_1.5.1-FOTA-TEST_to_1.5.1.bin``

Read the base version with ``AT+CGMR``, as described in :ref:`ug_nrf93m1_updating_modem_fw_versions`.
A package updates only a device that runs exactly its base version.

To update a device, complete the following steps:

1. Enable full functionality and wait until the device is registered to the network.
   The module boots with ``AT+CFUN=0``, and the download fails with a DNS error if the device is not registered.

   .. code-block:: console

      AT+CEREG=1
      OK
      AT+CFUN=1
      OK
      +CEREG: 2
      +CEREG: 1

#. If you use an ``https://`` URL, configure the security context that is used for the download.
   The context must trust the certificate authority of the server, verify the server, and use Server Name Indication (SNI).
   For ``firmware.nrfcloud.com``, provision the Amazon Root CA 1 certificate as described in :ref:`ug_nrf93m1_at_commands_credentials`, and then configure the context:

   .. code-block:: console

      AT%SSLCFG="cacert",0,"amazon_root_ca1.pem"
      OK
      AT%SSLCFG="seclevel",0,1
      OK
      AT%SSLCFG="sni",0,1
      OK

   With the default ``seclevel`` value ``0``, the module does not verify the server.

#. Start the download.
   The second parameter caps the download progress reporting:

   .. code-block:: console

      AT%HTTPFOTADL="http://firmware.nrfcloud.com/93m1/mfw_nrf93m1_update_from_1.5.1_to_1.5.1-FOTA-TEST.bin",50
      OK
      %HTTPURC: "FOTA","DOWNLOAD START"
      %HTTPURC: "FOTA","DOWNLOADING",12
      %HTTPURC: "FOTA","DOWNLOADING",25
      %HTTPURC: "FOTA","DOWNLOADING",38
      %HTTPURC: "FOTA","DOWNLOADING",50
      %HTTPURC: "FOTA","DOWNLOADED"
      %HTTPURC: "FOTA","VERIFIED"
      +CEREG: 0

#. Wait for the module to apply the delta.
   After ``VERIFIED``, the module reboots and applies the delta in the bootloader phase, and it reports the progress with ``%FOTA`` notifications:

   .. code-block:: console

      %FOTA: "APPLY"
      %FOTA: "UPDATING",0
      %FOTA: "UPDATING",8
      %FOTA: "UPDATING",16
      ...
      %FOTA: "UPDATING",91
      %FOTA: "UPDATING",100
      %FOTA: "DONE",0
      RDY

   ``DONE`` with error code ``0`` means that the update was applied.
   Any other code is an error, for example ``104`` for a delta built for a different base version.
   See the ``%FOTA`` command in the `nRF93M1 AT Commands Reference Guide`_ for the list.

#. Check the new version:

   .. code-block:: console

      AT+CGMR
      mfw_nrf93m1_1.5.1_fotatest
      OK

The URL must begin with ``http://`` or ``https://`` and is limited to 255 characters.
The second parameter caps download progress reporting: 0 disables it, and a value from ``50`` to
``100`` sets the highest percentage reported.
Lower the cap, or disable reporting, if the URCs would wake a sleeping host more often than you want.

The module handles the transfer, the verification, and the reboot.
``OK`` means the command was accepted, not that the update succeeded, so your host must wait for the
URCs rather than treating ``OK`` as completion.

.. list-table:: Progress URCs
   :header-rows: 1

   * - URC
     - Meaning
   * - ``DOWNLOAD START``
     - Transfer has begun.
   * - ``DOWNLOADING``, ``<percent>``
     - Progress, up to the configured cap.
   * - ``DOWNLOADED``
     - The whole package has arrived but is not yet verified.
   * - ``VERIFIED``
     - The package passed verification and is about to be applied.

If the download or the verification fails, the module sends a ``DOWNLOAD ERROR`` or ``VERIFICATION FAILURE`` URC with a cause and a detail, in the following format:

.. code-block:: console

   %HTTPURC: "FOTA","DOWNLOAD ERROR",<cause>,<detail>

For example, ``%HTTPURC: "FOTA","DOWNLOAD ERROR",2,12`` means a transport error because the DNS resolution failed, and ``%HTTPURC: "FOTA","DOWNLOAD ERROR",6,8`` means that the TLS handshake failed.
In the second case, check that the security context trusts the certificate authority of the server and has SNI enabled, as described in :ref:`ug_nrf93m1_at_commands_credentials`, or use an ``http://`` URL.
See the ``%HTTPFOTADL`` command in the `nRF93M1 AT Commands Reference Guide`_ for the causes and details.

.. note::
   On modem firmware v1.4.1, the first URC is ``HTTPSTART`` instead of ``DOWNLOAD START``, and there is no ``VERIFIED`` URC.
   Modem firmware earlier than v1.5.0 cannot download from ``https://`` URLs, so use ``http://`` URLs there.

.. _ug_nrf93m1_updating_modem_fw_production:

Planning for production
***********************

Treat modem firmware as a versioned dependency of your product, not as a fixed property of the hardware.

Record both versions
   Report the host application version and the modem firmware version in your telemetry.
   Field issues frequently depend on the combination.

Qualify before you deploy
   Test a new modem firmware release against your host application on a subset of devices before rolling it out to a fleet.

Stage fleet rollouts
   Update a small group first, confirm it reports back healthy, then expand. A modem firmware update affects connectivity, which is the same channel you need to diagnose a problem.

Keep a local recovery path
   Ensure your production design allows a serial update, so a device with broken connectivity can still be recovered at a service point.

.. note::
   Modem firmware images are variant specific.
   nRF93M1-LABA and nRF93M1-LACA use different images, so a release is not interchangeable between them.
   See :ref:`ug_nrf93m1_variants`.

   This has three consequences for a production design:

   * Track which variant is fitted to each device, and pair it with the correct image. Delivering an image for the wrong variant is a fleet-wide failure mode.
   * If you build products on both variants from one board design, your update infrastructure has to distinguish them even though the PCB and the host application are identical.
   * Qualify each variant separately when a new firmware release arrives. Testing on LABA does not establish that LACA is good.
