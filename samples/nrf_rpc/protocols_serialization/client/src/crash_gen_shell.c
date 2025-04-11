#include <zephyr/shell/shell.h>
#include <nrf_rpc/nrf_rpc_dev_info.h>

static int cmd_assert(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t delay = 0;

	if (argc > 1) {
		delay = shell_strtol(argv[1], 10, &rc);
	}

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

        nrf_rpc_crash_gen_assert(delay);

        return 0;
}

static int cmd_hard_fault(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t delay = 0;

	if (argc > 1) {
		delay = shell_strtol(argv[1], 10, &rc);
	}

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

        nrf_rpc_crash_gen_hard_fault(delay);

        return 0;
}

static int cmd_stack_overflow(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t delay = 0;

	if (argc > 1) {
		delay = shell_strtol(argv[1], 10, &rc);
	}

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

        nrf_rpc_crash_gen_stack_overflow(delay);

        return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(crash_cmds,
			       SHELL_CMD_ARG(assert, NULL, "Invoke assert", cmd_assert, 1, 1),
			       SHELL_CMD_ARG(hard_fault, NULL, "Invoke hard fault", cmd_hard_fault,
			       		     1, 1),
			       SHELL_CMD_ARG(stack_overflow, NULL, "Invoke stack overflow",
			       		     cmd_stack_overflow, 1, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(crash_gen, &crash_cmds, "nRF RPC crash generator commands", NULL, 1, 0);
