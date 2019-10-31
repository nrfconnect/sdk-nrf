.. _doc_fw_info:

Firmware information
####################

The firmware information module (``fw_info``) provides externally readable metadata about a firmware image.
This information is located at a specific offset in the image.
In addition, the module provides code to read such information.

Purpose
*******

The purpose of the firmware information is to allow other images such as bootloaders, or infrastructure such as firmware servers, to gain information about the firmware image.

The firmware information structure has a 12-byte magic header and a verified binary layout to ensure that the format is portable and identifiable.
It must be located at one of the following offsets from the start of the image: 0x0, 0x200, 0x400, 0x800, 0x1000.
The reason that the structure is not located at 0x00 is that this can be problematic in some use cases, such as when the vector table must be located at 0x00.

These rules make it simple to retrieve the information by checking for each possible offset (0x0, 0x200, 0x400, 0x800, 0x1000) if the first 12 bytes match the magic value.
If they do, the information can be retrieved according to the definition in :c:type:`fw_info`.

Information structure
*********************

The information structure contains a minimal set of information that allows other code to reason about the firmware with no other information available.
It includes the following information:

* The size of the firmware image.
* The single, monotonically increasing version number of the image.
* The address through which to boot into the firmware (the vector table address).
  This address is not necessarily the start of the image.
* A value that can be modified in place to invalidate the firmware.
  See :option:`CONFIG_FW_INFO_VALID_VAL`.
  If this option is set to any other value than :option:`CONFIG_FW_INFO_VALID_VAL` (for example, by the bootloader), the image can quickly be established as invalid.
  The bootloader sets this option to 0 when the image fails validation, so that there is no need to perform a costly validation on every boot.
  If the firmware is write-protected before being booted by the bootloader, only the bootloader can invalidate it.

At the end of the information structure, there is a variable-size list containing:

* External APIs
* External API requests

.. _doc_fw_info_ext_api:

External APIs
*************

The firmware information structure allows for exchange of arbitrary tagged and versioned interfaces called *external APIs* (EXT_APIs).

An EXT_API structure is a structure consisting of a header followed by arbitrary data.
The header consists of the following information:

* EXT_API ID (uniquely identifies the EXT_API)
* EXT_API version (single, monotonically increasing version number for the EXT_API with this ID)
* EXT_API flags (32 individual bits for indicating the particulars of the EXT_API)
* EXT_API length (length of the following data)

The EXT_API structures of the image are placed at the end of the information structure, immediately before its EXT_API requests.

An EXT_API request structure is a structure consisting of a description of the requested EXT_API (in the form of an EXT_API header), plus a few more pieces of data.
The structure also contains the location of a RAM buffer into which a pointer to a matching EXT_API can be placed before booting.
The RAM buffer must not be initialized or otherwise touched when booting the image, to ensure that the value written there before booting is preserved.
The :c:macro:`EXT_API_REQ` helper macro uses the ``__noinit`` decorator provided by Zephyr to achieve this.

The EXT_API system allows a bootloader to parse the image's requests and find matches for these requests by parsing its own (the bootloader's) EXT_APIs or EXT_APIs of other images.
After finding the matches, the bootloader populates the RAM buffers with pointers to the other images' EXT_APIs.
In this way, the bootloader will be aware of whether all images have access to their dependencies.

Each image can provide multiple EXT_APIs and make multiple requests.
It is also possible for images to search other images directly for matching EXT_APIs, without using EXT_API requests.

Typically, the data of an EXT_API structure contains a function pointer (or a list of function pointers), but it could consist of anything.
The data should be considered read-only though.

Binary compatibility
********************

When exposing an interface between images that have not been compiled and linked together, you must take extra care to ensure binary-compatibility between how the interface is implemented and how it is used.
The data placed into EXT_APIs must always have the same format for a given ID/version/flags combination.

To facilitate this, the :file:`fw_info.h` file contains a number of static asserts to ensure that the memory mapping of structures is the same every time the code is compiled.
There are a number of things that can change from compilation to compilation, depending on compiler options or just chance interactions with the rest of the code.

Some things to look out for are:

* Struct padding: The space between members of a struct can be different.
* Flag ordering: When splitting an integer into smaller chunks (with ``:``), like flags, the order in which they are mapped to memory can be different.
* Function ABI: The way the function uses registers and stack can be different.
* Size of certain types: The size of chars and enums can differ depending on compiler flags.
* Floating point ABI: The way floating point numbers are processed can be different (hard/soft/softfp).


Usage
*****

To locate and verify firmware information structures, use :cpp:func:`fw_info_find` and :cpp:func:`fw_info_check`, respectively.

To find an EXT_API with a given version and flags, call :cpp:func:`fw_info_ext_api_find`.
This function calls :cpp:member:`ext_api_in` under the hood, checks the EXT_API's version against the allowed range, and checks that it has all the flags set.

To populate an image's :cpp:member:`ext_api_in` (before booting the image), the booting image should call :cpp:func:`fw_info_ext_api_provide` with the other image's firmware information structure.
Note that if the booting (current) firmware image and the booted image's RAM overlap, :cpp:func:`fw_info_ext_api_provide` will corrupt the current firmware's RAM.
This is ok if it is done immediately before booting the other image, thus after it has performed its last RAM access.

Creating EXT_APIs
*****************

To create an EXT_API, complete the following steps:

1. Create a unique ID for the EXT_API:

   .. code-block:: c

      #define MY_EXT_API_ID 0xBEEF

#. Create Kconfig entries using :file:`Kconfig.template.fw_info_ext_api`:

   .. code-block:: Kconfig

      EXT_API = MY
      flags = 0
      ver = 1
      source "${ZEPHYR_BASE}/../nrf/subsys/fw_info/Kconfig.template.fw_info_ext_api"

#. Declare a new struct type that starts with the :c:type:`fw_info_ext_api` struct:

   .. code-block:: c

      struct my_ext_api {
      	struct fw_info_ext_api header;
      	struct {
      		/* Actual EXT_API/data goes here. */
      		my_ext_api_foo_t my_foo;
      	} ext_api;
      };

#. Use the :c:macro:`EXT_API` macro to initialize the EXT_API struct in an arbitrary location.
   :c:macro:`EXT_API` will automatically include the EXT_API in the list at the end of the firmware information structure.

   .. code-block:: c

      #ifdef CONFIG_MY_EXT_API_ENABLED
      EXT_API(struct my_ext_api, my_ext_api) = {
      	.header = FW_INFO_EXT_API_INIT(MY_EXT_API_ID,
      				CONFIG_MY_EXT_API_FLAGS,
      				CONFIG_MY_EXT_API_VER,
      				sizeof(struct my_ext_api)),
      	.ext_api = {
      		/* EXT_API initialization goes here. */
      		.my_foo = my_foo_impl,
      	}
      };
      #endif

#. To include function pointers in your EXT_API, call the :c:macro:`EXT_API_FUNCTION` macro to forward-declare the function and create a typedef for the function pointer:

   .. code-block:: c

      EXT_API_FUNCTION(int, my_ext_api_foo, bool arg1, int *arg2);

#. Enable the EXT_API in Kconfig:

   .. code-block:: none

      CONFIG_MY_EXT_API_ENABLED=y


Creating EXT_API requests
*************************

To create an EXT_API request, complete the following steps:

1. Assuming that the ID and the Kconfig entries are already created (see `Creating EXT_APIs`_), use the :c:macro:`EXT_API_REQ` macro to create a request structure:

   .. code-block:: c

      EXT_API_REQ(MY, 1, struct my_ext_api, my);

#. Use the EXT_API through the name given to :c:macro:`EXT_API_REQ`:

   .. code-block:: c

      my->ext_api.my_foo(my_arg1, my_arg2);

#. Request the EXT_API in Kconfig:

   .. code-block:: none

      CONFIG_MY_EXT_API_REQUIRED=y


API documentation
*****************

| Header file: :file:`include/fw_info.h`
| Source files: :file:`subsys/fw_info/`

.. doxygengroup:: fw_info
   :project: nrf
   :members:
