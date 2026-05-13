.. _nrf53_audio_app:
.. _nrf_audio_app:

nRF Audio applications
##########################

The nRF Audio applications demonstrate audio playback over isochronous channels (ISO) using LC3 codec compression and decompression, as per `Bluetooth® LE Audio specifications`_.

.. note::
   nRF Audio applications and their DFU/FOTA functionality are marked as :ref:`experimental <software_maturity>`.

.. nrf_audio_app_overview_table_start

The following table summarizes the differences between the available nRF Audio applications.

.. list-table:: Differences between nRF Audio applications
   :header-rows: 1

   * - :ref:`Application name (LE Audio role) <nrf_audio_app_overview>`
     - :ref:`Application mode <nrf_audio_app_overview_modes>`
     - Minimum amount of audio devices recommended for testing
     - :ref:`FEM support <nrf_audio_app_adding_FEM_support>`
   * - :ref:`Broadcast sink<nrf_audio_broadcast_sink_app>`
     - BIS (headset)
     - 2
     -
   * - :ref:`Broadcast source<nrf_audio_broadcast_source_app>`
     - BIS (gateway)
     - 2
     - ✔
   * - :ref:`Unicast client<nrf_audio_unicast_client_app>`
     - CIS (gateway)
     - 2 (three for CIS with TWS)
     - ✔
   * - :ref:`Unicast server<nrf_audio_unicast_server_app>`
     - CIS (headset)
     - 2 (three for CIS with TWS)
     - ✔

.. nrf_audio_app_overview_table_end

See the subpages for detailed documentation of each of the nRF Audio applications and their internal modules:

.. _nrf_audio_app_subpages:

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
   doc/adapting_application
   doc/configuration_options
   doc/audio_api
