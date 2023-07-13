.. _nrf53_audio_app:

nRF5340 Audio
#############

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio application demonstrates audio playback over isochronous channels (ISO) using LC3 codec compression and decompression, as per `Bluetooth® LE Audio specifications`_.

.. note::
   There is an ongoing process of restructuring the nRF5340 Audio application project.
   Several drivers and modules within the application folder are being moved to more suitable locations in the |NCS| or Zephyr.
   Before this process has finished, developing out-of-tree applications can be more complex.

.. note::
   This application and its DFU/FOTA functionality are marked as :ref:`experimental <software_maturity>`.

See the subpages for detailed documentation on the application and its internal modules:

.. _nrf53_audio_app_subpages:

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   firmware_architecture
   feature_support
   ../README
   adapting_application
