#include "ppr_prl.h"

#include <zephyr/cache.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>


#define PPR_LOAD_CONFIG_TASKS_TRIGGER *(volatile uint32_t *)(0x5F908030UL) 
#define PPR_START_SAMPLING_TASKS_TRIGGER *(volatile uint32_t *)(0x5F908034UL) 
#define STATE_PPR_PRL_READY 0x01

typedef struct {
    uint32_t vtim_cnttop; 
    uint8_t state;
    uint8_t rfu;
    uint16_t max_samples;
} log_conf_t;

/* log_conf inside the ppr ram, just writing straight to that */
static log_conf_t *log_conf = (log_conf_t*)((uint32_t)(0x2fc007f8));

void ppr_prl_configure(uint32_t max_samples, uint32_t vtim_cnttop) {
    log_conf->vtim_cnttop = vtim_cnttop;
    log_conf->max_samples = max_samples;
    log_conf->rfu = 0;
    sys_cache_data_flush_range(log_conf, sizeof(*log_conf));

    PPR_LOAD_CONFIG_TASKS_TRIGGER = 1ul;

    while (log_conf->state != STATE_PPR_PRL_READY) {
        sys_cache_data_flush_and_invd_range(log_conf, sizeof(*log_conf));
    }
}

void ppr_prl_start(void) {
    PPR_START_SAMPLING_TASKS_TRIGGER = 1ul;
}

#if CONFIG_PPR_PRL_AUTO_START

static int ppr_prl_auto_start(void) {
    ppr_prl_configure(CONFIG_PPR_PRL_AUTO_START_CONFIG_MAX_SAMPLES, CONFIG_PPR_PRL_AUTO_START_CONFIG_VTIM_CNTTOP);
    ppr_prl_start();
    return 0;
}

SYS_INIT(ppr_prl_auto_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
