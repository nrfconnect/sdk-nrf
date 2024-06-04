.. _ug_nrf53:
.. _nrf53_working:
.. _ug_nrf5340:
.. _ug_thingy53:

Developing with nRF53 Series
############################

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF53 Series devices:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Board targets
     - Documentation
   * - :ref:`zephyr:nrf5340dk_nrf5340`
     - PCA10095
     - ``nrf5340dk/nrf5340/cpuapp``, ``nrf5340dk/nrf5340/cpuapp/ns``, ``nrf5340dk/nrf5340/cpunet``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | `User Guide <nRF5340 DK User Guide_>`_
   * - :ref:`zephyr:nrf5340_audio_dk_nrf5340`
     - PCA10121
     - ``nrf5340_audio_dk/nrf5340/cpuapp``, ``nrf5340_audio_dk/nrf5340/cpuapp/ns``, ``nrf5340_audio_dk/nrf5340/cpunet``
     - | `Hardware Specification <nRF5340 Audio DK Hardware_>`_
       | :ref:`nrf53_audio_app`
   * - :ref:`zephyr:thingy53_nrf5340`
     - PCA20053
     - ``thingy53/nrf5340/cpuapp``, ``thingy53/nrf5340/cpuapp/ns``, ``thingy53/nrf5340/cpunet``
     - `Hardware Specification <Nordic Thingy:53 Hardware_>`_

.. note::
   The nRF5340 PDK has been deprecated with the introduction of the production-level nRF5340 DK.
   To determine if you have a PDK or DK, check the version number on the sticker on your kit.
   If the version is 0.11.0 or higher, the kit is an nRF5340 DK.

   See the `nRF Connect SDK v1.4.0 documentation`_ for the last release supporting the nRF5340 PDK.

:ref:`zephyr:nrf5340_audio_dk_nrf5340` is a custom reference hardware based on the nRF5340 DK.
It is meant for use with for :ref:`nrf53_audio_app`, which integrate the LE Audio standard.
Given their complexity, the applications use custom building and programming procedures.
Refer to their documentation for more information.

The Nordic Thingy:53 is a battery-operated prototyping platform for IoT Systems.
The Nordic Thingy:53 integrates the nRF5340 SoC that supports Bluetooth® Low Energy, IEEE 802.15.4 based protocols and Near Field Communication (NFC).
The nRF5340 is augmented with the nRF21540 RF FEM (Front-end Module) Range extender that has an integrated power amplifier (PA)/low-noise amplifier (LNA) and nPM1100 Power Management IC (PMIC) that has an integrated dual-mode buck regulator and battery charger.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features_nrf53
   building_nrf53
   fota_update_nrf5340
   multi_image_nrf5340
   simultaneous_multi_image_dfu_nrf5340
   serial_recovery
   logging_nrf5340
   thingy53_application_guide
   qspi_xip_guide_nrf5340
