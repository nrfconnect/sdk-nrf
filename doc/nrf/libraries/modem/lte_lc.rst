.. _lte_lc_readme:

LTE link controller
###################

.. contents::
   :local:
   :depth: 2

The LTE link controller library provides functionality to control the LTE link on an nRF9160 SiP.

The LTE link can be controlled through library configurations and API calls to enable a range of features such as specifying the Access Point Name (APN), switching between LTE network modes (NB-IoT or LTE-M), enabling GNSS support and power saving features such as Power Saving Mode (PSM) and enhanced Discontinuous Reception (eDRX).

The library also provides functionality that enables the application to receive notifications regarding LTE link health parameters such as Radio Resource Control (RRC) mode, cell information, and the provided PSM or eDRX timer values.

Configuration
*************

To enable the link controller library, set the option :kconfig:option:`CONFIG_LTE_LINK_CONTROL` option to ``y`` in the project configuration file :file:`prj.conf`.

Establishing an LTE connection
==============================

The following block of code shows how the link controller API can be used to establish an LTE connection and get callbacks from the library:

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <modem/lte_lc.h>

   /* Semaphore used to block the main thread until the link controller has
    * established an LTE connection.
    */
   K_SEM_DEFINE(lte_connected, 0, 1);

   static void lte_handler(const struct lte_lc_evt *const evt)
   {
   	switch (evt->type) {
   	case LTE_LC_EVT_NW_REG_STATUS:
   		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
   		(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
   			break;
   		}

   		printk("Connected to: %s network\n",
   		evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");

   		k_sem_give(&lte_connected);
   		break;
	case LTE_LC_EVT_PSM_UPDATE:
	case LTE_LC_EVT_EDRX_UPDATE:
	case LTE_LC_EVT_RRC_UPDATE:
	case LTE_LC_EVT_CELL_UPDATE:
	case LTE_LC_EVT_LTE_MODE_UPDATE:
	case LTE_LC_EVT_TAU_PRE_WARNING:
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS:
	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		/* Callback events carrying LTE link data */
		break;
   	default:
   		break;
   	}
   }

   void main(void)
   {
   	int err;

   	printk("Connecting to LTE network. This may take a few minutes...\n");

   	err = lte_lc_init_and_connect_async(lte_handler);
   	if (err) {
   		printk("lte_lc_init_and_connect_async, error: %d\n", err);
   		return;
   	}

   	k_sem_take(&lte_connected, K_FOREVER);

   	/* Continue execution... */
   }

The code block demonstrates how the link controller can be utilized to asynchronously setup an LTE connection.
For more information on the callback events received in :c:type:`lte_lc_evt_handler_t` and data associated with each event, see the documentation on :c:struct:`lte_lc_evt`.

The following list mentions some of the information that can be extracted from the received callback events:

* Network registration status
* PSM parameters
* eDRX parameters
* RRC mode
* Cell information
* TAU prewarning notifications
* Modem sleep notifications

.. note::
   Some of the functionalities might not be compatible with certain modem firmware versions.
   To check if a desired feature is compatible with a certain modem firmware version, see nRF9160 `AT Commands Reference Guide`_.

Enabling power-saving features
==============================

PSM and eDRX power saving features can be requested at run time using the :c:func:`lte_lc_psm_req` and :c:func:`lte_lc_edrx_req` function calls.
For an example implementation, see the following code:

.. code-block:: c

   /* ... */

   void main(void)
   {
	int err;

	err = lte_lc_init();
	if (err) {
		printk("lte_lc_init, error: %d\n", err);
		return;
	}

	err = lte_lc_psm_req(true);
	if (err) {
		printk("lte_lc_psm_req, error: %d\n", err);
		return;
	}

	err = lte_lc_edrx_req(true);
	if (err) {
		printk("lte_lc_edrx_req, error: %d\n", err);
		return;
	}

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		printk("Connecting to LTE network failed, error: %d\n", err);
		return;
	}

	/* ... */
   }

The recommended way of enabling power saving features is to request the respective feature before establishing an LTE connection.
In this approach, the modem includes the requested power saving timers in the initial LTE network ``ATTACH`` instead of requesting the timer values after establishing an LTE connection.
This saves the overhead related to the additional packet exchange.

The timer values requested by the modem can be configured with the following options and API calls:

* :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU`
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT`
* :kconfig:option:`CONFIG_LTE_EDRX_REQ_VALUE_LTE_M`
* :kconfig:option:`CONFIG_LTE_EDRX_REQ_VALUE_NBIOT`
* :c:func:`lte_lc_psm_param_set`
* :c:func:`lte_lc_edrx_param_set`

To request PSM and eDRX the following APIs must be used:

* :c:func:`lte_lc_psm_req` - Request PSM
* :c:func:`lte_lc_edrx_req` - Request eDRX

.. note::
   A timer value that is requested by the modem is not necessarily given by the network.
   The event callbacks :c:enum:`LTE_LC_EVT_PSM_UPDATE` and :c:enum:`LTE_LC_EVT_EDRX_UPDATE` contain the values that are actually decided by the network.

Connection pre-evaluation
=========================

Modem firmware version 1.3.0 and higher supports connection a pre-evaluation feature that allows the application to get information about a cell that is likely to be used for an RRC connection.
Based on the parameters received in the function call, the application can decide if it needs to send application data or not.
The function :func:`lte_lc_conn_eval_params_get` populates a structure of type :c:struct:`lte_lc_conn_eval_params` that includes information on the current consumption cost by the data transmission when utilizing the given cell.
The following code block shows a basic implementation of :c:func:`lte_lc_conn_eval_params_get`:

.. code-block:: c

   ...

   void main(void)
   {
   	int err;

   	printk("Connecting to LTE network. This may take a few minutes...\n");

   	err = lte_lc_init_and_connect_async(lte_handler);
   	if (err) {
   		printk("lte_lc_init_and_connect_async, error: %d\n", err);
   		return;
   	}

   	k_sem_take(&lte_connected, K_FOREVER);

	struct lte_lc_conn_eval_params params = {0};

	err = lte_lc_conn_eval_params_get(&params);
	if (err) {
		printk("lte_lc_conn_eval_params_get, error: %d\n", err);
		return;
	}

	/* Handle connection evaluation parameters... */
   	/* Continue execution... */
   }

:c:struct:`lte_lc_conn_eval_params` lists all information that is available when performing connection pre-evaluation.

Modem sleep and TAU prewarning notifications
============================================

Modem firmware version 1.3.0 and higher supports receiving callbacks from the modem related to Tracking Area Updates (TAU) and modem sleep.
Based on these notifications, the application can alter its behavior to optimize for a given metric.
For instance, TAU pre-warning notifications can be used to schedule data transfers prior to a TAU so that data transfer and TAU occurs within the same RRC connection window, thereby saving the potential overhead associated with the additional data exchange.
Modem sleep notifications can be used to schedule processing in the same operational window as the modem to limit the overall computation time of the nRF9160 SiP.
To enable modem sleep and TAU pre-warning notifications, enable the following options:

* :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS`
* :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS`

Additional configurations related to these features can be found in the API documentation for the link controller.

Connection fallback mode
========================
It is possible to try to switch between LTE-M and NB-IoT after a certain time period if a connection has not been established.
This is useful when the connection to either of these networks becomes unavailable.
You can also configure the switching period between the network modes.
If a connection cannot be established by using the fallback mode, the library reports an error.
You can use the following configuration options to configure the connection fallback mode:

* :kconfig:option:`CONFIG_LTE_NETWORK_USE_FALLBACK`
* :kconfig:option:`CONFIG_LTE_NETWORK_TIMEOUT`

Functional mode changes callback
================================

The library allows the application to define compile-time callbacks to receive the modem's functional mode changes.
These callbacks allow any part of the application to perform certain operations when the modem enters or re-enters a certain functional mode using the library :c:func:`lte_lc_func_mode_set` API.
For example, one kind of operation that the application or a library may need to perform and repeat, whenever the modem enters a certain functional mode is the subscription to AT notifications.
The application can set up a callback for modem`s functional mode changes using :c:macro:`LTE_LC_ON_CFUN` macro.

The following code snippet shows how to use :c:macro:`LTE_LC_ON_CFUN` macro:

.. code-block:: C

  /* define callback */
  LTE_LC_ON_CFUN(cfun_hook, on_cfun, NULL);

  /* callback implementation */
  static void on_cfun(enum lte_lc_func_mode mode, void *context)
  {
      printk("Functional mode changed to %d\n", mode);
  }

  void main(void)
  {
      /* change functional mode using the Link Controller API */
      lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
  }

API documentation
*****************

| Header file: :file:`include/modem/lte_lc.h`
| Source file: :file:`lib/lte_link_control/lte_lc.c`

.. doxygengroup:: lte_lc
   :project: nrf
   :members:
