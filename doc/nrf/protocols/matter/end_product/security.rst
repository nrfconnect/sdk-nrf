.. _ug_matter_device_security:

Securing production devices
###########################

.. contents::
   :local:
   :depth: 2

When finalizing work on a Matter device, make sure to adopt the following recommendations before sending the device to production.

Enable AP-Protect
*****************

Make sure to enable the AP-Protect feature for the production devices to disable the debug functionality.

.. include:: ../../../app_dev/ap_protect/index.rst
   :start-after: app_approtect_info_start
   :end-before: app_approtect_info_end

See :ref:`app_approtect` for more information.

Disable debug serial port
*************************

Make sure to disable the debug serial port, for example UART, so that logs and shell commands are not accessible for production devices.
See the :file:`prj_release.conf` files in :ref:`Matter samples <matter_samples>` for an example of how to disable debug functionalities.
