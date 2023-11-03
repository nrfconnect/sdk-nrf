.. _nrf_modem_lib_socket:

Socket offloading
#################

Zephyr Socket API offers the :ref:`socket offloading functionality <zephyr:net_socket_offloading>` to redirect or *offload* function calls to BSD socket APIs such as ``socket()`` and ``send()``.
The integration layer utilizes this functionality to offload the socket API calls to the Modem library and thus eases the task of porting the networking code to the nRF9160 by providing a wrapper for Modem library's native socket API such as :c:func:`nrf_socket` and :c:func:`nrf_send`.

The socket offloading functionality in the integration layer is implemented in :file:`nrf/lib/nrf_modem_lib/nrf91_sockets.c`.

Modem library socket API sets errnos as defined in the :file:`nrf_errno.h` file.
The socket offloading support in the integration layer in |NCS| converts those errnos to the errnos that adhere to the selected C library implementation.

The socket offloading functionality is enabled by default.
To disable the functionality, disable the :kconfig:option:`CONFIG_NET_SOCKETS_OFFLOAD` Kconfig option in your project configuration.
If you disable the socket offloading functionality, the socket calls will no longer be offloaded to the nRF9160 modem firmware.
Instead, the calls will be relayed to the native Zephyr TCP/IP implementation.
This can be useful to switch between an emulator and a real device while running networking code on these devices.
Even if the socket offloading is disabled, Modem library's own socket APIs such as :c:func:`nrf_socket` and :c:func:`nrf_send` remain available.
