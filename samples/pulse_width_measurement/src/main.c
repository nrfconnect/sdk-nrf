#include <zephyr/kernel.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <assert.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <nrf_pulse_meas.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define NUMBER_OF_MEASUREMENTS 16
#define NUMBER_OF_SERIES 10
K_MEM_SLAB_DEFINE(my_meas_slab, 8 + NUMBER_OF_MEASUREMENTS * sizeof(uint32_t), 128, 4);

#define NRF_PULSE_MEAS_NODE DT_NODELABEL(pulse_width_meas)
#define NRF_PULSE_MEAS_DEVICE DT_NODELABEL(pulse_width_meas)
const struct device *nrf_pulse_meas_instance = DEVICE_DT_GET(NRF_PULSE_MEAS_DEVICE);

static struct k_sem clock_ready_sem;
static struct onoff_manager *clk_mgr;
static struct onoff_client clk_cli;

void my_handler(void * context)
{
	(void)context;
	//printf("User Handler\n");
}

uint32_t * my_data;

struct api_nrf_pulse_meas_config api_cfg = {
	.num_of_meas = NUMBER_OF_MEASUREMENTS,
	.pulse_type = nrf_pulse_meas_pulse_positive,
	.mode = nrf_pulse_meas_mode_continuous,
	.pull_config = NRF_GPIO_PIN_NOPULL,
	.user_handler = my_handler,
	.user_context = NULL,
};

/* Callback for clock request. */
static void clock_started_callback(struct onoff_manager *mgr,
				   struct onoff_client *cli,
				   uint32_t state,
				   int res)
{
	(void)mgr;
	(void)cli;
	(void)state;
	(void)res;

	k_sem_give(&clock_ready_sem);
}

static int hf_clock_request(void)
{
	sys_notify_init_callback(&clk_cli.notify, clock_started_callback);
	clock_control_subsys_t subsys = CLOCK_CONTROL_NRF_SUBSYS_HF;

	clk_mgr = z_nrf_clock_control_get_onoff(subsys);
	return onoff_request(clk_mgr, &clk_cli);
}

int main(void)
{
	int ret;
	int err;
	uint32_t series_cnt = 0;

	k_sem_init(&clock_ready_sem, 0, 1);

	err = hf_clock_request();
	if (err != 0) {
		printk("Failed to request clock!\n");
		return err;
	}

	err = k_sem_take(&clock_ready_sem, K_MSEC(50));
	if (err != 0) {
		printk("Clock request timeout!\n");
		return err;
	}

	/* Scenario 1:
	 * 100 ms measurements period;
	 */
	nrf_pulse_meas_configure(nrf_pulse_meas_instance, &api_cfg);
	nrf_pulse_meas_start(nrf_pulse_meas_instance, &my_meas_slab);
	k_msleep(100);
	nrf_pulse_meas_stop(nrf_pulse_meas_instance, true);
	do {
		ret = nrf_pulse_meas_meas_get(nrf_pulse_meas_instance, &my_data);
		if (ret >= 0) {
			for (size_t i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
			{
				printf("d[%u] = %u\n", i, my_data[i]);
			}
			printk(" - %u measurements left\n",
				nrf_pulse_meas_meas_pending(nrf_pulse_meas_instance));
			nrf_pulse_meas_meas_free(nrf_pulse_meas_instance, my_data);
			series_cnt ++;
		} else if (ret == -EAGAIN) {
			// Wait for the last series to be finished
			ret = 0;
		}
		k_usleep(10);
	} while(ret >= 0);
	printk("Read %u series", series_cnt);

	/* Scenario 2:
	 * Meas and read in parallel
	 */
	nrf_pulse_meas_start(nrf_pulse_meas_instance, &my_meas_slab);
	uint8_t series_num = 0;
	do {
		ret = nrf_pulse_meas_meas_get(nrf_pulse_meas_instance, &my_data);
		if (ret >= 0) {
			for (size_t i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
			{
				printf("d%u[%u] = %u\n", series_num, i, my_data[i]);
			}
			nrf_pulse_meas_meas_free(nrf_pulse_meas_instance, my_data);
			series_num ++;
		}
		k_usleep(10);
	} while (series_num < NUMBER_OF_SERIES);
	nrf_pulse_meas_stop(nrf_pulse_meas_instance, true);
	//wait for the last series
	do {
		ret = nrf_pulse_meas_meas_get(nrf_pulse_meas_instance, &my_data);
	} while (ret == -EAGAIN);
	for (size_t i = 0; i < NUMBER_OF_MEASUREMENTS; i++)
	{
		printf("d%u[%u] = %u\n", series_num, i, my_data[i]);
	}
	nrf_pulse_meas_meas_free(nrf_pulse_meas_instance, my_data);

	printk("Test finished\n");
	return 0;
}
