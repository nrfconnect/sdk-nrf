#include <ztest.h>
#include <device.h>

int i2c_reg_read_byte(const struct device *dev, uint16_t dev_addr, uint8_t reg_addr, uint8_t *value)
{
	ztest_check_expected_value(reg_addr);
	ztest_check_expected_value(dev_addr);
	*value = ztest_get_return_value();

	return 0;
}

int i2c_reg_write_byte(const struct device *dev, uint16_t dev_addr, uint8_t reg_addr, uint8_t value)
{
	ztest_check_expected_value(reg_addr);
	ztest_check_expected_value(dev_addr);
	ztest_check_expected_value(value);

	return 0;
}
