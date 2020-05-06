/**
 * @brief Configuration parameters for pins that enable or disable (or both) either Power Amplifier (PA) or Low Noise Amplifier (LNA).
 */
typedef struct
{
    bool    enable;       /* Enable toggling for this pin. */
    bool    active_high;  /* If true, the pin will be active high. Otherwise, the pin will be active low. */
    uint8_t gpio_pin;     /* GPIO pin number for the pin. */
    uint8_t gpiote_ch_id; /* GPIOTE channel to be used for toggling pins. */
} nrf_fem_gpiote_pin_config_t;

/**
 * @brief Configuration parameters for the PA/LNA interface.
 */
typedef struct
{
    struct
    {
        uint32_t pa_time_gap_us;                /* Time between the activation of the PA pin and the start of the radio transmission. */
        uint32_t lna_time_gap_us;               /* Time between the activation of the LNA pin and the start of the radio reception. */
        uint32_t pdn_settle_us;                 /* The time between activating the PDN and asserting the PA/LNA pin. */
        uint32_t trx_hold_us;                   /* The time between deasserting the PA/LNA pin and deactivating PDN. */
        int8_t   pa_gain_db;                    /* Configurable PA gain. Ignored if the amplifier is not supporting this feature. */
        int8_t   lna_gain_db;                   /* Configurable LNA gain. Ignored if the amplifier is not supporting this feature. */
    }                           fem_config;

    nrf_fem_gpiote_pin_config_t pa_pin_config;  /* Power Amplifier pin configuration. */
    nrf_fem_gpiote_pin_config_t lna_pin_config; /* Low Noise Amplifier pin configuration. */
    nrf_fem_gpiote_pin_config_t pdn_pin_config; /* Power Down pin configuration. */

    uint8_t                     ppi_ch_id_set;  /* PPI channel to be used for setting pins. */
    uint8_t                     ppi_ch_id_clr;  /* PPI channel to be used for clearing pins. */
    uint8_t                     ppi_ch_id_pdn;  /* PPI channel to handle PDN pin. */
} nrf_fem_interface_config_t;

/**
 * @brief Mode of the antenna diversity module.
 *
 * Possible values:
 * - @ref NRF_802154_ANT_DIVERSITY_MODE_DISABLED,
 * - @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL,
 * - @ref NRF_802154_ANT_DIVERSITY_MODE_AUTO
 */
typedef uint8_t nrf_802154_ant_diversity_mode_t;

#define NRF_802154_ANT_DIVERSITY_MODE_DISABLED 0x00 // !< Antenna diversity is disabled - Antenna will not be controlled by ant_diversity module. While in this mode, current antenna is unspecified.
#define NRF_802154_ANT_DIVERSITY_MODE_MANUAL   0x01 // !< Antenna is selected manually
#define NRF_802154_ANT_DIVERSITY_MODE_AUTO     0x02 // !< Antenna is selected automatically based on RSSI - not supported for transmission.

/**
 * @brief Available antennas
 *
 * Possible values:
 * - @ref NRF_802154_ANT_DIVERSITY_ANTENNA_1,
 * - @ref NRF_802154_ANT_DIVERSITY_ANTENNA_2,
 * - @ref NRF_802154_ANT_DIVERSITY_ANTENNA_NONE
 */
typedef uint8_t nrf_802154_ant_diversity_antenna_t;

#define NRF_802154_ANT_DIVERSITY_ANTENNA_1    0x00 // !< First antenna
#define NRF_802154_ANT_DIVERSITY_ANTENNA_2    0x01 // !< Second antenna
#define NRF_802154_ANT_DIVERSITY_ANTENNA_NONE 0x02 // !< Used to indicate that antenna for the last reception was not selected via antenna diversity algorithm.

/**
 * Default antenna used in cases where no antenna was specified.
 */
#ifndef NRF_802154_ANT_DIVERSITY_DEFAULT_ANTENNA
#define NRF_802154_ANT_DIVERSITY_DEFAULT_ANTENNA NRF_802154_ANT_DIVERSITY_ANTENNA_1

/**
 * @brief Mode of triggering receive request to Coex arbiter.
 *
 * Possible values:
 * - @ref NRF_802154_COEX_RX_REQUEST_MODE_ENERGY_DETECTION,
 * - @ref NRF_802154_COEX_RX_REQUEST_MODE_PREAMBLE,
 * - @ref NRF_802154_COEX_RX_REQUEST_MODE_DESTINED
 */
typedef uint8_t nrf_802154_coex_rx_request_mode_t;

#define NRF_802154_COEX_RX_REQUEST_MODE_ENERGY_DETECTION 0x01 // !< Coex requests to arbiter in receive mode upon energy detected.
#define NRF_802154_COEX_RX_REQUEST_MODE_PREAMBLE         0x02 // !< Coex requests to arbiter in receive mode upon preamble reception.
#define NRF_802154_COEX_RX_REQUEST_MODE_DESTINED         0x03 // !< Coex requests to arbiter in receive mode upon detection that frame is addressed to this device.

/**
 * @brief Mode of triggering transmit request to Coex arbiter.
 *
 * Possible values:
 * - @ref NRF_802154_COEX_TX_REQUEST_MODE_FRAME_READY,
 * - @ref NRF_802154_COEX_TX_REQUEST_MODE_CCA_START,
 * - @ref NRF_802154_COEX_TX_REQUEST_MODE_CCA_DONE
 */
typedef uint8_t nrf_802154_coex_tx_request_mode_t;

#define NRF_802154_COEX_TX_REQUEST_MODE_FRAME_READY 0x01 // !< Coex requests to arbiter in transmit mode when the frame is ready to be transmitted.
#define NRF_802154_COEX_TX_REQUEST_MODE_CCA_START   0x02 // !< Coex requests to arbiter in transmit mode before CCA is started.
#define NRF_802154_COEX_TX_REQUEST_MODE_CCA_DONE    0x03 // !< Coex requests to arbiter in transmit mode after CCA is finished.

static inline int32_t nrf_fem_interface_configuration_set(nrf_fem_interface_config_t const * const p_config)
{
    (void)p_config;
    return 0;
}

/**
 * @brief Configuration of the antenna diversity module.
 *
 */
typedef struct
{
    uint8_t ant_sel_pin; // !< Pin used for antenna selection. Should not be changed after calling @ref nrf_802154_ant_diversity_init.
    uint8_t toggle_time; // !< Time between antenna switches in automatic mode [us].
} nrf_802154_ant_diversity_config_t;

/**
 * @brief Sets the antenna diversity rx mode.
 *
 * @note This function should not be called while reception or transmission are currently ongoing.
 *
 * @param[in] mode Antenna diversity rx mode to be set.
 *
 * @retval true  Antenna diversity rx mode set successfully.
 * @retval false Invalid mode passed as argument.
 */
static inline bool nrf_802154_antenna_diversity_rx_mode_set(nrf_802154_ant_diversity_mode_t mode)
{
    (void)mode;
    return true;
}

/**
 * @brief Gets current antenna diversity rx mode.
 *
 * @return Current antenna diversity mode for rx.
 */
static inline nrf_802154_ant_diversity_mode_t nrf_802154_antenna_diversity_rx_mode_get(void)
{
    return NRF_802154_ANT_DIVERSITY_MODE_DISABLED;
}

/**
 * @brief Sets the antenna diversity tx mode.
 *
 * @note This function should not be called while reception or transmission are currently ongoing.
 * @note NRF_802154_ANT_DIVERSITY_MODE_AUTO is not supported for transmission.
 *
 * @param[in] mode Antenna diversity tx mode to be set.
 *
 * @retval true  Antenna diversity tx mode set successfully.
 * @retval false Invalid mode passed as argument.
 */
static inline bool nrf_802154_antenna_diversity_tx_mode_set(nrf_802154_ant_diversity_mode_t mode)
{
    (void)mode;
    return false;
}

/**
 * @brief Gets current antenna diversity tx mode.
 *
 * @return Current antenna diversity mode for tx.
 */
static inline nrf_802154_ant_diversity_mode_t nrf_802154_antenna_diversity_tx_mode_get(void)
{
    return NRF_802154_ANT_DIVERSITY_MODE_DISABLED;
}

/**
 * @brief Manually selects the antenna to be used for rx.
 *
 * For antenna to be switched, antenna diversity rx mode needs
 * to be @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL. Otherwise, antenna will
 * be only switched after @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL is set.
 *
 * @param[in] antenna Antenna to be used.
 *
 * @retval true  Antenna set successfully.
 * @retval false Invalid antenna passed as argument.
 */
static inline bool nrf_802154_antenna_diversity_rx_antenna_set(nrf_802154_ant_diversity_antenna_t antenna)
{
    (void)antenna;
    return false;
}

/**
 * @brief Gets antenna currently used for rx.
 *
 * @note The antenna read by this function is currently used rx antenna only if
 * antenna diversity rx mode is set to @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL. Otherwise,
 * currently used antenna may be different.
 * @sa nrf_802154_ant_diversity_mode_set
 *
 * @return Currently used antenna.
 */
static inline nrf_802154_ant_diversity_antenna_t nrf_802154_antenna_diversity_rx_antenna_get(void)
{
    return NRF_802154_ANT_DIVERSITY_ANTENNA_1;
}

/**
 * @brief Manually selects the antenna to be used for tx.
 *
 * For antenna to be switched, antenna diversity tx mode needs
 * to be @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL. Otherwise, antenna will
 * be only switched after @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL is set.
 *
 * @param[in] antenna Antenna to be used.
 *
 * @retval true  Antenna set successfully.
 * @retval false Invalid antenna passed as argument.
 */
static inline bool nrf_802154_antenna_diversity_tx_antenna_set(nrf_802154_ant_diversity_antenna_t antenna)
{
    (void)antenna;
    return false;
}

/**
 * @brief Gets antenna currently used for tx.
 *
 * @note The antenna read by this function is currently used tx antenna only if
 * antenna diversity tx mode is set to @ref NRF_802154_ANT_DIVERSITY_MODE_MANUAL. Otherwise,
 * currently used antenna may be different.
 * @sa nrf_802154_ant_diversity_mode_set
 *
 * @return Currently used antenna.
 */
static inline nrf_802154_ant_diversity_antenna_t nrf_802154_antenna_diversity_tx_antenna_get(void)
{
    return NRF_802154_ANT_DIVERSITY_ANTENNA_1;
}

/**
 * @brief Gets which antenna was selected as best for the last reception.
 *
 * @note In three cases @ref NRF_802154_ANT_DIVERSITY_ANTENNA_NONE may be returned:
 *  - No frame was received yet.
 *  - Last frame was received with antenna diversity auto mode disabled.
 *  - RSSI measurements didn't have enough time to finish during last frame reception
 *    and it is unspecified which antenna was selected.
 *
 * @return Antenna selected during last successful reception in automatic mode.
 */
static inline nrf_802154_ant_diversity_antenna_t nrf_802154_antenna_diversity_last_rx_best_antenna_get(void)
{
    return NRF_802154_ANT_DIVERSITY_ANTENNA_1;
}

/**
 * @brief Sets antenna diversity configuration.
 *
 * If configuration other than default is required, this should be called
 * before @ref nrf_802154_antenna_diversity_init.
 *
 * @param[in] config Configuration of antenna diversity module to be set.
 */
static inline void nrf_802154_antenna_diversity_config_set(nrf_802154_ant_diversity_config_t config)
{
    (void)config;
}

/**
 * @brief Gets current antenna diversity configuration.
 *
 * @return Configuration of antenna diversity module.
 */
static inline nrf_802154_ant_diversity_config_t nrf_802154_antenna_diversity_config_get(void)
{
    static nrf_802154_ant_diversity_config_t cfg;
    return cfg;
}

/**
 * @brief Initializes antenna diversity module.
 *
 * This function should be called before starting radio operations, but at any time
 * after driver initialization. Also, if ant_sel pin other than default is required,
 * pin configuration should be set beforehand. See @ref nrf_802154_antenna_config_set.
 * Example:
 * @code
 * nrf_802154_init();
 * // If pin configuration is required
 * nrf_802154_ant_diversity_config_t cfg = nrf_802154_antenna_config_get();
 * cfg.ant_sel_pin = ANT_SEL_PIN;
 * nrf_802154_antenna_config_set(cfg);
 * // Pin configuration end
 * nrf_802154_antenna_diversity_init();
 * // At any later time
 * nrf_802154_receive();
 * @endcode
 */
static inline void nrf_802154_antenna_diversity_init(void)
{

}
#endif // ENABLE_ANT_DIVERSITY


/**
 * @brief Enables wifi coex signaling.
 *
 * When @ref nrf_802154_init is called, the wifi coex signaling is initially enabled or disabled
 * depending on @ref NRF_802154_COEX_INITIALLY_ENABLED. You can call this function
 * (after @ref nrf_802154_init) to enable the wifi coex signaling. When wifi coex signaling
 * has been already enabled, this function has no effect.
 *
 * When this function is called during receive or transmit operation, the effect on coex interface
 * may be delayed until current frame (or ack) is received or transmitted.
 * To avoid this issue please call this function when the driver is in sleep mode.
 *
 * @retval true     Wifi coex is supported and is enabled after call to this function.
 * @retval false    Wifi coex is not supported.
 */
static inline bool nrf_802154_wifi_coex_enable(void)
{
    return true;
}

/**
 * @brief Disables wifi coex signaling.
 *
 * You can call this function (after @ref nrf_802154_init) to disable the wifi coex signaling.
 * When wifi coex signaling has been already disabled, this function has no effect.
 *
 * When this function is called during receive or transmit operation, the effect on coex interface
 * may be delayed until current frame (or ack) is received or transmitted.
 * To avoid this issue please call this function when the driver is in sleep mode.
 */
static inline void nrf_802154_wifi_coex_disable(void)
{
}

/**
 * @brief Checks if wifi coex signaling is enabled.
 *
 * @retval true     Wifi coex signaling is enabled.
 * @retval false    Wifi coex signaling is disabled.
 */
static inline bool nrf_802154_wifi_coex_is_enabled(void)
{
    return true;
}

/**
 * @brief Sets Coex request mode used in receive operations.
 *
 * @param[in] mode  Coex receive request mode. For allowed values see @ref nrf_802154_coex_rx_request_mode_t type.
 *
 * @retval true     Operation succeeded.
 * @retval false    Requested mode is not supported.
 */
static inline bool nrf_802154_coex_rx_request_mode_set(nrf_802154_coex_rx_request_mode_t mode)
{
    (void)NRF_802154_COEX_RX_REQUEST_MODE_ENERGY_DETECTION;
    return false;
}

/**
 * @brief Gets Coex request mode used in receive operations.
 *
 * @return Current Coex receive request mode. For allowed values see @ref nrf_802154_coex_rx_request_mode_t type.
 */
static inline nrf_802154_coex_rx_request_mode_t nrf_802154_coex_rx_request_mode_get(void)
{
    return NRF_802154_COEX_RX_REQUEST_MODE_ENERGY_DETECTION;
}

/**
 * @brief Sets Coex request mode used in transmit operations.
 *
 * @param[in] mode  Coex transmit request mode. For allowed values see @ref nrf_802154_coex_tx_request_mode_t type.
 *
 * @retval true     Operation succeeded.
 * @retval false    Requested mode is not supported.
 */
static inline bool nrf_802154_coex_tx_request_mode_set(nrf_802154_coex_tx_request_mode_t mode)
{
    (void)mode;
    return true;
}

/**
 * @brief Gets Coex request mode used in transmit operations.
 *
 * @return Current Coex transmit request mode. For allowed values see @ref nrf_802154_coex_tx_request_mode_t type.
 */
static inline nrf_802154_coex_tx_request_mode_t nrf_802154_coex_tx_request_mode_get(void)
{
    return NRF_802154_COEX_TX_REQUEST_MODE_FRAME_READY;
}