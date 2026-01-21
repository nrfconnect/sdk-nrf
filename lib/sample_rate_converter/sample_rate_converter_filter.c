/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sample_rate_converter.h"
#include "sample_rate_converter_filter.h"

#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_rate_converter_filter, CONFIG_SAMPLE_RATE_CONVERTER_LOG_LEVEL);

/**
 * The following requirements are set for the filter types:
 * - 12 kHz and 8 kHz cut-offs at 48 kHz sampling.
 * - Coefficients in 16- and 32-bit formats.
 * - Filters should have a gain of 2 and 3 for 24 kHz <-> 48 kHz and 16 kHz <-> 48 kHz
 *   respectively.
 * - All filters for a type should have the same number of taps, and the number of taps must be
 *   divisible by the conversio ratio (2 or 3).
 */

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST
#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
static const q15_t filter_48khz_to_24khz_16bit_test[] = {0x3fff, 0x3fff};

static const q15_t filter_24khz_to_48khz_16bit_test[] = {0x7fff, 0x7fff};

static const q15_t filter_48khz_to_16khz_16bit_test[] = {0x2aaa, 0x2aaa, 0x2aaa};

static const q15_t filter_16khz_to_48khz_16bit_test[] = {0x7fff, 0x7fff, 0x7fff};
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32

static const q31_t filter_48khz_to_24khz_32bit_test[] = {0x3fffffff, 0x3fffffff};

static const q31_t filter_24khz_to_48khz_32bit_test[] = {0x7fffffff, 0x7fffffff};

static const q31_t filter_48khz_to_16khz_32bit_test[] = {0x2aaaaaaa, 0x2aaaaaaa, 0x2aaaaaaa};

static const q31_t filter_16khz_to_48khz_32bit_test[] = {0x7fffffff, 0x7fffffff, 0x7fffffff};
#endif
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST */

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_FILTER_SIMPLE
#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
static const q15_t filter_48khz_to_16khz_16bit_simple[] = {
	0xFF53, 0xFE59, 0xFFF6, 0x002C, 0x00BE, 0x004F, 0xFF93, 0xFF27, 0xFF9B, 0x008D, 0x010E,
	0x0080, 0xFF52, 0xFEAF, 0xFF5E, 0x00D7, 0x01A6, 0x00CF, 0xFEF4, 0xFDEA, 0xFEF4, 0x0152,
	0x02B3, 0x0164, 0xFE47, 0xFC5F, 0xFE10, 0x0263, 0x0540, 0x02F6, 0xFC44, 0xF70E, 0xFA52,
	0x0837, 0x1B20, 0x28A4, 0x28A4, 0x1B20, 0x0837, 0xFA52, 0xF70E, 0xFC44, 0x02F6, 0x0540,
	0x0263, 0xFE10, 0xFC5F, 0xFE47, 0x0164, 0x02B3, 0x0152, 0xFEF4, 0xFDEA, 0xFEF4, 0x00CF,
	0x01A6, 0x00D7, 0xFF5E, 0xFEAF, 0xFF52, 0x0080, 0x010E, 0x008D, 0xFF9B, 0xFF27, 0xFF93,
	0x004F, 0x00BE, 0x002C, 0xFFF6, 0xFE59, 0xFF53};

static const q15_t filter_16khz_to_48khz_16bit_simple[] = {
	0xFFC6, 0xFFA7, 0xFFDE, 0x0091, 0x0159, 0x0178, 0x008F, 0xFF39, 0xFEBC, 0xFFC5, 0x0175,
	0x01F4, 0x0052, 0xFDEA, 0xFD5C, 0xFFC7, 0x0311, 0x0391, 0xFFFE, 0xFB90, 0xFB48, 0x0082,
	0x066E, 0x0637, 0xFE88, 0xF688, 0xF7AF, 0x034D, 0x0EAB, 0x0BD3, 0xF8B4, 0xE5D9, 0xEBA1,
	0x14D3, 0x5125, 0x7D8C, 0x7D8C, 0x5125, 0x14D3, 0xEBA1, 0xE5D9, 0xF8B4, 0x0BD3, 0x0EAB,
	0x034D, 0xF7AF, 0xF688, 0xFE88, 0x0637, 0x066E, 0x0082, 0xFB48, 0xFB90, 0xFFFE, 0x0391,
	0x0311, 0xFFC7, 0xFD5C, 0xFDEA, 0x0052, 0x01F4, 0x0175, 0xFFC5, 0xFEBC, 0xFF39, 0x008F,
	0x0178, 0x0159, 0x0091, 0xFFDE, 0xFFA7, 0xFFC6};

static const q15_t filter_48khz_to_24khz_16bit_simple[] = {
	0xFDC1, 0x007B, 0x01AE, 0x011B, 0xFFD0, 0x0025, 0x013E, 0x00A4, 0xFF3C, 0xFFC0, 0x013D,
	0x009D, 0xFED7, 0xFF72, 0x0171, 0x00C9, 0xFE76, 0xFF1E, 0x01CB, 0x011F, 0xFDFC, 0xFEA5,
	0x0259, 0x01B4, 0xFD41, 0xFDD8, 0x0351, 0x02D4, 0xFBD7, 0xFC1C, 0x0598, 0x05E4, 0xF761,
	0xF4F6, 0x13A1, 0x392B, 0x392B, 0x13A1, 0xF4F6, 0xF761, 0x05E4, 0x0598, 0xFC1C, 0xFBD7,
	0x02D4, 0x0351, 0xFDD8, 0xFD41, 0x01B4, 0x0259, 0xFEA5, 0xFDFC, 0x011F, 0x01CB, 0xFF1E,
	0xFE76, 0x00C9, 0x0171, 0xFF72, 0xFED7, 0x009D, 0x013D, 0xFFC0, 0xFF3C, 0x00A4, 0x013E,
	0x0025, 0xFFD0, 0x011B, 0x01AE, 0x007B, 0xFDC1};

static const q15_t filter_24khz_to_48khz_16bit_simple[] = {
	0xFE9C, 0x0485, 0x0556, 0x01ED, 0xFE40, 0xFF4D, 0x01F1, 0x00AA, 0xFDE6, 0xFF48, 0x0253,
	0x00DA, 0xFD68, 0xFEF6, 0x02E7, 0x0149, 0xFCBD, 0xFE67, 0x03AF, 0x01FE, 0xFBCC, 0xFD80,
	0x04D5, 0x0329, 0xFA55, 0xFBEB, 0x06CE, 0x056C, 0xF77F, 0xF872, 0x0B61, 0x0B8E, 0xEE8E,
	0xEA24, 0x2778, 0x721E, 0x721E, 0x2778, 0xEA24, 0xEE8E, 0x0B8E, 0x0B61, 0xF872, 0xF77F,
	0x056C, 0x06CE, 0xFBEB, 0xFA55, 0x0329, 0x04D5, 0xFD80, 0xFBCC, 0x01FE, 0x03AF, 0xFE67,
	0xFCBD, 0x0149, 0x02E7, 0xFEF6, 0xFD68, 0x00DA, 0x0253, 0xFF48, 0xFDE6, 0x00AA, 0x01F1,
	0xFF4D, 0xFE40, 0x01ED, 0x0556, 0x0485, 0xFE9C};

#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
static const q31_t filter_48khz_to_16khz_32bit_simple[] = {
	0xFF52FC0F, 0xFE591B38, 0xFFF5BFB1, 0x002C1C33, 0x00BDDABD, 0x004E94D1, 0xFF92D24B,
	0xFF2714F2, 0xFF9ACB5E, 0x008CE88E, 0x010DD3B2, 0x007FB917, 0xFF51BA2B, 0xFEAF3FF4,
	0xFF5DC5DD, 0x00D7329C, 0x01A5DFD2, 0x00CF2E69, 0xFEF45F08, 0xFDE9FA34, 0xFEF3BBA1,
	0x01522151, 0x02B2EC44, 0x01643E3E, 0xFE4690BC, 0xFC5F4637, 0xFE1015E0, 0x026312A2,
	0x054003AF, 0x02F5D8E6, 0xFC4477DD, 0xF70E53B6, 0xFA51A0B5, 0x0836D87D, 0x1B2038D8,
	0x28A4084F, 0x28A4084F, 0x1B2038D8, 0x0836D87D, 0xFA51A0B5, 0xF70E53B6, 0xFC4477DD,
	0x02F5D8E6, 0x054003AF, 0x026312A2, 0xFE1015E0, 0xFC5F4637, 0xFE4690BC, 0x01643E3E,
	0x02B2EC44, 0x01522151, 0xFEF3BBA1, 0xFDE9FA34, 0xFEF45F08, 0x00CF2E69, 0x01A5DFD2,
	0x00D7329C, 0xFF5DC5DD, 0xFEAF3FF4, 0xFF51BA2B, 0x007FB917, 0x010DD3B2, 0x008CE88E,
	0xFF9ACB5E, 0xFF2714F2, 0xFF92D24B, 0x004E94D1, 0x00BDDABD, 0x002C1C33, 0xFFF5BFB1,
	0xFE591B38, 0xFF52FC0F};

static const q31_t filter_16khz_to_48khz_32bit_simple[] = {
	0xFFC5DFF7, 0xFFA6C169, 0xFFDE4FCC, 0x0090C12C, 0x0158D9E7, 0x0177E99B, 0x008E9CB2,
	0xFF393A7C, 0xFEBBC52E, 0xFFC4C47C, 0x01756B4F, 0x01F3B574, 0x00527E6D, 0xFDEA38C0,
	0xFD5B96FC, 0xFFC6B87B, 0x031112F6, 0x0390CF1C, 0xFFFE61C4, 0xFB8F901E, 0xFB485FFE,
	0x00824CAB, 0x066E301E, 0x0636DCB2, 0xFE883DFC, 0xF6882EAA, 0xF7AE91C8, 0x034D7AD9,
	0x0EAB5347, 0x0BD3421E, 0xF8B3BFC8, 0xE5D89E49, 0xEBA113C3, 0x14D2F78E, 0x512570D0,
	0x7D8C5277, 0x7D8C5277, 0x512570D0, 0x14D2F78E, 0xEBA113C3, 0xE5D89E49, 0xF8B3BFC8,
	0x0BD3421E, 0x0EAB5347, 0x034D7AD9, 0xF7AE91C8, 0xF6882EAA, 0xFE883DFC, 0x0636DCB2,
	0x066E301E, 0x00824CAB, 0xFB485FFE, 0xFB8F901E, 0xFFFE61C4, 0x0390CF1C, 0x031112F6,
	0xFFC6B87B, 0xFD5B96FC, 0xFDEA38C0, 0x00527E6D, 0x01F3B574, 0x01756B4F, 0xFFC4C47C,
	0xFEBBC52E, 0xFF393A7C, 0x008E9CB2, 0x0177E99B, 0x0158D9E7, 0x0090C12C, 0xFFDE4FCC,
	0xFFA6C169, 0xFFC5DFF7};

static const q31_t filter_48khz_to_24khz_32bit_simple[] = {
	0xFDC09FDD, 0x007A9B8F, 0x01AE2DDB, 0x011AF5CF, 0xFFD0140F, 0x00250EA6, 0x013DE420,
	0x00A3E126, 0xFF3B8666, 0xFFBF8005, 0x013D5C64, 0x009CE19A, 0xFED7368E, 0xFF726610,
	0x0170C7D5, 0x00C8F021, 0xFE767D64, 0xFF1DCFF1, 0x01CAB667, 0x011F57EE, 0xFDFBB77F,
	0xFEA54CBC, 0x0258C9E4, 0x01B4047D, 0xFD413B0B, 0xFDD85C3B, 0x035147B9, 0x02D3D10C,
	0xFBD6E9EC, 0xFC1BA8BF, 0x0597F613, 0x05E38531, 0xF7614B82, 0xF4F610AC, 0x13A1373A,
	0x392AD467, 0x392AD467, 0x13A1373A, 0xF4F610AC, 0xF7614B82, 0x05E38531, 0x0597F613,
	0xFC1BA8BF, 0xFBD6E9EC, 0x02D3D10C, 0x035147B9, 0xFDD85C3B, 0xFD413B0B, 0x01B4047D,
	0x0258C9E4, 0xFEA54CBC, 0xFDFBB77F, 0x011F57EE, 0x01CAB667, 0xFF1DCFF1, 0xFE767D64,
	0x00C8F021, 0x0170C7D5, 0xFF726610, 0xFED7368E, 0x009CE19A, 0x013D5C64, 0xFFBF8005,
	0xFF3B8666, 0x00A3E126, 0x013DE420, 0x00250EA6, 0xFFD0140F, 0x011AF5CF, 0x01AE2DDB,
	0x007A9B8F, 0xFDC09FDD};

static const q31_t filter_24khz_to_48khz_32bit_simple[] = {
	0xFE9C476A, 0x048567B3, 0x05566C5A, 0x01ED1027, 0xFE406E21, 0xFF4CA985, 0x01F1222A,
	0x00A98B53, 0xFDE60F06, 0xFF484E6B, 0x02535778, 0x00DA35E1, 0xFD67C70C, 0xFEF5C83F,
	0x02E7015B, 0x01489A69, 0xFCBC9695, 0xFE66EA75, 0x03AF7B41, 0x01FD8A0F, 0xFBCC4389,
	0xFD7FE2EF, 0x04D57444, 0x03296A60, 0xFA550A4B, 0xFBEABD25, 0x06CE728E, 0x056C09E3,
	0xF77F1181, 0xF871AFFE, 0x0B61674C, 0x0B8DAE0F, 0xEE8DEA8D, 0xEA23FBCD, 0x27784848,
	0x721E6BC2, 0x721E6BC2, 0x27784848, 0xEA23FBCD, 0xEE8DEA8D, 0x0B8DAE0F, 0x0B61674C,
	0xF871AFFE, 0xF77F1181, 0x056C09E3, 0x06CE728E, 0xFBEABD25, 0xFA550A4B, 0x03296A60,
	0x04D57444, 0xFD7FE2EF, 0xFBCC4389, 0x01FD8A0F, 0x03AF7B41, 0xFE66EA75, 0xFCBC9695,
	0x01489A69, 0x02E7015B, 0xFEF5C83F, 0xFD67C70C, 0x00DA35E1, 0x02535778, 0xFF484E6B,
	0xFDE60F06, 0x00A98B53, 0x01F1222A, 0xFF4CA985, 0xFE406E21, 0x01ED1027, 0x05566C5A,
	0x048567B3, 0xFE9C476A};
#endif
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_SIMPLE */

enum filter_conversion_ratio {
	CONVERSION_48KHZ_TO_16KHZ = -3,
	CONVERSION_48KHZ_TO_24KHZ = -2,
	CONVERSION_24KHZ_TO_48KHZ = 2,
	CONVERSION_16KHZ_TO_48KHZ = 3
};

int sample_rate_converter_filter_get(enum sample_rate_converter_filter filter_type,
				     int conversion_ratio, void const **filter_ptr,
				     size_t *filter_size)
{

	__ASSERT(filter_ptr != NULL, "Filter pointer cannot be NULL");
	__ASSERT(filter_size != NULL, "Filter size pointer cannot be NULL");

	/* Filters only support the following conversions:
	 * - 16 kHz <-> 48 kHz
	 * - 24 kHz <-> 48kHz
	 * which means that the supported ratio is limited to 2 and 3.
	 */
	if ((abs(conversion_ratio) != 2) && (abs(conversion_ratio) != 3)) {
		LOG_ERR("Invalid conversion ratio: %d", conversion_ratio);
		return -EINVAL;
	}

#if CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
	switch (filter_type) {
#if CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST
	case SAMPLE_RATE_FILTER_TEST:
		LOG_WRN("Test filter for the sample rate converter has been selected, do NOT use "
			"in final implementation.");
		switch (conversion_ratio) {
		case CONVERSION_48KHZ_TO_24KHZ:
			*filter_ptr = filter_48khz_to_24khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_24khz_16bit_test);
			break;

		case CONVERSION_24KHZ_TO_48KHZ:
			*filter_ptr = filter_24khz_to_48khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_24khz_to_48khz_16bit_test);
			break;

		case CONVERSION_48KHZ_TO_16KHZ:
			*filter_ptr = filter_48khz_to_16khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_16khz_16bit_test);
			break;

		case CONVERSION_16KHZ_TO_48KHZ:
			*filter_ptr = filter_16khz_to_48khz_16bit_test;
			*filter_size = ARRAY_SIZE(filter_16khz_to_48khz_16bit_test);
			break;

		default:
			LOG_ERR("No matching filter for filter type 'test' and conversion rate %d",
				conversion_ratio);
			return -EINVAL;
		}
		break;
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST */
#if CONFIG_SAMPLE_RATE_CONVERTER_FILTER_SIMPLE
	case SAMPLE_RATE_FILTER_SIMPLE:
		switch (conversion_ratio) {
		case CONVERSION_48KHZ_TO_24KHZ:
			*filter_ptr = filter_48khz_to_24khz_16bit_simple;
			*filter_size = ARRAY_SIZE(filter_48khz_to_24khz_16bit_simple);
			break;

		case CONVERSION_24KHZ_TO_48KHZ:
			*filter_ptr = filter_24khz_to_48khz_16bit_simple;
			*filter_size = ARRAY_SIZE(filter_24khz_to_48khz_16bit_simple);
			break;

		case CONVERSION_48KHZ_TO_16KHZ:
			*filter_ptr = filter_48khz_to_16khz_16bit_simple;
			*filter_size = ARRAY_SIZE(filter_48khz_to_16khz_16bit_simple);
			break;

		case CONVERSION_16KHZ_TO_48KHZ:
			*filter_ptr = filter_16khz_to_48khz_16bit_simple;
			*filter_size = ARRAY_SIZE(filter_16khz_to_48khz_16bit_simple);
			break;

		default:
			LOG_ERR("No matching filter for filter type 'simple' and conversion rate "
				"%d",
				conversion_ratio);
			return -EINVAL;
		}
		break;
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_SIMPLE */
	default:
		LOG_ERR("No matching filter for type %d found", filter_type);
		return -EINVAL;
	}
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16 */

#if CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
	switch (filter_type) {
#if CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST
	case SAMPLE_RATE_FILTER_TEST:
		switch (conversion_ratio) {
		case CONVERSION_48KHZ_TO_24KHZ:
			*filter_ptr = filter_48khz_to_24khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_24khz_32bit_test);
			break;

		case CONVERSION_24KHZ_TO_48KHZ:
			*filter_ptr = filter_24khz_to_48khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_24khz_to_48khz_32bit_test);
			break;

		case CONVERSION_48KHZ_TO_16KHZ:
			*filter_ptr = filter_48khz_to_16khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_48khz_to_16khz_32bit_test);
			break;

		case CONVERSION_16KHZ_TO_48KHZ:
			*filter_ptr = filter_16khz_to_48khz_32bit_test;
			*filter_size = ARRAY_SIZE(filter_16khz_to_48khz_32bit_test);
			break;

		default:
			LOG_ERR("No matching filter for filter type 'test' and conversion rate %d",
				conversion_ratio);
			return -EINVAL;
		}
		break;
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_TEST */
#if CONFIG_SAMPLE_RATE_CONVERTER_FILTER_SIMPLE
	case SAMPLE_RATE_FILTER_SIMPLE:
		switch (conversion_ratio) {
		case CONVERSION_48KHZ_TO_24KHZ:
			*filter_ptr = filter_48khz_to_24khz_32bit_simple;
			*filter_size = ARRAY_SIZE(filter_48khz_to_24khz_32bit_simple);
			break;

		case CONVERSION_24KHZ_TO_48KHZ:
			*filter_ptr = filter_24khz_to_48khz_32bit_simple;
			*filter_size = ARRAY_SIZE(filter_24khz_to_48khz_32bit_simple);
			break;

		case CONVERSION_48KHZ_TO_16KHZ:
			*filter_ptr = filter_48khz_to_16khz_32bit_simple;
			*filter_size = ARRAY_SIZE(filter_48khz_to_16khz_32bit_simple);
			break;

		case CONVERSION_16KHZ_TO_48KHZ:
			*filter_ptr = filter_16khz_to_48khz_32bit_simple;
			*filter_size = ARRAY_SIZE(filter_16khz_to_48khz_32bit_simple);
			break;

		default:
			LOG_ERR("No matching filter for filter type 'simple' and conversion rate "
				"%d",
				conversion_ratio);
			return -EINVAL;
		}
		break;
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_FILTER_SIMPLE */
	default:
		LOG_ERR("No matching filter for type %d found", filter_type);
		return -EINVAL;
	}
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16 */
	return 0;
}
