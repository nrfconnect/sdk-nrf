.. _ug_radio_fem_incomplete_connections:

Use case of incomplete physical connections to the FEM module
#############################################################

The method of configuring FEM using the devicetree file allows you to opt out of using some pins.
For example, if power consumption is not critical, the nRF21540 module PDN pin can be connected to a fixed logic level.
Then there is no need to define a GPIO to control the PDN signal. The line ``pdn-gpios = < .. >;`` can then be removed from the devicetree file.

Generally, if pin ``X`` is not used, the ``X-gpios = < .. >;`` property can be removed.
This applies to all properties with a ``-gpios`` suffix, for both nRF21540 and SKY66112-11.
