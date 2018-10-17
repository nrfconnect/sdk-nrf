.. _svc_common_readme:

Common service utilities
########################

This module can be used to dynamically create a service description which can be
later registered in the Zephyr GATT Database. Each service is essentially a set
of GATT attributes that are ordered in a particular way. There are different
types of attributes that can be registered in order to create a service:

* Primary Service attribute, containing service metadata
* Characteristic attribute, containing characteristic metadata
* CCC descriptor attribute, used for turning notifications on/off for a
  characteristic
* regular descriptor attribute, used for providing an additional description for
  a characteristic

The module's API allows to register different types of GATT attributes listed
above. After each registration, a part of the memory is reserved for each
attribute. You can also unregister attributes that are no longer needed by using
the module's API. In this case, the previously reserved memory will be released.
This can be useful when you want to restructure your service by using Service
Changed feature which is supported by the Zephyr Bluetooth stack (see
:ref:`_hids_readme`).

Additionally, you can adjust the memory footprint of this module to your needs by
changing the configuration options for the size of the module's memory pool. If
you are unsure about the proper values, you can print the module's statistics to
see how the pool utilization level is affected by the chosen configuration.

API documentation
*****************

.. doxygengroup:: nrf_bt_svc_common
   :project: nrf
