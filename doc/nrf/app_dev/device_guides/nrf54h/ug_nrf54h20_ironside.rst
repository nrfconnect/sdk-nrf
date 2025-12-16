.. _ug_nrf54h20_ironside:

IronSide Secure Element
#######################

The IronSide Secure Element (|ISE|) is a firmware for the :ref:`Secure Domain <ug_nrf54h20_secure_domain>` of the nRF54H20 SoC that provides security features based on the :ref:`PSA Certified Security Framework <ug_psa_certified_api_overview>`.

|ISE| provides the following features:

* :ref:`Global memory configuration <ug_nrf54h20_ironside_se_uicr>`
* :ref:`Peripheral configuration <ug_nrf54h20_ironside_se_periphconf_devicetree>`
* Boot commands

  * :ref:`ERASEALL <ug_nrf54h20_ironside_se_eraseall_command>`
  * :ref:`DEBUGWAIT <ug_nrf54h20_ironside_se_debugwait_command>`
* An alternative boot path with a :ref:`secondary firmware <ug_nrf54h20_ironside_se_secondary_firmware>`
* :ref:`CPUCONF service <ug_nrf54h20_ironside_se_cpuconf_service>`
* :ref:`Update service <ug_nrf54h20_ironside_se_update_service>`
* PSA Crypto service (:ref:`ug_crypto_architecture_implementation_standards_ironside`)
* PSA Internal Trusted Storage service (:ref:`ug_nrf54h20_ironside_se_secure_storage`)

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
