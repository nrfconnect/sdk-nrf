.. _ug_matter_device_configuring_cd:

Storing Certification Declaration in firmware
#############################################

.. contents::
   :local:
   :depth: 2

Certification Declaration (CD) is :ref:`one of the documents <ug_matter_device_certification_results>` obtained as a result of a successful Matter certification.
It is |matter_cd_definition|.
CD is used in the :ref:`ug_matter_device_attestation` procedure during commissioning, where it is provided to a device as an element of :ref:`Device Attestation data <ug_matter_device_attestation_device_data>`.

CD can only be stored in parts of firmware that can be modified during the device lifetime.
With each new software version, the device manufacturer must apply for :ref:`ug_matter_device_certification` using one of the accepted paths.
With an update to the device firmware, the new CD obtained as a result of the new certification process must replace the existing CD on the device.

The following sections describe where and how you can place CD in firmware and how you can test it before applying for certification.

Storing Certification Declaration
*********************************

In the |NCS| implementation of Matter, you can configure CD by adding the :c:macro:`CHIP_DEVICE_CONFIG_CERTIFICATION_DECLARATION` define in the :file:`chip_project_config.h` file.
You can locate the array of bytes related to CD by running the search with the following condition:

.. code-block:: console

   cat CD.der | xxd -I

Storing Certification Declaration in Zephyr
===========================================

Alternatively, you can opt for storing CD in Zephyr's Settings subsystem, which allows for storing data even after the device has been programmed.
For example, this lets you add CD to the subsystem through CLI, or use Zephyr's API to store CD in the Settings subsystem within the code.

To enable this configuration method, set the :kconfig:option:`CONFIG_CHIP_CERTIFICATION_DECLARATION_STORAGE` Kconfig option in the :file:`prj.conf` file instead of the define in :file:`chip_project_config.h`.

.. _ug_matter_device_configuring_cd_generating_steps:

Generating Certification Declaration for integration testing
************************************************************

To generate CD for integration testing, complete the following steps:

1. :ref:`Install the chip-cert tool <ug_matter_gs_tools_cert_installation>`.
2. Run the following command pattern to generate CD:

   .. parsed-literal::
      :class: highlight

      chip-cert gen-cd --key *path_to_key* --cert *path_to_cert* --out CD.der --format-version 1 --vendor-id *VID* --product-id *PID* --device-type-id *device_type* --certificate-id *CD_serial_number* --security-level 0 --security-info 0 --certification-type 1 --version-number *DCL_entry_value*

   In this command:

   * *path_to_key* corresponds to the path to the :file:`Chip-Test-CD-Signing-Key.pem` file, which usually can be found under :file:`modules/lib/matter/credentials/test/certification-declaration/`.
   * *path_to_cert* corresponds to the path to the :file:`Chip-Test-CD-Signing-Cert.pem` file, which usually can be found under :file:`modules/lib/matter/credentials/test/certification-declaration/`.
   * *VID* corresponds to your Vendor ID.
   * *PID* corresponds to your Product ID.
   * *device_type* corresponds to the device type identifier for the primary function of the device.
   * *CD_serial_number* corresponds to the serial number of CD, allocated by the CSA.
   * *DCL_entry_value* corresponds to the certification record associated with the product in the Distributed Compliance Ledger.

   For more information about some of these fields, see the section 6.3.1 of the Matter core specification.
   For example, the command can look like follows:

   .. code-block:: console

      chip-cert gen-cd --key credentials/test/certification-declaration/Chip-Test-CD-Signing-Key.pem --cert credentials/test/certification-declaration/Chip-Test-CD-Signing-Cert.pem --out CD.der --format-version 1 --vendor-id 0xFFF1 --product-id 0x8006 --device-type-id 0xA --certificate-id ZIG20142ZB330003-24 --security-level 0 --security-info 0 --certification-type 1 --version-number 0x2694
