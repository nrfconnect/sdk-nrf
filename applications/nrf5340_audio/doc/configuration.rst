.. _nrf53_audio_app_configuration:

Configuring the nRF5340 Audio applications
##########################################

.. contents::
   :local:
   :depth: 2

|config|

By default, if you have not made any changes to :file:`.conf` files at :file:`applications/nrf5340_audio/`, the nRF5340 :ref:`build script <nrf53_audio_app_building>` builds the :ref:`unicast server (CIS) <nrf53_audio_unicast_server_app>` application in the CIS unidirectional mode as a headset (with :kconfig:option:`CONFIG_TRANSPORT_CIS` set to ``y`` and :kconfig:option:`CONFIG_AUDIO_DEV` set to ``1``).

.. _nrf53_audio_app_configuration_select_build:

Selecting gateway or headset build
**********************************

Given the nRF5340 Audio :ref:`application architecture <nrf53_audio_app_overview>`, the nRF5340 Audio applications can be built for :ref:`either the gateway or the headset role <nrf53_audio_app_overview_gateway_headsets>`:

* The headset build is identified with :kconfig:option:`CONFIG_AUDIO_DEV` Kconfig option set to ``1``.
  This is the default configuration.
* The gateway build can be selected by adding :kconfig:option:`CONFIG_AUDIO_DEV` Kconfig option set to ``2`` to the :file:`prj.conf` file.

.. _nrf53_audio_app_configuration_select_bidirectional:

Selecting the CIS bidirectional communication
*********************************************

To switch to the bidirectional mode, set the ``CONFIG_STREAM_BIDIRECTIONAL`` Kconfig option to ``y``  in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_enable_walkie_talkie:

Enabling the walkie-talkie demo
===============================

The walkie-talkie demo uses one or two bidirectional streams from the gateway to one or two headsets.
The PDM microphone is used as input on both the gateway and headset device.
To switch to using the walkie-talkie, set the ``CONFIG_WALKIE_TALKIE_DEMO`` Kconfig option to ``y``  in the :file:`applications/nrf5340_audio/prj.conf` file (for the debug version) or in the :file:`applications/nrf5340_audio/prj_release.conf` file (for the release version).

Enabling the Auracast™ (broadcast) mode
=======================================

If you want to work with `Auracast™`_ (broadcast) sources and sinks, set the :kconfig:option:`CONFIG_TRANSPORT_BIS` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file.

.. _nrf53_audio_app_configuration_select_bis_two_gateways:

Enabling the BIS mode with two gateways
***************************************

In addition to the standard BIS mode with one gateway, you can also add a second gateway device.
The BIS headsets can then switch between the two gateways and receive audio stream from one of the two gateways.

To configure the second gateway, add both the ``CONFIG_TRANSPORT_BIS`` and the ``CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT`` Kconfig options set to ``y`` to the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and to the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.
You can provide an alternative name to the second gateway using the ``CONFIG_BT_AUDIO_BROADCAST_NAME_ALT`` or use the default alternative name.

You build each BIS gateway separately using the normal procedures from :ref:`nrf53_audio_app_building`.
After building the first gateway, configure the required Kconfig options for the second gateway and build the second gateway firmware.
Remember to program the two firmware versions to two separate gateway devices.

.. _nrf53_audio_app_configuration_select_i2s:

Selecting the analog jack input using I2S
*****************************************

In the default configuration, the gateway application uses USB as the audio source.
The :ref:`nrf53_audio_app_building` and the testing steps also refer to using the USB serial connection.

To switch to using the 3.5 mm jack analog input, set the ``CONFIG_AUDIO_SOURCE_I2S`` Kconfig option to ``y`` in the :file:`applications/nrf5340_audio/prj.conf` file for the debug version and in the :file:`applications/nrf5340_audio/prj_release.conf` file for the release version.

When testing the application, an additional audio jack cable is required to use I2S.
Use this cable to connect the audio source (PC) to the analog **LINE IN** on the development kit.

.. _nrf53_audio_app_adding_FEM_support:

Adding FEM support
******************

You can add support for the nRF21540 front-end module (FEM) to the following nRF5340 Audio applications:

* :ref:`Broadcast source <nrf53_audio_broadcast_source_app>`
* :ref:`Unicast client <nrf53_audio_unicast_client_app>`
* :ref:`Unicast server <nrf53_audio_unicast_server_app>`

The :ref:`broadcast sink application <nrf53_audio_broadcast_sink_app>` does not need FEM support as it only receives data.

Adding FEM support happens when :ref:`nrf53_audio_app_building`.
You can use one of the following options, depending on how you decide to build the application:

* If you opt for :ref:`nrf53_audio_app_building_script`, add the ``--nrf21540`` to the script's building command.
* If you opt for :ref:`nrf53_audio_app_building_standard`, add the ``-Dnrf5340_audio_SHIELD=nrf21540ek -Dipc_radio_SHIELD=nrf21540ek`` to the ``west build`` command.
  For example:

  .. code-block:: console

     west build -b nrf5340_audio_dk/nrf5340/cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -Dnrf5340_audio_SHIELD=nrf21540ek -Dipc_radio_SHIELD=nrf21540ek

To set the TX power output, use the ``CONFIG_BT_CTLR_TX_PWR_ANTENNA`` and ``CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB`` Kconfig options in :file:`applications/nrf5340_audio/sysbuild/ipc_radio/prj.conf`.

See :ref:`ug_radio_fem` for more information about FEM in the |NCS|.
