.. _supported_features:

Supported features
##################

The Modem library supports the following features:

* AT commands (:file:`nrf_modem_at.h`)
* Networking sockets (:file:`nrf_socket.h`)

  * TCP (client/server)
  * UDP
  * RAW
  * DTLS 1.2 client
  * TLS 1.2 client

* Global Navigation Satellite System (GNSS) (:file:`nrf_modem_gnss.h`)

  * Assisted GPS (A-GPS)

* Modem firmware updates (delta updates) (:file:`nrf_modem_delta_dfu.h`)
* Modem bootloader (full firmware updates) (:file:`nrf_modem_bootloader.h`)
* Modem Fault handling
* Modem tracing (:file:`nrf_modem_trace.h`)
* Logging
