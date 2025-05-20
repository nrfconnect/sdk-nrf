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

For a detailed guide that describes the process of creating and programming factory data in the Matter ecosystem using the nRF Connect platform from Nordic Semiconductor, read the :doc:`matter:nrfconnect_factory_data_configuration` guide in the Matter documentation.
You need to complete this process for :ref:`ug_matter_device_attestation` and :ref:`ug_matter_device_certification`.
You can also see `Matter factory data Kconfig options`_ for an overview of possible configuration variants.
