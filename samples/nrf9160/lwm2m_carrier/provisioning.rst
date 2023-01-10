.. _lwm2m_carrier_provisioning:

Preparing for production
########################

.. contents::
   :local:
   :depth: 2

Before deploying devices that use the :ref:`liblwm2m_carrier_readme` library, consider the following prerequisites.

.. _lwm2m_root_ca_certs:

Root CA certificates
********************

You can optimize your application by removing or adding certificates based on your product needs.
Refer to the respective documentation of one or more of your operators for more information on which certificates apply.

The :ref:`lwm2m_carrier` sample contains a couple of root CA certificates that are typically used for out-of-band firmware updates.
The carrier with which you plan to certify your product might not require the included certificates.
Also, the certificates needed by the carrier might not be present in the sample.
Hence it might become necessary to program the required certificates.

Pre-programming the certificates
================================

The certificates are typically stored in the modem.
The LwM2M carrier sample writes the certificates to the modem if they are not already present.
Alternatively, you can also write the certificates beforehand using the :ref:`at_client_sample` sample.

To pre-program the certificates, use one of the following methods:

* Build and flash the :ref:`at_client_sample` sample and provision the certificates using AT commands and the `LTE Link Monitor`_ application. See `Managing credentials`_ for more information.
* Build and flash the LwM2M carrier sample, while making sure that the appropriate certificates (:file:`*.pem`) are included in the :file:`certs` folder and referenced in :file:`carrier_certs.c`.

Other samples and applications like :ref:`asset_tracker_v2` and :ref:`modem_shell_application` with the carrier library integration do not write any certificates in the application.

Pre-shared Key (PSK)
********************

In live (production) environment, the correct PSK is generated and stored in the modem depending on which operator network the device is in.
The application should not provide a custom security tag using :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` in this case.

If the device is operating outside the supported operator network, then the custom security tag will be applied.

Carrier configuration and testing
*********************************

* The library supports carriers which have specific device management requirements.
  Not all requirements are documented here.
  You must contact your carrier for more information as mentioned in the :ref:`lwm2m_certification` documentation.

  * With a few exceptions, if you leave the :c:struct:`lwm2m_carrier_config_t` structure empty when calling :c:func:`lwm2m_carrier_main`, it configures the carrier library to behave correctly in the operator network it is currently connected to.

* The settings required to test and certify your product with the carrier will be different from the settings needed for mass deployment.

  * When :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is not set (when :c:struct:`lwm2m_carrier_config_t` is empty), the URI is predetermined to connect to the live device management server of the currently connected operator network.
  * During certification process, the :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` and :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` Kconfig options must be set accordingly to connect to the carrier's test (certification) servers instead of the live (production) servers.
    See :ref:`lwm2m_app_int` for more information on the required configurations.
  * During certification, only one carrier should be enabled using :c:macro:`carriers_enabled`.
    For example, when connecting to the Verizon's test servers, :kconfig:option:`CONFIG_LWM2M_CARRIER_VERIZON` must be set to ``y``, and the other Kconfig options must be explicitly set to ``n``, as they are enabled by default.

LG U+ operator requirements
===========================

Following are the configurations are required for using the library with the LG U+ operator network:

* Once the initial connection to device management is compete, the application must use :c:func:`lwm2m_carrier_request` when it wishes to reboot, or connect/disconnect from the network.
* :kconfig:option:`CONFIG_DFU_TARGET_MCUBOOT` is required to perform application FOTA.
  This in turn enables the Kconfig option :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS`.
* :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE` must be set.
  This service code is reflected in the Model Number resource of the Device object.
  Contact the carrier to obtain the correct service code.
* :kconfig:option:`CONFIG_LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NUMBER` can be changed depending on your product.
  Contact the carrier for more information.
* :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_APN` is not used when the subscriber ID is ``LG U+``.
