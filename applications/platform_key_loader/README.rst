.. platform_key_loader

Platform Key Loader
###################

.. contents::
   :local:
   :depth: 2

The Platform Key Loader (PKL) is an application that interfaces with the Secure Domain Firmware (SDFW) to provision platform keys.

Motivation
==========

The Secure Domain Firmware (SDFW) is responsible for handling platform keys on behalf of local domains.
These keys are stored in a manner that makes it hard to serialize in build time.
Hence, all platform keys must be provisioned in run time.

Operation
=========

All platform keys to install are included in the image binary for the PKL at build time.
When provisioning the device, the PKL image and its associated assets must be programmed and executed.
The PKL triggers a ``psa_import_key`` operation for all keys.
Once the PKL has executed the keys are stored persistently.

Configuring and building the PKL
--------------------------------

Set the path to the directory that contains the keys.
Build the application normally.

Running the PKL
---------------

The PKL is programmed and executed as any other application.
Provisioning the platform keys is a one time operation.
Once the keys are written they cannot be replaced by executing the PKL again, they must be erased first.
Platform keys are erased by purging the corresponding local domain.
Purge the SECURE domain to erase the OEM platform keys, this will only erase the keys, not the SDFw.
This can only be done if one or more local domains are in LCS EMPTY.
