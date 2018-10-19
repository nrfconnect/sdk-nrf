.. _gatt_db_discovery_readme:

GATT Database Discovery
#######################

The GATT Database Discovery Module handles service discovery on GATT servers. When a client connects to a peer device that has a desired server, service discovery is necessary to ensure that the client interacts with the servers characteristics using the correct attributes handles. Service discovery is also important because Bluetooth LE advertising does not mandate that all services are advertised. To actually know if a service is present on a peer device, you can perform a service discovery.

The GATT Database Discovery Module is used in the :ref:`bluetooth_central_hids` sample.

Limitations
***********
* The discovery procedure can be target at the chosen service - group type service discovery is not supported.
* One discovery procedure can be running at the same time.

API documentation
*****************

.. doxygengroup:: nrf_bt_gatt_db_discovery
   :project: nrf
