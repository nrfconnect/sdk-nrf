#include <zephyr.h>
#include <modem/lte_lc.h>
#include <modem/location.h>

void location_event_handler(enum location_event event, const struct location_data *location)
{
	switch (event) {
	case LOC_EVT_LOCATION:
		printk("LOC_EVT_LOCATION\n");
		printk("method: %d\n", location->used_method);
		printk("latitude: %.06f\n", location->latitude);
		printk("longitude: %.06f\n", location->longitude);
		printk("accuracy: %.01f m\n", location->accuracy);
		break;

	case LOC_EVT_TIMEOUT:
		printk("LOC_EVT_TIMEOUT\n");
		break;

	case LOC_EVT_ERROR:
		printk("LOC_EVT_ERROR\n");
		break;
	}
}

int main(void)
{
	int err;
	struct location_config config = { 0 };

	config.methods[0].method = LOC_METHOD_CELL_ID;

	config.methods[1].method = LOC_METHOD_GNSS;
	config.methods[1].config.gnss.timeout = 120;
	config.methods[1].config.gnss.allow_low_accuracy = false;

	/* Disable LTE to help get a GNSS fix faster for now */
	lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);

	err = location_init(location_event_handler);
	if (err) {
		printk("Initializing the Location library failed, err: %d\n", err);
		return -1;
	}

	err = location_get(&config);
	if (err) {
		printk("Requesting location failed, err: %d\n", err);
		return -1;
	}

	return 0;
}
