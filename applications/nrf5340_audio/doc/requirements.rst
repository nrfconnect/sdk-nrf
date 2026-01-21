.. _nrf53_audio_app_requirements:

nRF5340 Audio application requirements
######################################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio applications are designed to be used only with the following hardware:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340_audio_dk_nrf5340, nrf5340dk_nrf5340_cpuapp

You need at least two nRF5340 development kits (one with the gateway firmware and one with headset firmware) to test each of the applications.
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

The applications use the same :ref:`nrf53_audio_app_overview_files` and the same :file:`Kconfig.defaults` file in the :file:`nrf5340_audio` directory.
You can however select different :file:`*.conf` files to build them as either debug or release.
Each nRF5340 Audio application also uses its own, application-specific overlay file.

You need the following configuration files to :ref:`build the application <nrf53_audio_app_building>`:

* Application version configuration file: :file:`prj.conf` or :file:`prj_release.conf`.
* Application-specific overlay file (:file:`overlay-<app_name>.conf`) from the application directory.

Optionally, you can use the following configuration file:

* FOTA DFU configuration file: :file:`prj_fota.conf` for building the debug version of the application (:file:`prj.conf`), but with the features needed to perform DFU over Bluetooth LE.
  See :ref:`nrf53_audio_app_fota` for more information.

When building using the command line, you must explicitly specify the :file:`*.conf` files that are going to be included.
See :ref:`nrf53_audio_app_building_standard` for more information.

When building using the script, you :ref:`specify parameters for building <nrf53_audio_app_building_script_running>` instead of the file names.
