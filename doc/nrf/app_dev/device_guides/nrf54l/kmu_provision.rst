.. _ug_nrf54l_developing_provision_kmu:

Performing KMU provisioning
###########################

.. contents::
   :local:
   :depth: 2

.. note::

   The MCUboot bootloader does not yet support KMU for nRF54LM20.

When you perform KMU provisioning, you store one of the key types supported by the KMU and its metadata in the dedicated :ref:`provisioning slot <ug_nrf54l_crypto_kmu_slots>` in RRAM.

You can perform KMU provisioning using the following methods:

* Using the nRF Util and west tools in the |NCS| - for provisioning during development
* Using PSA Crypto API - for provisioning during development or production
* Using other tools for register-level programming - for provisioning during production

.. _ug_nrf54l_developing_provision_kmu_prerequisites:

Prerequisites
*************

Regardless of the method you use to perform KMU provisioning, you need to configure the following elements of the SRC data struct, as per the device datasheet (for example `KMU - Key management unit <nRF54L15 Key management unit_>`_ in the nRF54L15 datasheet):

.. list-table:: Required SRC data struct fields for KMU provisioning
   :widths: auto
   :header-rows: 1

   * - Field
     - Description
   * - VALUE
     - The key material to be stored in the KMU slot.
   * - RPOLICY
     - The revocation policy for the key.
   * - DEST
     - | For symmetric encryption/decryption keys, this is the CRACEN Protected RAM address.
       | For asymmetric keys, this is a different RAM location according to usage.
   * - METADATA
     - Information about the key type and its usage.
       It is required by the CRACEN driver for the verification of some properties of the key and its intended usage.

The CRACEN driver must be able to infer the correct values for these fields.
Depending on the method you use to perform KMU provisioning:

* For :ref:`ug_nrf54l_developing_provision_kmu_development` and :ref:`ug_nrf54l_developing_provision_kmu_psa_crypto_api`: VALUE, RPOLICY, DEST, and METADATA are automatically set for KMU slots.
* For :ref:`ug_nrf54l_developing_provision_kmu_production`: you need to set up METADATA, RPOLICY and DEST manually according to the table in :ref:`ug_nrf54l_developing_provision_kmu_production_metadata`.

See the following sections for more information about each of these steps.

.. _ug_nrf54l_developing_provision_kmu_generate:

Generating keys
***************

You can generate keys using different methods and tools, depending on whether you are in development or in production.
For example, you could generate the key on the device at the first reset (during production).
You can also use OpenSSL, a Hardware Security Module (HSM), or other methods.

If you need a new key for development, you can generate it using imgtool or another tool that produces the required kind and format of key as a PEM file.
For instructions on how to generate a key using imgtool, see the :doc:`imgtool page in the MCUboot documentation<mcuboot:imgtool>`.

See the following example for generating a private key:

.. parsed-literal::
   :class: highlight

   imgtool keygen -k my_ed25519_priv_key.pem -t ed25519

.. _ug_nrf54l_developing_provision_kmu_development:

Provisioning KMU with development tools
***************************************

You can use the following tools to provision keys to the device during development:

* west command ``ncs-provision``
* nRF Util command ``nrfutil device x-provision-keys``

Both methods allow to upload keys to the device though the Serial Write Debug (SWD) interface.

When you install the |NCS|, you get both the west command and the nRF Util tool with the ``nrfutil device`` command installed with the |NCS| toolchain bundle.

.. _ug_nrf54l_developing_provision_kmu_provisioning:

Provisioning keys to the board
==============================

Before uploading keys, ensure that the SoC is unprovisioned.
If the SoC has been previously provisioned and you need to use a different set of keys, you must first erase the SoC with the following erase command:

.. code-block::

   nrfutil device erase --all

Once you have an unprovisioned SoC, upload keys to the board by running one of the following commands:

.. tabs::

   .. tab:: west

      .. parsed-literal::
        :class: highlight

          west ncs-provision upload -k ed25519.pem -k ed25519-1.pem -k ed25519-2.pem --keyname UROT_PUBKEY

      * Parameter ``-k (-–key)`` specifies the private key PEM files to be provisioned to the SoC.
        You can specify up to three keys.

      * Parameter ``--keyname`` specifies the key name for which the key PEM files will be uploaded.

      * Parameter ``--dev-id`` specifies the interface serial number and should be used if multiple J-link interfaces are connected to the development machine.

      * Parameter ``-p (--policy)`` specifies the policy applied to the given set of keys.
        You can apply the following options:

            * ``lock-last`` - Uploads the last key as locked, while the preceding keys are revocable.
              This option is set by default.
            * ``revokable`` - Enables revocation for each key.
            * ``lock`` - Sets all keys to be permanent.

      * Parameter ``--build-dir`` specifies the path to a directory where a JSON file for nRF Util tool will be created.
        If this parameter is not provided, a temporary directory will be used instead.

      * Parameter ``-i (--input)`` specifies path to a YAML file that contains one or more key definitions intended for upload.
        This file can serve as a substitute for other parameters.

        The YAML file should look as follows:

        .. code-block:: YAML

            - keyname: UROT_PUBKEY
                keys: ["/path/private-key1.pem", "/path/private-key2.pem"]
                policy: lock
            - keyname: APP_PUBKEY
                keys: ["/path/private-key3.pem", "/path/private-key4.pem"]
                policy: lock

      * Parameter ``--dry-run`` specifies that a command should generate a keyfile for nRF Util without actually executing the command.

      The script generates the public key for each private key and uploads them to your device.
      These public keys generate the verification keys for the application image, which are then used by MCUboot for validation.
      The first key specified in the command is used for signing the application image.
      Currently, the script supports only ED25519 Keys.

      For MCUboot, take note of the following:

      * UROT_PUBKEY is the key name used by MCUboot.
      * By default, it uses one key.
      * It might utilize multiple keys, which is intended for use with key revocation.
        The number of keys is defined by the ``CONFIG_BOOT_SIGNATURE_KMU_SLOTS`` MCUboot's Kconfig option.
        You can enable the key revocation mechanism with the  ``CONFIG_BOOT_KEYS_REVOCATION`` MCUboot's Kconfig option.
      * KMU support in its configuration needs to be enabled by setting the :kconfig:option:`SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU` sysbuild Kconfig option.
        Otherwise, MCUboot will fallback to the compiled-in key.

      For NSIB, take note of the following:

      * BL_PUBKEY is the key name used by NSIB.
      * It utilizes tree keys, which is intended for use with key revocation.
      * Keys must be provisioned before any run of the bootloader.
        For details, see :ref:`note<ug_nrf54l_developing_basics_kmu_provisioning_keys>`.

      To provision one key to the board, run the following command:

      .. parsed-literal::
        :class: highlight

          west ncs-provision upload -k ed25519.pem --keyname UROT_PUBKEY

   .. tab:: nRF Util

      You can use the :ref:`generate_psa_key_attributes_script`, :ref:`similarly to nRF54H20<ug_nrf54h20_keys_generating>`, to generate the JSON file and the metadata from the PEM file you :ref:`generated earlier <ug_nrf54l_developing_provision_kmu_generate>`.

      .. include:: ../../../../../scripts/generate_psa_key_attributes/generate_psa_key_attributes.rst
         :start-after: nrfutil_provision_keys_info_start
         :end-before: nrfutil_provision_keys_info_end

      .. include:: ../../../../../scripts/generate_psa_key_attributes/generate_psa_key_attributes.rst
         :start-after: nrfutil_provision_keys_command_start
         :end-before: nrfutil_provision_keys_command_end

Alternative provisioning method during development
==================================================

To simplify the development process, keys can be generated and then provisioned at the same time as the flashing process.
You can provision keys during flashing when the build directory contains the :file:`keyfile.json` file with commands, such as ``west flash --recover`` or ``west flash --erase``.
When flashing a project that contains NSIB, you can only use the ``west flash --recover``, as the programming file contains UICR provisioning data as well.

You can generate the :file:`keyfile.json` file during the build process (for example, when running ``west build``) if the :Kconfig:option:`SB_CONFIG_SECURE_BOOT_GENERATE_DEFAULT_KMU_KEYFILE` or :Kconfig:option:`SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE` Kconfig options are enabled.
These options enable, respectively, the NSIB and the MCUboot keys, included in the generated :file:`keyfile.json` file.
This file contains the necessary key provisioning information.

If you set the :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE` Kconfig option to a PEM key file, that specific file will be used.
If not, the build will use the default key named :file:`GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem`, which is located in the build directory.
Similarly, MCUboot uses the key file designated by the :Kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` option.

At the end of the described process, the :file:`keyfile.json` file is generated in the build directory.
This file allows key provisioning to occur simultaneously with the flashing process.
Alternatively, you can bypass the mentioned Kconfig options and manually place a custom :file:`keyfile.json` in the build directory.

.. _ug_nrf54l_developing_provision_kmu_psa_crypto_api:

Provisioning KMU with PSA Crypto API
************************************

You can use the PSA Crypto API to create a provisioning image to be used to provision keys to the device during development or production.

Detailed steps for creating such a provisioning image for your application are beyond the scope of this guide.
Use the guidelines in :ref:`ug_nrf54l_crypto_kmu_key_programming_model` and in the `nRF54L Series Production Programming`_ guide.

.. _ug_nrf54l_developing_provision_kmu_production:

Provisioning KMU with production tools
**************************************

For production, `provision the KMU data <Provisioning KMU data_>`_ as described in the `nRF54L Series Production Programming`_ guide.

Nordic Semiconductor recommends using other tools than the ones used for :ref:`provisioning for development <ug_nrf54l_developing_provision_kmu_development>`.
For example, you could use SEGGER J-Link or other debugging tools.

Using other tools means that you need to manually make sure that the values set in the :c:struct:`kmu_metadata` data structure's bitfields match the required values for different key types and the CRACEN driver's own requirements.

.. _ug_nrf54l_developing_provision_kmu_production_metadata:

Setting up KMU metadata and destination addresses for production
================================================================

The :c:struct:`kmu_metadata` structure is a 32-bit bitfield defined in the CRACEN driver source code (:file:`nrf/subsys/nrf_security/src/drivers/cracen/cracenpsa/src/kmu.c`):

.. literalinclude:: ../../../../../subsys/nrf_security/src/drivers/cracen/cracenpsa/src/kmu.c
   :start-after: #endif
   :end-before: _Static_assert(sizeof(kmu_metadata)
   :language: c
   :class: highlight

When you configure :c:struct:`psa_key_attributes_t`, the CRACEN driver converts the attributes to KMU metadata (:c:func:`convert_from_psa_attributes`).

To make sure that this conversion is correct and the provisioning is successful, the key type used and the key you generated need to match the valid configuration ranges.

Common KMU provisioning configuration ranges
--------------------------------------------

The following table shows common KMU provisioning configuration ranges for the nRF54L15 device.
The values are provided for the :ref:`ug_nrf54l_developing_provision_kmu_prerequisites` (SRC data fields) mentioned at the beginning of this page.
The table includes multiple KMU slots that work together for keys larger than 128 bits.

.. list-table:: Example KMU provisioning configurations for nRF54L15
   :header-rows: 1
   :widths: auto

   * - Use case
     - METADATA (KMU slot #0)
     - DEST (KMU slot #0)
     - VALUE (KMU slot #0)
     - RPOLICY (KMU slot #0)
     - METADATA (KMU slot #1)
     - DEST (KMU slot #1)
     - VALUE (KMU slot #1)
     - RPOLICY (KMU slot #1)
   * - Ed25519 revokable signature verification key
     - TODO
     - ``0x20000000``
     - key_data[0..15] inclusive
     - TODO
     - ``0xffffffff``
     - ``0x20000010``
     - key_data[16..31] inclusive
     - TODO
   * - Ed25519 read-only signature verification key
     - TODO
     - ``0x20000000``
     - key_data[0..15] inclusive
     - TODO
     - ``0xffffffff``
     - ``0x20000010``
     - key_data[16..31] inclusive
     - TODO
   * - Ed25519 deletable key
     - TODO
     - ``0x20000000``
     - key_data[0..15] inclusive
     - TODO
     - ``0xffffffff``
     - ``0x20000010``
     - key_data[16..31] inclusive
     - TODO
   * - AES-CTR 128-bit protected decryption key
     - TODO
     - TODO (protected key RAM)
     - key_data
     - TODO
     - N/A
     - N/A
     - N/A
     - N/A
   * - AES-CTR 256 bit protected decryption key
     - TODO
     - TODO (protected key RAM + 16)
     - key_data[0..15] inclusive
     - TODO
     - ``0xffffffff``
     - TODO (protected key RAM + 16)
     - key_data[16..31] inclusive
     - TODO

Example conversion
------------------

The following example shows how the PSA key attribute values are converted when creating metadata for the Ed25519 revokable signature verification key.

1. User creates :c:struct:`psa_key_attributes_t` key attributes for the Ed25519 revokable verification key:

   .. code-block:: c

      psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
      psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
      psa_set_key_bits(&attr, 255);
      psa_set_key_algorithm(&attr, PSA_ALG_PURE_EDDSA);
      psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_VERIFY_HASH);
      psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
          CRACEN_KEY_PERSISTENCE_REVOKABLE, PSA_KEY_LOCATION_CRACEN_KMU));
      psa_set_key_id(&attr, PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
          CRACEN_KMU_KEY_USAGE_SCHEME_RAW, 226));

2. The CRACEN driver ``convert_from_psa_attributes()`` function converts these PSA attributes to the following :c:struct:`kmu_metadata` field values:

   * ``metadata_version`` = ``0``
   * ``key_usage_scheme`` = ``3`` (Raw, extracted from ``key_id``)
   * ``reserved`` = ``0``
   * ``algorithm`` = ``10`` (Ed25519)
   * ``size`` = ``3`` (255 bits)
   * ``rpolicy`` = ``3`` (Revokable)
   * ``usage_flags`` = ``0b0001000`` (bit 3 set for Verify)

   The metadata bitfields are packed from least-significant bit to most-significant bit:

   .. code-block:: none

      Bits  0-3:   metadata_version = 0        (0000)
      Bits  4-5:   key_usage_scheme = 3        (11)
      Bits  6-15:  reserved = 0                (0000000000)
      Bits 16-19:  algorithm = 10              (1010)
      Bits 20-22:  size = 3                    (011)
      Bits 23-24:  rpolicy = 3                 (11)
      Bits 25-31:  usage_flags = 8             (0001000)

      Binary representation:
      0001000_11_011_1010_0000000000_11_0000

      Grouped by bytes (MSB first):
      00001000 11011101 00000000 00110000

      Hexadecimal: 0x11BA0030

3. The KMU provisioning is executed with the following KMU slot configuration:

   * **KMU slot #0 (first slot):**

     * METADATA: ``0x11BA0030``
     * DEST: ``0x20000000`` (kmu_push_area start address)
     * VALUE: key_data[0..15] (first 16 bytes of the 32-byte Ed25519 public key)
     * RPOLICY: ``0x3`` (Revokable)

   * **KMU slot #1 (second slot):**

     * METADATA: ``0xffffffff`` (secondary slot marker)
     * DEST: ``0x20000010`` (kmu_push_area + 16 bytes)
     * VALUE: key_data[16..31] (last 16 bytes of the 32-byte Ed25519 public key)
     * RPOLICY: ``0x3`` (Revokable)
