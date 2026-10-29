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

.. note::
   By default, the library enables only the core features related to the network connectivity.

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
                   /* Handle LTE events that are enabled by default. */
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

Connection Parameters Evaluation:
  Use the :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL_MODULE` Kconfig option to enable the following functionality related to Connection Parameters Evaluation:

  * :c:func:`lte_lc_conn_eval_params_get`

eDRX (Extended Discontinuous Reception):
  Use the :kconfig:option:`CONFIG_LTE_LC_EDRX_MODULE` Kconfig option to enable all the following functionalities related to eDRX:

  * :c:enumerator:`LTE_LC_EVT_EDRX_UPDATE` events
  * :c:func:`lte_lc_ptw_set`
  * :c:func:`lte_lc_edrx_param_set`
  * :c:func:`lte_lc_edrx_req`
  * :c:func:`lte_lc_edrx_get`
  * :kconfig:option:`CONFIG_LTE_EDRX_REQ`

Neighboring Cell Measurements:
  Use the :kconfig:option:`CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE` Kconfig option to enable all the following functionalities related to Neighboring Cell Measurements:

  * :c:enumerator:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS` events
  * :c:func:`lte_lc_neighbor_cell_measurement_cancel`
  * :c:func:`lte_lc_neighbor_cell_measurement`

Periodic Search Configuration:
  Use the :kconfig:option:`CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` Kconfig option to enable all the following functionalities related to Periodic Search Configuration:

  * :c:func:`lte_lc_periodic_search_request`
  * :c:func:`lte_lc_periodic_search_clear`
  * :c:func:`lte_lc_periodic_search_get`
  * :c:func:`lte_lc_periodic_search_set`

  A periodic search configuration overrides the default search pattern of the data profile.

Data Profile:
  Use the :kconfig:option:`CONFIG_LTE_LC_DATA_PROFILE_MODULE` Kconfig option to enable all the following functionalities related to the data profile:

  * :c:func:`lte_lc_data_profile_set`
  * :c:func:`lte_lc_data_profile_get`
  * :kconfig:option:`CONFIG_LTE_DATA_PROFILE_INIT`, to set a data profile when the modem is initialized

  The data profile tells the modem about the application use case, so that the modem can optimize its power consumption.
  It controls how often the modem searches for a cell and when it deactivates the UICC during eDRX.
  The profiles are listed in :c:enum:`lte_lc_data_profile`.
  The normal profile is the default data profile.

  The modem deactivates the UICC during eDRX to save power.
  It activates the UICC again on every eDRX wake-up before it sets up any signaling or data connection.
  With the ultra-low power profile, the modem deactivates the UICC if the eDRX cycle is longer than 60 seconds and the UICC allows it.
  With the other profiles, the modem deactivates the UICC if the eDRX cycle is longer than 5 minutes.
  On the nRF9160, this requires modem firmware v1.3.2 or later.
  On the nRF91x1, the ``AT%UICCPOWERSAVE`` command can overrule these conditions.

  Each profile has a default periodic search pattern.
  The sleep times of the defaults are the same as in the following ``AT%PERIODICSEARCHCONF`` commands:

  .. list-table::
     :header-rows: 1

     * - Data profile
       - Command
     * - Ultra-low power
       - ``AT%PERIODICSEARCHCONF=0,0,0,1,"0,10,40,,5","1,300,600,1800,1800,3600"``
     * - Low power
       - ``AT%PERIODICSEARCHCONF=0,0,0,1,"0,10,40,,5","1,300,600"``
     * - Normal
       - ``AT%PERIODICSEARCHCONF=0,0,0,1,"0,10,40,5,15","1,60,90,300"``
     * - Performance
       - ``AT%PERIODICSEARCHCONF=0,0,0,1,"0,10,40,5,15","1,60,90"``
     * - High performance
       - ``AT%PERIODICSEARCHCONF=0,0,0,1,"1,10","0,10,40,,15"``

  The commands use the default band optimization of the modem, which also depends on the data profile.

  If you set a periodic search configuration using the :c:func:`lte_lc_periodic_search_set` function, it replaces the default pattern of the current data profile.
  For the meaning of the parameters, see the `nRF91x1 AT Commands Reference Guide`_ or the `nRF9160 AT Commands Reference Guide`_, depending on the SiP you are using.

  The modem stores the data profile to non-volatile memory about every 48 hours and when the functional mode is set to power off.
  Consider UICC non-volatile memory wear when you select the profile.
  When the library sets the profile at initialization, it reads the current profile first and writes the new profile only if it differs.

PSM (Power Saving Mode):
  Use the :kconfig:option:`CONFIG_LTE_LC_PSM_MODULE` Kconfig option to enable all the following functionalities related to PSM:

  * :c:enumerator:`LTE_LC_EVT_PSM_UPDATE` events
  * :c:func:`lte_lc_psm_param_set`
  * :c:func:`lte_lc_psm_param_set_seconds`
  * :c:func:`lte_lc_psm_req`
  * :c:func:`lte_lc_psm_get`
  * :c:func:`lte_lc_proprietary_psm_req`
  * :kconfig:option:`CONFIG_LTE_PSM_REQ`

Release Assistance Indication (RAI):
  Use the :kconfig:option:`CONFIG_LTE_LC_RAI_MODULE` Kconfig option to enable the following functionalities related to RAI:

  * :c:enumerator:`LTE_LC_EVT_RAI_UPDATE` events
  * :kconfig:option:`CONFIG_LTE_RAI_REQ`

Modem Sleep Notifications:
  Use the :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_MODULE` Kconfig option to enable all the following functionalities related to Modem Sleep Notifications:

  * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING` events
  * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_ENTER` events
  * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT` events
  * :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS`

Tracking Area Update (TAU) Pre-warning:
  Use the :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE` Kconfig option to enable the following functionalities related to TAU Pre-warning:

  * :c:enumerator:`LTE_LC_EVT_TAU_PRE_WARNING` events
  * :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS`

DNS Fallback:
  The :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` Kconfig option controls the use of a fallback DNS server address.

  The device might or might not receive a DNS server address by the network during a PDN connection.
  Even within the same network, the PDN connection establishment method (PCO vs ePCO) might change when the device operates in NB-IoT or LTE Cat-M1, and result in missing DNS server addresses when one method is used, but not the other.
  Setting a fallback DNS address ensures that the device always has a DNS server address to fallback to regardless of whether the network has provided one.

  The :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` Kconfig option is enabled by default.
  If the application has configured a DNS server address in Zephyr's native networking stack using the :kconfig:option:`CONFIG_DNS_SERVER1` Kconfig option, the same server is set as the fallback address for DNS queries offloaded to the nRF91 Series modem.
  Otherwise, the :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig option controls the fallback DNS server address that is set to Cloudflare's DNS server: 1.1.1.1 by default.

Environment Evaluation:
  Use the :kconfig:option:`CONFIG_LTE_LC_ENV_EVAL_MODULE` Kconfig option to enable the following functionalities related to Environment Evaluation:

  * :c:enumerator:`LTE_LC_EVT_ENV_EVAL_RESULT` events
  * :c:func:`lte_lc_env_eval`
  * :c:func:`lte_lc_env_eval_cancel`

PDN (Packet Data Network):
  Use the :kconfig:option:`CONFIG_LTE_LC_PDN_MODULE` Kconfig option to enable all the following functionalities related to PDP context and PDN connection management:

  * :c:enumerator:`LTE_LC_EVT_PDN_ACTIVATED` events
  * :c:enumerator:`LTE_LC_EVT_PDN_DEACTIVATED` events
  * :c:enumerator:`LTE_LC_EVT_PDN_IPV6_UP` events
  * :c:enumerator:`LTE_LC_EVT_PDN_IPV6_DOWN` events
  * :c:enumerator:`LTE_LC_EVT_PDN_NETWORK_DETACH` events
  * :c:enumerator:`LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON` events
  * :c:enumerator:`LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF` events
  * :c:enumerator:`LTE_LC_EVT_PDN_CTX_DESTROYED` events
  * :c:enumerator:`LTE_LC_EVT_PDN_ESM_ERROR` events
  * :c:func:`lte_lc_pdn_ctx_create`
  * :c:func:`lte_lc_pdn_ctx_configure`
  * :c:func:`lte_lc_pdn_ctx_auth_set`
  * :c:func:`lte_lc_pdn_activate`
  * :c:func:`lte_lc_pdn_deactivate`
  * :c:func:`lte_lc_pdn_ctx_destroy`
  * :c:func:`lte_lc_pdn_id_get`
  * :c:func:`lte_lc_pdn_dynamic_info_get`
  * :c:func:`lte_lc_pdn_default_ctx_apn_get`
  * :c:func:`lte_lc_pdn_esm_strerror`
  * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE`

UICC state:
  Use the :kconfig:option:`CONFIG_LTE_LC_UICC_STATE_MODULE` Kconfig option to enable UICC (SIM card) state notifications, reported in the :c:enumerator:`LTE_LC_EVT_UICC_STATE` event.

For more information on the callback events received in :c:type:`lte_lc_evt_handler_t` and data associated with each event, see the documentation on :c:struct:`lte_lc_evt`.
For more information on the functions and data associated with each, refer to the API documentation.

.. note::
   Some of the functionalities might not be compatible with certain modem firmware versions.
   To check if a desired feature is compatible with a certain modem firmware version, see the AT commands that are documented in the `nRF91x1 AT Commands Reference Guide`_  or `nRF9160 AT Commands Reference Guide`_ depending on the SiP you are using.

.. _lte_lc_power_saving:

Enabling power-saving features
==============================

To enable power-saving features, use the following options:

* :kconfig:option:`CONFIG_LTE_LC_PSM_MODULE`
* :kconfig:option:`CONFIG_LTE_LC_EDRX_MODULE`
* :kconfig:option:`CONFIG_LTE_PSM_REQ`
* :kconfig:option:`CONFIG_LTE_EDRX_REQ`

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

Modem firmware versions 1.3.0 and higher support a connection pre-evaluation feature that allows the application to get information about a cell that is likely to be used for an RRC connection.
Based on the parameters received in the function call, the application can decide whether to send application data or not.
To enable this module, use the :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL_MODULE` Kconfig option.
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

Modem firmware versions 1.3.0 and higher support receiving callbacks from the modem related to Tracking Area Updates (TAU) and modem sleep.
Based on these notifications, the application can alter its behavior to optimize for a given metric.

For instance, TAU pre-warning notifications can be used to schedule data transfers before a TAU so that data transfer and TAU occurs within the same RRC connection window, thereby saving the potential overhead associated with the additional data exchange.

Modem sleep notifications can be used to schedule processing in the same operational window as the modem to limit the overall computation time of the nRF91 Series SiP.

To enable modem sleep and TAU pre-warning notifications, use the following options:

* :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_MODULE`
* :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE`
* :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS`
* :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS`

For additional configurations related to these features, see the API documentation.

Environment evaluation
======================

Modem firmware mfw_nrf91x1 v2.0.3 and higher, and mfw_nrf9151-ntn support environment evaluation.
Environment evaluation allows the application to evaluate available PLMNs and select the best PLMN to use before connecting to the network.
This is useful especially in cases where the device has multiple SIMs or SIM profiles to select from.

Environment evaluation can only be performed in *receive only* functional mode.
During the environment evaluation, the device searches for the best cell for each PLMN.

The :c:func:`lte_lc_env_eval` function starts the environment evaluation for the given PLMNs.
When the environment evaluation is complete, an :c:enumerator:`LTE_LC_EVT_ENV_EVAL_RESULT` event with the evaluation results is received.
For each found PLMN, the :c:struct:`lte_lc_conn_eval_params` structure is populated with the evaluation results.

.. _lte_lc_band_lock:

Band lock
=========

Band lock restricts the modem to a subset of the LTE bands it supports.
The modem supports two band locks:

* The permanent band lock (:c:enumerator:`LTE_LC_BAND_LOCK_PERMANENT`), which is stored in the modem and kept when the modem is powered off.
* The runtime band lock (:c:enumerator:`LTE_LC_BAND_LOCK_RUNTIME`), which is lost when the modem is powered off.

When both band locks are set, the modem only uses the bands that are in both.
The second band lock must have at least one band in common with the first one, otherwise the modem rejects it.

To set the band lock at build time, set the :kconfig:option:`CONFIG_LTE_LOCK_BANDS` Kconfig option to ``y`` and configure the bands using the :kconfig:option:`CONFIG_LTE_LOCK_BAND_MASK` and :kconfig:option:`CONFIG_LTE_LOCK_BAND_LIST` Kconfig options.
The configured bands are set as the runtime band lock.
See :ref:`nrf91_ug_band_lock` for more information.

To change the band lock at runtime, set the :kconfig:option:`CONFIG_LTE_LC_BAND_LOCK_MODULE` Kconfig option to ``y``, and use the following functions:

* :c:func:`lte_lc_band_lock_set` - Sets the permanent or runtime band lock.
  Bands up to 88 are set using a band mask and bands above 88 using a band list, which is not supported by all modem firmware versions.
* :c:func:`lte_lc_band_lock_remove` - Removes both the permanent and the runtime band lock.

The band lock can only be changed when the modem library is initialized and the modem is offline or powered off.
The library stores the runtime band lock and sets it again when the modem is initialized or has been powered off.
A runtime band lock set using the :c:func:`lte_lc_band_lock_set` function replaces the bands configured using the Kconfig options.
After the :c:func:`lte_lc_band_lock_remove` function is called, the bands configured using the Kconfig options are not set again until the device is rebooted.

.. _lte_lc_pdn:

PDN management
==============

The LTE link control library provides functionality to manage Packet Data Protocol (PDP) contexts and Packet Data Network (PDN) connections.

The PDN functionality provides the following capabilities:

* Creating and configuring PDP contexts
* Receiving events pertaining to PDN connections
* Managing PDN connections
* Retrieving PDN connection information

To enable PDN functionality, set the :kconfig:option:`CONFIG_LTE_LC_PDN_MODULE` Kconfig option to ``y``.

AT commands used
----------------

The PDN functionality uses several AT commands, and it relies on the following two types of AT notifications to work:

* Packet domain events notifications (``+CGEV``) - Subscribed by using the ``AT+CGEREP=1`` command.
  See the `AT+CGEREP set command`_ section in the nRF9160 AT Commands Reference Guide or the `nRF91x1 AT+CGEREP set command`_  section in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.
* Notifications for unsolicited reporting of error codes sent by the network (``+CNEC``) - Subscribed by using the ``AT+CNEC=16`` command.
  See the `AT+CNEC set command`_ section in the nRF9160 AT Commands Reference Guide or the `nRF91x1 AT+CNEC set command`_  section in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

The LTE link control library automatically subscribes to the necessary AT notifications.
This includes automatically resubscribing to the notifications upon functional mode changes.

.. note::
   The subscription to AT notifications is lost upon changing the functional mode of the modem to ``+CFUN=0``.
   If the application subscribes to these notifications manually, it must also take care of resubscription.

Following are the AT commands that are used by the PDN functionality:

* ``AT%XNEWCID`` - To create a PDP context
* ``AT+CGDCONT`` - To configure or destroy a PDP context
* ``AT+CGACT`` - To activate or deactivate a PDN connection
* ``AT%XGETPDNID`` - To retrieve the PDN ID for a given PDP context
* ``AT+CGAUTH`` - To set the PDN connection authentication parameters

For more information about these commands, see `Packet Domain AT commands`_ in the nRF9160 AT Commands Reference Guide or the `nRF91x1 packet Domain AT commands`_ section in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

Creating and managing PDN connections
--------------------------------------

The application can create PDP contexts by using the :c:func:`lte_lc_pdn_ctx_create` function.
PDN events are sent to all registered LTE link control event handlers.
The application can use the :c:func:`lte_lc_register_handler` function to register an event handler for events pertaining to all PDP contexts, including the default PDP context.

A PDN connection is identified by an ID as reported by ``AT%XGETPDNID``, and it is distinct from the PDP context ID (CID).
The modem creates a PDN connection for a PDP context as necessary.
Multiple PDP contexts might share the same PDN connection if they are configured similarly.

The application can use the following functions to manage PDP contexts and PDN connections:

* :c:func:`lte_lc_pdn_ctx_create` - Creates a PDP context.
* :c:func:`lte_lc_register_handler` - Registers an event handler for events pertaining to all PDP contexts, including the default PDP context.
* :c:func:`lte_lc_pdn_ctx_configure` - Configures a PDP context with address family, access point name, and optional additional configuration options.
* :c:func:`lte_lc_pdn_ctx_auth_set` - Sets authentication parameters for a PDP context.
* :c:func:`lte_lc_pdn_activate` - Activates a PDN connection for a PDP context.
* :c:func:`lte_lc_pdn_deactivate` - Deactivates a PDN connection.
* :c:func:`lte_lc_pdn_ctx_destroy` - Destroys a PDP context.
  The PDN connection must be inactive when the PDP context is destroyed.
* :c:func:`lte_lc_pdn_id_get` - Retrieves the PDN ID for a given PDP context.
  The PDN ID can be used to route traffic through a specific PDN connection.
* :c:func:`lte_lc_pdn_dynamic_info_get` - Retrieves dynamic parameters such as DNS addresses and MTU sizes for a given PDN connection.
* :c:func:`lte_lc_pdn_default_ctx_apn_get` - Retrieves the default Access Point Name (APN) of the default PDP context (CID 0).

PDN events
----------

The following PDN events are sent to registered LTE link control event handlers:

* :c:enumerator:`LTE_LC_EVT_PDN_ACTIVATED` - PDN connection activated
* :c:enumerator:`LTE_LC_EVT_PDN_DEACTIVATED` - PDN connection deactivated
* :c:enumerator:`LTE_LC_EVT_PDN_IPV6_UP` - PDN has IPv6 connectivity
* :c:enumerator:`LTE_LC_EVT_PDN_IPV6_DOWN` - PDN has lost IPv6 connectivity
* :c:enumerator:`LTE_LC_EVT_PDN_NETWORK_DETACH` - PDN connection lost due to network detach
* :c:enumerator:`LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON` - APN rate control is active
* :c:enumerator:`LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF` - APN rate control is inactive
* :c:enumerator:`LTE_LC_EVT_PDN_CTX_DESTROYED` - PDP context destroyed
* :c:enumerator:`LTE_LC_EVT_PDN_ESM_ERROR` - ESM error occurred

The associated payload for PDN events is the :c:member:`lte_lc_evt.pdn` member of type :c:struct:`lte_lc_pdn_evt`.
This structure contains the PDP context ID (CID) and, for :c:enumerator:`LTE_LC_EVT_PDN_ESM_ERROR` events, the ESM error code.

PDN configuration
-----------------

The LTE link control library can override the default PDP context configuration automatically after the :ref:`nrfxlib:nrf_modem` is initialized, if the :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE` Kconfig option is set.
The default PDP context configuration consists of the following parameters, each controlled with a Kconfig setting:

* Access point name - :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_APN`
* Address family - :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_IPV4`, :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_IPV6`, :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_IPV4V6`, or :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_NONIP`
* Authentication method - :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_AUTH_NONE`, :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_AUTH_PAP`, or :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_AUTH_CHAP`
* Authentication credentials - :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_USERNAME` and :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_PASSWORD`

.. note::
   The default PDP context configuration must be overridden before the device is registered with the network.

Additional configuration options:

* :kconfig:option:`CONFIG_LTE_LC_PDN_LEGACY_PCO` - Use legacy PCO mode instead of ePCO during PDN connection establishment
* :kconfig:option:`CONFIG_LTE_LC_PDN_ESM_TIMEOUT` - Timeout for waiting for an ESM notification when activating a PDN
* :kconfig:option:`CONFIG_LTE_LC_PDN_ESM_STRERROR` - Compile a table with textual descriptions of ESM error reasons

PDN usage example
-----------------

The following code snippet demonstrates how to use the PDN functionality:

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <modem/lte_lc.h>

   static void lte_handler(const struct lte_lc_evt *const evt)
   {
           switch (evt->type) {
           case LTE_LC_EVT_PDN_ACTIVATED:
                   printk("PDN context %d activated\n", evt->pdn.cid);
                   break;
           case LTE_LC_EVT_PDN_DEACTIVATED:
                   printk("PDN context %d deactivated\n", evt->pdn.cid);
                   break;
           case LTE_LC_EVT_PDN_ESM_ERROR:
                   printk("PDN context %d ESM error: %d\n",
                          evt->pdn.cid, evt->pdn.esm_err);
                   break;
           default:
                   break;
           }
   }

   int main(void)
   {
           int err;
           uint8_t cid;
           int esm;
           char apn[32];
           int pdn_id;

           err = lte_lc_register_handler(lte_handler);
           if (err) {
                   LOG_ERR("Registration failed, error: %d", err);

                   return err;
           }

           err = lte_lc_connect();
           if (err) {
                   return err;
           }

           /* Get default APN */
           err = lte_lc_pdn_default_ctx_apn_get(apn, sizeof(apn));
           if (err) {
                   return err;
           }

           /* Create a new PDP context */
           err = lte_lc_pdn_ctx_create(&cid);
           if (err) {
                   return err;
           }

           /* Configure the PDP context */
           err = lte_lc_pdn_ctx_configure(cid, apn, LTE_LC_PDN_FAM_IPV4V6, NULL);
           if (err) {
                   return err;
           }

           /* Activate the PDN connection */
           err = lte_lc_pdn_activate(cid, &esm, NULL);
           if (err) {
                   LOG_ERR("Activation failed, ESM error: %d", esm);

                   return err;
           }

           /* Get PDN ID */
           pdn_id = lte_lc_pdn_id_get(cid);
           if (pdn_id < 0) {
                   return pdn_id;
           }

           LOG_INF("PDN connection activated with PDN ID: %d", pdn_id);

           /* Continue execution... */
   }

.. note::
   You have to register the event handler for LTE events before the device is registered to the network (``CFUN=1``) to receive the first activation event.

Limitations
***********

The LTE link control library and the :ref:`nrfxlib:nrf_modem_at` interface should be used at the same time with caution,
because the library also uses the same interface.
As a general rule, do not use the AT commands for features that the LTE link control library supports.

Dependencies
************

This library uses the following |NCS| library:

* :ref:`nrfxlib:nrf_modem`

API documentation
*****************

| Header file: :file:`include/modem/lte_lc.h`
| Source file: :file:`lib/lte_link_control/lte_lc.c`

.. doxygengroup:: lte_lc
