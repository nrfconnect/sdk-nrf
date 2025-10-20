.. _ug_nrf54l_developing_provision_kmu:

Performing KMU provisioning
###########################

.. contents::
   :local:
   :depth: 2

The nRF54L devices are equipped with Hardware Key Management Unit (KMU), that requires provisioning when in use.
The |NCS| provides a west command, ``ncs-provision``, allowing to upload keys to the device though the Serial Write Debug (SWD) interface.

Prerequisites
*************

First, ensure that the `nRF Util`_ tool is installed.
It should install automatically during the setup of the |NCS| working environment.
Once completed, install the required additional commands for nRF Util:

.. parsed-literal::
   :class: highlight

    nrfutil install device

Additionally, before provisioning, make sure you familiarized yourself with the :ref:`ug_nrf54l_developing_basics_kmu_provisioning_keys` section.

.. _ug_nrf54l_developing_provision_kmu_generate:

Key generation
**************

If you need a new key, you can generate it using imgtool or another tool that produces the required kind and format of key as a PEM file.

For instructions on how to generate a key using imgtool, see the :doc:`imgtool page in the MCUboot documentation<mcuboot:imgtool>`.
See the following example for generating a private key:

.. parsed-literal::
   :class: highlight

   imgtool keygen -k my_ed25519_priv_key.pem -t ed25519

.. _ug_nrf54l_developing_provision_kmu_provisioning:

Provisioning keys to the board
******************************

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

Alternative provisioning method
*******************************

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
