.. _ug_93m1_cloud_connecting:

Connecting the nRF93M1 DK to nRF Cloud
######################################

.. contents::
   :local:
   :depth: 2

The nRF Cloud client runs in the module firmware.
You can provision a device and use cloud services with AT commands, without building a host application that includes a cloud library.

Transport is CoAP over DTLS 1.2, which keeps message overhead small and works with cellular power saving.
Each cloud transaction emits a ``%COAP:`` unsolicited result code carrying the transport-layer status for that request.

.. _ug_93m1_cloud_connecting_reqs:

Requirements
************

* An nRF93M1 DK, with the supplied LTE antenna attached and the supplied SIM inserted.
* An `nRF Cloud`_ account.
* `nRF Util`_ and the `Serial Terminal app`_ from `nRF Connect for Desktop`_.
* A working network connection.
  See :ref:`ug_nrf93m1_testing_registration`.

Connect USB1 on the DK to your computer, then identify the AT command port:

.. code-block:: console

   nrfutil device list

Open the ``vcom1`` port at 115200 bps.
This is the port that communicates with the module.

.. _ug_93m1_cloud_connecting_identity:

Device identity
***************

Each device presents two stable identities and two short-lived credentials.

.. list-table::
   :header-rows: 1

   * - Command
     - What it returns
   * - ``AT%DEVICEUUID``
     - A 36-character device UUID, generated once and stored in non-volatile memory
   * - ``AT%CLOUDACCESSKEY``
     - A P-256 ECC public key in Base64-DER form.
       The private key is generated on-chip and never leaves the device.
   * - ``AT%REGJWT``
     - A signed registration JWT, used during onboarding
   * - ``AT%JWT``
     - A short-lived authentication JWT, valid for approximately one hour

.. important::
   Device system time must be valid for a JWT to be accepted by the cloud.
   If authentication fails immediately after a cold boot, check time before investigating anything else.

   Read the clock of the module with ``AT+CCLK?``.
   The ``%JWT`` command fails if the time has not been set.
   Most networks set the time when the device goes online, so register the device to the network before you request a JWT.

.. _ug_93m1_cloud_connecting_provisioning:

Provisioning a device
*********************

Onboarding needs your nRF Cloud Team ID, then a UUID and registration JWT read from the device.

1. Register for an `nRF Cloud`_ account and log in.

#. Navigate to :guilabel:`Fleet` > :guilabel:`Devices`, then click :guilabel:`Add devices`.

   .. figure:: images/nrf93m1_cloud_add_devices.png
      :alt: nRF Cloud - Add Devices

#. Select :guilabel:`nRF93M1` in the chip chooser and click :guilabel:`Continue`.

   .. figure:: images/nrf93m1_cloud_select.png
      :alt: nRF Cloud - Select nRF93M1

#. In the :guilabel:`Get Device Credentials` step, copy the command from :guilabel:`Get your JWT token`.
   The command already contains your team ID.
   Run it in the serial terminal connected to the nRF93M1 AT UART:

   .. figure:: images/nrf93m1_cloud_get_credentials.png
      :alt: nRF Cloud - Get Device Credentials

   .. code-block:: console

      AT%REGJWT=<team-id>
      %REGJWT: xxxxx.xxxxx.xxxxx
      OK

   .. note::
      The ``AT%DEVICEUUID`` command in the second box is needed only for bulk upload.
      See :ref:`ug_93m1_cloud_connecting_bulk`.

#. Click :guilabel:`Continue`.
   On the :guilabel:`Manual` tab, paste the registration JWT into the :guilabel:`JWT token` field.
   The device ID is read from the token, so you do not need to enter it.

   .. figure:: images/nrf93m1_cloud_input_credentials.png
      :alt: nRF Cloud - Input Device Credentials

#. Click :guilabel:`Continue`, then review and confirm.

#. Confirm the device appears in the device list.

   This might take a few seconds.

   .. figure:: images/nrf93m1_cloud_devices_list.png
      :alt: nRF Cloud - Devices

   You can also verify that the device was added successfully by going to Legacy nRF Cloud Experience:

   a. Open the bottom-left panel and click :guilabel:`Legacy App`.
   #. In the left-hand panel, select :guilabel:`Device Management`.
   #. Click :guilabel:`Devices`.
   #. Check the device list to confirm that your newly added devices are displayed.

      .. figure:: images/nrf93m1_cloud_Legacy_devices_list.png
         :alt: nRF Cloud - Legacy Devices

The device is now provisioned and authenticated.

.. _ug_93m1_cloud_connecting_bulk:

Provisioning many devices
=========================

For more than a handful of devices, use the :guilabel:`Bulk Upload` tab in the :guilabel:`Input Device Credentials` step instead of entering tokens by hand.
It accepts a CSV file of up to 1000 devices.
Each line must contain comma-separated ``deviceId, onboardingToken`` fields.
Read the device ID with ``AT%DEVICEUUID`` and the token with the ``AT%REGJWT`` command shown in the wizard.

.. figure:: images/nrf93m1_cloud_bulk_upload.png
   :alt: nRF Cloud - Bulk upload

.. _ug_93m1_cloud_connecting_location:

Location
********

Location requires no GNSS receiver.
The module collects Wi-Fi® or cellular measurements and nRF Cloud resolves them to a position.

Collect measurement data:

.. list-table::
   :header-rows: 1

   * - Command
     - Data collected
   * - ``AT%BCINFO``
     - Serving cell as ``%BCINFOSC``: EARFCN, PCI, RSRP, RSRQ, MCC, MNC, cell ID, TAC. Neighbor cells
       as ``%BCINFONC``: EARFCN, PCI, RSRP, RSRQ.
   * - ``AT%WIFISCAN``
     - Per access point: encryption, SSID, RSSI, MAC, channel. Scan time, rounds, and BSSID count are
       configurable.

Request a location with ``AT%NRFCLOUDLOCATION``:

.. code-block:: console

   AT%NRFCLOUDLOCATION=<method>,<fetch_result>

.. list-table:: Data collection methods
   :header-rows: 1

   * - ``<method>``
     - Sources used
   * - 1
     - Single-cell
   * - 2
     - Multicell
   * - 3
     - Single-cell and multicell
   * - 4
     - Wi-Fi only
   * - 5
     - Single-cell and Wi-Fi
   * - 6
     - Multicell and Wi-Fi
   * - 7
     - All

``<fetch_result>`` set to ``1`` returns the position to the host.
Set to ``0``, the module sends the collected data to nRF Cloud and reports only an acknowledgment:

.. code-block:: console

   %NRFCLOUDLOCATION: ACK RECEIVED

Use ``0`` when the position is only needed server side, since it avoids transferring the result back over the air.
For the full syntax, see the `nRF93M1 AT Commands Reference Guide`_.

With ``<fetch_result>`` set to ``1``, the result arrives as:

.. code-block:: console

   %NRFCLOUDLOCATION: <lat>,<lon>,<unc>,<fulfilled_method>

* ``<unc>`` is the uncertainty in meters.
* ``<fulfilled_method>`` is the method the cloud actually used, reported as ``1`` for single-cell, ``2`` for multicell, or ``4`` for Wi-Fi.
  It can differ from the method you requested whenever you allow multiple data sources.

Prefer a combined method such as ``7`` in a deployed device, and log ``<fulfilled_method>`` alongside ``<unc>``.
A device that silently falls back from Wi-Fi to single-cell reports a much larger uncertainty, and without the fulfilled method there is nothing to explain why.

.. caution::
   Multicell and Wi-Fi positioning do not work while the module is in RRC Connected mode.
   Request a location when the connection is idle, not immediately after a data transfer.

.. note::
   Unsolicited ``%COAP`` result codes can be emitted during any cloud transaction.

.. _ug_93m1_cloud_connecting_messaging:

Messaging and shadow
********************

.. list-table::
   :header-rows: 1

   * - Command
     - Purpose
   * - ``AT%NRFCLOUDMESSAGE=[<fmt>,]<message>``
     - Publish a device-to-cloud (D2C) message over CoAP.
       ``<fmt>`` selects the format: ``0`` for JSON (default, up to 511 characters, sent to ``msg/d2c``), ``1`` for CBOR (sent to ``msg/d2c``), or ``2`` for raw binary (sent to ``msg/d2c/raw``).
       For CBOR and raw binary, give the payload as base64, up to 1024 bytes after decoding.
       Only the decoded bytes are sent over the air.
       For example, ``AT%NRFCLOUDMESSAGE={"appId":"BUTTON","data":"1"}`` or ``AT%NRFCLOUDMESSAGE=1,"oWRkYXRhYTE="``.
   * - ``AT%NRFCLOUDSHADOW``
     - Read a dotted path from the desired shadow, and write a dotted path into the reported shadow

.. _ug_93m1_cloud_connecting_observability:

Firmware observability
**********************

The module integrates Memfault firmware observability.
Periodic metrics heartbeats are captured on the device and uploaded over the existing cellular connection.
Modem coredumps are collected when the modem crashes, but they are uploaded only if you enable it.
See :ref:`ug_nrf93m1_debugging_coredump`.

.. list-table::
   :header-rows: 1

   * - Command
     - Purpose
   * - ``AT%NRFCLOUDOBSUPLOAD[="<projectKey>"]``
     - Upload observability data. The optional project key overrides automatic routing.
   * - ``AT%NRFCLOUDOBSHEARTBEAT``
     - Trigger a heartbeat. Debug use.
   * - ``AT%NRFCLOUDOBSFORWARD="<base64>"``
     - Forward observability chunks from the host MCU

Observability is the practical way to diagnose intermittent field faults, where the failure does not reproduce on a bench.

.. note::
   Forwarding host MCU chunks means your host application can report its own diagnostics through the module's existing connection, without opening a second channel.

.. _ug_93m1_cloud_connecting_lte_metrics:

LTE metrics
===========

The modem collects LTE metrics about how the device connects to the network, and reports them to nRF Cloud as part of the observability data.
The modem closes a *heartbeat*, a snapshot of the collected metrics, once every hour and puts it in a small chunk queue.
The module does not upload the queue by itself.
Your host must send ``AT%NRFCLOUDOBSUPLOAD`` regularly.
The queue holds only a few heartbeats: when it is full, new heartbeats are discarded, and the queued data is lost on reset and in deep sleep modes.

The following table lists the metrics.
Some metrics are updated automatically by the modem.
The others are updated only when you run the listed AT command, so run these commands before the heartbeat closes to include them.

.. list-table::
   :header-rows: 1

   * - Metric
     - Description
     - Updated
     - Name in nRF Cloud
   * - Time to connect
     - Time to join the network after start-up or a dropout
     - Automatically
     - ``time_to_connect_ms``
   * - Connection losses
     - How often the device drops off the network
     - Automatically
     - ``connection_loss_count``
   * - Time connected
     - How long the device stayed on the network
     - Automatically
     - ``on_time_ms``
   * - Network type
     - LTE technology that the connection uses
     - Automatically
     - ``mode``
   * - Check-in interval
     - How often a sleeping device checks in (PSM TAU)
     - Automatically
     - ``psm_tau_seconds``
   * - Awake time
     - How long the device stays reachable before sleeping (PSM active time)
     - Automatically
     - ``psm_active_time_seconds``
   * - Listening cycle
     - How often the device listens for the network (eDRX)
     - Automatically
     - ``edrx_interval_ms``
   * - Listening window
     - How long the device listens each time (eDRX paging time window)
     - Automatically
     - ``edrx_ptw_ms``
   * - Signal strength
     - Signal strength from the cell (RSRP, in dBm)
     - When you run ``AT%BCINFO``
     - ``rsrp_dbm``
   * - Frequency band
     - LTE band that the cell uses
     - When you run ``AT%BCINFO``
     - ``band``
   * - Modem firmware
     - Modem firmware version that the device runs
     - First time you run ``AT+CGMR``
     - ``modem_fw_version``
   * - Operator
     - Mobile network that the device is on
     - When you run ``AT+COPS?``
     - ``operator_plmn``

To send LTE metrics to nRF Cloud, complete the following steps:

1. Read the modem firmware version:

   .. code-block:: console

      AT+CGMR
      mfw_nrf93m1_1.5.1
      OK

#. Download the symbol file that matches this modem firmware version from the `nRF93M1 guide in the nRF Cloud documentation <https://docs.nrfcloud.com/docs/mcu/nrf93m1-guide>`_.
   In nRF Cloud, open :guilabel:`Symbol Files` and upload the ``.elf`` file.
   nRF Cloud needs it to decode the metrics.

#. Turn on network status reports, then restart the modem so that it reconnects and records the connection metrics:

   .. code-block:: console

      AT+CEREG=5
      OK
      AT+CFUN=0
      OK
      +CEREG: 0
      AT+CFUN=1
      OK
      +CEREG: 2
      +CEREG: 1,"5A00","00197803",7

#. Update the signal strength and band, and the operator metrics:

   .. code-block:: console

      AT%BCINFO=1
      %BCINFOSC: 1825,49,-85,-6,"244","05","00197803","5A00"
      %BCINFONC: 1825,49,-85,-6,"244","45","00197803","5A00"
      OK
      AT+COPS?
      +COPS: 0,2,"24405",7
      OK

#. Close the current heartbeat and upload the metrics:

   .. code-block:: console

      AT%NRFCLOUDOBSHEARTBEAT
      OK
      AT%NRFCLOUDOBSUPLOAD
      %COAP: RESPONSE,2.01
      %COAP: DATA,8,Accepted
      %COAP: RESPONSE,2.01
      %COAP: DATA,8,Accepted
      %NRFCLOUDOBSUPLOAD: UPLOADED,2
      OK

#. In nRF Cloud, open the device page and check :guilabel:`Timeline` and :guilabel:`Processing Log` for the uploaded metrics.

The ``AT%NRFCLOUDOBSHEARTBEAT`` command is intended for debugging.
In normal operation, rely on the hourly heartbeat and upload regularly.

.. _ug_93m1_cloud_connecting_fota:

Firmware updates
****************

Both modem firmware and host application firmware updates are delivered through nRF Cloud with ``AT%NRFCLOUDFOTA``.
See :ref:`ug_nrf93m1_updating_modem_fw_ota` for the procedure to update the modem firmware of a provisioned device and for the other update methods.

.. _ug_93m1_cloud_connecting_production:

Moving to production
********************

The flow in this guide provisions one device by hand.
A production flow differs in the following ways:

* Credentials are collected and registered in bulk at manufacture, using the CSV upload path, rather than typed into a terminal.
* Your host application issues the AT sequence, rather than a person.
* SIM provisioning uses SGP.32 remote provisioning or SoftSIM rather than the physical trial SIM supplied with the DK. See :ref:`ug_nrf93m1_features_sim`.

Disable power saving during provisioning.
Peripheral and general-purpose I/O functionality must stay available.
See :ref:`ug_nrf93m1_power_optimization_config`.
