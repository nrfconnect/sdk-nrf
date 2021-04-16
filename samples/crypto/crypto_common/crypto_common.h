#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)

#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define PRINT_MESSAGE(p_text) ({ LOG_INF("%s", p_text); })

#define PRINT_ERROR(func_name, err) ({ LOG_INF("%s failed! (Error: %d)", (func_name), (err)); })

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})


int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return -1;

	return 0;
}
