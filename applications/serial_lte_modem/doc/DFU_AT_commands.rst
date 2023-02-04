.. _SLM_AT_DFU:

DFU AT commands
****************

.. contents::
   :local:
   :depth: 2

The following commands list contains AT commands related to DFU requests.

The SLM application supports the following types of cloud-to-device Device Firmware Update (DFU) for the nRF52 Series of MCUs:

* The application DFU based on MCUboot.
  This is supported by |NCS|.
  To use this, define :ref:`CONFIG_SLM_NRF52_DFU <CONFIG_SLM_NRF52_DFU>` but leave :ref:`CONFIG_SLM_NRF52_DFU_LEGACY <CONFIG_SLM_NRF52_DFU_LEGACY>` undefined.
* The legacy serial DFU.
  This is supported by the legacy nRF5 SDK.
  To use this, define both :ref:`CONFIG_SLM_NRF52_DFU <CONFIG_SLM_NRF52_DFU>` and :ref:`CONFIG_SLM_NRF52_DFU_LEGACY <CONFIG_SLM_NRF52_DFU_LEGACY>`.

.. note::
   You must configure SLM to use ``UART_2`` for the DFU service.

Get request #XDFUGET
====================

The ``#XDFUGET`` command sends various types of requests based on specific operation codes.

Set command
-----------

The set command allows you to send a DFU download request.

Syntax
~~~~~~

::

   AT#XDFUGET=<op>[,<host>,<image_1>[,<image_2>[,<sec_tag>]]]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Cancel DFU (during download only).
  * ``1`` - Start downloading ``<image_1>`` and ``<image_2>``.
  * ``8`` - Erase MCUboot secondary slot.

* The ``<host>`` parameter is a string.
  It indicates the HTTP or HTTPS server name and optionally a port number.
* The ``<image_1>`` parameter is a string.
  It indicates the first file image to download.
  If :ref:`CONFIG_SLM_NRF52_DFU_LEGACY <CONFIG_SLM_NRF52_DFU_LEGACY>` is defined, you must set ``<image_1>`` as the path to the init packet file (:file:`.dat`).
  Otherwise, you must set ``<image_1>`` as the path to the MCUboot application update file (:file:`.bin`).
* The ``<image_2>`` parameter is a string.
  It indicates the second file image to download.
  If :ref:`CONFIG_SLM_NRF52_DFU_LEGACY <CONFIG_SLM_NRF52_DFU_LEGACY>` is defined, you must set ``<image_2>`` as the path to the file image (:file:`.bin`).
  Otherwise, this parameter should not be set.
* The ``<sec_tag>`` parameter is an integer.
  It indicates to the modem the credential of the security tag used for establishing a secure connection for downloading the image.
  It is associated with the certificate or PSK.
  Specifying the ``<sec_tag>`` is mandatory when using HTTPS.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

::

  #XDFUGET: <dfu_step>,<info>

* The ``<dfu_step>`` value is an integer.
  It indicates which image is being downloaded.
* The ``<info>`` value is an integer.
  It can return the following values:

  * A value between ``1`` and ``100`` - The percentage of the download.
  * Any other value - Error code.

Examples
~~~~~~~~

Get the image files for the legacy DFU from ``http://myserver.com/path/*.*``:

::

   AT#XDFUGET=1,"http://myserver.com","path/nrf52840_xxaa.dat","path/nrf52840_xxaa.bin"
   OK
   #XDFUGET: 1,14
   ...
   #XDFUGET: 1,100

Erase the previous image after DFU:

::

   AT#XDFUGET=8
   OK

Get the image files for the |NCS| DFU from ``http://myserver.com/path/*.*``:

::

   AT#XDFUGET=1,"https://myserver.com","path/nrf52_app_update.bin","",1234
   OK
   #XDFUGET: 0,14
   ...
   #XDFUGET: 0,100

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XDFUGET=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XDFUGET: <list of op value>,<host>,<image_1>,<image_2>,<sec_tag>

Examples
~~~~~~~~

::

   AT#XDFUGET=?
   #XDFUGET: (0,1,8),<host>,<image_1>,<image_2>,<sec_tag>
   OK

Get file size #XDFUSIZE
=======================

The ``#XDFUSIZE`` command returns the size of the downloaded (:file:`.bin`) image file.

Set command
-----------

The set command allows you to get the size of the downloaded image file.

Syntax
~~~~~~

::

   AT#XDFUSIZE

Response syntax
~~~~~~~~~~~~~~~

::

  XDFUSIZE: <file_size>,<download_size>,<crc32_checksum>

* The ``<file_size>`` is an integer.
  It indicates the size of the DFU image file.
* The ``<download_size>`` is an integer.
  It indicates the size that has been downloaded so far.
* The ``<crc32_checksum>`` is a 32-bit unsigned integer.
  It indicates the IEEE CRC32 of the image content that has been downloaded.
  You can use this CRC32 to check data integrity when you use MCUboot to perform a DFU.

  You should check the CRC checksum after ``#XDFURUN`` has transferred the downloaded data image from nRF91 to nRF5x, before configuring MCUboot to use the new image.

Examples
~~~~~~~~

::

   #XDFUGET: 1,100
   OK

   AT#XDFUSIZE
   #XDFUSIZE: 17048,17048,2316010324
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

Run request #XDFURUN
====================

The ``#XDFURUN`` command starts to run the DFU protocol over the UART connection.

Set command
-----------

The set command allows you to send a DFU run request.

Syntax
~~~~~~

::

   AT#XDFURUN=<start_delay>[,<mtu>,<pause>]

* The ``<start_delay>`` parameter is an integer.
  It indicates the duration of the delay, in seconds, before the application starts to run the DFU protocol.
  This allows the nRF52 SoC to make the necessary preparations, like swapping to bootloader mode.
* The ``<mtu>`` parameter is an integer.
  It indicates the size of the data chunk that is sent from the SLM to the nRF52.
  This parameter should be a multiple of 256 bytes and should not be greater than 4096 bytes.
  This parameter is ignored if :ref:`CONFIG_SLM_NRF52_DFU_LEGACY <CONFIG_SLM_NRF52_DFU_LEGACY>` is defined.
* The ``<pause>`` parameter is an integer.
  It indicates the time, in milliseconds, that the SLM pauses after sending the data chunk of ``<mtu>`` size.
  This parameter must not be zero.
  This parameter is ignored if :ref:`CONFIG_SLM_NRF52_DFU_LEGACY <CONFIG_SLM_NRF52_DFU_LEGACY>` is defined.


Response syntax
~~~~~~~~~~~~~~~

::

  #XDFURUN: <dfu_step>,<info>

* The ``<dfu_step>`` is an integer.
  It indicates which step of the DFU protocol is being executed.
* The ``<info>`` is an integer.
  It returns an error code when an error happens.

Examples
~~~~~~~~

Run the legacy serial DFU protocol:

::

   AT#XDFURUN=2
   OK

Run the MCUboot-based DFU protocol:

::

   AT#XDFURUN=1,1024,200
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XDFURUN=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XDFURUN: <start_delay>,<mtu>,<pause>

Examples
~~~~~~~~

::

   AT#XDFURUN=?

   #XDFURUN: <start_delay>,<mtu>,<pause>

   OK
