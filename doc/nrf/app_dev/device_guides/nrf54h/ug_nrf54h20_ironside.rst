.. _ug_nrf54h20_ironside:

IronSide Secure Element
#######################

The IronSide Secure Element (|ISE|) is a firmware for the :ref:`Secure Domain <ug_nrf54h20_secure_domain>` of the nRF54H20 SoC that provides security features based on the :ref:`PSA Certified Security Framework <ug_psa_certified_api_overview>`.

|ISE| provides the following features:

* Global memory configuration
* Peripheral configuration (through UICR.PERIPHCONF)
* Boot commands (ERASEALL, DEBUGWAIT)
* An alternative boot path with a secondary firmware
* CPUCONF service
* Update service
* PSA Crypto service - see also :ref:`ug_crypto_architecture_implementation_standards_ironside`
* PSA Internal Trusted Storage service

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_ironside_update
   ug_nrf54h20_ironside_global_resources
   ug_nrf54h20_ironside_debug
   ug_nrf54h20_ironside_protect
   ug_nrf54h20_ironside_secure_storage
   ug_nrf54h20_ironside_boot
   ug_nrf54h20_ironside_services
