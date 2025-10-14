.. _ug_nrf54l_otp_map:

nRF54L One-Time Programmable memory map for |NCS|
#################################################

.. contents::
   :local:
   :depth: 2

The nRF54L Series devices are equipped with emulated One-Time Programmable (OTP) memory, designed to store assets permanently and in a non-volatile format.
This memory is a part of the User Information Configuration Register (UICR) memory space.
It can only be programmed with non-0xFFFFFFFF values, and it is possible only once after an erase all operation.
The OTP is organized into 4-byte words and is mapped as the ``UICR.OTP[]`` array.
For details, refer to the :ref:`datasheet<ug_nrf54l>` for the specific device model.

The allocation of this memory is determined by the software, and the |NCS| reserves a part of it for its own usage.


.. list-table:: OTP memory assignment map for nRF54L Series devices
    :header-rows: 1
    :align: center
    :widths: auto

    * - ``UICR.OTP[]`` range
      - Size (4-byte words)
      - Assignment
      - Description
    * - 0 - 287
      - 288
      - ``bl_storage`` space
      - Used for bootloaders and secure firmware storage realization.
    * - 288 - 319
      - 32
      - Custom usage
      - Available for custom, user-defined purposes.
