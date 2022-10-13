/*
 * Copyright (c) 2020 Amarula Solutions.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_sdmmc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <stm32_ll_rcc.h>

LOG_MODULE_REGISTER(stm32_sdmmc, CONFIG_SDMMC_LOG_LEVEL);

#define STM32_SDMMC_USE_DMA DT_NODE_HAS_PROP(DT_DRV_INST(0), dmas)

#if STM32_SDMMC_USE_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <stm32_ll_dma.h>
#endif

#ifndef MMC_TypeDef
#define MMC_TypeDef SDMMC_TypeDef
#endif

#ifndef SDMMC_BUS_WIDE_1B
#define SDMMC_BUS_WIDE_1B SDIO_BUS_WIDE_1B
#endif

#ifndef SDMMC_BUS_WIDE_4B
#define SDMMC_BUS_WIDE_4B SDIO_BUS_WIDE_4B
#endif

#ifndef SDMMC_BUS_WIDE_8B
#define SDMMC_BUS_WIDE_8B SDIO_BUS_WIDE_8B
#endif

typedef void (*irq_config_func_t)(const struct device *dev);

#if STM32_SDMMC_USE_DMA

static const uint32_t table_priority[] = {
	DMA_PRIORITY_LOW,
	DMA_PRIORITY_MEDIUM,
	DMA_PRIORITY_HIGH,
	DMA_PRIORITY_VERY_HIGH
};

struct sdmmc_dma_stream {
	const struct device *dev;
	uint32_t channel;
	uint32_t channel_nb;
	DMA_TypeDef *reg;
	struct dma_config cfg;
};
#endif

struct stm32_sdmmc_priv {
	irq_config_func_t irq_config;
	struct k_sem thread_lock;
	struct k_sem sync;
	SD_HandleTypeDef hsd;
	int status;
	struct k_work work;
	struct gpio_callback cd_cb;
	struct gpio_dt_spec cd;
	struct gpio_dt_spec pe;
	struct stm32_pclken pclken;
	const struct pinctrl_dev_config *pcfg;

#if STM32_SDMMC_USE_DMA
	struct sdmmc_dma_stream dma_rx;
	struct sdmmc_dma_stream dma_tx;
#endif
};

#ifdef CONFIG_SDMMC_STM32_HWFC
static void stm32_sdmmc_fc_enable(struct stm32_sdmmc_priv *priv)
{
	MMC_TypeDef *sdmmcx = priv->hsd.Instance;

	sdmmcx->CLKCR |= SDMMC_CLKCR_HWFC_EN;
}
#endif

static void stm32_sdmmc_isr(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;

	HAL_SD_IRQHandler(&priv->hsd);
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
	struct stm32_sdmmc_priv *priv =
		CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);

	priv->status = hsd->ErrorCode;

	k_sem_give(&priv->sync);
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
	struct stm32_sdmmc_priv *priv =
		CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);

	priv->status = hsd->ErrorCode;

	k_sem_give(&priv->sync);
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
	struct stm32_sdmmc_priv *priv =
		CONTAINER_OF(hsd, struct stm32_sdmmc_priv, hsd);

	priv->status = hsd->ErrorCode;

	k_sem_give(&priv->sync);
}

static int stm32_sdmmc_clock_enable(struct stm32_sdmmc_priv *priv)
{
	const struct device *clock;

#if CONFIG_SOC_SERIES_STM32L4X
	LL_RCC_PLLSAI1_Disable();
	/* Configure PLLSA11 to enable 48M domain */
	LL_RCC_PLLSAI1_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSI,
					LL_RCC_PLLM_DIV_1,
					8, LL_RCC_PLLSAI1Q_DIV_8);

	/* Enable PLLSA1 */
	LL_RCC_PLLSAI1_Enable();

	/*  Enable PLLSAI1 output mapped on 48MHz domain clock */
	LL_RCC_PLLSAI1_EnableDomain_48M();

	/* Wait for PLLSA1 ready flag */
	while (LL_RCC_PLLSAI1_IsReady() != 1)
		;

	LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC1_CLKSOURCE_PLLSAI1);
#endif

#if defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
	/* By default the SDMMC clock source is set to 0 --> 48MHz, must be enabled */
	LL_RCC_HSI48_Enable();
	while (!LL_RCC_HSI48_IsReady()) {
	}
#endif /* CONFIG_SOC_SERIES_STM32L5X ||
	* CONFIG_SOC_SERIES_STM32U5X
	*/
	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Enable the APB clock for stm32_sdmmc */
	return clock_control_on(clock, (clock_control_subsys_t *)&priv->pclken);
}

static int stm32_sdmmc_clock_disable(struct stm32_sdmmc_priv *priv)
{
	const struct device *clock;

	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	return clock_control_off(clock,
				 (clock_control_subsys_t *)&priv->pclken);
}

#if STM32_SDMMC_USE_DMA

static void stm32_sdmmc_dma_cb(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	if (status != 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);

	}

	HAL_DMA_IRQHandler(hdma);
}

static int stm32_sdmmc_configure_dma(DMA_HandleTypeDef *handle, struct sdmmc_dma_stream *dma)
{
	int ret;

	if (!device_is_ready(dma->dev)) {
		LOG_ERR("Failed to get dma dev");
		return -ENODEV;
	}

	dma->cfg.user_data = handle;

	ret = dma_config(dma->dev, dma->channel, &dma->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to conig");
		return ret;
	}

	handle->Instance                 = __LL_DMA_GET_STREAM_INSTANCE(dma->reg, dma->channel_nb);
	handle->Init.Channel             = dma->cfg.dma_slot * DMA_CHANNEL_1;
	handle->Init.PeriphInc           = DMA_PINC_DISABLE;
	handle->Init.MemInc              = DMA_MINC_ENABLE;
	handle->Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	handle->Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
	handle->Init.Mode                = DMA_PFCTRL;
	handle->Init.Priority            = table_priority[dma->cfg.channel_priority],
	handle->Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
	handle->Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	handle->Init.MemBurst            = DMA_MBURST_INC4;
	handle->Init.PeriphBurst         = DMA_PBURST_INC4;

	return ret;
}

static int stm32_sdmmc_dma_init(struct stm32_sdmmc_priv *priv)
{
	static DMA_HandleTypeDef dma_tx_handle;
	static DMA_HandleTypeDef dma_rx_handle;
	int err;

	LOG_DBG("using dma");

	err = stm32_sdmmc_configure_dma(&dma_tx_handle, &priv->dma_tx);
	if (err) {
		LOG_ERR("failed to init tx dma");
		return err;
	}
	__HAL_LINKDMA(&priv->hsd, hdmatx, dma_tx_handle);
	HAL_DMA_DeInit(&dma_tx_handle);
	HAL_DMA_Init(&dma_tx_handle);

	stm32_sdmmc_configure_dma(&dma_rx_handle, &priv->dma_rx);
	if (err) {
		LOG_ERR("failed to init rx dma");
		return err;
	}
	__HAL_LINKDMA(&priv->hsd, hdmarx, dma_rx_handle);
	HAL_DMA_DeInit(&dma_rx_handle);
	HAL_DMA_Init(&dma_rx_handle);

	return err;
}

#endif

static int stm32_sdmmc_access_init(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	if (priv->status == DISK_STATUS_OK) {
		return 0;
	}

	if (priv->status == DISK_STATUS_NOMEDIA) {
		return -ENODEV;
	}

#if STM32_SDMMC_USE_DMA
	stm32_sdmmc_dma_init(priv);
#endif

	err = stm32_sdmmc_clock_enable(priv);
	if (err) {
		LOG_ERR("failed to init clocks");
		return err;
	}

	err = HAL_SD_Init(&priv->hsd);
	if (err != HAL_OK) {
		LOG_ERR("failed to init stm32_sdmmc (ErrorCode 0x%X)", priv->hsd.ErrorCode);
		return -EIO;
	}

#ifdef CONFIG_SDMMC_STM32_HWFC
	stm32_sdmmc_fc_enable(priv);
#endif

	priv->status = DISK_STATUS_OK;
	return 0;
}

static void stm32_sdmmc_access_deinit(struct stm32_sdmmc_priv *priv)
{
	HAL_SD_DeInit(&priv->hsd);
	stm32_sdmmc_clock_disable(priv);
}

static int stm32_sdmmc_access_status(struct disk_info *disk)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;

	return priv->status;
}

static int stm32_sdmmc_access_read(struct disk_info *disk, uint8_t *data_buf,
				   uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	k_sem_take(&priv->thread_lock, K_FOREVER);

#if STM32_SDMMC_USE_DMA
	err = HAL_SD_ReadBlocks_DMA(&priv->hsd, data_buf, start_sector,
				num_sector);
#else
	err = HAL_SD_ReadBlocks_IT(&priv->hsd, data_buf, start_sector,
				num_sector);
#endif
	if (err != HAL_OK) {
		LOG_ERR("sd read block failed %d", err);
		err = -EIO;
		goto end;
	}

	k_sem_take(&priv->sync, K_FOREVER);

	if (priv->status != DISK_STATUS_OK) {
		LOG_ERR("sd read error %d", priv->status);
		err = -EIO;
		goto end;
	}

	while (HAL_SD_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER) {
	}

end:
	k_sem_give(&priv->thread_lock);
	return err;
}

static int stm32_sdmmc_access_write(struct disk_info *disk,
				    const uint8_t *data_buf,
				    uint32_t start_sector, uint32_t num_sector)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	int err;

	k_sem_take(&priv->thread_lock, K_FOREVER);

#if STM32_SDMMC_USE_DMA
	err = HAL_SD_WriteBlocks_DMA(&priv->hsd, (uint8_t *)data_buf, start_sector,
				 num_sector);
#else
	err = HAL_SD_WriteBlocks_IT(&priv->hsd, (uint8_t *)data_buf, start_sector,
				 num_sector);
#endif
	if (err != HAL_OK) {
		LOG_ERR("sd write block failed %d", err);
		err = -EIO;
		goto end;
	}

	k_sem_take(&priv->sync, K_FOREVER);

	if (priv->status != DISK_STATUS_OK) {
		LOG_ERR("sd write error %d", priv->status);
		err = -EIO;
		goto end;
	}

	while (HAL_SD_GetCardState(&priv->hsd) != HAL_SD_CARD_TRANSFER) {
	}

end:
	k_sem_give(&priv->thread_lock);
	return err;
}

static int stm32_sdmmc_access_ioctl(struct disk_info *disk, uint8_t cmd,
				    void *buff)
{
	const struct device *dev = disk->dev;
	struct stm32_sdmmc_priv *priv = dev->data;
	HAL_SD_CardInfoTypeDef info;
	int err;

	switch (cmd) {
	case DISK_IOCTL_GET_SECTOR_COUNT:
		err = HAL_SD_GetCardInfo(&priv->hsd, &info);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockNbr;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		err = HAL_SD_GetCardInfo(&priv->hsd, &info);
		if (err != HAL_OK) {
			return -EIO;
		}
		*(uint32_t *)buff = info.LogBlockSize;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff = 1;
		break;
	case DISK_IOCTL_CTRL_SYNC:
		/* we use a blocking API, so nothing to do for sync */
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct disk_operations stm32_sdmmc_ops = {
	.init = stm32_sdmmc_access_init,
	.status = stm32_sdmmc_access_status,
	.read = stm32_sdmmc_access_read,
	.write = stm32_sdmmc_access_write,
	.ioctl = stm32_sdmmc_access_ioctl,
};

static struct disk_info stm32_sdmmc_info = {
	.name = CONFIG_SDMMC_VOLUME_NAME,
	.ops = &stm32_sdmmc_ops,
};

/*
 * Check if the card is present or not. If no card detect gpio is set, assume
 * the card is present. If reading the gpio fails for some reason, assume the
 * card is there.
 */
static bool stm32_sdmmc_card_present(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->cd.port) {
		return true;
	}

	err = gpio_pin_get_dt(&priv->cd);
	if (err < 0) {
		LOG_WRN("reading card detect failed %d", err);
		return true;
	}
	return err;
}

static void stm32_sdmmc_cd_handler(struct k_work *item)
{
	struct stm32_sdmmc_priv *priv = CONTAINER_OF(item,
						     struct stm32_sdmmc_priv,
						     work);

	if (stm32_sdmmc_card_present(priv)) {
		LOG_DBG("card inserted");
		priv->status = DISK_STATUS_UNINIT;
	} else {
		LOG_DBG("card removed");
		stm32_sdmmc_access_deinit(priv);
		priv->status = DISK_STATUS_NOMEDIA;
	}
}

static void stm32_sdmmc_cd_callback(const struct device *gpiodev,
				    struct gpio_callback *cb,
				    uint32_t pin)
{
	struct stm32_sdmmc_priv *priv = CONTAINER_OF(cb,
						     struct stm32_sdmmc_priv,
						     cd_cb);

	k_work_submit(&priv->work);
}

static int stm32_sdmmc_card_detect_init(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->cd.port) {
		return 0;
	}

	if (!device_is_ready(priv->cd.port)) {
		return -ENODEV;
	}

	gpio_init_callback(&priv->cd_cb, stm32_sdmmc_cd_callback,
			   1 << priv->cd.pin);

	err = gpio_add_callback(priv->cd.port, &priv->cd_cb);
	if (err) {
		return err;
	}

	err = gpio_pin_configure_dt(&priv->cd, GPIO_INPUT);
	if (err) {
		goto remove_callback;
	}

	err = gpio_pin_interrupt_configure_dt(&priv->cd, GPIO_INT_EDGE_BOTH);
	if (err) {
		goto unconfigure_pin;
	}
	return 0;

unconfigure_pin:
	gpio_pin_configure_dt(&priv->cd, GPIO_DISCONNECTED);
remove_callback:
	gpio_remove_callback(priv->cd.port, &priv->cd_cb);
	return err;
}

static int stm32_sdmmc_card_detect_uninit(struct stm32_sdmmc_priv *priv)
{
	if (!priv->cd.port) {
		return 0;
	}

	gpio_pin_interrupt_configure_dt(&priv->cd, GPIO_INT_MODE_DISABLED);
	gpio_pin_configure_dt(&priv->cd, GPIO_DISCONNECTED);
	gpio_remove_callback(priv->cd.port, &priv->cd_cb);
	return 0;
}

static int stm32_sdmmc_pwr_init(struct stm32_sdmmc_priv *priv)
{
	int err;

	if (!priv->pe.port) {
		return 0;
	}

	if (!device_is_ready(priv->pe.port)) {
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&priv->pe, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}

	k_sleep(K_MSEC(50));

	return 0;
}

static int stm32_sdmmc_pwr_uninit(struct stm32_sdmmc_priv *priv)
{
	if (!priv->pe.port) {
		return 0;
	}

	gpio_pin_configure_dt(&priv->pe, GPIO_DISCONNECTED);
	return 0;
}

static int disk_stm32_sdmmc_init(const struct device *dev)
{
	struct stm32_sdmmc_priv *priv = dev->data;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	k_work_init(&priv->work, stm32_sdmmc_cd_handler);

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(priv->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	priv->irq_config(dev);

	/* Initialize semaphores */
	k_sem_init(&priv->thread_lock, 1, 1);
	k_sem_init(&priv->sync, 0, 1);

	err = stm32_sdmmc_card_detect_init(priv);
	if (err) {
		return err;
	}

	err = stm32_sdmmc_pwr_init(priv);
	if (err) {
		goto err_card_detect;
	}

	if (stm32_sdmmc_card_present(priv)) {
		priv->status = DISK_STATUS_UNINIT;
	} else {
		priv->status = DISK_STATUS_NOMEDIA;
	}

	stm32_sdmmc_info.dev = dev;
	err = disk_access_register(&stm32_sdmmc_info);
	if (err) {
		goto err_pwr;
	}
	return 0;

err_pwr:
	stm32_sdmmc_pwr_uninit(priv);
err_card_detect:
	stm32_sdmmc_card_detect_uninit(priv);
	return err;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

#if STM32_SDMMC_USE_DMA

#define SDMMC_DMA_CHANNEL_INIT(dir, dir_cap)				\
	.dev = DEVICE_DT_GET(STM32_DMA_CTLR(0, dir)),			\
	.channel = DT_INST_DMAS_CELL_BY_NAME(0, dir, channel),		\
	.channel_nb = DT_DMAS_CELL_BY_NAME(				\
			DT_DRV_INST(0), dir, channel),			\
	.reg = (DMA_TypeDef *)DT_REG_ADDR(				\
			DT_PHANDLE_BY_NAME(DT_DRV_INST(0), dmas, dir)),	\
	.cfg = {							\
		.dma_slot = STM32_DMA_SLOT(0, dir, slot),		\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
				STM32_DMA_CHANNEL_CONFIG(0, dir)),	\
		.dma_callback = stm32_sdmmc_dma_cb,			\
		.linked_channel = STM32_DMA_HAL_OVERRIDE,		\
	},								\


#define SDMMC_DMA_CHANNEL(dir, DIR)					\
.dma_##dir = {								\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(0, dir),			\
		 (SDMMC_DMA_CHANNEL_INIT(dir, DIR)),			\
		 (NULL))						\
	},

#else
#define SDMMC_DMA_CHANNEL(dir, DIR)
#endif

PINCTRL_DT_INST_DEFINE(0);

static void stm32_sdmmc_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		DT_INST_IRQ(0, priority),
		stm32_sdmmc_isr, DEVICE_DT_INST_GET(0),
		0);
	irq_enable(DT_INST_IRQN(0));
}

#if DT_INST_PROP(0, bus_width) == 1
#define SDMMC_BUS_WIDTH SDMMC_BUS_WIDE_1B
#elif DT_INST_PROP(0, bus_width) == 4
#define SDMMC_BUS_WIDTH SDMMC_BUS_WIDE_4B
#elif DT_INST_PROP(0, bus_width) == 8
#define SDMMC_BUS_WIDTH SDMMC_BUS_WIDE_8B
#endif /* DT_INST_PROP(0, bus_width) */

static struct stm32_sdmmc_priv stm32_sdmmc_priv_1 = {
	.irq_config = stm32_sdmmc_irq_config_func,
	.hsd = {
		.Instance = (MMC_TypeDef *)DT_INST_REG_ADDR(0),
		.Init.BusWide = SDMMC_BUS_WIDTH,
	},
#if DT_INST_NODE_HAS_PROP(0, cd_gpios)
	.cd = GPIO_DT_SPEC_INST_GET(0, cd_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, pwr_gpios)
	.pe = GPIO_DT_SPEC_INST_GET(0, pwr_gpios),
#endif
	.pclken = {
		.bus = DT_INST_CLOCKS_CELL(0, bus),
		.enr = DT_INST_CLOCKS_CELL(0, bits),
	},
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	SDMMC_DMA_CHANNEL(rx, RX)
	SDMMC_DMA_CHANNEL(tx, TX)
};

DEVICE_DT_INST_DEFINE(0, disk_stm32_sdmmc_init, NULL,
		    &stm32_sdmmc_priv_1, NULL, POST_KERNEL,
		    CONFIG_SDMMC_INIT_PRIORITY,
		    NULL);
#endif
