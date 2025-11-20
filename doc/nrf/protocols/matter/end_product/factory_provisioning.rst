.. _ug_matter_device_factory_provisioning:

Factory provisioning in Matter
##############################

.. contents::
   :local:
   :depth: 2

Factory data is a set of device parameters written to the non-volatile memory during the manufacturing process.

The factory data parameter set includes different types of information, for example about device certificates, cryptographic keys, device identifiers, and hardware.
All those parameters are vendor-specific and must be inserted into a device's persistent storage during the manufacturing process.
The factory data parameters are read at the boot time of a device and can be used in the Matter stack and user application, for example during commissioning.

This guide describes the process of creating and programming factory data using Matter and the nRF Connect platform from Nordic Semiconductor.
You need to complete this process for :ref:`ug_matter_device_attestation` and :ref:`ug_matter_device_certification`.
You can also see `Matter factory data Kconfig options`_ for an overview of possible configuration variants.

.. note::
   You must repeat the factory data generation process for every manufactured device.
   Do not use the same factory data for multiple devices, as there are specific data that have to be unique for each device.

Overview
********

All factory data parameters are protected against modifications by the software.
The firmware data parameter set must be kept unchanged during the lifetime of the device.
When implementing your firmware, make sure that the factory data parameters are not re-written or overwritten during the Device Firmware Update (DFU) or factory resets, except in some vendor-defined cases.

For the nRF Connect platform, the factory data is stored by default in a separate partition of the internal flash memory.
This helps to keep the factory data secure by applying hardware write protection.

.. note::
   Due to hardware limitations, in the nRF Connect platform, protection against writing can be applied only to the internal memory partition.
   `Fprotect`_ is the hardware flash protection driver that is used to ensure write protection of the factory data partition in internal flash memory.

You can implement the factory data set described in the :ref:`ug_matter_device_factory_provisioning_factory_data_component_table` in various ways, as long as the final HEX file contains all mandatory components defined in the table.
In this guide, the :ref:`ug_matter_device_factory_provisioning_generating_factory_data` and the :ref:`ug_matter_device_factory_provisioning_building_example_with_factory_data` sections describe one of the implementations of the factory data set created by the nRF Connect platform's maintainers.
At the end of the process, you get a HEX file that contains the factory data partition in the CBOR format.

The factory data accessor is a component that reads and decodes factory data parameters from the device's persistent storage and creates an interface to provide all of them to the Matter stack and to the user application.

The default implementation of the factory data accessor assumes that the factory data stored in the device's flash memory is provided in the CBOR format.
However, it is possible to generate the factory data set without using the `Matter nRF Connect scripts`_ and implement another parser and a factory data accessor.
This is possible if the newly provided implementation is consistent with the `Factory Data Provider`_.
For more information about preparing a factory data accessor, see the :ref:`ug_matter_device_factory_provisioning_using_own_factory_data_implementation` section.

.. note::
   Encryption and security of the factory data partition is not yet provided for this feature.

.. _ug_matter_device_factory_provisioning_factory_data_component_table:

Factory data component table
============================

The following table lists the parameters of a factory data set:

+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Key name               | Full name                            | Length               | Format       | Conformance | Description                                                                                                                                                                                                                 |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``version``            | factory data version                 | 2 B                  | uint16       | mandatory   | A version number of the current factory data set.                                                                                                                                                                           |
|                        |                                      |                      |              |             | This value is managed by the system and must match the version expected by the Factory Data Provider on the device.                                                                                                         |
|                        |                                      |                      |              |             | It cannot be changed by the user.                                                                                                                                                                                           |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``sn``                 | serial number                        | <1, 32> B            | ASCII string | mandatory   | A unique serial number assigned to each manufactured device.                                                                                                                                                                |
|                        |                                      |                      |              |             | The maximum length is 32 characters.                                                                                                                                                                                        |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``vendor_id``          | vendor ID                            | 2 B                  | uint16       | mandatory   | A CSA-assigned ID for the organization responsible for producing the device.                                                                                                                                                |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``product_id``         | product ID                           | 2 B                  | uint16       | mandatory   | A unique ID assigned by the device vendor to identify the product.                                                                                                                                                          |
|                        |                                      |                      |              |             | It defaults to a CSA-assigned ID that designates a non-production or test product.                                                                                                                                          |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``vendor_name``        | vendor name                          | <1, 32> B            | ASCII string | mandatory   | A human-readable vendor name that provides a simple string containing identification of device's vendor for the application and Matter stack purposes.                                                                      |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``product_name``       | product name                         | <1, 32> B            | ASCII string | mandatory   | A human-readable product name that provides a simple string containing identification of the product for the application and the Matter stack purposes.                                                                     |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``date``               | manufacturing date                   | 10 B                 | ISO 8601     | mandatory   | A manufacturing date specifies the date that the device was manufactured.                                                                                                                                                   |
|                        |                                      |                      |              |             | The date format used is ISO 8601, for example ``YYYY-MM-DD``.                                                                                                                                                               |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``hw_ver``             | hardware version                     | 2 B                  | uint16       | mandatory   | A hardware version number that specifies the version number of the hardware of the device.                                                                                                                                  |
|                        |                                      |                      |              |             | The value meaning and the versioning scheme is defined by the vendor.                                                                                                                                                       |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``hw_ver_str``         | hardware version string              | <1, 64> B            | uint16       | mandatory   | A hardware version string parameter that specifies the version of the hardware of the device as a more user-friendly value than that presented by the hardware version integer value.                                       |
|                        |                                      |                      |              |             | The value meaning and the versioning scheme is defined by the vendor.                                                                                                                                                       |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``dac_cert``           | Device Attestation Certificate (DAC) | <1, 602> B           | byte string  | mandatory   | The Device Attestation Certificate (DAC) and the corresponding private key are unique to each Matter device.                                                                                                                |
|                        |                                      |                      |              |             | The DAC is used for the Device Attestation process and to perform commissioning into a fabric.                                                                                                                              |
|                        |                                      |                      |              |             | The DAC is a DER-encoded X.509v3-compliant certificate, as defined in RFC 5280.                                                                                                                                             |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``dac_key``            | DAC private key                      | 68 B                 | byte string  | mandatory   | The private key associated with the Device Attestation Certificate (DAC).                                                                                                                                                   |
|                        |                                      |                      |              |             | This key should be encrypted and maximum security should be guaranteed while generating and providing it to factory data.                                                                                                   |
|                        |                                      |                      |              |             | To learn how to store DAC key in a secure manner, see :ref:`matter_platforms_security_dac_priv_key`.                                                                                                                        |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``pai_cert``           | Product Attestation Intermediate     | <1, 602> B           | byte string  | mandatory   | An intermediate certificate is an X.509 certificate, which has been signed by the root certificate.                                                                                                                         |
|                        |                                      |                      |              |             | The last intermediate certificate in a chain is used to sign the leaf (the Matter device) certificate.                                                                                                                      |
|                        |                                      |                      |              |             | The PAI is a DER-encoded X.509v3-compliant certificate as defined in RFC 5280.                                                                                                                                              |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``spake2_it``          | SPAKE2+ iteration counter            | 4 B                  | uint32       | mandatory   | The `SPAKE2+`_ is a protocol that is used for deriving a shared secret key between two devices.                                                                                                                             |
|                        |                                      |                      |              |             | The commissionee device does not generate key locally, but it uses SPAKE2+ Verifier generated by the manufacturer to establish secure session.                                                                              |
|                        |                                      |                      |              |             | A SPAKE2+ iteration counter is the amount of PBKDF2 (a key derivation function) iterations in a cryptographic process used during SPAKE2+ Verifier generation.                                                              |
|                        |                                      |                      |              |             | The iterations number must be a value between 1000 and 100000.                                                                                                                                                              |
|                        |                                      |                      |              |             | The default number of iterations used in this guide is 1000.                                                                                                                                                                |
|                        |                                      |                      |              |             | The greater the number of iterations, the stronger the generated crypto material is.                                                                                                                                        |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``spake2_salt``        | SPAKE2+ salt                         | <16, 32> B           | byte string  | mandatory   | The `SPAKE2+`_ is a protocol that is used for deriving a shared secret key between two devices.                                                                                                                             |
|                        |                                      |                      |              |             | The commissionee device does not generate key locally, but it uses SPAKE2+ Verifier generated by the manufacturer to establish secure session.                                                                              |
|                        |                                      |                      |              |             | The SPAKE2+ salt is a random piece of data, at least 16 bytes long and at most 32 bytes long.                                                                                                                               |
|                        |                                      |                      |              |             | It is used as an additional input to a one-way cryptographic function that is used to strengthen generated cryptographic material - the SPAKE2+ Verifier.                                                                   |
|                        |                                      |                      |              |             | A new salt should be randomly generated for each device.                                                                                                                                                                    |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``spake2_verifier``    | SPAKE2+ verifier                     | 97 B                 | byte string  | mandatory   | The `SPAKE2+`_ is a protocol that is used for deriving a shared secret key between two devices.                                                                                                                             |
|                        |                                      |                      |              |             | The commissionee device does not generate key locally, but it uses SPAKE2+ Verifier generated by the manufacturer to establish secure session.                                                                              |
|                        |                                      |                      |              |             | The SPAKE2+ verifier is generated using SPAKE2+ salt, iteration counter, and passcode.                                                                                                                                      |
|                        |                                      |                      |              |             | The device shall be supplied with SPAKE2+ verifier in its internal storage, for example, in the factory data partition.                                                                                                     |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``discriminator``      | Discriminator                        | 2 B                  | uint16       | mandatory   | The discriminator is a 12-bit value used to distinguish devices during the discovery process.                                                                                                                               |
|                        |                                      |                      |              |             | It is advertised by the device and it is recommended to use unique value for each manufactured device to reduce the risk of collision.                                                                                      |
|                        |                                      |                      |              |             | For example, the sequential generation with rollover can be used to assign values to following devices.                                                                                                                     |
|                        |                                      |                      |              |             | The full 12 bit value is used for machine-readable purposes, but the manual pairing code uses only upper 4 bits of the value.                                                                                               |
|                        |                                      |                      |              |             | The default discriminator value used in this guide is 0xF00.                                                                                                                                                                |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``passcode``           | SPAKE2+ passcode                     | 4 B                  | uint32       | optional    | The `SPAKE2+`_ is a protocol that is used for deriving a shared secret key between two devices.                                                                                                                             |
|                        |                                      |                      |              |             | The commissionee device does not generate key locally, but it uses SPAKE2+ Verifier generated by the manufacturer to establish secure session.                                                                              |
|                        |                                      |                      |              |             | A pairing passcode is a 27-bit unsigned integer that serves as a proof of possession during the commissioning.                                                                                                              |
|                        |                                      |                      |              |             | It is also used in a process of generation of SPAKE2+ Verifier.                                                                                                                                                             |
|                        |                                      |                      |              |             | Its value must be restricted to the values from ``0x0000001`` to ``0x5F5E0FE`` (``00000001`` to ``99999998`` in decimal).                                                                                                   |
|                        |                                      |                      |              |             | The following invalid passcode values are excluded: ``00000000``, ``11111111``, ``22222222``, ``33333333``, ``44444444``, ``55555555``, ``66666666``, ``77777777``, ``88888888``, ``99999999``, ``12345678``, ``87654321``. |
|                        |                                      |                      |              |             | The default passcode value used in this guide is 20202021.                                                                                                                                                                  |
|                        |                                      |                      |              |             | As the passcode provides proof of possession and is initial element of secure channel establishment it is recommended to generate it using a secure random number generator.                                                |
|                        |                                      |                      |              |             | The generated value should not be derived from public information such as a serial number or manufacturing date.                                                                                                            |
|                        |                                      |                      |              |             | The passcode supplied to the device shall be stored in a location isolated from the SPAKE2+ verifier and shall not be accessible during operational mode using any data model attributes or commands.                       |
|                        |                                      |                      |              |             | Including the passcode in factory data is optional, as the usage of NFC onboarding or printing a QR code in debug logs are the only use cases that requires it.                                                             |
|                        |                                      |                      |              |             | Please note that including the passcode in factory data does not meet the specification requirement about SPAKE2+ Verifier and passcode isolation.                                                                          |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``product_appearance`` | Product visible appearance           | 2 B                  | CBOR map     | optional    | The appearance field is a structure that describes the visible appearance of the product.                                                                                                                                   |
|                        |                                      |                      |              |             | This field is provided in a CBOR map and consists of two attributes: ``finish`` (1 B), ``primary_color`` (1 B).                                                                                                             |
|                        |                                      |                      |              |             | See the :ref:`ug_matter_device_factory_provisioning_appearance_field_description` to learn how to set all attributes.                                                                                                       |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``rd_uid``             | rotating device ID unique ID         | <16, 32> B           | byte string  | optional    | The unique ID for rotating device ID, which consists of a randomly-generated 128-bit (or longer) octet string.                                                                                                              |
|                        |                                      |                      |              |             | The rotating device ID is an optional identifier that is used for the :ref:`ug_matter_configuring_ffs` purposes.                                                                                                            |
|                        |                                      |                      |              |             | This parameter should be protected against reading or writing over-the-air after initial introduction into the device, and stay fixed during the lifetime of the device.                                                    |
|                        |                                      |                      |              |             | When building an application with the Factory Data support, the :ref:`CONFIG_CHIP_FACTORY_DATA_ROTATING_DEVICE_UID_MAX_LEN` must be set with the length of the actual ``rd_uid`` stored in the Factory Data partition.      |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``enable_key``         | enable key                           | 16 B                 | byte string  | optional    | The enable key is a 16-byte hexadecimal string value that triggers manufacturer-specific action while invoking the :ref:`ug_matter_test_event_triggers`.                                                                    |
|                        |                                      |                      |              |             | This value is used during Certification Tests, and should not be present on production devices.                                                                                                                             |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ``user``               | User data                            | variable, max 1024 B | CBOR map     | optional    | The user data is provided in the JSON format.                                                                                                                                                                               |
|                        |                                      |                      |              |             | This parameter is optional and depends on the device manufacturer's purpose.                                                                                                                                                |
|                        |                                      |                      |              |             | It is provided as a CBOR map type from persistent storage and should be parsed in the user application.                                                                                                                     |
|                        |                                      |                      |              |             | This data is not used by the Matter stack.                                                                                                                                                                                  |
|                        |                                      |                      |              |             | To learn how to work with user data, see the :ref:`ug_matter_device_factory_provisioning_how_to_set_user_data` section.                                                                                                     |
+------------------------+--------------------------------------+----------------------+--------------+-------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

Factory data format
===================

Save the factory data set into a HEX file that can be written to the flash memory of the Matter device.

In the implementation provided by nRF Connect platform, the factory data set is represented in the CBOR format and is stored in a HEX file.
The file is then programmed to a device.
The JSON format is used as an intermediate, human-readable representation of the data.
The format is regulated by the `Factory data schema`_ file.

All parameters of the factory data set are either mandatory or optional:

* Mandatory parameters must always be provided, as they are required, for example, to perform commissioning to the Matter network.
* Optional parameters can be used for development and testing purposes.
  For example, the ``user`` data parameter consists of all data that is needed by a specific manufacturer and that is not included in the mandatory parameters.

In the factory data set, the following formats are used:

* uint8, uint16, and uint32 - These are the numeric formats representing, respectively, one-byte length unsigned integer, two-bytes length unsigned integer, and four-bytes length unsigned integer.
  This value is stored in a HEX file in the big-endian order.
* Byte string - This parameter represents the sequence of integers between ``0`` and ``255`` (inclusive), without any encoding.
  Because the JSON format does not allow the use of byte strings, the ``hex:`` prefix is added to a parameter, and its representation is converted to a HEX string.
  For example, an ASCII string ``abba`` is represented as ``hex:61626261`` in the JSON file and then stored in the HEX file as ``0x61626261``.
  The HEX string length in the JSON file is two times greater than the byte string plus the size of the prefix.
* ASCII string is a string representation in ASCII encoding without null-terminating.
* ISO 8601 format is a `ISO 8601 date format`_ that represents a date provided in the ``YYYY-MM-DD`` or ``YYYYMMDD`` format.
* All certificates stored in factory data are provided in the `X.509`_ format.

.. _ug_matter_device_factory_provisioning_appearance_field_description:

Appearance field description
----------------------------

The ``appearance`` field in the factory data set describes the device's visible appearance.

* ``finish`` - A string name that indicates the visible exterior finish of the product.
  It refers to the ``ProductFinishEnum`` enum, and currently, you can choose one of the following names:

+--------------+------------+
| Name         | Enum value |
+--------------+------------+
| ``matte``    | 0          |
+--------------+------------+
| ``satin``    | 1          |
+--------------+------------+
| ``polished`` | 2          |
+--------------+------------+
| ``rugged``   | 3          |
+--------------+------------+
| ``fabric``   | 4          |
+--------------+------------+
| ``other``    | 255        |
+--------------+------------+

* ``primary_color`` - A string name that represents the RGB color space of the device's case color, which is the most representative.
  It refers to the ``ColorEnum`` enum, and currently, you can choose one of the following names:

+-------------+-------------+
| Color name  | RGB value   |
+-------------+-------------+
| ``black``   | ``#000000`` |
+-------------+-------------+
| ``navy``    | ``#000080`` |
+-------------+-------------+
| ``green``   | ``#008000`` |
+-------------+-------------+
| ``teal``    | ``#008080`` |
+-------------+-------------+
| ``maroon``  | ``#800080`` |
+-------------+-------------+
| ``purple``  | ``#800080`` |
+-------------+-------------+
| ``olive``   | ``#808000`` |
+-------------+-------------+
| ``gray``    | ``#808080`` |
+-------------+-------------+
| ``blue``    | ``#0000FF`` |
+-------------+-------------+
| ``lime``    | ``#00FF00`` |
+-------------+-------------+
| ``aqua``    | ``#00FFFF`` |
+-------------+-------------+
| ``red``     | ``#FF0000`` |
+-------------+-------------+
| ``fuchsia`` | ``#FF00FF`` |
+-------------+-------------+
| ``yellow``  | ``#FFFF00`` |
+-------------+-------------+
| ``white``   | ``#FFFFFF`` |
+-------------+-------------+
| ``nickel``  | ``#727472`` |
+-------------+-------------+
| ``chrome``  | ``#a8a9ad`` |
+-------------+-------------+
| ``brass``   | ``#E1C16E`` |
+-------------+-------------+
| ``copper``  | ``#B87333`` |
+-------------+-------------+
| ``silver``  | ``#C0C0C0`` |
+-------------+-------------+
| ``gold``    | ``#FFD700`` |
+-------------+-------------+

Enabling factory data support
*****************************

By default, the factory data support is disabled in all nRF Connect examples and the nRF Connect device uses predefined parameters from the Matter core, which you should not change.
To start using factory data stored in the flash memory and the ``Factory Data Provider`` from the nRF Connect platform, build an example with the following option (replace ``<build_target>`` with your board name, for example, ``nrf52840dk_nrf52840``):

.. parsed-literal::
   :class: highlight

   west build -b <build_target> -- -DCONFIG_CHIP_FACTORY_DATA=y

.. _ug_matter_device_factory_provisioning_generating_factory_data:

Generating factory data
***********************

This section describes generating factory data using the following `Matter nRF Connect scripts`_:

* The first script creates a JSON file that contains a user-friendly representation of the factory data.
* The second script uses the JSON file to create a factory data partition and save it to a HEX file.

After these operations, program a HEX file containing the factory data partition into the device's flash memory.

You can use the second script without invoking the first one by providing a JSON file written in another way.
To make sure that the JSON file is correct and the device is able to read out parameters, refer to the :ref:`ug_matter_device_factory_provisioning_verifying_using_json_schema_tool` section.

You can also use only the first script to generate both JSON and HEX files, by providing optional ``offset`` and ``size`` arguments, which results in invoking the script internally.
This option is the recommended one, but invoking two scripts one by one is also supported to provide backward compatibility.

.. _ug_matter_device_factory_provisioning_creating_the_factory_data_json_and_hex_files_with_the_first_script:

Creating the factory data JSON and HEX files with the first script
==================================================================

A Matter device needs a proper factory data partition stored in the flash memory to read out all required parameters during startup.
To simplify the factory data generation, you can use the `Generate factory data script`_ Python script to provide all required parameters and generate a human-readable JSON file and save it to a HEX file.

To use this script, complete the following steps:

1. Navigate to the :file:`connectedhomeip` root directory.
#. Run the script with ``-h`` option to see all possible options:

   .. code-block:: console

      python scripts/tools/nrfconnect/generate_nrfconnect_chip_factory_data.py -h

#. Prepare a list of arguments:

   a. Fill up all mandatory arguments:

      .. code-block:: console

         --sn --vendor_id, --product_id, --vendor_name, --product_name, --date, --hw_ver, --hw_ver_str, --spake2_it, --spake2_salt, --discriminator

   #. Add output path to store the :file:`.json` file, for example :file:`my_dir/output`:

      .. code-block:: console

         -o <path_to_output_file>

   #. Generate the SPAKE2 verifier using one of the following methods:

      * Automatic:

        .. code-block:: console

           --passcode <pass_code>

      * Manual:

        .. code-block:: console

           --spake2_verifier <verifier>

   #. Add paths to the :file:`.der` files that contain the PAI and DAC certificates and the DAC private key (replace the respective variables with the file names) using one of the following methods:

      * Automatic:

        .. code-block:: console

           --chip_cert_path <path to chip-cert executable> --gen_certs

        .. note::
           To generate new certificates, you need the ``chip-cert`` executable.
           See the note at the end of this section to learn how to get it.

      * Manual:

        .. code-block:: console

           --dac_cert <path to DAC certificate>.der --dac_key <path to DAC key>.der --pai_cert <path to PAI certificate>.der

   #. (optional) Add the new unique ID for rotating device ID using one of the following options:

      * Provide an existing ID:

        .. code-block:: console

           --rd_uid <rotating device ID unique ID>

      * (optional) Generate a new ID and provide it:

        .. code-block:: console

           --generate_rd_uid --rd_uid <rotating device ID unique ID>

        You can find the newly generated unique ID in the console output.

   #. (optional) Add the JSON schema to verify the JSON file (replace the respective variable with the file path):

      .. code-block:: console

         --schema <path to JSON Schema file>

   #. (optional) Add a request to include a pairing passcode in the JSON file:

      .. code-block:: console

         --include_passcode

   #. (optional) Add an enable key for initiating device-specific test scenarios:

      .. code-block:: console

         --enable_key <enable key>

   #. (optional) Add the request to overwrite existing the JSON file:

      .. code-block:: console

         --overwrite

   #. (optional) Add the appearance of the product:

      .. code-block:: console

         --product_finish <finish> --product_color <color>

   #. (optional) Generate the Certification Declaration for testing purposes:

      .. code-block:: console

         --chip_cert_path <path to chip-cert executable> --gen_cd

      .. note::
         To generate a new Certification Declaration, you need the ``chip-cert`` executable.
         See the note at the end of this section to learn how to get it.

   #. (optional) Set the partition offset that is an address in device's NVM memory, where factory data will be stored:

      .. code-block:: console

         --offset <offset>

      .. note::
         To generate a HEX file with factory data, you need to provide both ``offset`` and ``size`` optional arguments.
         As a result, the :file:`factory_data.hex` and :file:`factory_data.bin` files are created in the :file:`output` directory.
         The first file contains the required memory offset.
         For this reason, you can program it directly to the device using a programmer (for example, ``nrfutil device``).

   #. (optional) Set the maximum partition size in device's NVM memory, where factory data will be stored:

      .. code-block:: console

         --size <size>

      .. note::
         To generate a HEX file with factory data, you need to provide both ``offset`` and ``size`` optional arguments.
         As a result, the :file:`factory_data.hex` and :file:`factory_data.bin` files are created in the :file:`output` directory.
         The first file contains the required memory offset.
         For this reason, you can program it directly to the device using a programmer (for example, ``nrfutil device``).

#. Run the script using the prepared list of arguments:

    .. code-block:: console

       $ python generate_nrfconnect_chip_factory_data.py <arguments>

For example, a final invocation of the Python script can look similar to the following one:

.. code-block:: console

   $ python scripts/tools/nrfconnect/generate_nrfconnect_chip_factory_data.py \
   --sn "11223344556677889900" \
   --vendor_id 65521 \
   --product_id 32774 \
   --vendor_name "Nordic Semiconductor ASA" \
   --product_name "not-specified" \
   --date "2022-02-02" \
   --hw_ver 1 \
   --hw_ver_str "prerelase" \
   --dac_cert "credentials/development/attestation/Matter-Development-DAC-FFF1-8006-Cert.der" \
   --dac_key "credentials/development/attestation/Matter-Development-DAC-FFF1-8006-Key.der" \
   --pai_cert "credentials/development/attestation/Matter-Development-PAI-FFF1-noPID-Cert.der" \
   --spake2_it 1000 \
   --spake2_salt "U1BBS0UyUCBLZXkgU2FsdA==" \
   --discriminator 0xF00 \
   --generate_rd_uid \
   --passcode 20202021 \
   --product_finish "matte" \
   --product_color "black" \
   --out "build" \
   --schema "scripts/tools/nrfconnect/nrfconnect_factory_data.schema" \
   --offset 0xf7000 \
   --size 0x1000

As the result of the above example, a unique ID for the rotating device ID is created, SPAKE2+ verifier is generated using the ``spake2p`` executable, and the JSON file is verified using the prepared JSON Schema.

If the script completes successfully, go to the location you provided with the ``-o`` argument.
Use the JSON file you find there when :ref:`ug_matter_device_factory_provisioning_generating_factory_data`.

.. note::
   Generating new certificates is optional if default vendor and product IDs are used and requires providing a path to the ``chip-cert`` executable.
   Complete the following steps to generate the new certificates:

   1.  Navigate to the :file:`connectedhomeip` root directory.
   #.  In a terminal, run the following command to build the executable:

       .. code-block:: console

          cd src/tools/chip-cert && gn gen out && ninja -C out chip-cert

   #. Add the ``connectedhomeip/src/tools/chip-cert/out/chip-cert`` path as an argument of ``--chip_cert_path`` for the Python script.

.. note::
   By default, overwriting the existing JSON file is disabled.
   This means that you cannot create a new JSON file with the same name in the same location as an existing file.
   To allow overwriting, add the ``--overwrite`` option to the argument list of the Python script.

.. _ug_matter_device_factory_provisioning_how_to_set_user_data:

Setting user data
=================

The user data can be optionally provided in the factory data JSON file and depends on the manufacturer's purpose.
You can provide it using the ``user`` field in a JSON factory data file that is represented by a flat JSON map and it can consist of ``string`` or ``int32`` data types only.
On the device side, the ``user`` data will be available as a CBOR map containing all defined ``string`` and ``int32`` fields.

To add user data as an argument to the `Generate factory data script`_, add the following line to the argument list:

.. code-block:: console

   --user '{user data JSON}'

As ``user data JSON``, provide a flat JSON map with a value file that consists of ``string`` or ``int32`` types.
For example, you can use a JSON file that looks like follows:

.. code-block:: console

   {
        "name": "product_name",
        "version": 123,
        "revision": "0x123"
   }

When added to the argument line, the final result would look like follows:

.. code-block:: console

   --user '{"name": "product_name", "version": 123, "revision": "0x123"}'

Handling  user data
-------------------

The user data is not handled anywhere in the Matter stack, so you must handle it in your application.
Use the `Factory Data Provider`_ and apply one of the following methods:

* ``GetUserData`` method to obtain raw data in the CBOR format as a ``MutableByteSpan``.
* ``GetUserKey`` method that lets you search along the user data list using a specific key, and if the key exists in the user data, the method returns its value.

If you opt for ``GetUserKey``, complete the following steps to set up the search:

1. Add the ``GetUserKey`` method to your code.
#. Given that all integer fields of the ``user`` Factory Data field are ``int32``, provide a buffer that has a size of at least 4 B or an ``int32_t`` variable to ``GetUserKey``.
   To read a string field from user data, the buffer should have a size of at least the length of the expected string.
#. Set it up to read all user data fields.

Only after this setup is complete, you can use all variables in your code and cast the result to your own purpose.

The code example of how to read all fields from the JSON example one by one can look like follows:

.. code-block:: C++

   chip::DeviceLayer::FactoryDataProvider factoryDataProvider;

   factoryDataProvider.Init();

   uint8_t user_name[12];
   size_t name_len = sizeof(user_name);
   factoryDataProvider.GetUserKey("name", user_name, name_len);

   int32_t version;
   size_t version_len = sizeof(version);
   factoryDataProvider.GetUserKey("version", &version, version_len);

   uint8_t revision[5];
   size_t revision_len = sizeof(revision);
   factoryDataProvider.GetUserKey("revision", revision, revision_len);

.. _ug_matter_device_factory_provisioning_verifying_using_json_schema_tool:

Verifying using the JSON Schema tool
====================================

You can verify the JSON file that contains factory data using the `Factory data schema`_.
You can use one of the following three options to validate the structure and contents of the JSON data.

Option 1: Using the php-json-schema tool
----------------------------------------

To check the JSON file using a `PHP JSON Schema`_ tool manually on a Linux machine, complete the following steps:

1. Install the ``php-json-schema`` package:

   .. code-block:: console

      $ sudo apt install php-json-schema

#. Run the following command, with ``<path_to_JSON_file>`` and ``<path_to_schema_file>`` replaced with the paths to the JSON file and the Schema file, respectively:

   .. code-block:: console

      $ validate-json <path_to_JSON_file> <path_to_schema_file>

The tool returns empty output in case of success.

Option 2: Using a website validator
-----------------------------------

You can also use external websites instead of the ``php-json-schema`` tool to verify a factory data JSON file.
For example, open the `JSON Schema Validator`_ and copy-paste the content of the `Factory data schema`_ to the first window and a JSON file to the second one.
A message under the window indicates the validation status.

Option 3: Using the nRF Connect Python script
---------------------------------------------

You can have the JSON file checked automatically by the Python script during the file generation.
Install the ``jsonschema`` Python module in your Python environment and provide the path to the JSON schema file as an additional argument.
Complete the following steps:

1. Install the ``jsonschema`` Python module by invoking one of the following commands from the Matter root directory:

   * Install only the ``jsonschema`` module:

     .. code-block:: console

        $ python -m pip install jsonschema

   * Install the ``jsonschema`` module together with all dependencies for Matter:

     .. code-block:: console

        $ python -m pip install -r ./scripts/setup/requirements.nrfconnect.txt

#. Run the following command (remember to replace the ``<path_to_schema>`` variable):

   .. code-block:: console

      $ python generate_nrfconnect_chip_factory_data.py --schema <path_to_schema>

.. note::
   To learn more about the JSON schema, see the unofficial `JSON Schema`_ tool usage website.

Generating onboarding codes
===========================

The `Generate factory data script`_ lets you generate a manual code and a QR code from the given factory data parameters.
You can use these codes to perform commissioning to the Matter network over Bluetooth LE since they include all the pairing data required by the Matter controller.
You can place these codes on the device packaging or on the device itself during production.

To generate a manual pairing code and a QR code, complete the following steps:

1. Install all required Python dependencies for Matter:

   .. code-block:: console

      $ python -m pip install -r ./scripts/setup/requirements.nrfconnect.txt

#. Complete **Steps 1, 2, and 3** described in the :ref:`ug_matter_device_factory_provisioning_creating_the_factory_data_json_and_hex_files_with_the_first_script` section to prepare the final invocation of the Python script.
#. Add the ``--generate_onboarding`` argument to the Python script final invocation.
#. Run the script.
#. Navigate to the output directory provided as the ``-o`` argument.

The output directory contains the following files you need:

* JSON file containing the latest factory data set.
* Test file containing the generated manual code and the text version of the QR Code.
* PNG file containing the generated QR code as an image.

Enabling onboarding codes generation within the build system
------------------------------------------------------------

You can generate the onboarding codes using the nRF Connect platform build system described in :ref:`ug_matter_device_factory_provisioning_building_example_with_factory_data`, and build an example with the additional :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_GENERATE_ONBOARDING_CODES` Kconfig option set to ``y``.

For example, the build command for the nRF52840 DK could look like this:

.. parsed-literal::
   :class: highlight

   $ west build -b nrf52840dk_nrf52840 -- \
   -DCONFIG_CHIP_FACTORY_DATA=y \
   -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y \
   -DCONFIG_CHIP_FACTORY_DATA_GENERATE_ONBOARDING_CODES=y

Preparing factory data partition on a device
============================================

The factory data partition is an area in the device's persistent storage, where the factory data set is stored.
This area is configured using the :ref:`partition_manager`, within which all partitions are declared in the :file:`pm_static.yml` file.

To prepare an example that supports factory data, add a partition called ``factory_data`` to the :file:`pm_static.yml` file.
The partition size should be a multiple of one flash page (for nRF52 and nRF53 SoCs, a single page size equals 4 kB).

See the following code snippet for an example of a factory data partition in the :file:`pm_static.yml` file.
The snippet is based on the :file:`pm_static.yml` file from the :ref:`matter_lock_sample` and uses the nRF52840 DK:

.. parsed-literal::
   :class: highlight

   mcuboot_primary_app:
       orig_span: &id002
           - app
       span: \*id002
       address: 0x7200
       size: 0xf3e00

   factory_data:
       address: 0xfb00
       size: 0x1000
       region: flash_primary

   settings_storage:
       address: 0xfc000
       size: 0x4000
       region: flash_primary

In this example, a ``factory_data`` partition has been placed between the application partition (``mcuboot_primary_app``) and the settings storage.
Its size has been set to one flash page (4 kB).

Use Partition Manager's report tool to ensure you created the factory data partition correctly.
Navigate to the example directory and run the following command:

.. parsed-literal::
   :class: highlight

   $ west build -t partition_manager_report

The output will look similar to the following one:

.. parsed-literal::
   :class: highlight

   external_flash (0x800000 - 8192kB):
   +---------------------------------------------+
   | 0x0: mcuboot_secondary (0xf4000 - 976kB)    |
   | 0xf4000: external_flash (0x70c000 - 7216kB) |
   +---------------------------------------------+

   flash_primary (0x100000 - 1024kB):
   +-------------------------------------------------+
   | 0x0: mcuboot (0x7000 - 28kB)                    |
   +---0x7000: mcuboot_primary (0xf4000 - 976kB)-----+
   | 0x7000: mcuboot_pad (0x200 - 512B)              |
   +---0x7200: mcuboot_primary_app (0xf3e00 - 975kB)-+
   | 0x7200: app (0xf3e00 - 975kB)                   |
   +-------------------------------------------------+
   | 0xfb000: factory_data (0x1000 - 4kB)            |
   +-------------------------------------------------+
   | 0xfc000: settings_storage (0x4000 - 16kB)       |
   +-------------------------------------------------+

   sram_primary (0x40000 - 256kB):
   +--------------------------------------------+
   | 0x20000000: sram_primary (0x40000 - 256kB) |
   +--------------------------------------------+

.. _ug_matter_device_factory_provisioning_creating_the_factory_data_partition_with_the_second_script:

Creating a factory data partition with the second script
========================================================

To store the factory data set in the device's persistent storage, convert the data from the JSON file to its binary representation in the CBOR format.
This is done by the `Generate factory data script`_, if you provide the optional ``offset`` and ``size`` arguments.
If you provided these arguments, skip the following steps of this section.

You can skip the optional arguments and do this using the `Generate partition script`_, but this is an obsolete solution and kept only for backward compatibility:

1. Navigate to the :file:`connectedhomeip` root directory.
#. Run the following command pattern:

   .. code-block:: console

      $ python scripts/tools/nrfconnect/nrfconnect_generate_partition.py -i <path_to_JSON_file> -o <path_to_output> --offset <partition_address_in_memory> --size <partition_size>

   In this command:

    * ``<path_to_JSON_file>`` is a path to the JSON file containing the appropriate factory data.
    * ``<path_to_output>`` is a path to an output file without any prefix.
      For example, providing ``/build/output`` as an argument will result in creating the :file:`/build/output.hex` and :file:`/build/output.bin` files.
    * ``<partition_address_in_memory>`` is an address in the device's persistent storage area where a partition data set is to be stored.
    * ``<partition_size>`` is the size of partition in the device's persistent storage area.
      New data is checked according to this value of the JSON data to see if it fits the size.

To see the optional arguments for the script, use the following command:

.. code-block:: console

   $ python scripts/tools/nrfconnect/nrfconnect_generate_partition.py -h

**Example of the command for the nRF52840 DK:**

.. code-block:: console

   $ python scripts/tools/nrfconnect/nrfconnect_generate_partition.py -i build/light_bulb/zephyr/factory_data.json -o build/light_bulb/zephyr/factory_data --offset 0xfb000 --size 0x1000

As a result, the :file:`factory_data.hex` and :file:`factory_data.bin` files are created in the :file:`/build/light_bulb/zephyr/` directory.
The first file contains the memory offset.
For this reason, you can program it directly to the device using a programmer (for example, ``nrfutil device``).

.. _ug_matter_device_factory_provisioning_building_example_with_factory_data:

Building an example with factory data
*************************************

You can manually generate the factory data set using the instructions provided in the :ref:`ug_matter_device_factory_provisioning_generating_factory_data` section.
Another way is to use the nRF Connect platform build system that creates the factory data content automatically using Kconfig options and includes the content in the final firmware binary.

To enable generating the factory data set automatically, go to the example's directory and build the example with the following option (replace ``nrf52840dk_nrf52840`` with your board name):

.. parsed-literal::
   :class: highlight

   $ west build -b nrf52840dk_nrf52840 -- -DCONFIG_CHIP_FACTORY_DATA=y -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y

Alternatively, you can also add the :kconfig:option:`SB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y` Kconfig setting to the example's :file:`sysbuild.conf` file.

Each factory data parameter has a default value.
These are described in the `Matter nRF Connect Kconfig`_.
You can set a new value for the factory data parameter either by providing it as a build argument list or by using interactive Kconfig interfaces.

Providing factory data parameters as a build argument list
==========================================================

You can provide the factory data using a third-party build script, as it uses only one command.
You can edit all parameters manually by providing them as an additional option for the west command.
For example (replace ``nrf52840dk_nrf52840`` with own board name):

.. parsed-literal::
   :class: highlight

   $ west build -b nrf52840dk_nrf52840 -- -DCONFIG_CHIP_FACTORY_DATA=y --DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y --DCONFIG_CHIP_DEVICE_DISCRIMINATOR=0xF11

Alternatively, you can add the relevant Kconfig option lines to the example's :file:`prj.conf` file.

Setting factory data parameters using interactive Kconfig interfaces
====================================================================

You can edit all configuration options using the interactive Kconfig interface.

In the configuration window, expand the items ``Modules -> connectedhomeip (/home/arbl/matter/connectedhomeip/config/nrfconnect/chip-module) -> Connected Home over IP protocol stack``.
You will see all factory data configuration options, as in the following snippet:

.. code-block:: console

   (65521) Device vendor ID
   (32774) Device product ID
   [*] Enable Factory Data build
   [*]     Enable merging generated factory data with the build tar
   [*]     Use default certificates located in Matter repository
   [ ]     Enable SPAKE2 verifier generation
   [*]     Enable generation of a new Rotating device id unique id
   (11223344556677889900) Serial number of device
   (Nordic Semiconductor ASA) Human-readable vendor name
   (not-specified) Human-readable product name
   (2022-01-01) Manufacturing date in ISO 8601
   (0) Integer representation of hardware version
   (prerelease) user-friendly string representation of hardware ver
   (0xF00) Device pairing discriminator
   (20202021) SPAKE2+ passcode
   (1000) SPAKE2+ iteration count
   (U1BBS0UyUCBLZXkgU2FsdA==) SPAKE2+ salt in string format
   (uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ
   (91a9c12a7c80700a31ddcfa7fce63e44) A rotating device id unique i

Default Kconfig values and developing aspects
=============================================

Each factory data parameter has its default value reflected in the Kconfig.
The list below shows some Kconfig settings that are configured in the nRF Connect build system and have an impact on the application.
You can modify them to achieve the desired behavior of your application.

* The device uses the test certificates located in the :file:`credentials/development/attestation/` directory, which are generated using all default values.
  If you want to change the default ``vendor_id``, ``product_id``, ``vendor_name``, or ``device_name`` and generate new test certificates, set the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_CERT_SOURCE_GENERATED` Kconfig option to `` y``.
  Remember to build the ``chip-cert`` application and add it to the system PATH.

  For developing a production-ready device, you need to write the certificates obtained during the certification process.
  Set the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_CERT_SOURCE_USER` Kconfig option to ``y`` and set the appropriate paths for the following Kconfig options:

  * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_USER_CERTS_DAC_CERT`
  * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_USER_CERTS_DAC_KEY`
  * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_USER_CERTS_PAI_CERT`

* By default, the SPAKE2+ verifier is generated during each example's build.
  This means that this value will change automatically if you change any of the following parameters:

  * :kconfig:option:`CONFIG_CHIP_DEVICE_SPAKE2_PASSCODE`
  * :kconfig:option:`CONFIG_CHIP_DEVICE_SPAKE2_SALT`
  * :kconfig:option:`CONFIG_CHIP_DEVICE_SPAKE2_IT`

  You can disable the generation of the SPAKE2+ verifier by setting the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_GENERATE_SPAKE2_VERIFIER` Kconfig option to ``n``.
  You also need to provide the externally-generated SPAKE2+ verifier using the :kconfig:option:`CONFIG_CHIP_DEVICE_SPAKE2_TEST_VERIFIER` Kconfig value.

* Generating the unique ID for rotating device ID is disabled by default, but you can enable it by setting the :kconfig:option:`CONFIG_CHIP_ROTATING_DEVICE_ID` and :kconfig:option:`CONFIG_CHIP_DEVICE_GENERATE_ROTATING_DEVICE_UID` Kconfig options to ``y``.
  Moreover, if you set the :kconfig:option:`CONFIG_CHIP_ROTATING_DEVICE_ID` Kconfig option to ``y`` and disable the :kconfig:option:`CONFIG_CHIP_DEVICE_GENERATE_ROTATING_DEVICE_UID` Kconfig option, you need to provide it manually using the :kconfig:option:`CONFIG_CHIP_DEVICE_ROTATING_DEVICE_UID` Kconfig value.

* You can generate the test Certification Declaration by setting the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_GENERATE_CD` Kconfig option to ``y``.
  Remember to build the ``chip-cert`` application and add it to the system PATH.

Programming factory data
************************

You can program the HEX file containing factory data into the device's flash memory using ``nrfutil device`` and the J-Link programmer.
Use the following command:

.. code-block:: console

   $ nrfutil device program --firmware factory_data.hex

.. note::
   For more information about how to use the ``nrfutil device`` utility, see `nRF Util`_.

Another way to program the factory data to a device is to use the nRF Connect platform build system described in :ref:`ug_matter_device_factory_provisioning_building_example_with_factory_data`, and build an example with the additional :kconfig:option:`SB_CONFIG_MATTER_FACTORY_DATA_MERGE_WITH_FIRMWARE` Kconfig option set to ``y``:

.. parsed-literal::
   :class: highlight

   $ west build -b nrf52840dk_nrf52840 -- \
   -DCONFIG_CHIP_FACTORY_DATA=y \
   -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y \
   -DSB_CONFIG_MATTER_FACTORY_DATA_MERGE_WITH_FIRMWARE=y

You can also build an example with auto-generation of new CD, DAC and PAI certificates.
The newly generated certificates will be added to factory data set automatically.
To generate new certificates disable using default certificates by building an example with the additional option :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_USE_DEFAULT_CERTS` set to ``n``:

.. parsed-literal::
   :class: highlight

   $ west build -b nrf52840dk_nrf52840 -- \
   -DCONFIG_CHIP_FACTORY_DATA=y \
   -DSB_CONFIG_MATTER_FACTORY_DATA_GENERATE=y \
   -DSB_CONFIG_MATTER_FACTORY_DATA_MERGE_WITH_FIRMWARE=y \
   -DCONFIG_CHIP_FACTORY_DATA_USE_DEFAULT_CERTS=n

.. note::
   To generate new certificates using the nRF Connect platform build system, you need the ``chip-cert`` executable in your system variable PATH.
   To learn how to get ``chip-cert``, see the note at the end of the :ref:`ug_matter_device_factory_provisioning_creating_the_factory_data_partition_with_the_second_script` section, and then add the newly built executable to the system variable PATH.
   The CMake build system will find this executable automatically.

After that, use the following command from the example's directory to write the firmware and newly generated factory data at the same time:

.. parsed-literal::
   :class: highlight

   $ west flash

.. _ug_matter_device_factory_provisioning_using_own_factory_data_implementation:

Using own factory data implementation
*************************************

The :ref:`ug_matter_device_factory_provisioning_generating_factory_data` described above is only an example valid for the nRF Connect platform.
You can well create a HEX file containing all :ref:`ug_matter_device_factory_provisioning_factory_data_component_table` in any format and then implement a parser to read out all parameters and pass them to a provider.
Each manufacturer can implement their own factory data by implementing a parser and a factory data accessor inside the Matter stack.
Use the :file:`FactoryDataProvider.h` and :file:`FactoryDataParser.h` files from the `Matter nRF Connect platform source files`_ as examples.

You can read the factory data set from the device's flash memory in different ways, depending on the purpose and the format.
In the nRF Connect example, the factory data is stored in the CBOR format.
The device uses the :file:`FactoryDataParser.h` file to read out raw data, decode it, and store it in the ``FactoryData`` structure.
The :file:`FactoryDataProvider.c` implementation uses this parser to get all needed factory data parameters and provide them to the Matter core.

In the nRF Connect example, the ``FactoryDataProvider`` is a template class that inherits from the ``DeviceAttestationCredentialsProvider``, ``CommissionableDataProvider``, and ``DeviceInstanceInfoProvider`` classes.
Your custom implementation must also inherit from these classes and implement their functions to get all factory data parameters from the device's flash memory.
These classes are virtual and need to be overridden by the derived class.
To override the inherited classes, complete the following steps:

1. Override the following methods:

   .. code-block:: C++

      // ===== Members functions that implement the DeviceAttestationCredentialsProvider
      CHIP_ERROR GetCertificationDeclaration(MutableByteSpan & outBuffer) override;
      CHIP_ERROR GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer) override;
      CHIP_ERROR GetDeviceAttestationCert(MutableByteSpan & outBuffer) override;
      CHIP_ERROR GetProductAttestationIntermediateCert(MutableByteSpan & outBuffer) override;
      CHIP_ERROR SignWithDeviceAttestationKey(const ByteSpan & messageToSign, MutableByteSpan & outSignBuffer) override;

      // ===== Members functions that implement the CommissionableDataProvider
      CHIP_ERROR GetSetupDiscriminator(uint16_t & setupDiscriminator) override;
      CHIP_ERROR SetSetupDiscriminator(uint16_t setupDiscriminator) override;
      CHIP_ERROR GetSpake2pIterationCount(uint32_t & iterationCount) override;
      CHIP_ERROR GetSpake2pSalt(MutableByteSpan & saltBuf) override;
      CHIP_ERROR GetSpake2pVerifier(MutableByteSpan & verifierBuf, size_t & verifierLen) override;
      CHIP_ERROR GetSetupPasscode(uint32_t & setupPasscode) override;
      CHIP_ERROR SetSetupPasscode(uint32_t setupPasscode) override;

      // ===== Members functions that implement the DeviceInstanceInfoProvider
      CHIP_ERROR GetVendorName(char * buf, size_t bufSize) override;
      CHIP_ERROR GetVendorId(uint16_t & vendorId) override;
      CHIP_ERROR GetProductName(char * buf, size_t bufSize) override;
      CHIP_ERROR GetProductId(uint16_t & productId) override;
      CHIP_ERROR GetSerialNumber(char * buf, size_t bufSize) override;
      CHIP_ERROR GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & day) override;
      CHIP_ERROR GetHardwareVersion(uint16_t & hardwareVersion) override;
      CHIP_ERROR GetHardwareVersionString(char * buf, size_t bufSize) override;
      CHIP_ERROR GetRotatingDeviceIdUniqueId(MutableByteSpan & uniqueIdSpan) override;

#. Move the newly created parser and provider files to your project directory.
#. Add the files to the :file:`CMakeList.txt` file.
#. Disable building both the default and the nRF Connect implementations of factory data providers to start using your own implementation of factory data parser and provider.
   This can be done in one of the following ways:

   * Set the :ref:`CONFIG_FACTORY_DATA_CUSTOM_BACKEND` Kconfig option to ``y`` in the :file:`prj.conf` file.
   * Build an example with the following option (replace ``<build_target>`` with your board name, for example ``nrf52840dk_nrf52840``):

     .. parsed-literal::
        :class: highlight

        $ west build -b <build_target> -- -DCONFIG_FACTORY_DATA_CUSTOM_BACKEND=y
