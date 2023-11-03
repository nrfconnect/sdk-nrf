.. _nrf_modem_softsim:

SoftSIM interface
#################

.. contents::
   :local:
   :depth: 2

SoftSIM is a form of `SIM card`_ that is implemented in software and programmed into a device.
The SoftSIM interface in the Modem library is used to transfer SIM data between the modem and the application.
When the modem makes a SoftSIM request, the SoftSIM interface handles the request and forwards it to the application.
The application then generates a response and responds to the modem through the SoftSIM interface.
The modem can be configured at runtime to use a regular SIM or SoftSIM (iSIM), or both.
For more information on how to enable this feature in the modem, refer to the `nRF91x1 AT Commands Reference Guide`_  or `nRF9160 AT Commands Reference Guide`_ depending on the SiP you are using.
The `Selecting UICC slot %CSUS`_ section in the nRF9160 AT Commands Reference Guide or the same section in the `nRF91x1 AT Commands Reference Guide`_ explains the usage of the Nordic-proprietary ``%CSUS`` command for SIM selection.

Handling SoftSIM requests
*************************

To handle requests from the SoftSIM interface, the application needs to implement a :c:type:`nrf_modem_softsim_req_handler_t` handler and set it with the :c:func:`nrf_modem_softsim_req_handler_set` function.
The :c:type:`nrf_modem_softsim_req_handler_t` handler is then called by the Modem library whenever there is a new SoftSIM request.
The :c:type:`nrf_modem_softsim_req_handler_t` handler is called in an interrupt service routine.
The application is expected to take care of the offloading of any processing appropriately.
Once the request is handled, the application calls the :c:func:`nrf_modem_softsim_res` function with the available response data.
The `cmd` and `req_id` fields must match the original request from the Modem library.
In case of failure, the application is expected to call the :c:func:`nrf_modem_softsim_err` function to signal the modem that an error has occurred.
This is called instead of the :c:func:`nrf_modem_softsim_res` function.
If a SoftSIM request from the Modem library has data associated with it, the application is expected to free the data by calling the :c:func:`nrf_modem_softsim_data_free` function, if the data is not used anymore.

.. note::
   All requests and responses follows the standard IOS/IEC 7816-3, PPS sequence, or APDU.

The following code block shows an example implementation of the API.
The `softsim_handler` function is an implementation of the :c:type:`nrf_modem_softsim_req_handler_t` handler.

.. code-block:: c

   /* Example of asynchronous procedures that process the SoftSIM requests. */
   void softsim_init_async(uint16_t req_id);
   void softsim_apdu_async(uint16_t req_id, void *data, uint16_t data_len);
   void softsim_deinit_async(uint16_t req_id);
   void softsim_reset_async(uint16_t req_id);

   /* Implementation of `nrf_modem_softsim_req_handler_t`. */
   int softsim_handler(enum nrf_modem_softsim_cmd cmd, uint16_t req_id, const uint8_t *data, uint16_t data_len)
   {
       switch (cmd)
       {
       case NRF_MODEM_SOFTSIM_INIT:
           /* Process INIT. */
           ...
           /* Defer INIT request to `softsim_init_async()` and return immediately. */
           break;
       case NRF_MODEM_SOFTSIM_APDU:
           /* Process APDU. */
           ...
           /* Defer APDU request to `softsim_apdu_async()` and return immediately. */
           break;
       case NRF_MODEM_SOFTSIM_DEINIT:
           /* Process DEINIT. */
           ...
           /* Defer DEINIT request to `softsim_deinit_async()` and return immediately. */
           break;
       case NRF_MODEM_SOFTSIM_RESET:
           /* Process RESET. */
           ...
           /* Defer RESET request to `softsim_reset_async()` and return immediately. */
           break;
       }

       return 0;
   }

   void softsim_init_async(uint16_t req_id)
   {
       void *out = NULL;
       uint16_t out_len = 0;

       /* Implementation of SoftSIM INIT. */
       err = softsim_init_impl(&out, &out_len);
       if (err) {
           nrf_modem_softsim_err(NRF_MODEM_SOFTSIM_INIT, req_id);
           return;
       }

       nrf_modem_softsim_res(NRF_MODEM_SOFTSIM_INIT, req_id, out, out_len);
   }

   void softsim_apdu_async(uint16_t req_id, void *data, uint16_t data_len)
   {
       void *out = NULL;
       uint16_t out_len = 0;

       /* Implementation of SoftSIM APDU. */
       err = softsim_apdu_impl(data, data_len, &out, &out_len);
       if (err) {
           nrf_modem_softsim_err(NRF_MODEM_SOFTSIM_APDU, req_id);
           goto clean_exit;
       }

       nrf_modem_softsim_res(NRF_MODEM_SOFTSIM_APDU, req_id, out, out_len);

    clean_exit:
       if (data) {
           nrf_modem_softsim_free(data);
       }
   }

   void softsim_deinit_async(uint16_t req_id)
   {
       /* Implementation of SoftSIM DEINIT. */
       err = softsim_deinit_impl();
       if (err) {
           nrf_modem_softsim_err(NRF_MODEM_SOFTSIM_INIT, req_id);
           return;
       }

       nrf_modem_softsim_res(NRF_MODEM_SOFTSIM_DEINIT, req_id, NULL, 0);
   }

   void softsim_reset_async(uint16_t req_id)
   {
       /* Implementation of SoftSIM RESET. */
       err = softsim_reset_impl();
       if (err) {
           nrf_modem_softsim_err(NRF_MODEM_SOFTSIM_RESET, req_id);
           return;
       }

       nrf_modem_softsim_res(NRF_MODEM_SOFTSIM_RESET, req_id, NULL, 0);
   }

   int main(void)
   {
       ...
       nrf_modem_softsim_req_handler_set(softsim_handler);
       ...
   }

.. note::
   The :c:type:`nrf_modem_softsim_req_handler_t` handler is called in an interrupt context so it is recommended to handle the requests asynchronously.

   The :c:enumerator:`NRF_MODEM_SOFTSIM_RESET` request is issued whenever the processing of a request becomes unresponsive.
   The :c:enumerator:`NRF_MODEM_SOFTSIM_RESET` request must thus be handled in an independent thread context with higher priority than the other commands to guarantee responsiveness of the application.
