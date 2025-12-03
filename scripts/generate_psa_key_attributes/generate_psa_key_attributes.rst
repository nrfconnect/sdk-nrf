.. _generate_psa_key_attributes_script:

PSA Key Attributes generator script
###################################

.. contents::
   :local:
   :depth: 2

This Python script generates Platform Security Architecture (PSA) key attribute binary blobs and creates JSON files for key provisioning with `nRF Util`_.

.. note::
   The script is primarily used for Key Management Unit (KMU) provisioning on nRF54L Series devices and nRF54H20.
   For more details about KMU provisioning, see the :ref:`ug_nrf54l_developing_provision_kmu` in the nRF54L Series device guide.

Overview
********

The script generates PSA key attributes that define the properties and usage of cryptographic keys.
It supports multiple key sources and output formats:

.. list-table:: Script Features
   :header-rows: 1
   :widths: 30 70

   * - Feature
     - Supported options
   * - Key sources
     - | Raw HEX strings
       | TRNG-generated keys
       | PEM files
   * - Output formats
     - | :ref:`JSON files for nRF Util <generate_psa_key_attributes_script_nrfutil>`
       | Binary metadata
       | Standard output
   * - Key types
     - | AES
       | HMAC
       | ECC (various curves)
       | ChaCha20
       | Raw data
   * - Storage locations
     - | CRACEN
       | CRACEN KMU

The generated attributes include:

* Key type and size
* Storage location and persistence mode
* Usage permissions (encrypt, decrypt, sign, verify, and more)
* Cryptographic algorithms
* CRACEN usage scheme (for KMU keys)

.. _generate_psa_key_attributes_script_details:

Implementation details
======================

The script creates PSA key attributes compatible with the `psa_key_attributes_s`_ struct.

The binary format includes:

* Key type and bit size
* Key lifetime (location and persistence)
* Usage policy (usage flags, algorithms)
* Key identifier
* Reserved field for owner ID

For KMU keys (``LOCATION_CRACEN_KMU``), the key identifier encodes the following:

* CRACEN usage scheme (bits 12-13)
* Key slot number (bits 0-7)
* Fixed prefix (0x7FFF0000)

.. _generate_psa_key_attributes_script_nrfutil:

Compatibility with nRF Util
===========================

.. nrfutil_provision_keys_info_start

`nRF Util`_ provides the ``nrfutil device x-provision-keys`` command to provision keys onto the KMU.

The command requires a JSON file with the keys and the key metadata.
You can create a JSON file when you run the script with the ``--file`` option.
You can also create a JSON file from the PEM file you generated earlier with the ``--key-from-file`` option.
Each next invocation using the same ``--file`` option adds multiple keys to the same JSON file.

Each key entry in the JSON file contains the following fields:

.. csv-table:: "Key entry fields"
   :header: "Field", "Description"
   :widths: 20, 80

   "``metadata``", "HEX-encoded PSA key attributes"
   "``value``", "Key value (HEX string or TRNG specification)"

When you use nRF Util to provision keys, the tool does not validate the JSON file.
It functions as a transport layer, transferring the key data to the correct location in the device.

.. nrfutil_provision_keys_info_end

.. nrfutil_provision_keys_command_start

To provision keys onto the KMU, use the following nRF Util command, with the ``<snr>`` being the serial number of the device and ``<key-file>`` being the name of the key file in the JSON format:

.. parsed-literal::
   :class: highlight

   nrfutil device x-provision-keys --serial-number <snr> --key-file <JSON-key-file>

You can call this command multiple times also to provision multiple keys, as long as each key has a different ID that is part of the ``metadata`` string.

For more information about this command, see the `Provisioning cryptographic keys`_ page in the nRF Util documentation.

.. nrfutil_provision_keys_command_end

Requirements
************

The :file:`generate_psa_key_attributes.py` script is located in the :file:`scripts` directory and requires the following Python dependencies:

* Python's cryptographic library for PEM file processing.
  You can install it by running one of the following commands in the :file:`scripts` directory:

  .. code-block:: console

     python3 -m pip install cryptography

  This command installs the `pyca/cryptography`_ library and its dependencies.
  Alternatively, you can install the `PyCryptodome`_ library by running the following command:

  .. code-block:: console

     python3 -m pip install pycryptodome

* Standard Python libraries (argparse, struct, binascii, json)

Using the script
****************

Run the script from the :file:`scripts` directory.

The script requires the following arguments:

.. csv-table:: "Required arguments"
   :header: "Argument", "Description"
   :widths: 20, 80

   "--usage", "PSA key usage flags that encode the permitted usage of a key"
   "--id", "Key identifier (KMU slot number (0-255) for KMU keys)"
   "--type", "PSA key type"
   "--key-bits", "Key size in bits"
   "--location", "PSA key storage location"
   "--persistence", "Persistence mode for the key"

.. note::
   For the nRF54L Series devices, the script also requires the ``--cracen-usage`` argument.
   The argument can take different values.
   If you use ``--cracen-usage ENCRYPTED``, make sure that the device already has a seed value before you provision the key.
   See :ref:`ug_nrf54l_crypto_cracen_ikg` for more information.

To list all available arguments, including the optional ones, run the ``generate_psa_key_attributes.py -h`` command.

.. note::

   Using the optional ``--allow-usage-export`` flag is not recommended.
   The flag makes the key accessible outside of the security boundary through a call to the ``psa_export_key`` function.
   See `PSA_KEY_USAGE_EXPORT policy`_ for details.

Examples
========

The following commands are examples of how you can use the script to generate PSA key attributes:

* Generate an AES key for encryption/decryption for algorithm AES-CBC:

  .. code-block:: console

     python3 generate_psa_key_attributes.py \
       --usage ENCRYPT_DECRYPT \
       --id 1 \
       --type AES \
       --key-bits 256 \
       --location LOCATION_CRACEN_KMU \
       --cracen-usage RAW \
       --algorithm CBC_PKCS7 \
       --persistence PERSISTENCE_DEFAULT \
       --key 0x1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF \
       --file keys.json

* Generate an ECDSA secp256r1 SHA-256 public key with revokable persistency from a PEM file:

  .. code-block:: console

     python3 generate_psa_key_attributes.py \
       --usage VERIFY \
       --id 3 \
       --type ECC_PUBLIC_KEY_SECP_R1 \
       --key-bits 256 \
       --location LOCATION_CRACEN_KMU \
       --cracen-usage RAW \
       --algorithm ECDSA_SHA256 \
       --persistence PERSISTENCE_REVOKABLE \
       --key-from-file public_key.pem \
       --file keys.json

* Generate a TRNG key for HMAC:

  .. code-block:: console

     python3 generate_psa_key_attributes.py \
       --usage SIGN_VERIFY \
       --id 7 \
       --type HMAC \
       --key-bits 128 \
       --location LOCATION_CRACEN_KMU \
       --cracen-usage RAW \
       --algorithm HMAC_SHA256 \
       --persistence PERSISTENCE_DEFAULT \
       --trng-key \
       --file keys.json
