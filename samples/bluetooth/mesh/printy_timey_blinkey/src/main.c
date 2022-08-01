#include <zephyr.h>
#include <sys/printk.h>

#include <dk_buttons_and_leds.h>

static struct k_work_delayable continuous_printing;
static struct k_work_delayable blinky_work;

static void message_print(struct k_work *work)
{
	printk("Hello World! %s\n", CONFIG_BOARD);
	k_work_reschedule(&continuous_printing, K_MSEC(1000));
}


static bool attention=0;

struct k_timer blinky_timer;
static void leds_blinking(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
		BIT(3),
#endif
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
		k_work_reschedule(&blinky_work, K_MSEC(300));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}



}

static void blinky_expired(struct k_timer *timer_id)
{
	attention = false;
	printk("Blinky timer expired.\n");
}

static void blinky_stopped(struct k_timer *timer_id) {
	attention = false;
	printk("Blinky timer stopped.\n");
}

struct button {

    /** Light status*/

    bool status;

};

struct button buttons[4] = {0,0,0,0};


static void button_handler_cb(uint32_t pressed, uint32_t changed)
{

        for (int i = 0; i < ARRAY_SIZE(buttons); i++) {
                if (i == 0 && (pressed & changed & BIT(i))) {
                        buttons[i].status = !buttons[i].status;
                        dk_set_led(i, buttons[i].status);
                        printk("Button %d: Received response: %s\n", i + 1, buttons[i].status ? "on" : "off");  
                }

                if (i == 1 && (pressed & changed & BIT(i))) {
			if (k_work_busy_get(&blinky_work) == 0) {
				k_timer_start(&blinky_timer, K_MSEC(3000), K_NO_WAIT);
				attention = true;
				k_work_schedule(&blinky_work, K_NO_WAIT);
			}
			else {
				printk("Error: Blinky timer already active.\n");
			}
		}
		if (i == 2 && (pressed & changed & BIT(i))) {
			if (k_timer_remaining_get(&blinky_timer) > 0) {
				k_timer_stop(&blinky_timer);
			}
			else {
				printk("Error: Blinky timer not active.\n");
			}
		}
	}

    }




static struct button_handler button_handler =
{
        .cb = button_handler_cb
};


void main(void)
{

        dk_leds_init();
        dk_buttons_init(NULL);
        dk_button_handler_add(&button_handler);


        k_work_init_delayable(&continuous_printing, message_print);
        k_work_submit(&continuous_printing);

        k_work_init_delayable(&blinky_work, leds_blinking);
        k_work_submit(&blinky_work);
        k_timer_init(&blinky_timer, blinky_expired, blinky_stopped);
        
}
