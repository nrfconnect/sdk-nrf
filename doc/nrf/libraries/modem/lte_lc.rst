.. _lte_lc_readme:

LTE link control
################

.. contents::
   :local:
   :depth: 2

The LTE link control library provides functionality to control the LTE link on an nRF91 Series SiP.

The LTE link can be controlled through library configurations and API calls to enable a range of features such as specifying the Access Point Name (APN), switching between LTE network modes (NB-IoT or LTE-M), enabling GNSS support and power saving features such as Power Saving Mode (PSM) and enhanced Discontinuous Reception (eDRX).

The library also provides functionality that enables the application to receive notifications regarding LTE link parameters such as Radio Resource Control (RRC) connection state, cell information, and the provided PSM or eDRX timer values.

To use the LTE link control library, the :ref:`nrfxlib:nrf_modem` is required.

Configuration
*************

To enable the library, set the Kconfig option :kconfig:option:`CONFIG_LTE_LINK_CONTROL` to ``y`` in the project configuration file :file:`prj.conf`.

Establishing an LTE connection
==============================

The following block of code shows how you can use the API to establish an LTE connection and get callbacks from the library:

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <modem/lte_lc.h>

   /* Semaphore used to block the main thread until modem has established
    * an LTE connection.
    */
   K_SEM_DEFINE(lte_connected, 0, 1);

   static void lte_handler(const struct lte_lc_evt *const evt)
   {
           switch (evt->type) {
           case LTE_LC_EVT_NW_REG_STATUS:
                   if (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME &&
                       evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING) {
                           break;
                   }

                   printk("Connected to: %s network\n",
                          evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");

                   k_sem_give(&lte_connected);
                   break;

           case LTE_LC_EVT_RRC_UPDATE:
           case LTE_LC_EVT_CELL_UPDATE:
           case LTE_LC_EVT_LTE_MODE_UPDATE:
           case LTE_LC_EVT_MODEM_EVENT:
                   /* Other events that are enabled by default. */
                   break;

           default:
                   break;
           }
   }

   int main(void)
   {
           int err;

           printk("Connecting to LTE network. This may take a few minutes...\n");

           err = lte_lc_connect_async(lte_handler);
           if (err) {
                   printk("lte_lc_connect_async, error: %d\n", err);
                   return 0;
           }

           k_sem_take(&lte_connected, K_FOREVER);

           /* Continue execution... */
   }

The code block demonstrates how you can use the library to asynchronously set up an LTE connection.

Additionally, to enable specific functionalities and receive specific events from the library, you must enable the corresponding modules through their respective Kconfig options:

* Connection Parameters Evaluation:

  Enable the :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL` Kconfig option to enable all the functionalities related to Connection Parameters Evaluation such as:

  * :c:func:`lte_lc_conn_eval_params_get`

* eDRX (Extended Discontinuous Reception):

  Enable the :kconfig:option:`CONFIG_LTE_LC_EDRX` Kconfig option to enable all the functionalities related to eDRX such as:

  * :c:enumerator:`LTE_LC_EVT_EDRX_UPDATE` events
  * :c:func:`lte_lc_ptw_set`
  * :c:func:`lte_lc_edrx_param_set`
  * :c:func:`lte_lc_edrx_req`
  * :c:func:`lte_lc_edrx_get`

* Neighboring Cell Measurements:

  Enable the :kconfig:option:`CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS` Kconfig option to enable all the functionalities related to Neighboring Cell Measurements such as:

  * :c:enumerator:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS` events
  * :c:func:`lte_lc_neighbor_cell_measurement_cancel`
  * :c:func:`lte_lc_neighbor_cell_measurement`

* Periodic Search Configuration:

  Enable the :kconfig:option:`CONFIG_LTE_LC_PERIODIC_SEARCH` Kconfig option to enable all the functionalities related to Periodic Search Configuration such as:

  * :c:func:`lte_lc_periodic_search_request`
  * :c:func:`lte_lc_periodic_search_clear`
  * :c:func:`lte_lc_periodic_search_get`
  * :c:func:`lte_lc_periodic_search_set`

* PSM (Power Saving Mode):

  Enable the :kconfig:option:`CONFIG_LTE_LC_PSM` Kconfig option to enable all the functionalities related to PSM such as:

  * :c:enumerator:`LTE_LC_EVT_PSM_UPDATE` events
  * :c:func:`lte_lc_psm_param_set`
  * :c:func:`lte_lc_psm_param_set_seconds`
  * :c:func:`lte_lc_psm_req`
  * :c:func:`lte_lc_psm_get`
  * :c:func:`lte_lc_proprietary_psm_req`

* Release Assistance Indication (RAI):

  Enable the :kconfig:option:`CONFIG_LTE_LC_RAI` Kconfig option to enable all the functionalities related to RAI such as:

  * :c:enumerator:`LTE_LC_EVT_RAI_UPDATE` events

* Modem Sleep Notifications:

  Enable the :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP` Kconfig option to enable all the functionalities related to Modem Sleep Notifications such as:

  * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING` events
  * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_ENTER` events
  * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT` events

* Tracking Area Update (TAU) Pre-warning:

  Enable the :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING` Kconfig option to enable all the functionalities related to TAU Pre-warning such as:

  * :c:enumerator:`LTE_LC_EVT_TAU_PRE_WARNING` events

For more information on the callback events received in :c:type:`lte_lc_evt_handler_t` and data associated with each event, see the documentation on :c:struct:`lte_lc_evt`.
For more information on the functions and data associated with each, refer to the API documentation.

.. note::
   Some of the functionalities might not be compatible with certain modem firmware versions.
   To check if a desired feature is compatible with a certain modem firmware version, see the AT commands that are documented in the `nRF91x1 AT Commands Reference Guide`_  or `nRF9160 AT Commands Reference Guide`_ depending on the SiP you are using.

.. _lte_lc_power_saving:

Enabling power-saving features
==============================

To enable power-saving features, enable the following options:

* :kconfig:option:`CONFIG_LTE_LC_PSM`
* :kconfig:option:`CONFIG_LTE_LC_EDRX`
* :kconfig:option:`CONFIG_LTE_LC_PSM_REQ`
* :kconfig:option:`CONFIG_LTE_LC_EDRX_REQ`

PSM and eDRX can also be requested at run time using the :c:func:`lte_lc_psm_req` and :c:func:`lte_lc_edrx_req` function calls.
However, calling the functions during modem initialization can lead to conflicts with the value set by the Kconfig options.

You can set the timer values requested by the modem using the following options:

* :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU`
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT`
* :kconfig:option:`CONFIG_LTE_EDRX_REQ_VALUE_LTE_M`
* :kconfig:option:`CONFIG_LTE_EDRX_REQ_VALUE_NBIOT`
* :kconfig:option:`CONFIG_LTE_PTW_VALUE_LTE_M`
* :kconfig:option:`CONFIG_LTE_PTW_VALUE_NBIOT`

.. note::
   A timer value that is requested by the modem is not necessarily given by the network.
   The event callbacks :c:enum:`LTE_LC_EVT_PSM_UPDATE` and :c:enum:`LTE_LC_EVT_EDRX_UPDATE` contain the values that are actually decided by the network.

Connection pre-evaluation
=========================

Modem firmware version 1.3.0 and higher supports connection a pre-evaluation feature that allows the application to get information about a cell that is likely to be used for an RRC connection.
Based on the parameters received in the function call, the application can decide whether to send application data or not.
To enable this module, enable the :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL`.
The function :c:func:`lte_lc_conn_eval_params_get` populates a structure of type :c:struct:`lte_lc_conn_eval_params` that includes information on the current consumption cost by the data transmission when utilizing the given cell.
The following code snippet shows a basic implementation of :c:func:`lte_lc_conn_eval_params_get`:

.. code-block:: c

   ...

   int main(void)
   {
           int err;

           printk("Connecting to LTE network. This may take a few minutes...\n");

           err = lte_lc_connect_async(lte_handler);
           if (err) {
                   printk("lte_lc_connect_async, error: %d\n", err);
                   return 0;
           }

           k_sem_take(&lte_connected, K_FOREVER);

           struct lte_lc_conn_eval_params params = {0};

           err = lte_lc_conn_eval_params_get(&params);
           if (err) {
                   printk("lte_lc_conn_eval_params_get, error: %d\n", err);
                   return 0;
           }

           /* Handle connection evaluation parameters... */

           /* Continue execution... */
   }

The :c:struct:`lte_lc_conn_eval_params` structure lists all information that is available when performing connection pre-evaluation.

Modem sleep and TAU pre-warning notifications
=============================================

Modem firmware v1.3.0 and higher supports receiving callbacks from the modem related to Tracking Area Updates (TAU) and modem sleep.
Based on these notifications, the application can alter its behavior to optimize for a given metric.

For instance, TAU pre-warning notifications can be used to schedule data transfers before a TAU so that data transfer and TAU occurs within the same RRC connection window, thereby saving the potential overhead associated with the additional data exchange.

Modem sleep notifications can be used to schedule processing in the same operational window as the modem to limit the overall computation time of the nRF91 Series SiP.

To enable modem sleep and TAU pre-warning notifications, enable the following options:

* :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP`
* :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING`
* :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS`
* :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS`

For additional configurations related to these features, see the API documentation.

Functional mode changes callback
================================

The library allows the application to define compile-time callbacks to receive the modem's functional mode changes.
These callbacks allow any part of the application to perform certain operations when the modem enters or re-enters a certain functional mode using the library :c:func:`lte_lc_func_mode_set` API.
For example, one kind of operation that the application or a library may need to perform and repeat, whenever the modem enters a certain functional mode is the subscription to AT notifications.
The application can set up a callback for modem`s functional mode changes using the :c:macro:`LTE_LC_ON_CFUN` macro.

.. important::
   When the :c:macro:`LTE_LC_ON_CFUN` macro is used, the application must not call :c:func:`nrf_modem_at_cfun_handler_set` as that will override the handler set by the modem library integration layer.

.. note::
   The CFUN callback is not supported with :c:func:`nrf_modem_at_cmd_async`.

The following code snippet shows how to use the :c:macro:`LTE_LC_ON_CFUN` macro:

.. code-block:: c

   /* Define a callback */
   LTE_LC_ON_CFUN(cfun_hook, on_cfun, NULL);

   /* Callback implementation */
   static void on_cfun(enum lte_lc_func_mode mode, void *context)
   {
           printk("Functional mode changed to %d\n", mode);
   }

   int main(void)
   {
           /* Change functional mode using the LTE link control API */
           lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
           return 0;
   }

Dependencies
************

This library uses the following |NCS| library:

* :ref:`nrfxlib:nrf_modem`

API documentation
*****************

| Header file: :file:`include/modem/lte_lc.h`
| Source file: :file:`lib/lte_link_control/lte_lc.c`

.. doxygengroup:: lte_lc
