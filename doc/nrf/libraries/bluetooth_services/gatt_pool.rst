.. _gatt_pool_readme:

Bluetooth GATT attribute pools
##############################

.. contents::
   :local:
   :depth: 2

You can use GATT attribute pools to dynamically create a service description that can later be registered in the Zephyr GATT database.

Every GATT service is essentially a set of GATT attributes that are ordered in a particular way.
There are different types of attributes that can be registered to create a service:

* Primary Service attribute, containing service metadata
* Characteristic attribute, containing characteristic metadata
* CCC descriptor attribute, used for turning notifications for a characteristic on or off
* Regular descriptor attribute, used for providing an additional description for a characteristic

The API of the GATT attribute pools module allows to register different types of GATT attributes listed above.
After each registration, a part of the memory is reserved for each attribute.
You can also unregister unnecessary attributes using the module's API.
In this case, the previously reserved memory is released.
This can be useful when you want to restructure your service by using the Service Changed feature that is supported by the Zephyr Bluetooth® stack (see, for example, the :ref:`hids_readme`).

Additionally, you can adjust the memory footprint of this module to your needs by changing the configuration options for the size of the module's memory pool.
If you are unsure about the proper values, print the module's statistics to see how the pool utilization level is affected by the chosen configuration.

API documentation
*****************

| Header file: :file:`include/bluetooth/gatt_pool.h`
| Source file: :file:`subsys/bluetooth/gatt_pool.c`

.. doxygengroup:: bt_gatt_pool
   :project: nrf
   :members:
