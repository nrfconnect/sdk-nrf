.. _nrf_machine_learning_configuration:

nRF Machine Learning: Application-specific Kconfig options
##########################################################

.. contents::
   :local:
   :depth: 2

The nRF Machine Learning introduces Kconfig options that you can use to simplify the application configuration.
The application introduces configuration options related specifically to its modules and data providers.

Configurations for modules
**************************

The nRF Machine Learning application is modular and event driven.
You can enable and configure the modules separately for the selected board and build type.
See the documentation page of the selected module for information about the functionalities provided by the module and its configuration.
See :ref:`nrf_machine_learning_app_internal_modules` for a list of modules available in the application.

Advertising configuration
*************************

If the given build type enables Bluetooth, the :ref:`caf_ble_adv` is used to control the Bluetooth advertising.
The data providers used by the nRF Machine Learning application are configured using the :file:`src/util/Kconfig` file.
The nRF Machine Learning application configures the data providers in :file:`src/util/Kconfig`.
By default, the application enables a set of data providers available in the |NCS| and adds a custom provider that appends UUID128 of Nordic UART Service (NUS) to the scan response data if the NUS is enabled in the configuration and the Bluetooth local identity in use has no bond.

Following are the application-specific configuration options that you can configure for the nRF Machine Learning and the modules:

.. options-from-kconfig::
   :show-type:
