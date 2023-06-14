.. _SLM_AT_FOTA:

FOTA AT commands
****************

.. contents::
   :local:
   :depth: 2

The following commands list contains AT commands related to firmware-over-the-air updates (FOTA) requests.

FOTA request #XFOTA
===================

The ``#XFOTA`` command sends various types of FOTA requests, based on specific operation codes.

Set command
-----------

The set command allows you to send a FOTA request.

Syntax
~~~~~~

::

   AT#XFOTA=<op>,<file_url>[,<sec_tag>[,<pdn_id>]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Cancel FOTA (during download only).
  * ``1`` - Start FOTA for application update.
  * ``2`` - Start FOTA for modem delta update.
  * ``6`` - Read application image size and version (optional for application FOTA).
  * ``7`` - Read modem scratch space size and offset (optional for modem FOTA).
  * ``8`` - Erase MCUboot secondary slot (optional for application FOTA).
  * ``9`` - Erase modem scratch space (optional for modem FOTA).

* The ``<file url>`` parameter is a string.
  It represents the full HTTP or HTTPS path of the target image to download.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection for downloading the image.
  It is associated with the certificate or PSK.
  Specifying the ``<sec_tag>`` is mandatory when using HTTPS.
* The ``<pdn_id>`` parameter is an integer.
  It represents the Packet Data Network (PDN) ID that can be used instead of the default PDN for downloading.

Response syntax
~~~~~~~~~~~~~~~

::

  #XFOTA: <primary_partition_id>,<MCUBOOT_version>,<img_size>,<img_version>
  #XFOTA: <secondary_partition_id>,<MCUBOOT_version>,<img_size>,<img_version>

* The ``<primary_partition_id>`` and ``<secondary_partition_id>`` integers give the partition ID.
* The ``<MCUBOOT_version>`` major version integer. Value 1 corresponds to MCUboot versions 1.x.y.
* The ``<img_size>`` integer gives the size of the application image in this partition in bytes.
* The ``<img_version>`` string gives the version of the application image as "<major>.<minor>.<revision>+<build_number>".

::

  #XFOTA: <size>,<offset>

* The ``<size>`` integer gives the size of the modem DFU area in bytes.
* The ``<offset>`` integer gives the offset of the firmware image in the modem DFU area.

Example
~~~~~~~

::

   Read image size and version
   at#xfota=6
   #XFOTA: 3,1,295288,"0.0.0+0"
   #XFOTA: 7,1,294376,"0.0.0+0"
   OK

   Application download and activate
   AT#XFOTA=1,"http://remote.host/fota/slm_app_update.bin"
   OK
   #XFOTA: 1,0,0
   ...
   #XFOTA:4,0
   AT#XRESET
   OK
   READY
   #XFOTA: 5,0

   Erase previous image after FOTA
   AT#XFOTA=8
   OK

   Read modem scratch space size and offset
   at#xfota=7
   #XFOTA: 368640,0
   OK

   Erase modem scratch space before FOTA
   AT#XFOTA=9
   OK

   Modem download and activate
   AT#XFOTA=2,"http://remote.host/fota/mfw_nrf9160_update_from_1.3.0_to_1.3.0-FOTA-TEST.bin"
   OK
   #XFOTA: 1,0,0
   ...
   #XFOTA: 4,0
   AT#XRESET
   OK
   READY
   #XFOTA: 5,0

   Application download and activate if NSIB is enabled
   AT#XFOTA=1,"http://remote.host/fota/slm_app_update.bin+slm_app_update.bin"
   OK
   #XFOTA: 1,0,0
   ...
   #XFOTA:4,0
   AT#XRESET
   OK
   READY
   #XFOTA: 5,0

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

   #XFOTA: <fota_stage>,<fota_status>[,<fota_info>]

* The ``<fota_stage>`` value is an integer and can return one of the following values:

  * ``0`` - Init
  * ``1`` - Download
  * ``2`` - Download, erase pending (modem FOTA only)
  * ``3`` - Download, erased (modem FOTA only)
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
  |``5`` (namely *Complete*)| ``1`` (namely *ERROR*)     | Error Code                                                                    |
  +-------------------------+----------------------------+-------------------------------------------------------------------------------+
  |``1`` (namely *Download*)| ``2`` (namely *CANCELLED*) | ``0`` - Downloading is cancelled before completion                            |
  +-------------------------+------------------------+---+-------------------------------------------------------------------------------+

  The error codes can be the following:

  * ``1`` - Download failed
  * ``2`` - Update image rejected (for example modem firmware version error)
  * ``3`` - Update image mismatch (for example ``<op>`` is ``1`` but ``<file_url>`` points to a modem image)

  For modem FOTA, the error codes can be the following:

  * ``0x4400001u`` - The modem encountered a fatal internal error during firmware update.
  * ``0x4400002u`` - The modem encountered a fatal hardware error during firmware update.
  * ``0x4400003u`` - Modem firmware update failed, due to an authentication error.
  * ``0x4400004u`` - Modem firmware update failed, due to UUID mismatch.

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

   #XFOTA: (0,1,2,6,7,8,9),<file_url>,<sec_tag>,<apn>

   OK
