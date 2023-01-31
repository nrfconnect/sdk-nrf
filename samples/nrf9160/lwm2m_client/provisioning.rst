.. _lwm2m_client_provisioning:

Preparing for production
########################

.. contents::
   :local:
   :depth: 2

To use the :ref:`lwm2m_client` sample in production, you must prepare the sample for production by completing the following steps:

#. Program the :ref:`at_client_sample` sample to your device.
#. Provision the identity and security credentials.
#. Program the LwM2M client sample.

.. figure:: /images/lwm2m_client_production.svg
   :alt: LwM2M client production diagram

   Preparing the sample for production

The following sections provide the guidelines on setting up the sample for production using AVSystem's `Coiote Device Management`_ server.

Programming the AT Client sample
********************************

You must program the :ref:`at_client_sample` sample to your device to control the security tags in the modem.
See `nRF91 AT Commands Reference Guide <AT Commands Reference Guide_>`_ for documentation on each AT command.
Also, you must provision the bootstrap credentials for the security tag (that you have specified in :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_BOOTSTRAP_TLS_TAG` Kconfig option) to the nRF9160 modem.

Provisioning the identity and security credentials
**************************************************

To provision the credentials, complete the following steps:

1. Ensure that you have removed the previous security tags from the modem by issuing the `AT%CMNG <Credential storage management %CMNG_>`_ command:

   .. code-block:: none

      AT%CMNG=3,<TAG>,3
      AT%CMNG=3,<TAG>,4


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

For automated provisioning of credentials, you can use the script :file:`provision.py` that is available in the :file:`samples/nrf9160/lwm2m_client/scripts/` folder.
To set up the script, you must set your username and password for the AVSystem's Coiote Device Management server as environment variables and pass the device serial port as a parameter when you run the script.
See the following code:

.. code-block:: console

   # Setup phase
   [nrf@dev]:~/scripts# export COIOTE_PASSWD='my-password'
   [nrf@dev]:~/scripts# export COIOTE_USER='my-username'

   # Find the serial port
   [nrf@dev]:~/scripts# nrfjprog -f NRF91 --com
   960033095    /dev/ttyACM0    VCOM0
   960033095    /dev/ttyACM1    VCOM1
   960033095    /dev/ttyACM2    VCOM2

   # Run
   [nrf@dev]:~/scripts# ./provision.py /dev/ttyACM0
   AT interface ready
   Identity: urn:imei:352656100394546
   Security tag 35724862 cleared
   PSK credentials stored to sec_tag 35724862
   Coiote: Deleted device urn:imei:352656100394546
   Coiote: Deleted device urn:imei:352656100394546-bs
   Coiote: Created device urn:imei:352656100394546 to domain /IoT/NordicSemi/Interop/


You can now program the device with the final sample image.

Configuring and programing the sample
*************************************

To configure and program the sample, complete the following steps:

1. Make the sample programmable to multiple devices by removing all hard coded credentials. This can be done by setting the :ref:`CONFIG_APP_LWM2M_PSK <CONFIG_APP_LWM2M_PSK>` Kconfig option to empty value.
#. Enable bootstrapping using the configuration overlay file :file:`overlay-avsystem-bootstrap.conf`.
   Bootstrapping is required for an LwM2M client to rotate security credentials.
#. Prepare the production script or steps for your nRF9160-based device.
#. Program the sample.
