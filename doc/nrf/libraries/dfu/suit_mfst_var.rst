.. _subsys_suit_mfst_var:

SUIT Manifest-accessible variables
##################################

.. contents::
   :local:
   :depth: 2

The SUIT Manifest-accessible variables are new type of components that allow to store integer values.
They are readable by any SUIT manifest, the SDFW, and the IPC clients.
The modifiability of these variables depends on the variable ID.

Overview
********

The manifest-controlled variable can be defined as one of the components inside the SUIT manifest.
The component ID of the variables is constructed as follows:

.. code-block:: console

   [ MFST_VAR, <id> ]

There is a fixed number of such components for a given platform.
Their functionality and access privileges are defined by using ``<id>`` values from the following predefined ranges:

  * IDs from ``0`` to ``7`` are meant to hold persistently stored configuration data, which can be modified by OEM-controlled elements.
    They can be utilized to tune the installation and boot process.
    They are stored in two copies, protected by the digest, ensuring that only a valid value will be returned.
    If the system detects that it is impossible to synchronize those two copies, further updates of all NVM variables are blocked.

    .. warning::
       Non-volatile variables are stored at the fixed location inside the memory.
       There are no mechanisms managing the NVM wearing, so the frequency of the variable updates should be managed by manifest designer.

    The non-volatile variables can be modified by Application, Radio Manifests, and by IPC clients running on Application and Radio Domains.

  * IDs from ``128`` to ``129`` are meant to hold runtime, volatile information, provided by the SDFW.
    Currently there are no functionalities assigned to those variables.

    The platform-controlled variables cannot be modified by Manifests, and IPC clients.

  * IDs from ``256`` to ``259`` are meant to hold runtime, volatile information, provided by OEM-controlled manifests.
    These variables can be used to pass the information about executed boot scenario (selected set of images boot in the "A/B" method) as well as allow for a minimal manifest execution tracing.

    The volatile variables can be modified by Application and Radio Manifests.
    IPC clients are not allowed to modify them.

Configuration
*************

All configuration option values are defined by the SDFW.
They are also available in read-only mode for all types of applications.
All the APIs are serialized using the SSF framework.

The following configuration options are defined:

* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_NVM_BASE_ID` - Defines the first ID of manifest variables, that are stored in the non-volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_NVM_COUNT` - Defines the number of available manifest variables, that are stored in the non-volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_NVM_ACCESS_MASK` - Defines the access mask of manifest variables, that are stored in the non-volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_BASE_ID` - Defines the first ID of platform-controlled manifest variables, that are stored in the volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_COUNT` - Defines the number of available platform-controlled manifest variables, that are stored in the volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_PLAT_VOLATILE_ACCESS_MASK` - Defines the access mask of platform-controlled manifest variables, that are stored in the volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_BASE_ID` - Defines the first ID of manifest variables, that are stored in the volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_COUNT` - Defines the number of available manifest variables, that are stored in the volatile memory.
* :kconfig:option:`CONFIG_SUIT_MANIFEST_VARIABLES_MFST_VOLATILE_ACCESS_MASK` - Defines the access mask of manifest variables, that are stored in the volatile memory.

API documentation
*****************

| Header file: :file:`subsys/suit/manifest_variables/include/suit_manifest_variables.h`
| Source files: :file:`subsys/suit/manifest_variables/src/suit_manifest_variables.c`
