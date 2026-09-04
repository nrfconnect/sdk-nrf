.. _nrf_desktop_ble_latency:

Bluetooth LE latency module
###########################

.. contents::
   :local:
   :depth: 2

The Bluetooth® LE latency module manages Bluetooth LE connection parameters to regulate data exchange latencies and power consumption.
Use the Bluetooth LE latency module for the following purposes:

* Lower the Bluetooth LE connection latency when the :ref:`nrf_desktop_config_channel` is in use or when a firmware update is received either by the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` (low latency ensures quick data exchange).
* Request setting the initial connection parameters for a new Bluetooth connection.
* Keep the connection latency low for the LLPM (Low Latency Packet Mode) connections to improve performance.
* Handle mode change requests for HID Shorter Connection Intervals (SCI) and adjust the peripheral latency within the active SCI mode.
* Disconnect the Bluetooth Central if the connection has not been secured in the predefined amount of time after the connection occurred.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_latency_start
    :end-before: table_ble_latency_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
Make sure that both :option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL` and :option:`CONFIG_DESKTOP_BT_PERIPHERAL` options are enabled.
The Bluetooth LE latency application module is enabled by the :option:`CONFIG_DESKTOP_BLE_LATENCY_ENABLE` option.
The option is implied by :option:`CONFIG_DESKTOP_BT_PERIPHERAL` together with other features used by a HID peripheral device.

You can use the option :option:`CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S` to define the maximum allowed time for establishing the connection security.
If the connection is not secured during this period of time, the peripheral device disconnects.

You can set the option :option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` to keep the connection latency low for the LLPM connections.
The option requires :kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`.
This speeds up sending the first HID report after not sending a report for some connection intervals.
Enabling this option increases the power consumption - the connection latency is kept low unless the device is in the low power mode.

You can use the :option:`CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS` Kconfig option to enable or disable handling of the power management events, such as :c:struct:`power_down_event` and :c:struct:`wake_up_event`.
The option depends on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.
It is enabled by default when either :option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` or :option:`CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE` is selected.
Without one of these options, there is no power-management behavior for the module to apply to these events.

When the :option:`CONFIG_DESKTOP_HIDS_SCI_ENABLE` Kconfig option is enabled in the :ref:`nrf_desktop_hids`, the |ble_latency| sets the promptless :option:`CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE` Kconfig option.
With this option set, the module handles HID SCI mode change requests and adjusts connection latency using the connection rate API.
See the :ref:`nrf_desktop_hids` documentation for details about enabling HID SCI support on the peripheral.

Implementation details
**********************

The |ble_latency| uses delayed works (:c:struct:`k_work_delayable`) to control the connection latency and trigger the security timeout.

.. note::
   The module does not request an increase in the connection latency until the connection is secured.
   Increasing the slave latency can significantly increase the amount of time required to establish the Bluetooth connection security level on some hosts.

The module listens for the following events related to data transfer initiated by the connected Bluetooth central:

* ``config_event`` - This event is received when the :ref:`nrf_desktop_config_channel` is in use.
* ``ble_smp_transfer_event`` - This event is received when either the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` receives a firmware update.

When these events are received, the module sets the connection latency to low.
When the :ref:`nrf_desktop_config_channel` is no longer in use, and neither :ref:`nrf_desktop_ble_smp` nor :ref:`nrf_desktop_dfu_mcumgr` receive firmware updates (no mentioned events for ``LOW_LATENCY_CHECK_PERIOD_MS``), the module sets the connection latency to :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_LATENCY` to reduce the power consumption.

.. note::
   If the :option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` Kconfig option is enabled, the LLPM connection latency is not increased unless the device is in the low power mode.

   When the device is in the low power mode and the events related to data transfer are not received, the connection latency is set to higher value to reduce the power consumption.

The ``ble_latency`` module receives :ref:`nrf_desktop_config_channel` events, but it is not configurable with the :ref:`nrf_desktop_config_channel`.
The module does not register itself using the ``GEN_CONFIG_EVENT_HANDLERS`` macro.

.. note::
   Zephyr's :ref:`zephyr:bluetooth` API does not allow to use the LLPM connection intervals in the connection parameter update request.
   If the LLPM connection interval is in use:

   * The nRF Desktop peripheral uses a 7.5-ms interval in the request.
   * The nRF Desktop central ignores the requested connection interval, and only the connection latency is updated.

   For more detailed information, see the :ref:`nrf_desktop_ble_conn_params` documentation page.

HID Shorter Connection Intervals
==================================

When the :option:`CONFIG_DESKTOP_BLE_LATENCY_HID_SCI_ENABLE` Kconfig option is enabled, the module can handle connection latency using the connection rate API (:c:func:`bt_conn_le_conn_rate_request`) instead of the standard connection parameter update API (:c:func:`bt_conn_le_param_update`).
The connection rate API is used for Bluetooth LE peers that support HID SCI.

Connection parameter API selection
-----------------------------------

By default, the module uses the standard connection parameter update API (:c:func:`bt_conn_le_param_update`) to handle connection parameters.
Once a :c:struct:`hid_sci_mode_request_event` or a :c:struct:`ble_peer_sci_conn_rate_event` is received, the module marks the peer as SCI capable.
All the subsequent connection parameter adjustments for the peer rely on :c:func:`bt_conn_le_conn_rate_request` and :c:func:`bt_conn_le_param_update` is no longer used.

Initial connection parameters on HID SCI peripherals
----------------------------------------------------

The module uses :c:func:`bt_conn_le_param_update` to request zero peripheral latency as the initial connection parameters.
Usually, after a new connection is established, the Zephyr Bluetooth LE Host does not send this request immediately.
If the API is called during that post-connect window, the stack stores the parameters and sends the Connection Parameter Update Request only after the time (in milliseconds) set in the :kconfig:option:`CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT` Kconfig option from connect.

nRF Desktop peripherals instead use an application-level delay before calling :c:func:`bt_conn_le_param_update`.
The module schedules a delayed work item that invokes the API after the same time set in the :kconfig:option:`CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT` Kconfig option.
This keeps the update out of the Bluetooth stack until that timeout elapses, so it can be canceled if the host switches to HID SCI first.
Without this delay, the stack already holds a pending connection parameter update that would still be sent after the timeout even if the host had moved the link to HID SCI.
The peer might switch the link to HID SCI during this window.
After the link uses HID SCI, only the connection rate API might be used to adjust transport parameters.

If a HID SCI mode request through the HID Control Point characteristic or Connection Rate Update Request is received before the delayed work runs, the module cancels the scheduled initial connection parameter request and switches to the connection rate API instead.
If a HID SCI mode request arrives while the initial connection parameter update is already in progress, the requested SCI mode is made pending and applied after that update completes or fails.
As of NCS 3.4.0, no connection parameter update rejection callback was available.
As a workaround, a timeout is scheduled that treats the update as failed if a completion callback is not received within 5 seconds after the update is requested.

Module events
-------------

The module listens for the following SCI-related events:

* :c:struct:`hid_sci_mode_request_event` - Submitted by the :ref:`nrf_desktop_hids` when the connected Bluetooth host requests an SCI mode change through the HID control point characteristic.
  The Bluetooth LE latency module requests a connection rate update that matches the requested mode.
* :c:struct:`ble_peer_sci_conn_rate_event` - Submitted by the :ref:`nrf_desktop_ble_state` when a connection rate update completes or fails.
  The Bluetooth LE latency module validates the new connection rate parameters against the requested or current SCI mode.
  If the parameters are not valid for the requested nor current mode, the module attempts to find a valid SCI mode, starting from the most restrictive mode (FAST) and ending with the least restrictive mode (FULL_RANGE).
  If no valid SCI mode is found, the module defaults to the NONE mode.

When an SCI mode other than NONE is active, the module adjusts the peripheral latency within the limits of the active mode in response to the same data transfer events as in the non-SCI case (:c:struct:`config_event` and :c:struct:`ble_smp_transfer_event`).
The module skips the latency update request if the maximum latency configured for the active SCI mode is zero, because such a request would have no effect.

Power down and wake up
----------------------

When the :option:`CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS` Kconfig option is enabled, the module reacts to the power management events.

On a :c:struct:`power_down_event`, the module requests the LOW_POWER mode.
On a :c:struct:`wake_up_event`, the module restores the last SCI mode requested by the host through the HID Control Point characteristic.
If the connection is in the out-of-spec state described in :ref:`nrf_desktop_ble_latency_sci_host_updates`, the module does not change SCI mode in response to power management events.

If the host requests a mode other than LOW_POWER while the device is suspended, the module will save the requested mode but will remain in the LOW_POWER SCI mode.
On wake up, the module will restore the saved mode.

Pending SCI mode and latency updates
------------------------------------

The module allows only one connection rate update to be in flight at a time.
When a connection rate change is already in progress, further SCI-related actions are deferred instead of issuing another request immediately.

A pending state is set when:

* A HID SCI mode is requested while a connection rate update is already in progress.
* A HID SCI mode is requested while the initial connection parameter update is still in progress.
* A low-latency or high-latency adjustment requested while a connection rate update is already in progress.

In these cases, the module stores the most recently requested SCI mode and latency preference and sets the pending flag.
If a pending state was already set, the module overrides the previous HID SCI mode and latency preference with the new one.

When the in-flight connection rate update completes, the module checks whether a deferred mode change or latency change is still needed.
If so, it submits a new connection rate request that applies the stored preferences.

The pending flag is cleared after this check, even when no follow-up request is sent.
This prevents cyclic connection rate requests when a previous update failed or did not take effect.

Connection interval optimization in the LOW_POWER mode
------------------------------------------------------

During connection rate negotiation, the Bluetooth controller selects the lowest connection interval from the allowed range by default.
There is no application-level API to request a different interval within a range.
This is not optimal for the LOW_POWER mode, because it results in higher power consumption due to the use of shorter connection intervals than are actually possible with this mode.

The |ble_latency| implements a workaround to optimize the power consumption for the LOW_POWER mode.
When the module requests the LOW_POWER HID SCI mode, it first attempts to negotiate the maximum connection interval allowed for that mode.
This is done by setting both the minimum and maximum connection intervals in the connection rate request to the LOW_POWER mode maximum interval.

If the connected host rejects this request, the module performs the following operations:

* Records that the maximum LOW_POWER interval is not supported for the current connection.
  This blocks the module from trying to negotiate the maximum interval again for the current connection.
* Automatically retries the LOW_POWER mode request using the full connection interval range defined for the mode.

.. _nrf_desktop_ble_latency_sci_host_updates:

Out-of-spec host-initiated transport parameter updates
------------------------------------------------------

The HID over GATT Profile specification defines how HID SCI transport parameters must be negotiated.
According to `HID Over GATT Profile Specification`_, Section 7.5.2 (*HID Host-initiated transport parameter updates*):

   The HID Host **must** negotiate transport parameters by writing to the HID Control Point characteristic to initiate a new negotiation.

Section 7.5.1 defines the complementary device-initiated path:

   The HID Device must initiate the transport parameters update by initiating the Connection Update Request with the HID Host.

The nRF Desktop peripheral follows the device-initiated path when it adjusts peripheral latency within the active SCI mode.
It expects the connected HID host to use the HID Control Point characteristic when requesting an SCI mode change.

If the connected host updates transport parameters directly at the Link Layer (for example, by initiating a connection rate update instead of writing to the HID Control Point characteristic), the peripheral treats this as out-of-spec behavior.
When a :c:struct:`ble_peer_sci_conn_rate_event` is received without a prior HID SCI mode request, the module:

* Attempts to determine a matching SCI mode for the received parameters, starting from the current mode and then trying modes in the order of FAST, DEFAULT, LOW_POWER, and FULL_RANGE.
  If no SCI mode matches the parameters, the HID SCI mode is set to NONE.
* From this point on, the module will not attempt to update the latency until it returns to normal operation.
* The module drops any pending HID SCI mode or latency update requests.

This recovery behavior is application-specific and is not mandated by the specification.
It allows the peripheral to remain connected and usable with hosts that do not follow the Control Point negotiation procedure.
In order to return to the normal operation, the host must:

1. Update the connection rate parameters to a range allowing to support all the HID SCI modes.
2. Request an HID SCI mode change through the HID Control Point characteristic.
   As a result, the Bluetooth LE latency module requests a matching connection rate update and clears the out-of-spec host-initiated transport parameter update state if the request succeeds.
   If the current connection parameters are already valid for the requested mode, the module clears the out-of-spec state but no connection rate update is requested.

   .. note::
      If the peripheral is in suspended/power-down state, the module will save the requested mode and attempt to request LOW_POWER mode parameters.
      Only if this succeeds, the peripheral will exit the out-of-spec state.
