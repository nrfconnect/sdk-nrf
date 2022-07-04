.. _lwm2m_carrier:

nRF9160: LwM2M carrier
######################

.. contents::
   :local:
   :depth: 2

The LwM2M carrier sample demonstrates how to run the :ref:`liblwm2m_carrier_readme` library in an application in order to connect to the operator LwM2M network.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

The sample is configured to compile and run as a non-secure application on nRF91's Cortex-M33.
Therefore, it automatically includes the :ref:`secure_partition_manager` that prepares the required peripherals to be available for the application.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lwm2m_carrier`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample and all prerequisites to the development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the kit prints the following information::

        LWM2M Carrier library sample.
#. Observe that the application receives events from the :ref:`liblwm2m_carrier_readme` library using the registered event handler.


Configuration
*************

|config|


.. _dtls_support_carrier:

DTLS Support
============

The sample has DTLS security enabled by default.
You need to provide the following information to the LwM2M server before you can make a successful connection:

* Client endpoint
* Identity
* `Pre-Shared Key (PSK)`_

See :ref:`server setup <server_setup_lwm2m_carrier>` for instructions on providing the information to the server.


Setup
=====

Before building and running the sample, complete the following steps:

1. Select the device to be tested.
2. Select the LwM2M server to be used for testing and register the device on it.
3. Configure the application to work with the chosen LwM2M server.

.. _server_setup_lwm2m_carrier:

Server setup
------------

The following instructions describe how to register your device to `Leshan Demo Server`_ or `Coiote Device Management server`_:

1. For adding the device to the LwM2M server, complete the following steps and for adding the device to an LwM2M bootstrap server, see the procedure in :ref:`registering the device to an LwM2M bootstrap server <bootstrap_server_reg>`:

   .. tabs::

      .. tab:: Leshan Demo Server

         1. Open the `Leshan Demo Server web UI`_.
         #. Click on :guilabel:`SECURITY` in the upper right corner in the UI.
         #. Click on :guilabel:`ADD SECURITY INFORMATION`.
         #. Enter the following data and click :guilabel:`ADD`:

            * Endpoint - urn\:imei\:*your Device IMEI*
            * Security Mode - psk
            * Identity: - urn\:imei\:*your Device IMEI*
            * Key - 000102030405060708090a0b0c0d0e0f

      .. tab:: Coiote Device Management

         1. Open `Coiote Device Management server`_.
         #. Click on :guilabel:`Device inventory` in the left menu in the UI.
         #. Click on :guilabel:`Add new device`.
         #. Click on :guilabel:`Connect your LwM2M device directly via the management server`.
         #. Enter the following data and click :guilabel:`Add device`:

            * Endpoint - urn\:imei\:*your Device IMEI*
            * Friendly Name - *recognizable name*
            * Security mode - psk (Pre-Shared Key)
            * Key - 000102030405060708090a0b0c0d0e0f

            Also, make sure to select the :guilabel:`Key in hexadecimal` checkbox.

   .. _bootstrap_server_reg:

   For registering the device to an LwM2M bootstrap server, complete the following steps:

   .. tabs::

      .. tab:: Leshan Demo Server

         1. Open the `Leshan Bootstrap Server Demo web UI <public Leshan Bootstrap Server Demo_>`_.
         #. Click on :guilabel:`BOOTSTRAP` in the top right corner.
         #. In the :guilabel:`BOOTSTRAP` tab, click on :guilabel:`ADD CLIENTS CONFIGURATION`.
         #. Click on :guilabel:`Add clients configuration`.
         #. Enter your Client Endpoint name - urn\:imei\:*your device IMEI*.
         #. Click :guilabel:`NEXT` and select :guilabel:`Using (D)TLS` and enter following data:

            * Identity - urn\:imei\:*your device IMEI*
            * Key - ``000102030405060708090a0b0c0d0e0f``
         #. Click :guilabel:`NEXT` and leave default paths to be deleted.
         #. Click :guilabel:`NEXT` and in the :guilabel:`LWM2M Server Configuration` section, enter the following data:

            * Server URL - ``coaps://leshan.eclipseprojects.io:5684``
            * Select :guilabel:`Pre-shared Key` as the :guilabel:`Security Mode`
            * Identity - urn\:imei\:*your device IMEI*
            * Key - ``000102030405060708090a0b0c0d0e0f``

            This information is used when your client connects to the server.
            If you choose :guilabel:`Pre-shared Key`, you must add the values for :guilabel:`Identity` and :guilabel:`Key` fields (the configured Key need not match the Bootstrap Server configuration).
            The same credentials must be provided in the :guilabel:`Leshan Demo Server Security configuration` page (see :ref:`_dtls_support_carrier` for instructions).

         #. Click :guilabel:`NEXT` and do not select :guilabel:`Add a Bootstrap Server`.
         #. Click :guilabel:`ADD`.


      .. tab:: Coiote Device Management

         1. Open `Coiote Device Management server`_.
         #. Click on :guilabel:`Device inventory` in the menu on the left.
         #. Click on :guilabel:`Add new device`.
         #. Click on :guilabel:`Connect your LwM2M device via the Bootstrap server`.
         #. Enter the following data and click :guilabel:`Configuration`:

            * Endpoint - urn\:imei\:*your Device IMEI*
            * Friendly Name - *recognisable name*
            * Security mode - psk (Pre-Shared Key)
            * Key - 000102030405060708090a0b0c0d0e0f

            Also, make sure to select the :guilabel:`Key in hexadecimal` checkbox.

            The Coiote bootstrap server automatically creates an account for the LwM2M server using the same device endpoint name and random PSK key.

         #. Click :guilabel:`Add device`.

.. note::

   The :guilabel:`Client Configuration` page of the LWM2M Bootstrap server and the :guilabel:`Registered Clients` page of the LWM2M server display only a limited number of devices by default.
   You can increase the number of displayed devices from the drop-down menu associated with :guilabel:`Rows per page`.
   In both cases, the menu is displayed at the bottom-right corner of the :guilabel:`Client Configuration` pages.

2. Set up the server address and PSK in the client:

   a. Open :file:`prj.conf`.
   #. Set :kconfig:option:`CONFIG_LWM2M_CARRIER_USE_CUSTOM_URI` and set :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` to the correct server URL:

      * For `Leshan Demo Server`_ - ``coaps://leshan.eclipseprojects.io:5684`` (`public Leshan Demo Server`_).
      * For `Coiote Device Management`_ - ``coaps://eu.iot.avsystem.cloud:5684`` (`Coiote Device Management server`_).
      * For `Leshan Bootstrap Server Demo web UI <public Leshan Bootstrap Server Demo_>`_ - ``coaps://leshan.eclipseprojects.io:5784``
      * For Coiote bootstrap server - ``coaps://eu.iot.avsystem.cloud:5694``
   #. Set :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP` if bootstrap is used. If bootstrap is not used, set :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME` to specify the lifetime of the LwM2M server.
   #. Set :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_PSK` to the hexadecimal representation of the PSK used when registering the device with the server.
   #. Disable :kconfig:option:`CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD` to give precedence to the custom configuration over the configuration in the SIM card.
  For more information, see the :ref:`lwm2m_configuration` section in the LwM2M carrier library documentation.

Troubleshooting
===============

Bootstrapping can take several minutes.
This is expected and dependent on the availability of the LTE link.
During bootstrap, the application will receive the :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP` and :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN` events.
This is expected and is part of the bootstrapping procedure.
For more information, see the :ref:`lwm2m_events` and :ref:`lwm2m_msc` sections in the LwM2M carrier library documentation.

To completely restart and trigger a new bootstrap, the device must be erased and re-programmed, as mentioned in :ref:`lwm2m_app_int`.


Dependencies
************

This sample uses the following |NCS| libraries:

* |NCS| modules abstracted by the LwM2M carrier OS abstraction layer (:file:`lwm2m_os.h`)

  .. include:: /libraries/bin/lwm2m_carrier/app_integration.rst
    :start-after: lwm2m_osal_mod_list_start
    :end-before: lwm2m_osal_mod_list_end

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
