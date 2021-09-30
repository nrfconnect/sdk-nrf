#include <zephyr.h>
#include <ztest.h>
#include <drivers/uart.h>

const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS
};

void poll_until_carriage_return(const struct device *uart_dev)
{
    uint8_t rx;
    while (1) 
    {
		while (uart_poll_in(uart_dev, &rx) < 0) { }

		if (rx == '\r') {
			return;
		}
	}
}

void put_confirmation(const struct device *uart_dev, size_t iteration)
{
    char msg[32];
    int size = sprintf(msg, "Confirming %d\n", iteration);

    for (int i = 0; i < size; i++)
    {
        uart_poll_out(uart_dev, msg[i]);
    }
}


void test_uart_robustness(void)
{
    const struct device *uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
    zassert_not_null(uart_dev, "UART device binding");

	zassert_equal(0, uart_configure(uart_dev, &uart_cfg), "UART configure");

    size_t iteration = 0;

    while(iteration < 1000) {
        poll_until_carriage_return(uart_dev);
        put_confirmation(uart_dev, iteration);

        iteration += 1;
    }
}

void test_main(void)
{
	ztest_test_suite(uart_robustness,
        ztest_unit_test(test_uart_robustness)
    );
	ztest_run_test_suite(uart_robustness);
}