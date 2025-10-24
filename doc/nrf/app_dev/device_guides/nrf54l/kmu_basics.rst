.. _ug_nrf54l_developing_basics_kmu:

Introduction to KMU key provisioning
####################################

.. contents::
   :local:
   :depth: 2

.. note::

   The MCUboot bootloader does not yet support KMU for nRF54LM20.

The nRF54L devices are equipped with a Key Management Unit (KMU) that facilitates secure and confidential storage of keys in a dedicated region of RRAM (secure information configuration region, or SICR).
The process of storing a key and its metadata in a key slot in SICR is known as the KMU provisioning.

This feature is crucial not only for private keys but also for public keys, as the :ref:`KMU can directly transfer a key to the CRACEN RAM<ug_nrf54l_crypto_kmu_cracen_peripherals>`.
Even when keys must pass through addressable RAM, the KMU significantly reduces the risk of key exposure.
Therefore, you should use KMU for managing secrets whenever possible.

When using an nRF54L device with Trusted Firmware-M, you can use the KMU to store keys instead of using the :ref:`ug_tfm_services_its`.
For this to work, you need to enable the :ref:`tfm_partition_crypto`, which enables the :ref:`ug_tfm_services_its` by default.
You can then manually disable the ITS service and start using KMU instead.

.. _ug_nrf54l_developing_basics_kmu_key_types:

Key types
*********

Different types of keys, such as revocable and locked keys, serve distinct purposes and have unique policies associated with their use and management.
In the PSA abstraction, key types are mapped by the ``psa_set_key_lifetime`` function.
Refer to :ref:`PSA Key programming model<ug_nrf54l_crypto_kmu_key_programming_model>` for details.

Revocable keys
==============

Revocable keys can be set as invalid for further use, which prevents the keys from being reused or new keys from being provisioned onto the same slots.
For these keys, the revocation policy (RPOLICY) must be marked as ``revoked``.
For some technical solutions, a few keys of the same key type (for example, generation 0, 1, 2) might be provisioned.
At any given time, only one generation should be active.
If a key generation is compromised, it should be revoked to prevent its use.
The specifics of the revocation scheme depend on the technical solution, but it is recommended to designate one generation as non-revocable (``locked``) to prevent a Denial of Service (DoS) attack on the key's availability for the solution.

Locked keys
===========

Once provisioned, locked keys are permanently available for use and cannot be deleted without erasing the device.
For these keys, the revocation policy (RPOLICY) must be marked as ``locked``.

.. _ug_nrf54l_developing_basics_kmu_provisioning_keys:

Provisioning keys for the bootloader
************************************

The bootloader can use multiple key generations for image verification (up to three for nRF54L SoCs).
To safeguard against unauthorized provisioning by attackers, you must :ref:`provision all key generations onto the device<ug_nrf54l_developing_provision_kmu>`.

Make sure to provision the setup’s relevant key sets before any run with bootloaders, including the first boot.
Failure to do so can lead to unwanted actions by the bootloader on your firmware setup.
You may experience the following issues:

  * The nRF Secure Immutable Bootloader (NSIB) will mark the image as permanently invalid without a key available for verification.
  * In direct-xip mode, MCUboot will delete the image if no appropriate key is provisioned.
  * The firmware will simply not boot, indicating a lack of proper key provisioning.

By default, MCUboot uses a single key.
You can configure the number of key generations that MCUboot uses for application verification with the ``CONFIG_BOOT_SIGNATURE_KMU_SLOTS`` MCUboot's Kconfig option.

NSIB for nRF54L SoCs supports three key generations.

Limitations on key types and trade-offs
***************************************

Access to the KMU is restricted to secure code.
When an SoC runs applications code as secure, the application has some control over the KMU.
A provisioned key cannot be overwritten, but its content might be blocked from use at runtime (push block mechanism) by unexpected routines.
However, applications can revoke a revocable key on the nRF54L15, nRF54L10, and nRF54L05 devices.
The revoked key might not necessarily belong to an application; it could, for example, belong to a bootloader.

Revocable KMU keys are exposed to revocation by applications running in secure mode.
If you cannot trust the application running in the secure mode, you should provision locked keys.
You can consider the following options:

* Supporting key generations with revocable keys - The advantage of this method is that the application or bootloader can revoke a key generation if the private key is compromised or lost.
  The disadvantage is that a malicious application could also revoke the key.
* Supporting one locked key - The advantage of this method is that the key cannot be deleted by the application.
  The disadvantage is that the method does not support multiple generations of keys.

Always consider the specific security needs of your application and choose the most appropriate key management approach to safeguard your digital assets.

More information
****************

For more conceptual information about the KMU hardware peripheral, see the page about KMU in the device datasheet, for example `KMU - Key management unit <nRF54L15 Key management unit_>`_ in the nRF54L15 datasheet.

For the description of the KMU provisioning procedure, see :ref:`ug_nrf54l_developing_provision_kmu` in the |NCS| documentation and the `Provisioning KMU data`_ page in the `nRF54L Series Production Programming`_ guide.
