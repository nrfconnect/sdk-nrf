.. _rd_feature_description:

Driver features
###############

.. contents::
   :local:
   :depth: 2

This page describes the nRF IEEE 802.15.4 radio driver features.

.. _features_description_frames_filtering:

Receiving and filtering frames
******************************

The radio driver can receive unicast 802.15.4 frames and broadcast 802.15.4 frames on channels 11-26 from channel page 0.
The driver performs the filtering procedure (as per IEEE 802.15.4-2006: 7.5.6.2) during the frame reception, dividing it as follows:

1. When the FCF is received (first ``BCMATCH`` event), the driver checks if the length of the frame is valid, and it verifies the frame type and version.
#. When the destination address fields (PAN ID and address) are present and received (second ``BCMATCH`` event), the driver checks if the frame is destined to this node (broadcast or unicast).
#. When the entire frame is received (``END`` event), the driver verifies if the FCS field contains a valid value.

If all checks are passed, the driver passes the received frame to the MAC layer.

.. note::
   Steps 1 and 2 can be bypassed in :ref:`features_description_promiscuous_mode`.

A received frame includes a timestamp captured when the last symbol of the frame is received.
The timestamp can be used to support synchronous communication like CSL or TSCH.

.. _features_description_automatic_sending_ack:

Sending automatically ACK frames
********************************

The MAC layer can configure the driver to automatically send ACK frames.
This feature is enabled by default.

This automatically created ACK frame is compliant with IEEE 802.15.4-2006: 7.2.2.3 or IEEE 802.15.4-2015: 6.7.2 and 6.7.4.2.
This frame is sent exactly 192 us (``aTurnaroundTime``) after a data frame is received.
The ACK frame is sent only if the received frame passes all the steps of :ref:`the filter <features_description_frames_filtering>`, even in promiscuous mode, and if the ACK request bit is present in the FCF of the received frame.

The automatic ACK procedure uses a dedicated TIMER peripheral to keep valid inter-frame spacing, regardless of the ISR latency.
The driver sets the PPI and TIMER registers to end the transmitter ramp up exactly 192 us (``aTurnaroundTime``) after the frame is received.
If any of the filter steps fails, the driver does not set PPI to start the transmitter ramp up when TIMER fires.
If all filter steps pass, the driver checks the version of the received frame and prepares either an immediate acknowledgment frame (Imm-Ack) or an enhanced acknowledgment frame (Enh-Ack):

* In the *Imm-Ack* frame, the driver sets the correct sequence number in the ACK frame, prepares PPIs to ramp up the transmitter, and optionally performs the :ref:`automatic pending bit procedure <features_description_setting_pending_bit>`.
* In the *Enh-Ack* frame, the driver fills all the required fields (including the :ref:`automatic pending bit procedure <features_description_setting_pending_bit>`) and optionally adds the last configured information elements to the acknowledgment frame.
  The ACK frame is sent automatically by TIMER, RADIO shorts, and PPIs.

.. _features_description_setting_pending_bit:

Setting pending bit in ACK frames
*********************************

The MAC layer can configure the driver to automatically set a pending bit in the automatically generated ACK frames.
This feature is enabled by default.

The driver handles the pending bit as follows, depending on the protocol used:

* Blacklist mode (Zigbee):

  * If the frame being acknowledged is not a data poll command frame, the pending bit is cleared (``0``).
  * If the driver matches the source address with an entry in the array, the pending bit is cleared (``0``).
  * If the array does not contain an address matching the source address and the frame being acknowledged is a data poll command frame, the pending bit is set (``1``).
* Thread mode:

  * If the driver matches the source address with an entry in the array, the pending bit is set (``1``).
  * If the array does not contain an address matching the source address, the pending bit is cleared (``0``).

The driver does not transmit the ACK frame if it cannot prepare the ACK frame before the transmission is ready to begin.

.. _features_description_transmission:

Transmitting unicast and broadcast frames
*****************************************

The radio driver allows the MAC layer to transmit frames containing any PSDU.
The RADIO peripheral updates automatically the FCS field of every frame.

The CCA procedure can precede a transmission.
The driver automatically receives an ACK frame if requested.

.. _features_description_cca:

Performing an automatic CCA procedure before transmission
*********************************************************

If the MAC layer requests the driver to perform CCA before transmission, the driver performs it, depending on the activity of the channel:

* If the channel is busy, the driver notifies the MAC layer and ends the transmission procedure.
* If the channel is idle, the driver starts the transmission immediately after the CCA procedure ends.

.. _features_description_receiving_ack:

Receiving automatically ACK frames
**********************************

If the FCF of the frame requested for transmission has the ACK request bit cleared, the driver ends the transmission procedure and notifies the MAC layer right after the RADIO peripheral ends the transmission of the frame.

If the FCF of the frame has the ACK request bit set, the driver waits for the ACK frame.
The wait can be interrupted by the following events:

* The driver receives the expected ACK frame.
  In this case, the driver resets the receiver, enters receive mode, and notifies the MAC layer that the transmission succeeded.
* The driver receives a frame different from the expected ACK.
  In this case, the driver resets the receiver, enters receive mode, and notifies the MAC layer that the transmission failed.
* The ACK timer expires.
  In this case, the driver resets the receiver, enters receive mode, and notifies the MAC layer that the transmission failed.
* Another radio operation requested by the driver terminates the wait for ACK.
  Such operation can be requested by a higher layer using a public API call, or internally by a scheduled operation like :ref:`delayed TX or delayed RX <features_description_delayed_ops>`.
  If the wait for ACK is terminated, the driver notifies the MAC layer that the transmission was terminated.

.. _features_description_standalone_cca:

Performing a stand-alone CCA procedure
**************************************

The driver can perform a stand-alone CCA procedure.

The driver notifies the MAC layer about the result of the CCA procedure through the :c:func:`cca_done` call.
After the CCA procedure ends, the driver enters receive mode.

.. _features_description_low_power:

Entering low-power mode
***********************

The MAC layer can request the driver to enter low-power mode (sleep).

In this mode, the RADIO peripheral cannot receive or transmit any frames, but power consumption is minimal.

.. _features_description_energy_detection:

Performing energy detection
***************************

The driver can perform an energy detection procedure for the time given by the MAC layer.
This returns the maximal energy level detected during the procedure.
The time given by the MAC layer is rounded up to a multiple of 128 us.

.. note::
   The energy detection procedure in a multiprotocol configuration may take longer than the requested time.
   Energy detection is interrupted by any radio activity from other protocols, but the total time of energy-detection periods is greater or equal to the time requested by the MAC layer.

.. _features_description_promiscuous_mode:

Running in promiscuous mode
***************************

While in promiscuous mode, the driver reports to the MAC layer the received frames that meet one of the following requirements:

* Pass all the steps listed in the :ref:`Receiving and filtering frames <features_description_frames_filtering>` section.
* Fail step 1 or 2 of the abovementioned steps.

If any step of the filter fails, the driver does not :ref:`automatically transmit an ACK frame <features_description_receiving_ack>` in response to the received frame.

.. _features_description_cc_transmission:

Transmitting a continuous unmodulated carrier wave
**************************************************

The driver can send a continuous unmodulated carrier wave on a selected channel.

The continuous carrier transmission forces CCA (ED mode) to report a busy channel on nearby devices.
To stop the continuous carrier transmission, the MAC layer must request the driver to enter either receive or sleep mode.

The continuous carrier wave is transmitted when the RADIO peripheral is in TXIDLE mode.

.. note::
   * This mode is intended for device testing and must not be used in an end-user application.

.. _features_description_mc_transmission:

Transmitting a continuous modulated carrier wave
************************************************

The driver can send a continuous modulated carrier wave on a selected channel.
The wave is modulated with the payload given by the MAC layer.
SHR, PHR, and FCS are applied to the payload.
The FCS of the previous frame is transmitted back-to-back with the SHR of the next frame.

The :ref:`continuous carrier transmission <features_description_cc_transmission>` forces CCA (ED mode) to report a busy channel on nearby devices.
To stop a continuous carrier transmission, the MAC layer must request the driver to enter receive mode.

The modulated carrier is transmitted when the RADIO peripheral is in TX mode with the PHYEND_START short enabled.

.. note::
   * This mode is intended for device testing and must not be used in an end-user application.

.. _features_description_csma:

Performing CSMA-CA
******************

The driver can perform the CSMA-CA procedure followed by the frame transmission.

The MAC layer must call :c:func:`csma_ca` to initiate this procedure.
The end of the procedure is notified by either the :c:func:`tx_started` or the :c:func:`transmit_failed` functions.
The driver :ref:`receives ACK frames <features_description_receiving_ack>` like after any other transmission procedure.

.. note::
   Using this feature requires the proprietary 802.15.4 Service Layer.

.. _features_description_delayed_ops:

Performing delayed operations
*****************************

The driver can transmit or receive a frame at a specific requested time.
This provides support for synchronous communication and can be used by a higher layer to support features like CSL, TSCH, or Zigbee GP Proxy.

The radio driver can also schedule up to one delayed transmission or two delayed receptions for a given moment in time.
In this scenario, the driver does not verify if the scheduled delayed operations do overlap but, still, it can execute only a single operation at a time.
If a new delayed operation is scheduled to be executed while a previous one is still ongoing, the driver prematurely aborts the previous operation.

.. note::
   This feature requires the support for scheduling radio operations in the 802.15.4 Service Layer.

.. _features_description_delayed_rx:

Performing delayed reception
============================

The driver can perform a delayed reception, entering RECEIVE mode for a given time period.

When the driver detects the start of a frame at the end of the reception window, it automatically extends the window to be able to receive the whole frame and transmit the acknowledgment.
It then notifies the end of the window to the MAC layer with the ``rx_failed`` (RX_TIMEOUT) notification.

At the end of the reception window, the driver does not automatically transit to SLEEP mode.
Instead, the MAC layer must request the transition to the required state and, optionally, request the next delayed-reception operation.

To distinguish notifications issued by different delayed-reception windows, the higher layer must also provide a unique identifier when requesting a window.
The driver passes that identifier to the notifications as a parameter.

.. _features_description_ie_csl_phase_injection:

Injecting the CSL Phase Information Element
*******************************************

The driver can update the Coordinated Sampled Listening (CSL) phase in a transmitted frame at the moment of the frame transmission, by performing a CSL phase injection, for both data frames and enhanced ACK frames.

The driver calculates the injected CSL phase value from the moment it ended the transmission of the PHY header (PHR) to the middle of the first pending delayed-reception window.
If there are no pending delayed-reception windows, or the frame does not contain a CSL Information Element (IE), the driver does not perform any action, and it does not modify the frame.
The higher layer must call :c:func:`nrf_802154_csl_writer_period_set` when it knows the period to be used, to let the driver set correctly the ``CSL Period`` field.
The driver stores the provided value and uses it to fill the ``Period`` field in the transmissions that follow.

As such, the higher layer must prepare a properly formatted frame and the enhanced ACK data, containing the placeholder values for the following fields in the CSL Information Element:

* The ``Period`` field
* The ``CSL Phase`` field

To set the enhanced ACK data containing the CSL Information Element, the higher layer must call the :c:func:`nrf_802154_ack_data_set` function.

.. note::
   This feature requires the support for scheduling radio operations in the 802.15.4 Service Layer.

.. _features_description_frame_security:

Supporting IEEE 802.15.4 frame security
***************************************

The driver can perform the following security-related transformations on the outgoing frames and Enh-Acks:

* Frame counter injection
* Payload encryption and authentication

You can secure outgoing frames using the following API calls:

* :c:func:`security_global_frame_counter_set`
* :c:func:`security_key_store`
* :c:func:`security_key_remove`

To use them, you must enable the following driver config options:

* :c:macro:`NRF_802154_SECURITY_WRITER_ENABLED`
* :c:macro:`NRF_802154_ENCRYPTION_ENABLED`

When you enable the support to frame security, the driver parses each of the outgoing frames to check for the presence of the auxiliary security header.
If the driver finds the header, it encrypts and authenticates the frame using the key specified by the key identifier field.
The upper layer must fill all the necessary auxiliary security header fields, except the frame counter one.
The driver populates the frame counter field before the frame is transmitted.

If the frame security level requires a Message Integrity Code, the upper layer must leave a placeholder between the payload and the MAC footer to let the driver write the Message Integrity Code.
The placeholder for the Message Integrity Code can be 4, 8, or 16 bytes long, depending on the security level.
The driver does not interpret the placeholder and will overwrite it after it calculates the Message Integrity Code.
If the upper layer fails to leave a placeholder of the correct length, the resulting frame will have a corrupted encrypted payload.

If the key identifier and key mode do not match any key entry added using :c:func:`security_key_store`, or if the frame counter overflows, the frame transmission will not occur and the driver will notify about the transmission failure using the :c:func:`transmit_failed` function.

.. _features_description_thread_link_metrics:

Injecting Thread Link Metrics IEs
*********************************

The driver can inject Thread Link Metrics Information Elements into Enh-Acks.

The driver supports the following metrics:

* The LQI of the last received frame
* The RSSI of the last received frame
* The link margin, or the RSSI above the sensitivity threshold, of the last received frame

To enable the automatic injection of link metrics, the upper layer must prepare Thread-Link-Metrics Information Elements that are properly formatted with the appropriate tokens in place of the RSSI, LQI, and (or) link margin, and set them using :c:func:`ack_data_for_addr_set`.
The injector module recognizes the following tokens, defined in :file:`nrf_802154_const.h`, and replaces them with the proper values when the Enh-Ack is generated:

* :c:macro:`IE_VENDOR_THREAD_RSSI_TOKEN`
* :c:macro:`IE_VENDOR_THREAD_MARGIN_TOKEN`
* :c:macro:`IE_VENDOR_THREAD_LQI_TOKEN`

Performing retransmissions
**************************

The driver can modify the content of a frame that has to be transmitted to support features related to the IEEE 802.15.4 security and information elements.

When the driver modifies a frame, it also informs the higher layer about the modifications performed to let the higher layer correctly handle retransmissions.

To do so, the following functions take an additional parameter:

* :c:func:`nrf_802154_transmitted`
* :c:func:`nrf_802154_transmitted_raw`
* :c:func:`nrf_802154_transmit_failed`

This additional parameter contains two flags:

* One flag indicates if the driver secured the frame in question, according to its IEEE 802.15.4 Auxiliary Security Header.
  When set, this flag indicates one of the following scenarios:

  * The MAC payload of the frame is encrypted.
  * The frame got authenticated with a Message Integrity Code (MIC).
  * Both the previous scenarios.

* The other flag indicates if the fields set by the driver in runtime, namely the Frame Counter field and any Information Element field that requires strict timing precision (like Coordinated Sample Listening IE), have been set.

After receiving information about the modifications performed, the higher layer can order the driver to skip specific steps of the frame transformation, also avoiding some of the pre-transmission processing.

To do so, the following functions take an additional parameter:

* :c:func:`nrf_802154_transmit`
* :c:func:`nrf_802154_transmit_raw`
* :c:func:`nrf_802154_transmit_csma_ca`
* :c:func:`nrf_802154_transmit_csma_ca_raw`
* :c:func:`nrf_802154_transmit_raw_at`

This additional parameter contains two flags:

* One indicates if the driver should secure the frame in question, according to its IEEE 802.15.4 Auxiliary Security Header.
  When set, the driver attempts to authenticate and encrypt the frame, as configured by the MAC header of the frame.
* The other indicates if the driver should update the Frame Counter field and any Information Elements field that requires strict timing precision (like Coordinated Sample Listening IE).
  When set, the driver overwrites the values present in the fields of the provided frame.

The higher layer can implement various retransmission schemes by combining the information provided by the driver in the callouts with the ability to control the processing performed by the driver on the frames that have to be transmitted.

.. caution::
   If the higher layer marks a frame as already secured, it must not expect the driver to update the dynamic data of the frame.
   Transmitting an encrypted frame with its header modified afterward results in a security breach.
   An attempt to transmit a frame with such parameters will fail unconditionally.

Supporting Front-End Modules
****************************

The driver supports Front-End Modules (FEMs) using an implementation provided by the :ref:`mpsl`.
If a FEM is used, the value of the transmit power is the output power on the antenna rather than the output power of the SoC.
The runtime gain control of the FEM is supported.
