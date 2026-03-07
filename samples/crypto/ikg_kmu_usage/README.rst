.. _crypto_ikg_kmu_usage:

Crypto: IKG and KMU usage with CRACEN
#####################################

.. contents::
   :local:
   :depth: 2

The IKG and KMU usage with CRACEN sample demonstrates how to use the IKG to derive keys as well as using protected keys in Key Management Unit (KMU) on devices with the CRACEN hardware peripheral.

Overview
********

The KMU usage with CRACEN sample enables the :ref:`PSA Crypto API <psa_crypto_support_enable>` to generate IKG seed and protected keys to store persistently in KMU.
These keys are stored in the :ref:`Key Management Unit (KMU) <ug_nrf54l_developing_basics_kmu>` of the device.
The KMU is one of the :ref:`key storage <key_storage>` options in the |NCS|.

The sample uses the :ref:`Oberon PSA Crypto <ug_crypto_architecture_implementation_standards_oberon>` and the :ref:`crypto_drivers_cracen`.
The usage of the :ref:`crypto_drivers_oberon` is disabled in the sample.
