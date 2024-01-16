.. _nrf53_audio_app_configuration:

Configuring the nRF5340 Audio applications
##########################################

.. contents::
   :local:
   :depth: 2

|config|

.. _nrf53_audio_app_configuration_select_bidirectional:

Selecting the CIS unidirectional communication
**********************************************

By default, the nRF5340 build script tries to build the applications in the CIS bidirectional mode.
To switch to the unidirectional mode, set the ``CONFIG_STREAM_BIDIRECTIONAL`` Kconfig option to ``n``  in the :file:`prj.conf` file (for the debug version) or in the :file:`prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_enable_walkie_talkie:

Enabling the walkie-talkie demo
===============================

The walkie-talkie demo uses one or two bidirectional streams from the gateway to one or two headsets.
The PDM microphone is used as input on both the gateway and headset device.
To switch to using the walkie-talkie, set the ``CONFIG_WALKIE_TALKIE_DEMO`` Kconfig option to ``y``  in the :file:`prj.conf` file (for the debug version) or in the :file:`prj_release.conf` file (for the release version).

.. _nrf53_audio_app_configuration_select_i2s:

Selecting the analog jack input using I2S
*****************************************

In the default configuration, the gateway application uses USB as the audio source.
The :ref:`nrf53_audio_app_building` and the testing steps also refer to using the USB serial connection.

To switch to using the 3.5 mm jack analog input, set the ``CONFIG_AUDIO_SOURCE_I2S`` Kconfig option to ``y`` in the :file:`prj.conf` file for the debug version and in the :file:`prj_release.conf` file for the release version.

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
* If you opt for :ref:`nrf53_audio_app_building_standard`, add the ``-DSHIELD=nrf21540ek_fwd`` to the ``west build`` command.
  For example:

  .. code-block:: console

     west build -b nrf5340_audio_dk_nrf5340_cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -DSHIELD=nrf21540ek_fwd -DCONF_FILE=prj_release.conf

To set the TX power output, use the ``CONFIG_NRF_21540_MAIN_TX_POWER`` and ``CONFIG_NRF_21540_PRI_ADV_TX_POWER`` Kconfig options.

.. note::
   When you build the nRF5340 Audio application with the nRF21540 FEM support, the :ref:`lib_bt_ll_acs_nrf53_readme` does not support the +20 dBm setting.
   This is because of a power class restriction in the controller's QDID.

See :ref:`ug_radio_fem` for more information about FEM in the |NCS|.
