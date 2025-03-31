.. _ug_nrf54h20_suit_signing:

Signing of SUIT envelopes
#########################

.. contents::
   :local:
   :depth: 2

This document describes how to sign the envelopes when using SUIT for booting and upgrading the firmware.

Signing SUIT envelopes
**********************

This section provides the following:

* A quick step-by-step guide on how to enable signing SUIT envelopes and verify the signature during boot and update procedures.
* The process to customize the signing process for more advanced use cases.

SUIT signing and verification process
=====================================

The SUIT envelope is not signed directly.
Instead, a CBOR structure, ``Sig_structure``, is created and used as the payload for signing.
This structure includes the following elements:

* A context string
* Protected headers, such as the signing algorithm and key ID
* The digest of the manifest

For more information, see `CBOR Object Signing and Encryption (COSE): Structures and Process`_.

The resulting signature is embedded in the SUIT envelope using the ``COSE_Sign_Tagged`` structure, located within the ``SUIT_Authentication_Block``.
To check whether the ``COSE_Sign_Tagged`` structure is present in a signed SUIT envelope, use the ``suit-generator parse`` command.

When parsing the envelope on the device, the ``Sig_structure`` is recreated and used to verify the signature using the public key stored on the device.


Basic step-by-step signing guide
================================

In this scenario, private keys are stored in a local directory on the build host.
The algorithm used is EdDSA with the Ed25519 curve.
For testing purposes, OpenSSL is used to generate the key pairs.

To enable signing of recovery envelopes, see :ref:`Using custom private key file names <ug_custom_private_key_file_names>`.

.. caution::
   Storing keys in a local directory poses a potential security risk.
   Use this approach only for development purposes or when building in a secure environment.

To sign the envelopes, follow these steps:

1. Prepare the private and public keys.
   In this example, the keys are stored in the ``<KEYS_DIR>`` directory.

   To simplify the build process, use the default private key file names:

   * Application private key: ``MANIFEST_APPLICATION_GEN1_priv.pem``
   * Radio private key: ``MANIFEST_RADIOCORE_GEN1_priv.pem``
   * OEM root private key: ``MANIFEST_OEM_ROOT_GEN1_priv.pem``

   To use custom private key file names, see :ref:`Signing the recovery envelopes <ug_suit_sign_recovery_envelopes>`.

   To generate the private keys using OpenSSL, run the following commands:

   .. code-block:: bash

      openssl genpkey -algorithm Ed25519 -out <KEYS_DIR>/MANIFEST_APPLICATION_GEN1_priv.pem
      openssl genpkey -algorithm Ed25519 -out <KEYS_DIR>/MANIFEST_RADIOCORE_GEN1_priv.pem
      openssl genpkey -algorithm Ed25519 -out <KEYS_DIR>/MANIFEST_OEM_ROOT_GEN1_priv.pem

   To generate the corresponding public keys from the private keys, use the following commands:

   .. code-block:: bash

      openssl pkey -in <KEYS_DIR>/MANIFEST_APPLICATION_GEN1_priv.pem -pubout -out <KEYS_DIR>/MANIFEST_APPLICATION_GEN1_pub.pem
      openssl pkey -in <KEYS_DIR>/MANIFEST_RADIOCORE_GEN1_priv.pem -pubout -out <KEYS_DIR>/MANIFEST_RADIOCORE_GEN1_pub.pem
      openssl pkey -in <KEYS_DIR>/MANIFEST_OEM_ROOT_GEN1_priv.pem -pubout -out <KEYS_DIR>/MANIFEST_OEM_ROOT_GEN1_pub.pem

#. Modify the SUIT MPI configuration to enable signature verification of the SUIT envelopes.
   To generate an appropriate MPI configuration during the build process, set the following sysbuild configuration options if signature verification is required during both the boot and update processes:

   * :kconfig:option:`SB_CONFIG_SUIT_MPI_ROOT_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`
   * :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_LOCAL_1_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`
   * :kconfig:option:`SB_CONFIG_SUIT_MPI_RAD_LOCAL_1_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`

   If signature verification is required only during the update process, set the following sysbuild configuration options:

   * :kconfig:option:`SB_CONFIG_SUIT_MPI_ROOT_SIGNATURE_CHECK_ENABLED_ON_UPDATE`
   * :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_LOCAL_1_SIGNATURE_CHECK_ENABLED_ON_UPDATE`
   * :kconfig:option:`SB_CONFIG_SUIT_MPI_RAD_LOCAL_1_SIGNATURE_CHECK_ENABLED_ON_UPDATE`

   These options are required if the OEM Root, ``APP_LOCAL_1``, and ``RAD_LOCAL_1`` manifests are signed.
   If other manifests are signed, set the corresponding configuration options.
   For example, to enable signature verification for ``APP_LOCAL_2`` during boot and update, set :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_LOCAL_2_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`.

   The MPI files are flashed along with the application when running ``west flash``.
   However, similar to the UICR regions, they cannot be updated through the DFU process.

#. Enable signing of the SUIT envelopes within the build system.
   To do this, set the following configuration options:

   * :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN` for each image that requires a signed SUIT envelope.
   * :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE_ROOT_SIGN` if the OEM root envelope must be signed.

#. Point the build system to the directory where the keys are stored by setting :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE_BASIC_KMS_SCRIPT_KEY_DIRECTORY` to ``"<KEYS_DIR>"``.

   .. note::
      If this option is passed to the ``west build`` command, the double quotes must be escaped.
      For example: ``-DSB_CONFIG_SUIT_ENVELOPE_BASIC_KMS_SCRIPT_KEY_DIRECTORY="\"<KEYS_DIR>\""``.

#. Build and flash the application to the device.
   Ensure that the application is running.

#. Provision the public keys to the device.
   After provisioning, reboot the device.
   For more information, see :ref:`ug_nrf54h20_keys`.

#. Transition the local domains to the life cycle state (LCS) ``DEPLOYED``.
   Signature verification is skipped for a given domain if its LCS is not ``DEPLOYED``.

   You can perform the transition using the following commands:

   .. code-block:: bash

      nrfutil device x-adac-local-domain-lcs-set --life-cycle ld-rot --x-domain application --serial-number <dk_serial_number>
      nrfutil device x-adac-local-domain-lcs-set --life-cycle ld-rot --x-domain radiocore --serial-number <dk_serial_number>
      nrfutil device x-adac-local-domain-lcs-set --life-cycle ld-deployed --x-domain application --serial-number <dk_serial_number>
      nrfutil device x-adac-local-domain-lcs-set --life-cycle ld-deployed --x-domain radiocore --serial-number <dk_serial_number>

   After this, signature verification is performed during the update and/or boot process, depending on the MPI configuration.

   .. note::
      To revert to the LCS ``EMPTY`` (for example, when repeating the process for testing) use the following commands:

      * ``nrfutil device recover --core Application`` – To transition the application core
      * ``nrfutil device recover --core Network`` – To transition the radio core

.. _ug_suit_sign_recovery_envelopes:

Signing the recovery envelopes
------------------------------

In the default case, where the :kconfig:option:`SB_CONFIG_SUIT_RECOVERY_APPLICATION_IMAGE_MANIFEST_APP_LOCAL_3`  Kconfig option is set to ``y``, the ``APP_RECOVERY`` envelope manages the recovery dependency envelopes.
The ``APP_LOCAL_3`` envelope manages the application recovery image.

Both envelopes must be signed with an Application Domain key using the following Kconfig options:

* To sign the ``APP_RECOVERY`` envelope, set :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_SIGN` to ``y``.
* To sign the ``APP_LOCAL_3`` envelope, enable :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN` for the recovery image.

For the radio recovery image, only the ``RAD_RECOVERY`` envelope must be signed using a Radio Domain key.
Enable the :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN` Kconfig option to sign the ``RAD_RECOVERY`` envelope.

You must also set the appropriate MPI configuration options.

To verify the signature of the recovery envelopes during the boot and update processes, set the following MPI configuration options:

* :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_RECOVERY_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`
* :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_LOCAL_3_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`
* :kconfig:option:`SB_CONFIG_SUIT_MPI_RAD_RECOVERY_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT`

To verify the signature of the recovery envelopes only during the update process, set the following MPI configuration options:

* :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_RECOVERY_SIGNATURE_CHECK_ENABLED_ON_UPDATE`
* :kconfig:option:`SB_CONFIG_SUIT_MPI_APP_LOCAL_3_SIGNATURE_CHECK_ENABLED_ON_UPDATE`
* :kconfig:option:`SB_CONFIG_SUIT_MPI_RAD_RECOVERY_SIGNATURE_CHECK_ENABLED_ON_UPDATE`

Customizing signing of the SUIT envelopes
=========================================

This section describes how to customize the signing process for more advanced use cases.

.. _ug_custom_private_key_file_names:

Using custom private key file names
-----------------------------------

You can configure the name used to identify the private key by setting the following options:

* :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN_PRIVATE_KEY_NAME` for images corresponding to SUIT local envelopes.
* :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE_ROOT_SIGN_PRIVATE_KEY_NAME` for the OEM root envelope.
* :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_SIGN_PRIVATE_KEY_NAME` for the ``APP_RECOVERY`` envelope.

By default, the build system searches the keys directory for a file named ``<private_key_name>.pem``, where ``<private_key_name>`` is the value of the corresponding configuration option.

If this file is not found, the build system searches for a file named ``<private_key_name>.der``.

Changing the used signature algorithm
-------------------------------------

To change the signature algorithm used for signing a SUIT local envelope, the appropriate configuration option from the choice :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN_ALG` must be set.
The corresponding choices for the OEM root and ``APP_RECOVERY`` envelopes are ``SB_CONFIG_SUIT_ENVELOPE_ROOT_SIGN_ALG`` and ``SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_SIGN_ALG``, respectively.

The following algorithms are available:

* EdDSA
* HashEdDSA (ed25519ph)

Changing the used key generation
--------------------------------

If a key has been compromised, it is possible to change the key generation used for signing the SUIT local envelope.

To change the key generation used for signing a SUIT local envelope, set the appropriate configuration option :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN_KEY_GEN<x>`, where ``<x>`` is the generation number.
For example, :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN_KEY_GEN2`.

The corresponding options for the OEM root and ``APP_RECOVERY`` envelopes are the following:

* ``SB_CONFIG_SUIT_ENVELOPE_ROOT_SIGN_KEY_GEN<x>``
* ``SB_CONFIG_SUIT_ENVELOPE_APP_RECOVERY_SIGN_KEY_GEN<x>``

Generations ``1`` to ``3`` are available.

Setting a different key generation results in two changes:

* If default private key names are used, the ``_GEN<N>`` suffix is changed to match the selected generation number.
* The key ID pointed to by the configuration option :kconfig:option:`CONFIG_SUIT_ENVELOPE_TARGET_SIGN_KEY_ID`, and the corresponding options for the OEM root and ``APP_RECOVERY`` envelopes, is updated to the appropriate value.
  This key ID is used to correctly generate the data stored in the SUIT envelope authentication wrapper.

Signing with keys not stored on the local drive
-----------------------------------------------

To sign with keys that are not stored on the local drive (for example, when using an external key management system), you must implement a wrapper for the tool used to sign payloads.
This is done by creating a KMS script in Python and passing it to the build system.
For details on how to implement the KMS script, see :ref:`KMS script <ug_suit_kms_script>`.

Advanced: Custom sign script
----------------------------

The SUIT build system uses a *sign script*, which is responsible for generating the appropriate COSE structures.
It calls the ``sign`` method from the KMS script on the payload that needs to be signed and integrates the signature into the SUIT envelope.
The default sign script is located here: :file:`modules/lib/suit-generator/ncs/sign_script.py`.

The default script handles the majority of use cases.
However, in rare cases where it is not sufficient, you can provide a custom sign script.
The sign script must be a Python script that meets the following requirements:

* Implements a class that inherits from the abstract class ``suit_generator.suit_sign_script_base.SuitEnvelopeSignerBase`` and implements all its abstract methods.
  See :file:`modules/lib/suit-generator/suit_generator/suit_sign_script_base.py`.
* Implements the ``suit_signer_factory`` function, which returns an instance of the class.

Use the default sign script as a reference.

To use a custom script, create a :file:`sysbuild.cmake` file in the application directory, if it is not already present.
Add the following line to the file:

.. code-block:: cmake

   set_property(GLOBAL PROPERTY SUIT_SIGN_SCRIPT <sign_script_path>)

Alternatively, you can provide the sign script by passing the ``SUIT_SIGN_SCRIPT`` CMake variable to the build command.
For example:

.. code-block:: bash

   west build -b nrf54h20dk/nrf54h20/cpuapp -- -DSUIT_SIGN_SCRIPT=<sign_script_path>

Signing envelopes outside of the build system
=============================================

In some cases, it might be necessary to sign an already built SUIT envelope.
For example, an envelope might be signed in the build system using debug keys and then tested.
In this case, it is reasonable to re-sign the envelope with production keys without rebuilding the application.

To achieve this, you can use the ``suit-generator sign`` command.
If ``suit-generator`` is not installed, it can be installed using pip:

.. code-block:: bash

   pip install modules/lib/suit-generator

The ``suit-generator sign`` command provides two subcommands:

* ``single-level`` – Signs a SUIT envelope at a single level, without parsing its dependencies.
  To see the available options, run ``suit-generator sign single-level --help``.
* ``recursive`` – Signs an envelope and its dependencies recursively, based on a provided configuration file.

Signing the envelope recursively
--------------------------------

The ``suit-generator sign recursive`` command accepts the following parameters:

* ``--input-envelope`` – Path to the input envelope to be signed.
* ``--output-envelope`` – Path to the file where the signed envelope will be stored.
* ``--configuration`` – Path to the configuration file that contains information about the keys and signing process.

The configuration file is a JSON file with the following structure:

.. code-block:: json

   {
      "key-name": "<KEY_NAME>",
      "key-id": "<KEY_ID>",
      "alg": "<SIGNATURE_ALGORITHM>",
      "context": "<CONTEXT>",
      "sign-script": "<SIGN_SCRIPT path>",
      "kms-script": "<KMS_SCRIPT path>",
      "omit-signing": false,
      "already-signed-action": "<ACTION>",
      "dependencies" : {
          "<DEP1_NAME>" : {
            "key-name": "<KEY_NAME_DEP1>",
            "key-id": "<KEY_ID_DEP1>",
            "<OTHER_PARAMETERS>"
          },
          "<DEP2_NAME>" : {
            "key-name": "<KEY_NAME_DEP2>",
            "key-id": "<KEY_ID_DEP2>",
            "<OTHER_PARAMETERS>"
          }
      }
   }

No comments are allowed inside the file.
Some of the parameters are optional and have default values.
If a parameter is not provided for a dependency, the values from the parent envelope are used.

The role of the parameters is as follows:

   * ``key-name`` - Name of the key used for signing the envelope.
   * ``key-id`` - Numeric key ID of the public key used to identify the key on the device.
   * ``alg`` - Algorithm used for signing (default: ``ed25519``), supported values are: ``es-256``, ``es-384``, ``es-521``, ``eddsa``, ``hashed-eddsa``.
   * ``context`` - Context used by the KMS script (default: None).
   * ``sign-script`` - Path to the script used for signing a single envelope.
     If not set and ``NCS_SUIT_SIGN_SCRIPT`` is set in the environment, the value of ``NCS_SUIT_SIGN_SCRIPT`` is used.
     If ``ZEPHYR_BASE`` is set in the environment, the sign script from ``$ZEPHYR_BASE/../modules/lib/suit-generator/ncs/sign_script.py`` is used.
     If none of the above is set, the script exits with an error.
   * ``kms-script`` - Path to the KMS script used by the ``sign-script``.
     If not set and ``NCS_SUIT_KMS_SCRIPT`` is set in the environment, the value of ``NCS_SUIT_KMS_SCRIPT`` is used.
     If ``ZEPHYR_BASE`` is set in the environment, the KMS script from ``$ZEPHYR_BASE/../modules/lib/suit-generator/ncs/basic_kms.py`` is used.
     If none of the above is set, the script exits with an error.
   * ``omit-signing`` - Boolean value indicating whether the envelope should be signed or not.
     By default, the envelope is signed (``omit-signing`` is set to ``false``).
     Note that even if set to ``true``, the dependencies will still be parsed and optionally signed.
   * ``already-signed-action`` - Action to be taken if the envelope is already signed (default: ``error``).
     Possible values: ``error``, ``skip``, ``remove-old``.
     If you want to sign the envelope again with a different key, set this to ``remove-old``.
   * ``dependencies`` - Dictionary containing the integrated dependency envelopes.
     The keys are the names matching the names of the integrated dependencies in the parent envelope.
     If a key is not present, even though the envelope contains a dependency with the same name, the given dependency is ignored.
     The values are configuration dictionaries, and all the attributes which are available for the parent envelope, are also available inside these dictionaries for dependency envelopes.

.. _ug_suit_kms_script:

The KMS script
**************

You can manage secret keys used for signing or encrypting payloads using various approaches, such as external Key Management Systems (KMS) or secure elements.

Nordic Semiconductor does not impose a specific key storage method or KMS interface.
Instead, it provides an integration approach that allows you to connect your KMS with the SUIT build system.

To achieve this integration, you must provide a Python script (the KMS script) that implements wrappers around your KMS.
This script must match the interface expected by the SUIT build system.

The KMS script must be a Python script that does the following:

* Implements a class derived from the abstract class ``suit_generator.suit_kms_base.SuitKMSBase`` and fully implements all its abstract methods (see :file:`modules/lib/suit-generator/suit_generator/suit_kms_base.py`).
* Implements the ``suit_kms_factory`` function, returning an instance of the implemented class.

.. note::
   Even if you only need to sign SUIT envelopes, the KMS script must still provide the ``encrypt`` method to comply with the abstract class interface.
   In such cases, this method can simply raise an exception.

By default, the SUIT build system uses the KMS script located at :file:`modules/lib/suit-generator/ncs/basic_kms.py`.
In this script, the *key management system* is represented by a local directory that stores the private keys.
The script reads the private keys from the directory and uses them to sign or encrypt payloads.

To use a custom KMS script, create a :file:`sysbuild.cmake` file in your application directory (if it doesn't already exist) and add the following line:

.. code-block:: cmake

   set_property(GLOBAL PROPERTY SUIT_KMS_SCRIPT <kms_script_path>)

To pass non-standard parameters to the KMS script, use the ``context`` parameter.
Set this by defining the sysbuild configuration option :kconfig:option:`SB_CONFIG_SUIT_ENVELOPE_KMS_SCRIPT_CONTEXT`.
This option accepts any string value and is passed directly to the KMS script without modification.
For the default KMS script, the context is the path to the directory containing the keys.
