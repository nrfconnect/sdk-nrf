.. _doc_bl_validation:

Bootloader firmware validation
##############################

The bootloader firmware validation library provides the function that the :ref:`bootloader` uses to validate a firmware image before booting it.

The API is public because applications that are booted by the immutable bootloader can call the function from this library via the bootloader's code, through external APIs.
See :ref:`doc_fw_info_ext_api` for more information.
Using this mechanism can be useful when the application receives a DFU package and wants to determine whether it will be accepted by the bootloader.

Validation
**********

The :cpp:func:`bl_validate_firmware` function validates the following information:

* The digest and the signature of the whole image (see :cpp:func:`bl_root_of_trust_verify`)
* The fields of the ``fw_info`` struct that is part of the firmware image (see :ref:`doc_fw_info`)

API documentation
*****************

| Header file: :file:`include/bl_validation.h`
| Source files: :file:`subsys/bootloader/bl_validation/`

.. doxygengroup:: bl_validation
   :project: nrf
   :members:
