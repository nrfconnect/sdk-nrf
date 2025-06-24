.. _config_audio_app_options:

nRF5340 Audio: Application-specific Kconfig options
###################################################

.. contents::
   :local:
   :depth: 2

Following are the application-specific configuration options that can be configured for the nRF5340 Audio applications and their internal modules:

.. _config_audio_app_options_main:

Main Kconfig options
********************

The following Kconfig options are available for both unicast and broadcast modes.

.. options-from-kconfig:: ../Kconfig
   :show-type:

.. _config_audio_app_options_unicast:

Unicast mode Kconfig options
****************************

The following Kconfig options are available for the Connected Isochronous Stream (CIS) :ref:`transport mode <nrf53_audio_transport_mode_configuration>`.

.. options-from-kconfig:: ../src/bluetooth/bt_stream/unicast/Kconfig
   :show-type:

.. _config_audio_app_options_broadcast:

Broadcast mode Kconfig options
******************************

The following Kconfig options are available for the Broadcast Isochronous Stream (BIS) :ref:`transport mode <nrf53_audio_transport_mode_configuration>`.

.. options-from-kconfig:: ../src/bluetooth/bt_stream/broadcast/Kconfig
   :show-type:
