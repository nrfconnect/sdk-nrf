.. _lib_zzhc:

China Telecom ZZHC library
##########################

The China Telecom ZZHC library implements the self-registration (Zi-ZHu-Ce, ZZHC) functionality that registers the module information on the server when the network is available.

Overview
********

The telecommunication modules that are part of the China Telecom network are required to provide certain information stored on the module to the ``zzhc.vnet.cn`` server.
This is required for conformity with Chapter 6 of the China Telecom IoT Module Requirements Whitepaper.

The ZZHC library is started as a background thread on boot and it automatically detects the availability of the operator network.
Once the Internet connectivity is ready and a new SIM card is detected on boot, the library proactively uploads the following information to the ZZHC server:

    * IMEI
    * model number (that is, nRF9160)
    * modem revision
    * nRF9160 SIM ICCID
    * nRF9160 SIM IMSI

On any failure, which includes an unknown or negative response from the server or any error during the program execution, the ZZHC library retries to upload the information to the server once per hour, for a maximum of 5 attempts.

.. _lib_zzhc_configuration:

Configuration
*************

By default, the ZZHC library is not used in any sample.

Dependencies
------------

Configure the following option if you want to use this library:

:option:`CONFIG_TRUSTED_EXECUTION_NONSECURE`
    The ZZHC library works only when this option is enabled (Trusted Execution: Non-Secure firmware image).

The following required options are automatically selected when the ZZHC library is enabled:
    * :option:`CONFIG_BSD_LIBRARY`
    * :option:`CONFIG_SETTINGS`
    * :option:`CONFIG_AT_CMD`
    * :option:`CONFIG_AT_CMD_PARSER`
    * :option:`CONFIG_AT_NOTIF`
    * :option:`CONFIG_BASE64`
    * :option:`CONFIG_JSON_LIBRARY`


Enabling the ZZHC library
-------------------------

To enable the library and ensure that the end product can be approved by China Telecom:
    * In Kconfig, enable :option:`CONFIG_ZZHC`.
    * In ``prj.conf``, set ``CONFIG_ZZHC=y``.

Additionally, make sure to configure the following options in ``prj.conf``:
    * The minimal heap size required (2560 bytes): ``CONFIG_HEAP_MEM_POOL_SIZE=2560``.
    * The following options required by :option:`CONFIG_BSD_LIBRARY`:
        * ``CONFIG_NETWORKING=y``
        * ``CONFIG_NET_NATIVE=n``
        * ``CONFIG_NET_SOCKETS=y``
        * ``CONFIG_NET_SOCKETS_OFFLOAD=y``
    * The following options required by :option:`CONFIG_SETTINGS`:
        * ``CONFIG_FLASH=y``
        * ``CONFIG_FLASH_PAGE_LAYOUT=y``
        * ``CONFIG_FLASH_MAP=y``
        * ``CONFIG_MPU_ALLOW_FLASH_WRITE=y``
        * ``CONFIG_NVS=y``
        * ``CONFIG_SETTINGS_NVS_SECTOR_COUNT=6``


Configuring additional thread behavior
--------------------------------------

The thread behavior can be configured by using the following Kconfig options:
   * To configure the stack size for the thread, change :option:`CONFIG_ZZHC_STACK_SIZE`.
   * To configure the thread priority, change :option:`CONFIG_ZZHC_THREAD_PRIO`.


Allowing for automatic registration to Lte-M or NB-IoT on boot
--------------------------------------------------------------

To allow for automatic registration to Lte-M or NB-IoT network on boot, set the following options in ``prj.conf``:
   * ``CONFIG_LTE_LINK_CONTROL=y``
   * ``CONFIG_LTE_AUTO_INIT_AND_CONNECT=y``

