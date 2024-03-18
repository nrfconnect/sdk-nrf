.. _nrf53_audio_app:

nRF5340 Audio applications
##########################

The nRF5340 Audio applications demonstrate audio playback over isochronous channels (ISO) using LC3 codec compression and decompression, as per `Bluetooth® LE Audio specifications`_.

.. note::
   nRF5340 Audio applications and their DFU/FOTA functionality are marked as :ref:`experimental <software_maturity>`.

The following table summarizes the differences between the available nRF5340 Audio applications.

.. list-table:: Differences between nRF5340 Audio applications
   :header-rows: 1

   * - :ref:`Application name (LE Audio role) <nrf53_audio_app_overview>`
     - :ref:`Application mode <nrf53_audio_app_overview_modes>`
     - Minimum amount of nRF5340 Audio DKs recommended for testing
     - :ref:`FEM support <nrf53_audio_app_adding_FEM_support>`
   * - :ref:`Broadcast sink<nrf53_audio_broadcast_sink_app>`
     - BIS (headset)
     - 2
     -
   * - :ref:`Broadcast source<nrf53_audio_broadcast_source_app>`
     - BIS (gateway)
     - 2
     - ✔
   * - :ref:`Unicast client<nrf53_audio_unicast_client_app>`
     - CIS (gateway)
     - 3
     - ✔
   * - :ref:`Unicast server<nrf53_audio_unicast_server_app>`
     - CIS (headset)
     - 3
     - ✔

See the subpages for detailed documentation of each of the nRF5340 applications and their internal modules:

.. _nrf53_audio_app_subpages:

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   doc/firmware_architecture
   doc/feature_support
   doc/requirements
   doc/user_interface
   doc/configuration
   doc/building
   broadcast_sink/README
   broadcast_source/README
   unicast_client/README
   unicast_server/README
   doc/fota
   doc/adapting_application
