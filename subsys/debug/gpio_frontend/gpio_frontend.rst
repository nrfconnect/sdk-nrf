.. _gpio_frontend:

GPIO frontend
#############

The GPIO frontend for logger allows immediate transfer of string addresses and arguments using GPIO pins.
Host PC can then obtain proper strings from ELF file and format the message properly.
This allows for lower latency, memory usage and load, higher throughput.

Note that as this is a logging frontend, the logs are dispatched directly to GPIO. Logger core and
backends are not used. See also :ref:`zephyr:logger`.

Requirements
************

GPIO frontend is currently supported on following boards:

** nrf52_pca10040
** nrf52840_pca10056
** nrf9160_pca10090

9 consecutive GPIO pins are necessary for communication.

For more information, refer to :ref:`gpio_frontend_sample`.

.. doxygengroup:: gpio_frontend
   :project: nrf
   :members:
