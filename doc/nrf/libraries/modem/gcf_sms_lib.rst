.. _lib_gcf_sms_readme:

GCF SMS
#######

.. contents::
   :local:
   :depth: 2

The GCF SMS Library uses the :ref:`at_cmd_custom_readme` library and adds custom AT commands for the :c:func:`nrf_modem_at_cmd` function to comply with GCF SMS Certification criteria.
The library implements the following AT commands:

* ``AT+CPMS``
* ``AT+CSMS``
* ``AT+CSCA``
* ``AT+CSCS``
* ``AT+CMGD``
* ``AT+CMSS``
* ``AT+CMMS``
* ``AT+CMGW``
* ``AT+CMGF=0``

Configuration
*************

You can enable the GCF SMS Library by setting the :kconfig:option:`CONFIG_GCF_SMS` Kconfig option.

API documentation
*****************

The library does not expose any API of its own.

| Source files: :file:`lib/gcf_sms/gcf_sms.c`
