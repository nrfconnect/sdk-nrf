.. _ug_nrf54l_developing_provision_kmu:


nRF54L15 KMU provisioning
#########################

.. contents::
   :local:
   :depth: 2

The nRF54L15 DK is equipped with Hardware Key Management Unit (KMU), that requires provisioning when in use.
The |NCS| provides a west command, ``ncs-provision``, allowing to upload keys to the device though the Serial Write Debug (SWD) interface.

Prerequisites
*************

First, ensure that the nrfprovision script is installed.
It should install automatically during the setup of the |NCS| working environment.
If it was not installed, or if you wish to install it manually, run the following command:

.. parsed-literal::
   :class: highlight

    pip install nrfprovision==0.9.0 --extra-index-url https://files.nordicsemi.com/artifactory/api/pypi/nordic-pypi/simple


Key generation
**************

If you need a new key, you can generate it using imgtool or another tool that produces the required kind and format of key.
For instructions on how to generate a key, see the :doc:`imgtool page in the MCUboot documentation<mcuboot:imgtool>`.
See the following example for generating a private key:

.. parsed-literal::
   :class: highlight

   imgtool keygen -k my_ed25519_priv_key.pem -t ed25519

Provisioning keys to the board
******************************

Before uploading keys, ensure that the SoC is unprovisioned.
If the SoC has been previously provisioned and you need to use a different set of keys, you must first erase the SoC with the erase command:

.. tabs::

    .. group-tab:: erase using nrfutil

      .. parsed-literal::
         :class: highlight

          nrfutil device erase

    .. group-tab:: erase using nrfjprog

      .. parsed-literal::
         :class: highlight

          nrfjprog --eraseall

Once you have an unprovisioned SoC, upload keys to the board by running the following command:

.. parsed-literal::
   :class: highlight

    west ncs-provision upload -s nrf54l15 -k ed25519.pem -k ed25519-1.pem -k ed25519-2.pem

* Parameter ``-s (-–soc)`` specifies the target device.
  Currently, only the nRF54L15 DK is supported.

* Parameter ``-k (-–key)`` specifies the private key PEM files to be provisioned to the SoC.
  You can specify up to three keys.

* Parameter ``--dev-id`` specifies the interface serial number and should be used if multiple J-link interfaces are connected to the development machine.

The script generates the public key for each private key and uploads them to your device.
These public keys generate the verification keys for the application image, which are then used by MCUboot for validation.
The first key specified in the command is used for signing the application image.
Currently, the script supports only ED25519 Keys.
