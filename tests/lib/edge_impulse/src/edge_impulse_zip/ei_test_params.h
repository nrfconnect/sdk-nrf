/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EI_TEST_PARAMS_H_
#define _EI_TEST_PARAMS_H_

#include <zephyr/kernel.h>


/* Float comparison tolerance. */
#define FLOAT_CMP_EPSILON			0.000001

/* Definitions provided by the EI library. */
#define EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME	15
#define EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE	300
#define EI_CLASSIFIER_HAS_ANOMALY		1
#define EI_CLASSIFIER_FREQUENCY			60

/* Mocked results. */
static const char * const ei_classifier_inferencing_categories[] = {
	"label1", "label2", "label3", "label4"
};
#define EI_CLASSIFIER_LABEL_COUNT	((int)ARRAY_SIZE(ei_classifier_inferencing_categories))

/* Definitions for input and return values of the mocked library. */
#define EI_MOCK_GEN_FIRST_INPUT(PRED_IDX) \
	((float)((PRED_IDX) * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME))

#define EI_MOCK_GEN_LABEL_IDX(PRED_IDX) ((PRED_IDX) % EI_CLASSIFIER_LABEL_COUNT)
#define EI_MOCK_GEN_LABEL(PRED_IDX)	\
	(ei_classifier_inferencing_categories[EI_MOCK_GEN_LABEL_IDX(PRED_IDX)])
#define EI_MOCK_GEN_VALUE(PRED_IDX)	(0.5 + ((PRED_IDX) * 0.001))
#define EI_MOCK_GEN_VALUE_OTHERS(PRED_IDX) ((1.0 - EI_MOCK_GEN_VALUE(PRED_IDX)) / \
					   (EI_CLASSIFIER_LABEL_COUNT - 1))
#define EI_MOCK_GEN_ANOMALY(PRED_IDX)	((float)(PRED_IDX))

#define EI_MOCK_GEN_DSP_TIME(PRED_IDX)			((int)(PRED_IDX) + 1)
#define EI_MOCK_GEN_CLASSIFICATION_TIME(PRED_IDX)	((int)(PRED_IDX) + 2)
#define EI_MOCK_GEN_ANOMALY_TIME(PRED_IDX)		((int)(PRED_IDX) + 3)

/* Data processing is simulated as busy wait. */
#define EI_MOCK_BUSY_WAIT_TIME				(100U)

#endif /* _EI_TEST_PARAMS_H_ */
