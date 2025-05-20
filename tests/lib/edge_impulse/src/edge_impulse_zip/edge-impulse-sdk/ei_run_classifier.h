/* Mocked header of the Edge Impulse library. Used for unit tests. */

#ifndef _EI_RUN_CLASSIFIER_H_
#define _EI_RUN_CLASSIFIER_H_

#include <ei_test_params.h>


/* Mock data types. */
typedef struct {
	const char *label;
	float value;
} ei_impulse_result_classification_t;

typedef struct {
	int sampling;
	int dsp;
	int classification;
	int anomaly;
} ei_impulse_result_timing_t;

typedef struct {
	ei_impulse_result_classification_t classification[EI_CLASSIFIER_LABEL_COUNT];
	float anomaly;
	ei_impulse_result_timing_t timing;
} ei_impulse_result_t;

typedef struct ei_signal_t {
	int (*get_data)(size_t, size_t, float *);
	size_t total_length;
} signal_t;

typedef enum {
	EI_IMPULSE_OK = 0,
	EI_IMPULSE_ERROR_SHAPES_DONT_MATCH = -1,
	EI_IMPULSE_CANCELED = -2,
	EI_IMPULSE_TFLITE_ERROR = -3,
	EI_IMPULSE_DSP_ERROR = -5,
	EI_IMPULSE_TFLITE_ARENA_ALLOC_FAILED = -6,
	EI_IMPULSE_CUBEAI_ERROR = -7,
	EI_IMPULSE_ALLOC_FAILED = -8,
	EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES = -9,
	EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE = -10
} EI_IMPULSE_ERROR;

/* Mock function used by ei_wrapper. */
extern "C" EI_IMPULSE_ERROR run_classifier(signal_t *signal,
					   ei_impulse_result_t *result,
					   bool debug);

#endif /* _EI_RUN_CLASSIFIER_H_ */
