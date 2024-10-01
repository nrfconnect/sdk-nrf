.. _nrf53_audio_app_requirements:

nRF5340 Audio application requirements
######################################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio applications are designed to be used only with the following hardware:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340_audio_dk_nrf5340

.. note::
   The applications supports PCA10121 revisions 1.0.0 or above.
   The applications are also compatible with the following pre-launch revisions:

   * Revisions 0.8.0 and above.

You need at least two nRF5340 Audio development kits (one with the gateway firmware and one with headset firmware) to test each of the applications.
For CIS with TWS in mind, three kits are required.

If you want to test with other hardware (for example, a mobile phone or PC), it is highly recommended to test with Audio DKs on both the gateway and headset side first to verify basic functionality before moving on to testing with other vendors.

.. _nrf53_audio_app_requirements_codec:

Software codec requirements
***************************

The nRF5340 Audio applications only support the :ref:`LC3 software codec <nrfxlib:lc3>`, developed specifically for use with LE Audio.

The applications can be configured for other alternative codecs, but this integration is beyond the scope of this documentation.

.. _nrf53_audio_app_dk:
.. _nrf53_audio_app_dk_features:

nRF5340 Audio development kit
*****************************

The nRF5340 Audio development kit is a hardware development platform that demonstrates the nRF5340 Audio applications.
Read the `nRF5340 Audio DK Hardware`_ documentation for more information about this development kit.

You can :ref:`test the DK out of the box <nrf53_audio_app_dk_testing_out_of_the_box>` before you program it.

.. _nrf53_audio_app_configuration_files:

nRF5340 Audio configuration files
*********************************

All applications use the :file:`Kconfig.defaults` located in the :file:`nrf5340_audio` directory.
Additionally, each nRF5340 Audio application uses its own, application-specific :file:`Kconfig.defaults` file from the application directory, which includes configuration specific to the given application.
These files change the configuration defaults automatically, based on the different application versions and device types.

For each application, only one of the following :file:`.conf` files is included when building:

* :file:`prj.conf` is the default configuration file and it implements the debug application version.
* :file:`prj_release.conf` is the optional configuration file and it implements the release application version.
  No debug features are enabled in the release application version.
  When building using the command line, you must explicitly specify if :file:`prj_release.conf` is going to be included instead of :file:`prj.conf`.
  See :ref:`nrf53_audio_app_building` for details.

In addition, the application features the :file:`child_image` directory with :file:`hci_ipc.conf`.
This file contains the necessary configurations for nRF5340 Audio applications to run the :zephyr:code-sample:`bluetooth_hci_ipc` sample with :ref:`SoftDevice Controller for LE Isochronous Channels <nrfxlib:softdevice_controller_iso>` support.
