.. _ug_nrf54h20_suit_components:

SUIT components
###############

.. contents::
   :local:
   :depth: 2

:ref:`Components <ug_suit_dfu_suit_concepts>` are the fundamental elements that the SUIT manifest operates on and are declared in :ref:`suit-common <ug_suit_dfu_suit_common>` (which is represented as the bstr CBOR value ``suit-common`` within the manifest).
Using and modifying the component types allows for operational, fine-tuned customization of the DFU procedure.

See the :ref:`component_ID` section for more information.

Component types
***************

The current available component types are as follows:

* ``MEM`` - MCU memory-mapped slot (such as the MRAM or RAM).
  This is used to define a specific area in the memory where an image can be placed, and by default, it is defined automatically from the build system.
  The memory components are the abstractions that map to memory locations on the device's memory.

* ``CAND_MFST`` - The candidate manifest component refers to the dependency candidate in the envelope or in the download cache.
  Different values allow you to declare more than one component of that type per manifest.
  It points to the candidate manifest instead of copying the content to a memory location.
  For example, if an envelope contains all the candidate manifests, then you can refer to the manifest in the envelope using the candidate manifest component.

* ``CAND_IMG`` - The candidate image component refers to the integrated payload in the candidate envelope or payload in the download cache.
  For example, the ``CAND_IMG`` component allows you to validate the candidate before the installation step is performed.
  It points to the candidate memory address instead of copying the content to a memory location.

  The following example shows the installation procedure, including the checking of the coherency of the image by using the ``CAND_IMG`` component.
  During the installation process, ``directive-fetch`` puts information about the location of the image into component candidate image ``0``.
  ``condition-image-match`` performs the digest check on the candidate image ``0`` and verifies the digest of the image.
  After the verification, the installation of the image at the final location is done by the ``directive-copy``.

 .. code-block:: console

   components: [
   [MEM/2/0xeff8000/0x10000],
   [CAND_IMG/0]
   ]

   install: [
      directive-set-component-index 1,
      directive-override-parameters {
        uri: '#file.bin'
        image-digest: [
          algorithm-id: "sha256",
          digest-bytes: '00112233445566778899aabbccddeeff0123456789abcdeffedcba9876543210'
        ]
      },
      directive-fetch,
      condition-image-match,
      directive-set-component-index 0,
      directive-override-parameters {
            source-component: 1
      },
      directive-copy
   ]

* ``INSTLD_MFST`` - The installed manifest component is the slot for the envelope containing the installed manifest, which holds the severed manifest and its authentication block.
  It points to the manifest, which is already installed.

* ``SOC_SPEC`` - SOC-specific components are reserved for Nordic internal usage.
  The installation of this component type goes beyond memory CPU-like operations.

* ``CACHE_POOL`` - The ``CACHE_POOL`` is a space where images downloaded during the ``payload-fetch`` sequence execution can be stored and are ready for installation.
  In the context of the Application Domain, the ``payload-fetch`` sequence can contain instructions to fetch images from external sources (such as an HTTP server).
  Typically, these images must not be installed directly into the destination components by the ``payload-fetch`` sequence.
  They must be verified and installed by the ``install`` sequence, executed in the context of the SDFW or bootloader.


  ``CACHE_POOL`` allows you to push the envelope to the device, but without any payloads.
  When the manifest is processed, it may evaluate what is currently installed on the device.
  For example, the manifest calculates the digests of specific images, and based on those calculations, it fetches missing images from outside.

  When the value of ``CACHE_POOL`` is:

  * ``0`` - describes a location in the DFU partition, located right after the candidate envelope.
    When the ``CACHE_POOL`` value is ``0``, it is directly accessible by the SDFW, therefore, it may be used to store a companion image that, when executed on the Application MCU, allows the SDFW to access otherwise unsupported memory areas (such as external flash).

  * ``1..n`` -  describes a deployment-specific ``CACHE_POOL`` such as, for example, external flash partitions or other MRAM partitions.
    It defines multiple different cache pools.

  **Benefits of the ``CACHE_POOL`` component**

  The following are the benefits of the ``CACHE_POOL`` component:

  * Memory optimization - ``CACHE_POOL`` component allows you to conditionally pull missing images from the Application Domain or application framework before installation starts.
    It helps in the gradual update process by installing one of the two images in the first installation step and then repeating the process to install another image.

  * Save on data transfer costs -  The manifest and the candidate manifest only pull the missing images instead of pushing all the images in the update.

.. _component_ID:

Component ID parameters
***********************

The component types that can be modified at this time are listed in the following table.
Fields indicate different parameters for component types.

+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
| Field 0 - Type       | Field 1                                                | Field 2                | Field 3                | Field 4      | Component ID - example                                                           |
+======================+========================================================+========================+========================+==============+==================================================================================+
| ``MEM``              | CPU ID: ``int``                                        | Slot address: ``uint`` | Slot size: ``uint``    |              | Application MCU bootable:                                                        |
|                      |                                                        |                        |                        |              |                                                                                  |
|                      | (``-1`` indicates no booting capability)               |                        |                        |              | ``MEM/2/0xeff8000/0x10000``                                                      |
+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
| ``CAND_MFST``        | ID: ``uint``                                           |                        |                        |              | ``CAND_MFST/0``                                                                  |
+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
| ``CAND_IMG``         | ID: ``uint``                                           |                        |                        |              | ``CAND_IMG/0``                                                                   |
+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
| ``INSTLD_MFST``      | Manifest Class ID: ``bst``                             |                        |                        |              | ``INSTLD_MFST/0x3f6a3a4dcdfa58c5accef9f584c41124``                               |
+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
| ``SOC_SPEC``         | ID: ``uint``                                           |                        |                        |              | nRF54H20:                                                                        |
|                      |                                                        |                        |                        |              |                                                                                  |
|                      | Identifier valid within the namespace of a specific    |                        |                        |              | ``SOC_SPEC/1`` - SDFW                                                            |
|                      | SOC.                                                   |                        |                        |              |                                                                                  |
|                      |                                                        |                        |                        |              | ``SOC_SPEC/2`` - SDFW_Recovery                                                   |
+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
| ``CACHE_POOL``       | ID: ``uint``                                           |                        |                        |              | ``CACHE_POOL`` in DFU Partition (MRAM):                                          |
|                      |                                                        |                        |                        |              |                                                                                  |
|                      |                                                        |                        |                        |              | ``CACHE_POOL/0``                                                                 |
+----------------------+--------------------------------------------------------+------------------------+------------------------+--------------+----------------------------------------------------------------------------------+
