.. _lwm2m_client_provisioning:

Preparing for production
########################

.. contents::
   :local:
   :depth: 2

To use the :ref:`lwm2m_client` sample in production, you must prepare the sample for production by completing the following steps:

#. Program the :ref:`at_client_sample` sample to your device.
#. Provision the identity and security credentials.
#. Program the LwM2M Client sample.

.. figure:: /images/lwm2m_client_production.svg
   :alt: LwM2M Client production diagram

   Preparing the sample for production

The following sections provide the guidelines on setting up the sample for production using AVSystem's `Coiote Device Management`_ server.

Programming the AT Client sample
********************************

You must program the :ref:`at_client_sample` sample to your device to control the security tags in the modem.
See the `nRF91x1 AT Commands Reference Guide`_  or `nRF9160 AT Commands Reference Guide`_ for documentation on each AT command.
Also, you must provision the bootstrap credentials for the security tag (that you have specified in :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG` Kconfig option) to the nRF91 Series modem.

Provisioning the identity and security credentials
**************************************************

To provision the credentials, complete the following steps:

1. Ensure that you have removed the previous security tags from the modem by issuing the ``AT%CMNG`` command:

   .. code-block:: none

      AT%CMNG=3,<TAG>,3
      AT%CMNG=3,<TAG>,4

   See the `Credential storage management %CMNG`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 credential storage management %CMNG_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

#. Identify the device IMEI by issuing the command ``AT+CGSN``:

   .. code-block:: none

      AT+CGSN
      352656100367872
      OK

#. Create an identity ``urn:imei:<IMEI CODE>`` based on the IMEI of your device.
   For the example in previous step, the identity of the device is ``urn:imei:352656100367872``.

#. Generate a secure PSK key and store that to the security tag.
   For example, to write the key ``000102030405060708090a0b0c0d0e0f``, run the following commands:

   .. code-block:: none

      AT%CMNG=0,<TAG>,4,"urn:imei:352656100367872"
      OK
      AT%CMNG=0,<TAG>,3,"000102030405060708090a0b0c0d0e0f"
      OK

Automated provisioning
----------------------

For automated provisioning of credentials, you can use the script :file:`provision.py` that is available in the :file:`samples/cellular/lwm2m_client/scripts/` folder.
If you use AVSystem's Coiote Device Management server, you must set your username and password for the server as environment variables when you run the script.
See the following code:

.. code-block:: console

   # Setup phase
   [nrf@dev]:~/scripts# export COIOTE_PASSWD='my-password'
   [nrf@dev]:~/scripts# export COIOTE_USER='my-username'

   # Run
   [nrf@dev]:~/scripts# ./provision.py
   AT interface ready
   Identity: urn:imei:352656100394546
   Security tag 35724862 cleared
   PSK credentials stored to sec_tag 35724862
   Coiote: Deleted device urn:imei:352656100394546
   Coiote: Deleted device urn:imei:352656100394546-bs
   Coiote: Created device urn:imei:352656100394546 to domain /IoT/NordicSemi/Interop/

When Leshan demo server is used, script does not require password:

.. code-block:: console

   # Run
   [nrf@dev]:~/scripts# ./provision.py --leshan
   [INFO] provision.py - Identity: urn:imei:351358814369747
   [INFO] device.py - Security tag 35724861 cleared
   [INFO] device.py - Security tag 35724862 cleared
   [INFO] device.py - PSK credentials stored to sec_tag 35724862

You can now program the device with the final sample image.

Configuring and programing the sample
*************************************

To configure and program the sample, complete the following steps:

1. Make the sample programmable to multiple devices by removing all hard coded credentials. This can be done by setting the :ref:`CONFIG_APP_LWM2M_PSK <CONFIG_APP_LWM2M_PSK>` Kconfig option to empty value.
#. Enable bootstrapping using the configuration overlay file :file:`overlay-avsystem-bootstrap.conf` or :file:`overlay-leshan-bootstrap.conf`.
   Bootstrapping is required for an LwM2M Client to rotate security credentials.
#. Prepare the production script or steps for your nRF91 Series device.
#. Program the sample.
