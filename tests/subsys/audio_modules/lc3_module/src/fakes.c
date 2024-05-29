/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <stdint.h>

#include "LC3API.h"
#include "fakes.h"
#include "lc3_test_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(module_fakes, 4);

#define TEST_ENC_SESSIONS_NUM (5)
#define TEST_DEC_SESSIONS_NUM (10)

struct lc3_dec_session {
	bool session_init;
	uint8_t *ref_data;
	int ref_read_size;
	int ref_remaining;
};

uint8_t *wav_ref;
int wav_ref_size;
int wav_ref_read_size;

static int enc_count;
static int dec_count;
static LC3DecoderHandle_t enc_session[TEST_ENC_SESSIONS_NUM];

struct lc3_dec_session dec_session_ctx[TEST_DEC_SESSIONS_NUM];

/*
 * Stubs are defined here, so that multiple .C files can share them
 * without having linker issues.
 */
DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int32_t OSALCALL, LC3Initialize, uint8_t, uint8_t, LC3FrameSizeConfig_t,
		       uint8_t, uint8_t *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int32_t OSALCALL, LC3Deinitialize);
DEFINE_FAKE_VALUE_FUNC(LC3EncoderHandle_t OSALCALL, LC3EncodeSessionOpen, uint16_t, uint8_t,
		       LC3FrameSize_t, uint8_t *, uint16_t *, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int32_t OSALCALL, LC3EncodeSessionData, LC3EncoderHandle_t,
		       LC3EncodeInput_t *, LC3EncodeOutput_t *);
DEFINE_FAKE_VOID_FUNC1(LC3EncodeSessionClose, LC3EncoderHandle_t);
DEFINE_FAKE_VALUE_FUNC(LC3DecoderHandle_t, LC3DecodeSessionOpen, uint16_t, uint8_t, LC3FrameSize_t,
		       uint8_t *, uint16_t *, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int32_t, LC3DecodeSessionData, LC3DecoderHandle_t, LC3DecodeInput_t *,
		       LC3DecodeOutput_t *);
DEFINE_FAKE_VOID_FUNC1(LC3DecodeSessionClose, LC3DecoderHandle_t);
DEFINE_FAKE_VALUE_FUNC(uint16_t, LC3BitstreamBuffersize, uint16_t, uint32_t, LC3FrameSize_t,
		       int32_t *);
DEFINE_FAKE_VALUE_FUNC(uint16_t, LC3PCMBuffersize, uint16_t, uint8_t, LC3FrameSize_t, int32_t *);

/* Custom fakes implementation */
int32_t fake_LC3Initialize__succeeds(uint8_t encoderSampleRates, uint8_t decoderSampleRates,
				     LC3FrameSizeConfig_t frameSizeConfig, uint8_t uniqueSessions,
				     uint8_t *buffer, uint32_t *bufferSize)
{
	ARG_UNUSED(encoderSampleRates);
	ARG_UNUSED(decoderSampleRates);
	ARG_UNUSED(frameSizeConfig);
	ARG_UNUSED(uniqueSessions);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufferSize);

	enc_count = 0;
	dec_count = 0;

	for (int i = 0; i < TEST_DEC_SESSIONS_NUM; i++) {
		dec_session_ctx[i].session_init = false;
		dec_session_ctx[i].ref_data = NULL;
		dec_session_ctx[i].ref_read_size = 0;
		dec_session_ctx[i].ref_remaining = 0;
	}

	return 0;
}

/* Custom fakes implementation */
int32_t fake_LC3Initialize__fails(uint8_t encoderSampleRates, uint8_t decoderSampleRates,
				  LC3FrameSizeConfig_t frameSizeConfig, uint8_t uniqueSessions,
				  uint8_t *buffer, uint32_t *bufferSize)
{
	ARG_UNUSED(encoderSampleRates);
	ARG_UNUSED(decoderSampleRates);
	ARG_UNUSED(frameSizeConfig);
	ARG_UNUSED(uniqueSessions);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufferSize);

	return -1;
}

/* Custom fakes implementation */
int32_t fake_LC3Deinitialize__succeeds(void)
{
	enc_count = 0;
	dec_count = 0;

	for (int i = 0; i < TEST_DEC_SESSIONS_NUM; i++) {
		dec_session_ctx[i].session_init = false;
		dec_session_ctx[i].ref_data = NULL;
		dec_session_ctx[i].ref_read_size = 0;
		dec_session_ctx[i].ref_remaining = 0;
	}

	return 0;
}

/* Custom fakes implementation */
int32_t fake_LC3Deinitialize__fails(void)
{
	return -1;
}

/* Custom fakes implementation */
LC3EncoderHandle_t fake_LC3EncodeSessionOpen__succeeds(uint16_t sampleRate, uint8_t bitsPerSample,
						       LC3FrameSize_t frameSize, uint8_t *buffer,
						       uint16_t *bufferSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(bitsPerSample);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufferSize);
	ARG_UNUSED(result);

	return &enc_session[enc_count++];
}

/* Custom fakes implementation */
LC3EncoderHandle_t fake_LC3EncodeSessionOpen__fails(uint16_t sampleRate, uint8_t bitsPerSample,
						    LC3FrameSize_t frameSize, uint8_t *buffer,
						    uint16_t *bufferSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(bitsPerSample);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufferSize);
	ARG_UNUSED(result);

	return NULL;
}

int32_t fake_LC3EncodeSessionData__succeeds(LC3EncoderHandle_t encodeHandle,
					    LC3EncodeInput_t *encodeInput,
					    LC3EncodeOutput_t *encodeOutput)
{
	ARG_UNUSED(encodeHandle);
	ARG_UNUSED(encodeInput);
	ARG_UNUSED(encodeOutput);

	return 0;
}

int32_t fake_LC3EncodeSessionData__fails(LC3EncoderHandle_t encodeHandle,
					 LC3EncodeInput_t *encodeInput,
					 LC3EncodeOutput_t *encodeOutput)
{
	ARG_UNUSED(encodeHandle);
	ARG_UNUSED(encodeInput);
	ARG_UNUSED(encodeOutput);

	return -1;
}

void fake_LC3EncodeSessionClose__succeeds(LC3EncoderHandle_t encodeHandle)
{
	ARG_UNUSED(encodeHandle);
}

void fake_LC3EncodeSessionClose__fails(LC3EncoderHandle_t encodeHandle)
{
	ARG_UNUSED(encodeHandle);
}

LC3DecoderHandle_t fake_LC3DecodeSessionOpen__succeeds(uint16_t sampleRate, uint8_t bitsPerSample,
						       LC3FrameSize_t frameSize, uint8_t *buffer,
						       uint16_t *bufferSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(bitsPerSample);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufferSize);
	struct lc3_dec_session *session = (struct lc3_dec_session *)&dec_session_ctx[dec_count];

	session->session_init = true;
	session->ref_data = wav_ref;
	session->ref_read_size = wav_ref_read_size;
	session->ref_remaining = wav_ref_size;

	dec_count++;

	*result = 0;

	return (void *)session;
}

LC3DecoderHandle_t fake_LC3DecodeSessionOpen__fails(uint16_t sampleRate, uint8_t bitsPerSample,
						    LC3FrameSize_t frameSize, uint8_t *buffer,
						    uint16_t *bufferSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(bitsPerSample);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(buffer);
	ARG_UNUSED(bufferSize);

	*result = -1;

	return NULL;
}

int32_t fake_LC3DecodeSessionData__succeeds(LC3DecoderHandle_t decodeHandle,
					    LC3DecodeInput_t *decodeInput,
					    LC3DecodeOutput_t *decodeOutput)
{
	ARG_UNUSED(decodeInput);
	struct lc3_dec_session *session = (struct lc3_dec_session *)decodeHandle;
	uint8_t *data_out = (uint8_t *)decodeOutput->PCMData;
	uint8_t *ref_data = session->ref_data;
	int size;

	if (session->ref_read_size > session->ref_remaining) {
		size = session->ref_remaining;
		session->ref_remaining = 0;
	} else {
		size = session->ref_read_size;
		session->ref_remaining -= session->ref_read_size;
	}

	for (int i = 0; i < size; i++) {
		*data_out++ = *ref_data++;
	}

	session->ref_data = ref_data;

	decodeOutput->bytesWritten = size;

	return 0;
}

int32_t fake_LC3DecodeSessionData__fails(LC3DecoderHandle_t decodeHandle,
					 LC3DecodeInput_t *decodeInput,
					 LC3DecodeOutput_t *decodeOutput)
{
	ARG_UNUSED(decodeHandle);
	ARG_UNUSED(decodeInput);
	ARG_UNUSED(decodeOutput);

	return -1;
}

void fake_LC3DecodeSessionClose__succeeds(LC3DecoderHandle_t decodeHandle)
{
	struct lc3_dec_session *session = (struct lc3_dec_session *)decodeHandle;

	session->session_init = false;
	session->ref_data = NULL;
	session->ref_read_size = 0;
}

void fake_LC3DecodeSessionClose__fails(LC3DecoderHandle_t decodeHandle)
{
	ARG_UNUSED(decodeHandle);
}

uint16_t fake_LC3BitstreamBuffersize__succeeds(uint16_t sampleRate, uint32_t maxBitRate,
					       LC3FrameSize_t frameSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(maxBitRate);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(result);

	return TEST_ENC_MONO_BUF_SIZE;
}

uint16_t fake_LC3BitstreamBuffersize__fails(uint16_t sampleRate, uint32_t maxBitRate,
					    LC3FrameSize_t frameSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(maxBitRate);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(result);

	return 0;
}

uint16_t fake_LC3PCMBuffersize__succeeds(uint16_t sampleRate, uint8_t bitDepth,
					 LC3FrameSize_t frameSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(bitDepth);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(result);

	return 0;
}

uint16_t fake_LC3PCMBuffersize__fails(uint16_t sampleRate, uint8_t bitDepth,
				      LC3FrameSize_t frameSize, int32_t *result)
{
	ARG_UNUSED(sampleRate);
	ARG_UNUSED(bitDepth);
	ARG_UNUSED(frameSize);
	ARG_UNUSED(result);

	return -1;
}
