.. _lib_zzhc:

China Telecom ZZHC library
##########################

.. contents::
   :local:
   :depth: 2

The China Telecom ZZHC library implements the self-registration (Zi-ZHu-Ce, ZZHC) functionality that registers the module information on the server when the network is available.

Overview
********

The telecommunication modules that are part of the China Telecom network are required to provide certain information stored on the module to the ZZHC server (``zzhc.vnet.cn``).
This is required for conformity with Chapter 6 of the China Telecom IoT Module Requirements Whitepaper.

The ZZHC library is started as a background thread on boot and it automatically detects the availability of the operator network.
Once the Internet connectivity is ready and a new SIM card is detected on boot, the library proactively uploads the following information to the ZZHC server:

* IMEI
* model number (for example, nRF9160)
* modem revision
* nRF91 Series SIM ICCID
* nRF91 Series SIM IMSI

On any failure, which includes an unknown or negative response from the server or any error during the program execution, the ZZHC library retries to upload the information to the server once every hour, for a maximum of five attempts.

.. _lib_zzhc_configuration:

Configuration
*************

By default, the ZZHC library is not used in any sample.

Enabling the ZZHC library
-------------------------

To enable the ZZHC library, edit the :file:`prj.conf` file according to the following steps:

1. Set :kconfig:option:`CONFIG_ZZHC` to ``y``.
   This option enables the ZZHC library and ensures that China Telecom can approve the end product.
   The following required options are automatically selected when the ZZHC library is enabled:

   * :kconfig:option:`CONFIG_NRF_MODEM_LIB`
   * :kconfig:option:`CONFIG_SETTINGS`
   * :kconfig:option:`CONFIG_AT_CMD_PARSER`
   * :kconfig:option:`CONFIG_BASE64`
   * :kconfig:option:`CONFIG_JSON_LIBRARY`

#. Set :kconfig:option:`CONFIG_TRUSTED_EXECUTION_NONSECURE` to ``y``.
   This option enables the Trusted Execution: Non-Secure firmware image.
   The ZZHC library only works when this option is enabled.
#. Set :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` to the minimum heap size required (2560 bytes): ``CONFIG_HEAP_MEM_POOL_SIZE=2560``.
#. Set the following options required by :kconfig:option:`CONFIG_NRF_MODEM_LIB`:

   * ``CONFIG_NETWORKING=y``
   * ``CONFIG_NET_NATIVE=n``
   * ``CONFIG_NET_SOCKETS=y``
   * ``CONFIG_NET_SOCKETS_OFFLOAD=y``

#. Set the following options required by :kconfig:option:`CONFIG_SETTINGS`:

   * ``CONFIG_FLASH=y``
   * ``CONFIG_FLASH_PAGE_LAYOUT=y``
   * ``CONFIG_FLASH_MAP=y``
   * ``CONFIG_MPU_ALLOW_FLASH_WRITE=y``
   * ``CONFIG_NVS=y``
   * ``CONFIG_SETTINGS_NVS_SECTOR_COUNT=6``

Configuring additional thread behavior
--------------------------------------

You can configure the thread behavior using the following Kconfig options:

* To adjust the stack size for the thread, change :kconfig:option:`CONFIG_ZZHC_STACK_SIZE`.
* To adjust the thread priority, change :kconfig:option:`CONFIG_ZZHC_THREAD_PRIO`.
