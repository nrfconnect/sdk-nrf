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
It must be located at one of three offsets from the start of the image: 0x200, 0x400, or 0x800.
The reason that the structure is not located at 0x00 is that this can be problematic in some use cases, such as when the vector table must be located at 0x00.

These rules make it simple to retrieve the information by checking for each possible offset (0x200, 0x400, 0x800) if the first 12 bytes match the magic value.
If they do, the information can be retrieved according to the definition in :c:type:`fw_info`.

Information structure
*********************

The information structure contains a minimal set of information that allows other code to reason about the firmware with no other information available.
It includes the following information:

* The size of the firmware image.
* The single, monotonically increasing version number of the image.
* The address through which to boot into the firmware (the vector table address).
  This address is not necessarily the start of the image.

Additionally, there is information for exchanging arbitrary data:

* External API (EXT_API) getter (:cpp:member:`ext_api_out`)
* Pointer to EXT_API getter (:cpp:member:`ext_api_in`)

.. _doc_fw_info_ext_api:

EXT_APIs
********

The firmware information structure allows for exchange of arbitrary tagged and versioned interfaces called External APIs (EXT_APIs).

An EXT_API structure is a structure consisting of a header followed by arbitrary data.
The header consists of the following information:

* EXT_API ID (uniquely identifies the EXT_API)
* EXT_API version (single, monotonically increasing version number for the EXT_API with this ID)
* EXT_API flags (32 individual bits for indicating the particulars of the EXT_API)
* EXT_API length (length of the following data)

To retrieve an EXT_API, a firmware image calls another firmware image's EXT_API getter.
Every image must provide an EXT_API getter (:cpp:member:`ext_api_out`) that other images can use to retrieve its EXT_APIs.
This EXT_API getter is a function pointer that retrieves EXT_API structs (or rather pointers to EXT_API structs).

In addition, every image can access other EXT_APIs through a second EXT_API getter (:cpp:member:`ext_api_in`).
This EXT_API getter must be provided when booting the image.
In other words, an image should expect its :cpp:member:`ext_api_in` EXT_API getter to be filled at the time it boots, and will not touch it during booting.
After booting, an image can call :cpp:member:`ext_api_in` to retrieve EXT_APIs from the image or images that booted it without knowing where they are located.

Each image can provide multiple EXT_APIs.
An EXT_API getter function takes an index, and each index from 0 to *n* must return a different EXT_API, given that the image provides *n* EXT_APIs with the same ID.

Typically, the actual EXT_API will be a function pointer (or a list of function pointers), but the data can be anything, though it should be considered read-only.
The reason for making the EXT_API getters function pointers instead of pointing to a list of EXT_APIs is to allow dynamic creation of EXT_API lists without using RAM.

Usage
*****

To locate and verify firmware info structures, use :cpp:func:`fw_info_find` and :cpp:func:`fw_info_check`, respectively.

To find an EXT_API with a given version and flags, call :cpp:func:`fw_info_ext_api_find`.
This function calls :cpp:member:`ext_api_in` under the hood, checks the EXT_API's version against the allowed range, and checks that it has all the flags set.

To populate an image's :cpp:member:`ext_api_in` (before booting the image), the booting image should call :cpp:func:`fw_info_ext_api_provide` with the other image's firmware information structure.
Note that if the booting (current) firmware image and the booted image's RAM overlap, :cpp:func:`fw_info_ext_api_provide` will corrupt the current firmware's RAM.
This is ok if it is done immediately before booting the other image, thus after it has performed its last RAM access.

Creating EXT_APIs
*****************

To create an EXT_API, complete the following steps:

1. Declare a new struct type that starts with the :c:type:`fw_info_ext_api` struct:

   .. code-block:: c

      struct my_ext_api {
      	   struct fw_info_ext_api header;
   	   struct {
   		   /* Actual EXT_API/data goes here. */
   	   } ext_api;
      };

#. Use the :c:macro:`__ext_api` macro to initialize the EXT_API struct in an arbitrary location.
   :c:macro:`__ext_api` will automatically include the EXT_API in the list provided via :cpp:func:`fw_info_ext_api_provide`.

   .. code-block:: c

      __ext_api(struct my_ext_api, my_ext_api) = {
   	   .header = FW_INFO_EXT_API_INIT(MY_EXT_API_ID,
   				   CONFIG_MY_EXT_API_FLAGS,
   				   CONFIG_MY_EXT_API_VER,
   				   sizeof(struct my_ext_api)),
   	   .ext_api = {
   		   /* EXT_API initialization goes here. */
   	   }
      };

#. To include function pointers in your EXT_API, call the :c:macro:`EXT_API_FUNCTION` macro to forward-declare the function and create a typedef for the function pointer:

   .. code-block:: c

      EXT_API_FUNCTION(int, my_ext_api_foo, bool arg1, int *arg2);



API documentation
*****************

| Header file: :file:`include/fw_info.h`
| Source files: :file:`subsys/fw_info/`

.. doxygengroup:: fw_info
   :project: nrf
   :members:
