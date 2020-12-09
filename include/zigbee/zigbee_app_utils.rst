.. _lib_zigbee_application_utilities:

Zigbee application utilities
############################

.. contents::
   :local:
   :depth: 2

Zigbee application utilities library provides a set of components that are ready for use in Zigbee applications:

* :ref:`lib_zigbee_signal_handler` for handling common ZBOSS stack signals.
* API for parsing and converting Zigbee data types.
  Available functions are listed in :file:`include/zigbee/zigbee_app_utils.h`.
* :c:func:`zigbee_led_status_update` for indicating the status of the device in a network using LEDs.

.. _lib_zigbee_signal_handler:

Zigbee default signal handler
*****************************

The :ref:`nrfxlib:zboss` interacts with the user application by invoking the :c:func:`zboss_signal_handler` function whenever a stack event, such as network steering, occurs.
It is mandatory to define :c:func:`zboss_signal_handler` in the application.

Because most of Zigbee devices behave in a similar way, :c:func:`zigbee_default_signal_handler` was introduced to provide a default logic to handle stack signals.

.. note::
    |zigbee_library|

.. _zarco_signal_handler_minimal:

Minimal zboss_signal_handler implementation
===========================================

This function can be called in the application's :c:func:`zboss_signal_handler` to simplify the implementation.
In such case, this minimal implementation includes only a call to the default signal handler:


.. code-block:: c

    void zboss_signal_handler(zb_uint8_t param)
    {
        /* Call default signal handler. */
        zigbee_default_signal_handler(param);

        /* Free the Zigbee stack buffer. */
        zb_buf_free(param);
    }

With this call, the device will be able to join the Zigbee network or join a new network when it leaves the previous one.

In general, using the default signal handler is worth considering because of the following reasons:

* It simplifies the application.
* It provides a default behavior for each signal, which reduces the risk that an application will break due to an unimplemented signal handler.
* It provides Zigbee role specific behavior (for example, finite join/rejoin time)
* It makes the application less sensitive to changes in the Zigbee stack commissioning API.

The default signal handler also serves as a good starting point for a custom signal handler implementation.

.. _zarco_signal_handler_full:

Complete zboss_signal_handler implementation
============================================

In its complete implementation, the ``zboss_signal_handler`` allows the application to control a broader set of basic functionalities, including joining, commissioning, and network formation.

There are cases in which the default handler will not be sufficient and needs to be extended.
For example, when the application wants to use the procedure of the initiator of finding & binding or use the production configuration feature.

Extending zboss_signal_handler
++++++++++++++++++++++++++++++

If you want to extend ``zboss_signal_handler`` to cover additional functionalities, write signal handler implementation for the required ZBOSS signal.
For example for a device that initiates a F&B procedure, extend ``zboss_signal_handler`` with a case for ``ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED``:

.. code-block:: c

    void zboss_signal_handler(zb_bufid_t bufid)
    {
        zb_zdo_app_signal_hdr_t   *sg_p  = NULL;
        zb_zdo_app_signal_type_t  sig    = zb_get_app_signal(bufid, &sg_p);
        zb_ret_t                  status = ZB_GET_APP_SIGNAL_STATUS(bufid);
        zb_nwk_device_type_t      role   = zb_get_network_role();
        zb_bool_t                 comm_status;

        switch (sig) {
        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            /* Check status of signal. */
            if (status == RET_OK) {
                /* This signal is received with additional data. Read additional information to get status of F&B procedure. */
                zb_zdo_signal_fb_initiator_finished_params_t *fnb_params =
                    ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_fb_initiator_finished_params_t);

                switch (fnb_params->status) {
                case ZB_ZDO_FB_INITIATOR_STATUS_SUCCESS:
                    /* F&B with a Target on the Initiator side is completed with a success. */
                    break;

                case ZB_ZDO_FB_INITIATOR_STATUS_CANCEL:
                    /* F&B on the Initiator side is canceled. */
                    break;

                case ZB_ZDO_FB_INITIATOR_STATUS_ALARM:
                    /* F&B on the Initiator side is finished by timeout. */
                    break;

                case ZB_ZDO_FB_INITIATOR_STATUS_ERROR:
                    /* F&B on the Initiator side finished with a failure. */
                    break;

                default:
                    /* Unrecognised status of F&B procedure. */
                    break;
                }
            } else {
                /* F&B procedure failed. */
            }
            break;

        default:
            /* All other signals are forwarded to the zigbee default signal handler. */
            ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
            break;
        }

        if (bufid) {
            zb_buf_free(bufid);
        }
    }


Custom commissioning behavior
+++++++++++++++++++++++++++++

For the application to use a custom commissioning behavior, the default ``rejoin_procedure`` should be overwritten by writing a custom signal handler implementation for the following signals:

* ZB_BDB_SIGNAL_DEVICE_FIRST_START
* ZB_BDB_SIGNAL_DEVICE_REBOOT
* ZB_BDB_SIGNAL_STEERING
* ZB_BDB_SIGNAL_FORMATION
* ZB_ZDO_SIGNAL_LEAVE

Use the following code as reference:

.. code-block:: c

    void zboss_signal_handler(zb_bufid_t bufid)
    {
        zb_zdo_app_signal_hdr_t   *sg_p  = NULL;
        zb_zdo_app_signal_type_t  sig    = zb_get_app_signal(bufid, &sg_p);
        zb_ret_t                  status = ZB_GET_APP_SIGNAL_STATUS(bufid);
        zb_nwk_device_type_t      role   = zb_get_network_role();
        zb_bool_t                 comm_status;

        switch (sig) {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            if (status == RET_OK) {
                if (role != ZB_NWK_DEVICE_TYPE_COORDINATOR) {
                    /* If device is Router or End Device, start network steering. */
                    comm_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
                } else {
                    /* If device is Coordinator, start network formation. */
                    comm_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_FORMATION);
                }
            } else {
                /* Failed to initialize Zigbee stack. */
            }
            break;

        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            /* fall-through */
        case ZB_BDB_SIGNAL_STEERING:
            if (status == RET_OK) {
                /* Joined network successfully. */
                /* TODO: Start application-specific logic that requires the device to be connected to a Zigbee network. */
            } else {
                /* Unable to join the network. Restart network steering. */
                comm_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
                ZB_COMM_STATUS_CHECK(comm_status);
            }
            break;

        case ZB_ZDO_SIGNAL_LEAVE:
            if (status == RET_OK) {
                /* Device has left the network. */
                /* TODO: Start application-specific logic or start network steering to join a new network. */

                /* This signal comes with additional data in which type of leave is stored. */
                zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);

                switch (leave_params->leave_type) {
                case ZB_NWK_LEAVE_TYPE_RESET:
                    /* Device left network without rejoining. */
                    break;

                case ZB_NWK_LEAVE_TYPE_REJOIN:
                    /* Device left network with rejoin. */
                    break;

                default:
                    /* Unrecognised leave type. */
                    break;
                }
            } else {
                /* Device was unable to leave network. */
            }
            break;

        case ZB_BDB_SIGNAL_FORMATION:
            if (status == RET_OK) {
                /* Network formed successfully, start network steering. */
                comm_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            } else {
                /* Network formation failed, restart. */
                ret_code = ZB_SCHEDULE_APP_ALARM((zb_callback_t)bdb_start_top_level_commissioning, ZB_BDB_NETWORK_FORMATION, ZB_TIME_ONE_SECOND);
            }
            break;

        default:
            /* Call default signal handler. */
            ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
            break;
        }

        if (bufid) {
            zb_buf_free(bufid);
        }
    }


.. _zarco_signal_handler_startup:

Behavior on stack start
=======================

When the stack is started through :c:func:`zigbee_enable`, the stack generates the following signals:

* ``ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY`` -- indicating that the stack attempted to load application-specific production configuration from flash memory.
* ``ZB_ZDO_SIGNAL_SKIP_STARTUP`` -- indicating that the stack has initialized all internal structures and the Zigbee scheduler has started.

The reception of these signals determines the behavior of the default signal handler:

* Upon reception of ``ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY``, the default signal handler will print out a log with the signal status and exit.

.. figure:: /images/zigbee_signal_handler_01_production_config.png
   :alt: ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY signal handler

   ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY signal handler

* Upon reception of ``ZB_ZDO_SIGNAL_SKIP_STARTUP`` signal, the default signal handler will perform the BDB initialization procedure, and then exit.

.. figure:: /images/zigbee_signal_handler_02_startup.png
   :alt: ZB_ZDO_SIGNAL_SKIP_STARTUP signal handler

   ZB_ZDO_SIGNAL_SKIP_STARTUP signal handler

.. note::
    If you want to perform some actions before the stack attempts to join or rejoin the Zigbee network, you can overwrite this behavior by providing a custom ``ZB_ZDO_SIGNAL_SKIP_STARTUP`` signal handler implementation.

.. _zarco_signal_handler_bdb_initialization:

Zigbee Base Device Behavior initialization
==========================================

Once the BDB initialization procedure is finished, depending on the data stored inside the Zigbee persistent storage, the stack will complete one of the following scenarios:

* New devices - Generate the ``ZB_BDB_SIGNAL_DEVICE_FIRST_START`` signal for factory new devices.
* Commissioned devices - Perform a single attempt to rejoin the Zigbee network based on NVRAM contents and then generate the ``ZB_BDB_SIGNAL_DEVICE_REBOOT`` signal.

Both scenarios will cause different behavior of the the default signal handler.

.. _zarco_signal_handler_bdb_initialization_new_devices:

New device scenario
+++++++++++++++++++

For factory new devices, the default signal handler will:

* Start the BDB network formation on coordinator devices.
  Once finished, the stack will generate ``ZB_BDB_SIGNAL_FORMATION`` signal, and continue to :ref:`zarco_signal_handler_network`.
* Call :c:func:`start_network_rejoin` to start the :ref:`zarco_network_rejoin` on routers and end devices.
  Once the procedure is started, the device tries to join the network until cancellation.
  Each try takes place after a longer period of waiting time, for a total maximum of 15 minutes.
  Devices may behave differently because the implementation of :c:func:`start_network_rejoin` is different for different Zigbee roles.
  See :ref:`zarco_network_rejoin` for more information.

Once handling of the signal is finished, the stack will generate the ``ZB_BDB_SIGNAL_STEERING`` signal, and will continue to :ref:`zarco_signal_handler_network`.

.. figure:: /images/zigbee_signal_handler_03_first_start.png
   :alt: Scenario for factory new devices (ZB_BDB_SIGNAL_DEVICE_FIRST_START)

   Scenario for factory new devices (ZB_BDB_SIGNAL_DEVICE_FIRST_START)

.. _zarco_signal_handler_bdb_initialization_commissioned:

Commissioned device scenario
++++++++++++++++++++++++++++

For devices that have been already commissioned, the default handler will:

* Not perform additional actions if the device implements a coordinator role.

    * This will keep the network closed for new Zigbee devices even if the coordinator is reset.

* Not perform additional actions if the device successfully rejoins Zigbee network.

    * This will not open the network for new devices if one of existing devices is reset.
    * In case of the :ref:`zarco_network_rejoin` is running, it will be cancelled.

* For routers and end devices, if they did not join the Zigbee network successfully, :ref:`zarco_network_rejoin` is started by calling :c:func:`start_network_rejoin`.

Once finished, the stack will generate the ``ZB_BDB_SIGNAL_STEERING`` signal, and continue to :ref:`zarco_signal_handler_network`.

.. figure:: /images/zigbee_signal_handler_04_reboot.png
   :alt: Scenario for already commissioned devices (ZB_BDB_SIGNAL_DEVICE_REBOOT)

   Scenario for already commissioned devices (ZB_BDB_SIGNAL_DEVICE_REBOOT)

.. _zarco_signal_handler_network:

Zigbee network formation and commissioning
++++++++++++++++++++++++++++++++++++++++++

According to the logic implemented inside the default signal handler, the devices can either form a network or join an existing network:

1. Coordinators will first form a network.
   Attempts to form the network will continue infinitely, with a one-second delay between each attempt.

   .. figure:: /images/zigbee_signal_handler_05_formation.png
      :alt: Forming a network following the generation of ZB_BDB_SIGNAL_FORMATION

      Forming a network following the generation of ZB_BDB_SIGNAL_FORMATION

   By default, after the successful network formation on the coordinator node, a single-permit join period of 180 seconds will be started, which will allow new Zigbee devices to join the network.
#. Other devices will then join an existing network during this join period.

    * When a device has joined and :ref:`zarco_network_rejoin` is running, the procedure is cancelled.
    * If no device has joined and the procedure is not running, the procedure will be started.

   .. figure:: /images/zigbee_signal_handler_06_steering.png
      :alt: Forming a network following the generation of ZB_BDB_SIGNAL_STEERING

      Forming a network following the generation of ZB_BDB_SIGNAL_STEERING

.. _zarco_signal_handler_leave:

Zigbee network leaving
======================

The default signal handler implements the same behavior for handling ``ZB_ZDO_SIGNAL_LEAVE`` for both routers and end devices.
When leaving the network, the default handler calls :c:func:`start_network_rejoin` to start :ref:`zarco_network_rejoin` to join a new network.

Once :c:func:`start_network_rejoin` is called, the stack will generate the ``ZB_BDB_SIGNAL_STEERING`` signal and will continue to :ref:`zarco_signal_handler_network`.

.. figure:: /images/zigbee_signal_handler_09_leave.png
   :alt: Leaving the network following ZB_ZDO_SIGNAL_LEAVE

   Leaving the network following ZB_ZDO_SIGNAL_LEAVE

.. _zarco_network_rejoin:

Zigbee network rejoining
========================

The Zigee network rejoin procedure is a mechanism that is similar to the ZDO rejoin back-off procedure.
It is implemented to work with both routers and end devices and simplify handling of cases such as device joining, rejoining, or leaving the network.
It is used in :c:func:`default_signal_handler` by default.

If the network is left by a router or an end device, the device will try to join any open network.

* The router will use the default signal handler to try to join or rejoin the network until it succeeds.
* The end device will use the default signal handler to try to join or rejoin the network for a finite period of time, because the end devices are often powered by batteries.

  * The procedure to join or rejoin the network is restarted after the device reset or power cycle.
  * The procedure to join or rejoin the network can be restarted by calling :c:func:`user_input_indicate`, but it needs to be implemented in the application (for example, by calling :c:func:`user_input_indicate` when a button is pressed).
    The procedure will be restarted only if the device does not join and the procedure is not running.

The Zigbee rejoin procedure retries to join a network with each try after a specified amount of time: ``2^n`` seconds, where ``n`` is the number of retries.

The period is limited to 15 minutes if the result is higher than that.

* When :c:func:`start_network_rejoin` is called, the rejoin procedure is started, and depending on the device role:

  * For the end device, the application alarm is scheduled with ``stop_network_rejoin(ZB_TRUE)``, to be called after the amount of time specified in ``ZB_DEV_REJOIN_TIMEOUT_MS``.
    Once called, the alarm stops the rejoin.

* When ``stop_network_rejoin(was_scheduled)`` is called, the network rejoin is canceled and the alarms scheduled by :c:func:`start_network_rejoin` are canceled.

  * Additionally for the end device, if :c:func:`stop_network_rejoin` is called with ``was_scheduled`` set to ``ZB_TRUE``, :c:func:`user_input_indicate` can restart the rejoin procedure.

* For end devices only, :c:func:`user_input_indicate` restarts the rejoin procedure if the device did not join the network and is not trying to join a network.
  It is safe to call this function from an interrupt and to call it multiple times.


.. note::
    The Zigbee network rejoin procedure is managed from multiple signals in :c:func:`default_signal_handler`.
    If the application controls the network joining, rejoining, or leaving, each signal in which the Zigbee network rejoin procedure is managed should be handled in the application.
    In this case, :c:func:`user_input_indicate` must not be called.

.. _zarco_sleep:

Zigbee stack sleep routines
===========================

For all device types, the Zigbee stack informs the application about periods of inactivity by generating a ``ZB_COMMON_SIGNAL_CAN_SLEEP`` signal.

The minimal inactivity duration that causes the signal to be generated is defined by ``sleep_threshold``.
By default, the inactivity duration equals approximately 15 ms.
The value can be modified by the ``zb_sleep_set_threshold`` API.

.. figure:: /images/zigbee_signal_handler_07_idle.png
   :alt: Generation of the ZB_COMMON_SIGNAL_CAN_SLEEP signal

   Generation of the ZB_COMMON_SIGNAL_CAN_SLEEP signal

The signal can be used to suspend the Zigbee task for the inactivity period.
This allows the Zephyr kernel to switch to other tasks with lower priority.
Additionally, it allows to implement a Zigbee Sleepy End Device.
For more information about the power optimization of the Zigbee stack, see :ref:`zigbee_ug_sed`.

The inactivity signal can be handled using the Zigbee default signal handler.
If so, it will allow the Zigbee stack to enter the sleep state and suspend the Zigbee task by calling :c:func:`zigbee_event_poll` function.

If the default behavior is not applicable for the application, you can customize the sleep functionality by overwriting the :c:func:`zb_osif_sleep` weak function and implementing a custom logic for handling the stack sleep state.

.. figure:: /images/zigbee_signal_handler_08_deep_sleep.png
   :alt: Implementing a custom logic for putting the stack into the sleep mode

   Implementing a custom logic for putting the stack into the sleep mode

.. _lib_zigbee_application_utilities_options:

Configuration
*************

To enable the Zigbee application utilities library, set the :option:`CONFIG_ZIGBEE_APP_UTILS` Kconfig option.

To configure the logging level of the library, use the :option:`CONFIG_ZIGBEE_APP_UTILS_LOG_LEVEL` Kconfig option.

For detailed steps about configuring the library in a Zigbee sample or application, see :ref:`ug_zigbee_configuring_components_application_utilities`.

API documentation
*****************

| Header file: :file:`include/zigbee/zigbee_app_utils.h`
| Source file: :file:`subsys/zigbee/lib/zigbee_app_utils/zigbee_app_utils.c`

.. doxygengroup:: zigbee_app_utils
   :project: nrf
   :members:
