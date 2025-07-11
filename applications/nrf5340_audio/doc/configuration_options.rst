.. _nrf53_audio_app_configuration_options:

nRF5340 Audio Kconfig options
#############################

.. contents::
   :local:
   :depth: 2

Following are the application-specific configuration options that can be configured for the nRF5340 Audio applications and their internal modules:

Main audio application configuration
************************************

.. options-from-kconfig:: /../Kconfig
   :show-type:

Main audio module configuration
*******************************

.. options-from-kconfig:: /../src/audio/Kconfig
   :show-type:

Bluetooth module configuration
******************************

.. options-from-kconfig:: /../src/bluetooth/Kconfig
   :show-type:

Bluetooth stream broadcast configuration
========================================

.. options-from-kconfig:: /../src/bluetooth/bt_stream/broadcast/Kconfig
   :show-type:

Bluetooth stream unicast configuration
======================================

.. options-from-kconfig:: /../src/bluetooth/bt_stream/unicast/Kconfig
   :show-type:

Bluetooth rendering and capture volume configuration
====================================================

.. options-from-kconfig:: /../src/bluetooth/bt_rendering_and_capture/volume/Kconfig
   :show-type:

Audio driver configuration
**************************

.. options-from-kconfig:: /../src/drivers/Kconfig
   :show-type:

Audio modules configuration
***************************

.. options-from-kconfig:: /../src/modules/Kconfig
   :show-type:

Audio utils configuration
*************************

.. options-from-kconfig:: /../src/utils/Kconfig
   :show-type:
