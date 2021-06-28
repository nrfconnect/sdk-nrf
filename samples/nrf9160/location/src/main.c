#include <zephyr.h>
#include <modem/lte_lc.h>
#include <modem/location.h>

void location_event_handler(const struct loc_event_data *event_data)
{
	switch (event_data->id) {
	case LOC_EVT_LOCATION:
		printk("LOC_EVT_LOCATION\n");
		printk("method: %d\n", event_data->method);
		printk("latitude: %.06f\n", event_data->location.latitude);
		printk("longitude: %.06f\n", event_data->location.longitude);
		printk("accuracy: %.01f m\n", event_data->location.accuracy);
		if (event_data->location.datetime.valid) {
			printk("date: %04d-%02d-%02d\n",
				event_data->location.datetime.year,
				event_data->location.datetime.month,
				event_data->location.datetime.day);
			printk("time: %02d:%02d:%02d.%03d UTC\n",
				event_data->location.datetime.hour,
				event_data->location.datetime.minute,
				event_data->location.datetime.second,
				event_data->location.datetime.ms);
		}
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
	struct loc_config config = { 0 };

	config.interval = 0; // Single position

	config.methods[0].method = LOC_METHOD_CELL_ID;

	config.methods[1].method = LOC_METHOD_GNSS;
	config.methods[1].config.gnss.timeout = 120;
	config.methods[1].config.gnss.accuracy = LOC_ACCURACY_NORMAL;

	/* Disable LTE to help get a GNSS fix faster for now */
	lte_lc_func_mode_set(LTE_LC_FUNC_MODE_DEACTIVATE_LTE);

	err = location_init(location_event_handler);
	if (err) {
		printk("Initializing the Location library failed, err: %d\n", err);
		return -1;
	}

	err = location_request(&config);
	if (err) {
		printk("Requesting location failed, err: %d\n", err);
		return -1;
	}

	return 0;
}
