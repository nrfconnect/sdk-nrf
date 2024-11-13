.. _ug_nrf54h20_kms:

SDFW platform key management
############################

.. contents::
   :local:
   :depth: 2

Platform keys are non-volatile keys that defines the ownership of a local domain.
There are two types of platform keys: symmetric AES256 keys and asymmetric ECC public keys (Twisted Edwards).
These keys are imported using ``psa_import_key`` as any other key, using a specific set of attributes.

Preconditions
-------------

It is only possible to import platform keys for local domains that are not in a protected :ref:`Local Domain Life Cycle State (LDLCS) <_secdom_LDLCS>`.
A platform key can only be stored if a key with the same key identifier has not already been stored.
The only way to erase a platform key is to purge the corresponding local domain.

Importing and managing keys
---------------------------

##To be defined
##nrfutil support to be clarified

Post-condition
--------------

Once a platform key has been imported it can be leveraged by SDFW.
To verify that a key has been correctly imported it must be used in a cryptographic operation where the host knows the expected result.
For symmetric keys this can be done by decrypting some data with a known plaintext using ``AEAD``.
For asymmetric keys this is done by invoking ``psa_export_public_key`` and checking the result.
