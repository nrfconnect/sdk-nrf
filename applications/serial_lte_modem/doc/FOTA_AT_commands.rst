.. _SLM_AT_FOTA:

FOTA AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes AT commands related to Firmware Over-The-Air (FOTA) updates.

FOTA request #XFOTA
===================

The ``#XFOTA`` command sends various types of FOTA requests, based on specific operation codes.

Set command
-----------

The set command performs FOTA-related operations.

Syntax
~~~~~~

::

   AT#XFOTA=<op>[,<file_url>[,<sec_tag>[,<pdn_id>]]]

* The ``<op>`` parameter must be one of the following values:

  * ``0`` - Stop FOTA (during download only).
    Can be used as a way to pause and resume the download, by issuing the same FOTA start command to resume.
  * ``1`` - Start FOTA for application update.
  * ``2`` - Start FOTA for modem delta update.
  * ``3`` - Start FOTA for full modem update.
    Can only be used when the :file:`overlay-full_fota.conf` configuration file is used.

    Not supported on the :ref:`Thingy:91 <thingy91_ug_intro>` or nRF9160 DK board revisions older than 0.14.0 as they lack an external flash to store the firmware image.
    See :ref:`nrf9160_board_revisions` for more details.
    Also, not currently supported on Thingy:91 X.

  * ``7`` - Read modem DFU area size and firmware image offset (for modem delta update).
  * ``9`` - Erase modem DFU area (for modem delta update).

* The ``<file url>`` parameter is a string.
  It represents the full HTTP or HTTPS path of the target image to download.
  It must be provided to the start operations.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection for downloading the image.
  It is associated with the certificate or PSK.
  It must be provided to the start operations when using HTTPS.
* The ``<pdn_id>`` parameter is an integer.
  It represents the Packet Data Network (PDN) ID that can be used instead of the default PDN for downloading.
  It can be provided to the start operations.

.. note::

   During a delta update to the modem, the modem DFU area is automatically erased as needed as part of the update process. Erasing this area manually is optional.

   However, this leads to the command starting the update taking longer to complete, and also leaves the connection to the FOTA server idle while the area is being erased, which can provoke issues.

.. note::

   The firmware image is stored in external flash memory during a full modem update.
   The external flash is erased automatically after a new firmware activation.

   Activating the new modem firmware is done identically to a modem delta update, by resetting either the whole device or only the modem.

Response syntax
~~~~~~~~~~~~~~~

::

  AT#XFOTA=7
  #XFOTA: <size>,<offset>

* The ``<size>`` integer gives the size of the modem DFU area in bytes.
* The ``<offset>`` integer gives the offset of the firmware image in the modem DFU area.
  It is ``0`` if no image is in the modem DFU area, and ``2621440`` (:c:macro:`NRF_MODEM_DELTA_DFU_OFFSET_DIRTY`) if the modem DFU area needs to be erased before a new firmware update can be received.

Example
~~~~~~~

::

   application firmware download and update
   AT#XFOTA=1,"http://remote.host/fota/slm_app_update.bin"
   OK
   #XFOTA: 1,0,0
   ...
   #XFOTA: 4,0
   AT#XRESET
   OK
   Ready
   #XFOTA: 5,0

   read modem DFU info
   AT#XFOTA=7
   #XFOTA: 294912,0
   OK

   modem firmware download and update
   AT#XFOTA=2,"http://remote.host/fota/mfw_nrf9160_update_from_1.3.4_to_1.3.5.bin"
   OK
   #XFOTA: 1,0,0
   ...
   #XFOTA: 4,0
   AT#XMODEMRESET
   #XFOTA: 5,0
   #XMODEMRESET: 0
   OK

   read modem DFU info
   AT#XFOTA=7
   #XFOTA: 294912,2621440
   OK

   erase modem DFU area for next modem delta update (optional)
   AT#XFOTA=9
   OK

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XFOTA: <fota_stage>,<fota_status>[,<fota_info>]

* The ``<fota_stage>`` value is an integer and can return one of the following values:

  * ``0`` - Init
  * ``1`` - Download
  * ``2`` - Download, erase pending (modem delta update only)
  * ``3`` - Download, erase complete (modem delta update only)
  * ``4`` - Downloaded, to be activated
  * ``5`` - Complete

* The ``<fota_status>`` value is an integer and can return one of the following values:

  * ``0`` - OK
  * ``1`` - Error
  * ``2`` - Cancelled
  * ``3`` - Reverted (application FOTA only)

* The ``<fota_info>`` value is an integer.
  Its value can have different meanings based on the values returned by ``<fota_stage>`` and ``<fota_status>``.
  See the following table:

  +-------------------------+----------------------------+-------------------------------------------------------------------------------+
  |``<fota_stage>`` value   |``<fota_status>`` value     | ``<fota_info>`` value                                                         |
  +=========================+============================+===============================================================================+
  |``1`` (namely *Download*)| ``0`` (namely *OK*)        | Percentage of the download                                                    |
  +-------------------------+----------------------------+-------------------------------------------------------------------------------+
  |``1`` (namely *Download*)| ``1`` (namely *ERROR*)     | Error Code                                                                    |
  +-------------------------+----------------------------+-------------------------------------------------------------------------------+
  |``1`` (namely *Download*)| ``2`` (namely *CANCELLED*) | ``0`` - Downloading is cancelled before completion                            |
  +-------------------------+----------------------------+-------------------------------------------------------------------------------+
  |``5`` (namely *Complete*)| ``1`` (namely *ERROR*)     | Error Code                                                                    |
  +-------------------------+------------------------+---+-------------------------------------------------------------------------------+

  The error codes can be the following:

  * ``1`` - Download failed
  * ``2`` - Update image rejected (for example modem firmware version error)
  * ``3`` - Update image mismatch (for example ``<op>`` is ``1`` but ``<file_url>`` points to a modem image)

  For modem FOTA, the error codes can be the following:

  * ``71303169`` (:c:macro:`NRF_MODEM_DFU_RESULT_INTERNAL_ERROR`) - The modem encountered a fatal internal error during firmware update.
  * ``71303170`` (:c:macro:`NRF_MODEM_DFU_RESULT_HARDWARE_ERROR`) - The modem encountered a fatal hardware error during firmware update.
  * ``71303171`` (:c:macro:`NRF_MODEM_DFU_RESULT_AUTH_ERROR`) - Modem firmware update failed due to an authentication error.
  * ``71303172`` (:c:macro:`NRF_MODEM_DFU_RESULT_UUID_ERROR`) - Modem firmware update failed due to UUID mismatch.
  * ``71303173`` (:c:macro:`NRF_MODEM_DFU_RESULT_VOLTAGE_LOW`) - Modem firmware update not executed due to low voltage. The modem will retry the update on reboot.

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XFOTA=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XFOTA: <list of op value>,<file_url>,<sec_tag>,<apn>

Examples
~~~~~~~~

::

   AT#XFOTA=?

   #XFOTA: (0,1,2,3,7,9),<file_url>,<sec_tag>,<apn>

   OK
