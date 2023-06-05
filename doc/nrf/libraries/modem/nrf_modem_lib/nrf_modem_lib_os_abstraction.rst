.. _nrf_modem_lib_os_abstraction:

OS abstraction layer
####################

The Modem library requires the implementation of an OS abstraction layer, which is an interface over the operating system functionalities such as interrupt setup, threads, and heap.
The integration layer provides an implementation of the OS abstraction layer using |NCS| components.
The OS abstraction layer is implemented in :file:`nrfxlib/nrf_modem/include/nrf_modem_os.c`.

The behavior of the functions in the OS abstraction layer is dependent on the |NCS| components that are used in their implementation.
This is relevant for functions such as :c:func:`nrf_modem_os_shm_tx_alloc`, which uses :ref:`Zephyr's Heap implementation <zephyr:heap_v2>` to dynamically allocate memory.
In this case, the characteristics of the allocations made by these functions depend on the heap implementation by Zephyr.
