.. _mspi_sqspi:

sQSPI MSPI shim driver
######################

.. contents::
   :local:
   :depth: 2

This driver integrates the :ref:`sQSPI` soft peripheral with the Zephyr MSPI API, enabling control over the peripheral.
This allows for communication with devices that use MSPI bus-based Zephyr drivers, such as flash chips or displays.

Configuration
*************

To configure the sQSPI MSPI shim driver with the devicetree, complete the following steps:

1. Specify the base address of the sQSPI registers in RAM by creating the :dtcompatible:`nordic,nrf-sqspi` node within the RAM region reserved for the soft peripheral.
#. Specify an interrupt for communication between the application and FLPR cores, and determine the pins for sQSPI operation.
   Provide this information in the properties of the node labeled as ``cpuflpr_vpr``.
#. If the ``execution-memory`` property exists in the ``cpuflpr_vpr`` node, delete it to ensure the FLPR core does not use the Nordic VPR coprocessor launcher (see the :kconfig:option:`CONFIG_NORDIC_VPR_LAUNCHER` Kconfig option).

For RAM configuration details on supported SoCs, refer to the following guides:

* :ref:`nRF54L15 RAM configuration <nrf54L15_porting_guide_ram_configuration>`
* :ref:`nRF54H20 RAM configuration <nrf54H20_porting_guide_ram_configuration>` - On the nRF54H20 SoC, ensure the RAM region reserved for the sQSPI is labeled ``softperiph_ram`` to set it as non-cacheable in the MPU configuration.

See the following configuration example for the nRF54L15 SoC:

.. code-block:: devicetree

	&pinctrl {
		sqspi_default: sqspi_default {
			group1 {
				psels = <NRF_PSEL(SDP_MSPI_SCK, 2, 1)>,
					<NRF_PSEL(SDP_MSPI_CS0, 2, 5)>,
					<NRF_PSEL(SDP_MSPI_DQ0, 2, 2)>;
				nordic,drive-mode = <NRF_DRIVE_E0E1>;
			};
			group2 {
				psels = <NRF_PSEL(SDP_MSPI_DQ1, 2, 4)>,
					<NRF_PSEL(SDP_MSPI_DQ2, 2, 3)>,
					<NRF_PSEL(SDP_MSPI_DQ3, 2, 0)>;
				nordic,drive-mode = <NRF_DRIVE_E0E1>;
				bias-pull-up;
			};
		};

		sqspi_sleep: sqspi_sleep {
			group1 {
				low-power-enable;
				psels = <NRF_PSEL(SDP_MSPI_SCK, 2, 1)>,
					<NRF_PSEL(SDP_MSPI_CS0, 2, 5)>,
					<NRF_PSEL(SDP_MSPI_DQ0, 2, 2)>,
					<NRF_PSEL(SDP_MSPI_DQ1, 2, 4)>,
					<NRF_PSEL(SDP_MSPI_DQ2, 2, 3)>,
					<NRF_PSEL(SDP_MSPI_DQ3, 2, 0)>;
			};
		};
	};

	&cpuflpr_vpr {
		pinctrl-0 = <&sqspi_default>;
		pinctrl-1 = <&sqspi_sleep>;
		pinctrl-names = "default", "sleep";
		interrupts = <76 NRF_DEFAULT_IRQ_PRIORITY>;
		status = "okay";
	};

	/ {
		reserved-memory {
			softperiph_ram: memory@2003c000 {
				reg = <0x2003c000 0x4000>;
				ranges = <0 0x2003c000 0x4000>;
				#address-cells = <1>;
				#size-cells = <1>;

				sqspi: sqspi@3c00 {
					compatible = "nordic,nrf-sqspi";
					#address-cells = <1>;
					#size-cells = <0>;
					reg = <0x3c00 0x200>;
					status = "okay";
					zephyr,pm-device-runtime-auto;
				};
			};
		};
	};

For the nRF54H20 SoC, you must also reserve a RAM region for data buffers used in sQSPI transfers, accessible by the FLPR core.
Indicate this region to the shim driver using the ``memory-regions`` property in the sQSPI node.
If you initiate an MSPI transfer with a buffer outside this region, the shim driver will temporarily allocate a buffer within the region and correctly transfer data between the two buffers - before transmitting (TX) or after receiving (RX).
To avoid overhead from automatic allocation and copying, allocate buffers statically using :c:macro:`DMM_MEMORY_SECTION`.
The shim driver will then directly pass these buffers to the sQSPI.

The following example configuration for the nRF54H20 SoC sets up the necessary parameters:

.. code-block:: devicetree

	&pinctrl {
		sqspi_default: sqspi_default {
			group1 {
				psels = <NRF_PSEL(SDP_MSPI_SCK, 7, 0)>,
					<NRF_PSEL(SDP_MSPI_CS0, 7, 5)>,
					<NRF_PSEL(SDP_MSPI_DQ0, 7, 1)>;
				nordic,drive-mode = <NRF_DRIVE_E0E1>;
			};
			group2 {
				psels = <NRF_PSEL(SDP_MSPI_DQ1, 7, 2)>,
					<NRF_PSEL(SDP_MSPI_DQ2, 7, 3)>,
					<NRF_PSEL(SDP_MSPI_DQ3, 7, 4)>;
				nordic,drive-mode = <NRF_DRIVE_E0E1>;
				bias-pull-up;
			};
		};

		sqspi_sleep: sqspi_sleep {
			group1 {
				low-power-enable;
				psels = <NRF_PSEL(SDP_MSPI_SCK, 7, 0)>,
					<NRF_PSEL(SDP_MSPI_CS0, 7, 5)>,
					<NRF_PSEL(SDP_MSPI_DQ0, 7, 1)>,
					<NRF_PSEL(SDP_MSPI_DQ1, 7, 2)>,
					<NRF_PSEL(SDP_MSPI_DQ2, 7, 3)>,
					<NRF_PSEL(SDP_MSPI_DQ3, 7, 4)>;
			};
		};
	};

	&cpuflpr_vpr {
		pinctrl-0 = <&sqspi_default>;
		pinctrl-1 = <&sqspi_sleep>;
		pinctrl-names = "default", "sleep";
		interrupts = <212 NRF_DEFAULT_IRQ_PRIORITY>;
		status = "okay";
		/delete-property/ execution-memory;
	};

	/delete-node/ &cpuflpr_code_data;
	/delete-node/ &cpuapp_cpuflpr_ipc_shm;
	/delete-node/ &cpuflpr_cpuapp_ipc_shm;
	/delete-node/ &cpuapp_cpuflpr_ipc;

	/ {
		reserved-memory {
			#address-cells = <1>;
			#size-cells = <1>;
			ranges;

			softperiph_ram: memory@2f890000 {
				reg = <0x2f890000 0x4000>;
				ranges;
				#address-cells = <1>;
				#size-cells = <1>;

				dut: sqspi: sqspi@3e00 {
					compatible = "nordic,nrf-sqspi";
					#address-cells = <1>;
					#size-cells = <0>;
					reg = <0x3e00 0x200>;
					zephyr,pm-device-runtime-auto;
					memory-regions = <&sqspi_buffers>;
				};
			};

			sqspi_buffers: memory@2f894000 {
				compatible = "zephyr,memory-region";
				reg = <0x2f894000 0x4000>;
				#memory-region-cells = <0>;
				zephyr,memory-region = "SQSPI_BUFFERS";
				zephyr,memory-attr = <DT_MEM_CACHEABLE>;
			};
		};
	};

API documentation
*****************

| Header file: :file:`include/zephyr/drivers/mspi.h`
| Source file: :file:`drivers/mspi/mspi_sqspi.c`

.. doxygengroup:: mspi_interface
