.. _mpsl_tx_power_control:

TX Power Control
################

The TX Power control feature allows the application to set a maximum TX Power envelope.
That means that the application can limit the TX power to be used per PHY and per channel.
Limiting the Radio TX power on edge bands might be necessary to pass teleregulatory requirements when using an external amplifier.

Radio protocols can define their own TX power levels for different types of radio activity.
However, when utilizing the MPSL TX Power feature, the actual Radio TX Power can be limited.

To ensure that the feature behaves in a consistent way, the TX Power channel maps must always be configured when the MPSL is in an idle state with no active users.
