.. _mpsl_assert:

Multiprotocol Service Layer assert
##################################

.. contents::
   :local:
   :depth: 2

The Multiprotocol Service Layer assert library makes it possible to add a custom assert handler to the :ref:`nrfxlib:mpsl` library.
You can then use this assert handler to print custom error messages or log assert information.

:kconfig:option:`CONFIG_MPSL_ASSERT_HANDLER` enables the custom assert handler.
If enabled, the application must provide the definition of :c:func:`mpsl_assert_handle`.
The :c:func:`mpsl_assert_handle` function is invoked whenever the MPSL code encounters an unrecoverable error.

API documentation
*****************

| Header file: :file:`include/mpsl/mpsl_assert.h`

.. doxygengroup:: mpsl_assert
   :project: nrf
   :members:
