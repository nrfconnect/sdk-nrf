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

* Application Binary Interface (ABI) getter (:cpp:member:`abi_out`)
* Pointer to ABI getter (:cpp:member:`abi_in`)

.. _doc_fw_info_abi:

ABIs
****

The firmware information structure allows for exchange of arbitrary tagged and versioned interfaces called Application Binary Interfaces (ABIs).

An ABI structure is a structure consisting of a header followed by arbitrary data.
The header consists of the following information:

* ABI ID (uniquely identifies the ABI)
* ABI version (single, monotonically increasing version number for the ABI with this ID)
* ABI flags (32 individual bits for indicating the particulars of the ABI)
* ABI length (length of the following data)

To retrieve an ABI, a firmware image calls another firmware image's ABI getter.
Every image must provide an ABI getter (:cpp:member:`abi_out`) that other images can use to retrieve its ABIs.
This ABI getter is a function pointer that retrieves ABI structs (or rather pointers to ABI structs).

In addition, every image can access other ABIs through a second ABI getter (:cpp:member:`abi_in`).
This ABI getter must be provided when booting the image.
In other words, an image should expect its :cpp:member:`abi_in` ABI getter to be filled at the time it boots, and will not touch it during booting.
After booting, an image can call :cpp:member:`abi_in` to retrieve ABIs from the image or images that booted it without knowing where they are located.

Each image can provide multiple ABIs.
An ABI getter function takes an index, and each index from 0 to *n* must return a different ABI, given that the image provides *n* ABIs with the same ID.

Typically, the actual ABI will be a function pointer (or a list of function pointers), but the data can be anything, though it should be considered read-only.
The reason for making the ABI getters function pointers instead of pointing to a list of ABIs is to allow dynamic creation of ABI lists without using RAM.

Usage
*****

To locate and verify firmware info structures, use :cpp:func:`fw_info_find` and :cpp:func:`fw_info_check`, respectively.

To find an ABI with a given version and flags, call :cpp:func:`fw_info_abi_find`.
This function calls :cpp:member:`abi_in` under the hood, checks the ABI's version against the allowed range, and checks that it has all the flags set.

To populate an image's :cpp:member:`abi_in` (before booting the image), the booting image should call :cpp:func:`fw_info_abi_provide` with the other image's firmware information structure.
Note that if the booting (current) firmware image and the booted image's RAM overlap, :cpp:func:`fw_info_abi_provide` will corrupt the current firmware's RAM.
This is ok if it is done immediately before booting the other image, thus after it has performed its last RAM access.

Creating ABIs
*************

To create an ABI, complete the following steps:

1. Declare a new struct type that starts with the :c:type:`fw_info_abi` struct:

   .. code-block:: c

      struct my_abi {
      	   struct fw_info_abi header;
   	   struct {
   		   /* Actual ABI/data goes here. */
   	   } abi;
      };

#. Use the :c:macro:`__ext_abi` macro to initialize the ABI struct in an arbitrary location.
   :c:macro:`__ext_abi` will automatically include the ABI in the list provided via :cpp:func:`fw_info_abi_provide`.

   .. code-block:: c

      __ext_abi(struct my_abi, my_abi) = {
   	   .header = FW_INFO_ABI_INIT(MY_ABI_ID,
   				   CONFIG_MY_ABI_FLAGS,
   				   CONFIG_MY_ABI_VER,
   				   sizeof(struct my_abi)),
   	   .abi = {
   		   /* ABI initialization goes here. */
   	   }
      };

#. To include function pointers in your ABI, call the :c:macro:`EXT_ABI_FUNCTION` macro to forward-declare the function and create a typedef for the function pointer:

   .. code-block:: c

      EXT_ABI_FUNCTION(int, my_abi_foo, bool arg1, int *arg2);



API documentation
*****************

| Header file: :file:`include/fw_info.h`
| Source files: :file:`subsys/fw_info/`

.. doxygengroup:: fw_info
   :project: nrf
   :members:
