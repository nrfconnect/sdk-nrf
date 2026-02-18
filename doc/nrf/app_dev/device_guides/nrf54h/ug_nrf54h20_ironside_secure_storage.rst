.. _ug_nrf54h20_ironside_se_secure_storage:
.. _ug_nrf54h20_ironside_secure_storage:

Configuring secure storage
##########################

.. contents::
   :local:
   :depth: 2

|ISE| provides secure storage functionality through the :kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE` configuration.
This feature enables applications to store sensitive data in dedicated, encrypted storage regions that are protected by device-unique keys and access controls.

Configuring UICR.SECURESTORAGE
==============================

The :kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE` field configures secure storage regions for PSA Crypto keys and PSA Internal Trusted Storage (ITS) data.
To leverage this secure storage functionality, applications must set the key location to ``PSA_KEY_LOCATION_LOCAL_STORAGE`` (``0x000000``).

The secure storage configuration includes two separate storage regions:

* UICR.SECURESTORAGE.CRYPTO - Used for PSA Crypto API operations when storing cryptographic keys
* UICR.SECURESTORAGE.ITS - Used for PSA Internal Trusted Storage (ITS) API operations when storing general secure data


Secure storage through PSA Crypto API
=====================================

When using the PSA Crypto API to operate on keys, the storage region specified by UICR.SECURESTORAGE.CRYPTO is automatically used if the key attributes are configured with ``key location`` set to ``PSA_KEY_LOCATION_LOCAL_STORAGE``.

This ensures that cryptographic keys are stored in the dedicated secure storage region rather than in regular application memory.

Secure storage through PSA ITS API
==================================

When using the PSA ITS API for storing general secure data, the storage region specified by UICR.SECURESTORAGE.ITS is used automatically.
No special configuration is required for PSA ITS operations, as they inherently use the secure storage when available.

Security properties
===================

The secure storage provided by |ISE| has the following security characteristics:

Access control
--------------

* *Domain Isolation*: Secure storage regions are not accessible by local domains directly.
* *Ironside Exclusive Access*: Only the |ISE| can access the secure storage regions.
* *Domain Separation*: Each local domain can only access its own secure storage data, ensuring isolation between different domains.

Data protection
---------------

* *Encryption*: All data stored in the secure storage regions is encrypted using device-unique keys.
* *Integrity*: The stored data is protected against tampering through cryptographic integrity checks.
* *Confidentiality*: The encryption ensures that stored data remains confidential even if the storage medium is physically accessed.

.. note::
   The device-unique encryption keys are managed entirely by |ISE| and are not accessible to application code.
   This ensures that the secure storage remains protected even in cases where application-level vulnerabilities exist.

Configuration considerations
============================

When configuring secure storage, consider the following:

* Ensure sufficient storage space is allocated in both UICR.SECURESTORAGE.CRYPTO and UICR.SECURESTORAGE.ITS regions based on your application's requirements.
* The sum of these two regions must be 4kB aligned.
* The secure storage regions should be properly sized to accommodate the expected number of keys and data items.
* Access to secure storage is only available when the key location is explicitly set to ``PSA_KEY_LOCATION_LOCAL_STORAGE``.
