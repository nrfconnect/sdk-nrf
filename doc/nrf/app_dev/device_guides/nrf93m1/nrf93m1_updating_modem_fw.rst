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
   mfw_nrf93m1_1.5.0
   OK

The version string identifies the module and the firmware revision.

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

To check for and apply a modem firmware update:

.. code-block:: console

   AT%NRFCLOUDFOTA=3,"<projectKey>"

Unsolicited ``%COAP`` and ``%FOTA`` result codes can be emitted during the cloud transaction.

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

Use the :ref:`nrf93m1dk_modem_bypass` sample to expose the module UART to your computer, then drive the on-device DFU engine with ``AT%FWUPD``.
The command writes an update package to the module over the serial link in numbered packets, each carrying a length and an XOR-8 checksum.

Updating over HTTP or HTTPS
***************************

The ``AT%HTTPFOTADL`` command downloads a delta update package directly from a web server and applies it, without nRF Cloud and without the host having to carry the image.
Use it when you host firmware yourself, for example, on your own CDN or an internal server during development.

.. code-block:: console

   AT%HTTPFOTADL="https://example.com/delta.binpkg",100
   OK

   %HTTPURC: "FOTA","DOWNLOAD START"
   %HTTPURC: "FOTA","DOWNLOADING",10
   %HTTPURC: "FOTA","DOWNLOADING",50
   %HTTPURC: "FOTA","DOWNLOADING",100
   %HTTPURC: "FOTA","DOWNLOADED"
   %HTTPURC: "FOTA","VERIFIED"

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

After ``VERIFIED``, the module reboots and applies the delta during the bootloader phase.
Confirm the result with ``AT+CGMR`` once it comes back, rather than assuming ``VERIFIED`` means the new firmware is running.

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
