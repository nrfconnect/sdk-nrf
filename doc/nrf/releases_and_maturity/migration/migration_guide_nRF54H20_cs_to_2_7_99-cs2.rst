:orphan:

.. _migration_nrf54h20_to_2.7.99-cs2:

Migration notes for |NCS| v2.7.99-cs2 and the nRF54H20 DK
#########################################################

This document describes what existing users of the |NCS| v2.7.0 and v2.7.99-cs1 must consider when migrating their environment to the |NCS| v2.7.99-cs2:

nRF54H20 DK compatibility
  The |NCS| v2.7.99-cs2 is compatible only with the nRF54H20 DK, version PCA10175 v0.8.0 (ES4) or later revisions.

|NCS| v2.7.99-cs2
  You can update your |NCS| v2.7.0 installation and toolchain to v2.7.99-cs2 by using the Toolchain Manager:

  1. Open `nRF Connect for Desktop`_.
  #. Open the Toolchain Manager application in nRF Connect for Desktop.
  #. Click the button with the arrow pointing down next to the installed |NCS| version to expand the drop-down menu with options.
  #. In the drop-down menu, click :guilabel:`Update toolchain`.
  #. In the same drop-down menu, click :guilabel:`Update SDK`.

nrfutil device
  ``nrfutil device`` has been updated to version 2.5.4.

  Install the nRF Util ``device`` command version 2.5.4, as follows::

     nrfutil install device=2.5.4 --force

  For more information, consult the `nRF Util`_ documentation.

nrfutil trace
  ``nrfutil trace`` has been updated to version 2.11.0.

  Install the nRF Util ``trace`` command version 2.11.0 as follows::

     nrfutil install trace=2.11.0 --force

  For more information, consult the `nRF Util`_ documentation.

nRF54H20 BICR
  The nRF54H20 BICR has been updated (from the one supporting |NCS| v2.7.0).

  .. note::
     BICR update is not required if migrating from |NCS| 2.7.99-cs1.

  To update the BICR of your development kit while in Root of Trust, do the following:

  1. Download the `nRF54H20 DK BICR binary file`_.
  #. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

     .. note::
        On MacOS, connecting the DK might repeatedly trigger a popup displaying the message ``Disk Not Ejected Properly``.
        To disable this, run ``JLinkExe``, then run ``MSDDisable`` in the J-Link Commander interface.

  #. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

        nrfutil device list

  #. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

        nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware <path_to_bicr.hex> --core Application --serial-number <serialnumber>

SDFW and SCFW firmwares
  The *nRF54H20 SoC binaries* bundle has been updated to version 0.6.5.

  .. caution::
     If migrating from |NCS| v2.7.0, before proceeding with the SoC binaries update, you must first update the BICR as described in the previous chapter.

  To update the SoC binaries bundle of your development kit while in Root of Trust, do the following:

  1. Download the `nRF54H20 SoC binaries v0.6.5`_.

     .. note::
        On MacOS, ensure that the ZIP file is not unpacked automatically upon download.

  #. Move the bundle's :file:`.zip` archive to a folder of your choice, unzip the archive, and then run nRF Util to program the :file:`nordic_top.suit` using the following command::

        nrfutil device x-suit-dfu --firmware nordic_top.suit --serial-number <serialnumber>

New SEGGER J-Link version
  A new version of SEGGER J-Link is supported: `SEGGER J-Link` version 7.94i.

  .. note::
     On Windows, to update to the new J-link version, including the USB Driver for J-Link, you must manually install J-Link v7.94i from the command line, using the ``-InstUSBDriver=1`` parameter:

    1. Navigate to the download location of the J-Link executable and run one of the following commands:

        * From the Command Prompt::

             JLink_Windows_V794i_x86_64.exe -InstUSBDriver=1

        * From PowerShell::

             .\JLink_Windows_V794i_x86_64.exe -InstUSBDriver=1

    #. In the :guilabel:`Choose optional components` window, select :guilabel:`update existing installation`.
    #. Add the J-Link executable to the system path on Linux and MacOS, or to the environment variables on Windows, to run it from anywhere on the system.
