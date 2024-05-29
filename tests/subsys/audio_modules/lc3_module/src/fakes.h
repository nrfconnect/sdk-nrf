/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FAKES_H_
#define _FAKES_H_

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <errno.h>
#include <stdint.h>

#include "LC3API.h"

/* Fake functions declaration */
DECLARE_FAKE_VALUE_FUNC(int32_t, LC3Initialize, uint8_t, uint8_t, LC3FrameSizeConfig_t, uint8_t,
			uint8_t *, uint32_t *);
DECLARE_FAKE_VALUE_FUNC(int32_t OSALCALL, LC3Deinitialize);
DECLARE_FAKE_VALUE_FUNC(LC3EncoderHandle_t OSALCALL, LC3EncodeSessionOpen, uint16_t, uint8_t,
			LC3FrameSize_t, uint8_t *, uint16_t *, int32_t *);
DECLARE_FAKE_VALUE_FUNC(int32_t, LC3EncodeSessionData, LC3EncoderHandle_t, LC3EncodeInput_t *,
			LC3EncodeOutput_t *);
DECLARE_FAKE_VOID_FUNC1(LC3EncodeSessionClose, LC3EncoderHandle_t);
DECLARE_FAKE_VALUE_FUNC(LC3DecoderHandle_t, LC3DecodeSessionOpen, uint16_t, uint8_t, LC3FrameSize_t,
			uint8_t *, uint16_t *, int32_t *);
DECLARE_FAKE_VALUE_FUNC(int32_t, LC3DecodeSessionData, LC3DecoderHandle_t, LC3DecodeInput_t *,
			LC3DecodeOutput_t *);
DECLARE_FAKE_VOID_FUNC1(LC3DecodeSessionClose, LC3DecoderHandle_t);
DECLARE_FAKE_VALUE_FUNC(uint16_t, LC3BitstreamBuffersize, uint16_t, uint32_t, LC3FrameSize_t,
			int32_t *);
DECLARE_FAKE_VALUE_FUNC(uint16_t, LC3PCMBuffersize, uint16_t, uint8_t, LC3FrameSize_t, int32_t *);

/* List of fakes used by this unit tester */
#define DO_FOREACH_FAKE(FUNC)                                                                      \
	do {                                                                                       \
		FUNC(LC3Initialize)                                                                \
		FUNC(LC3Deinitialize)                                                              \
		FUNC(LC3EncodeSessionOpen)                                                         \
		FUNC(LC3EncodeSessionData)                                                         \
		FUNC(LC3EncodeSessionClose)                                                        \
		FUNC(LC3DecodeSessionOpen)                                                         \
		FUNC(LC3DecodeSessionData)                                                         \
		FUNC(LC3DecodeSessionClose)                                                        \
		FUNC(LC3BitstreamBuffersize)                                                       \
		FUNC(LC3PCMBuffersize)                                                             \
	} while (0)

int32_t fake_LC3Initialize__succeeds(uint8_t encoderSampleRates, uint8_t decoderSampleRates,
				     LC3FrameSizeConfig_t frameSizeConfig, uint8_t uniqueSessions,
				     uint8_t *buffer, uint32_t *bufferSize);
int32_t fake_LC3Initialize__fails(uint8_t encoderSampleRates, uint8_t decoderSampleRates,
				  LC3FrameSizeConfig_t frameSizeConfig, uint8_t uniqueSessions,
				  uint8_t *buffer, uint32_t *bufferSize);
int32_t fake_LC3Deinitialize__succeeds(void);
int32_t fake_LC3Deinitialize__fails(void);
LC3EncoderHandle_t fake_LC3EncodeSessionOpen__succeeds(uint16_t sampleRate, uint8_t bitsPerSample,
						       LC3FrameSize_t frameSize, uint8_t *buffer,
						       uint16_t *bufferSize, int32_t *result);
LC3EncoderHandle_t fake_LC3EncodeSessionOpen__fails(uint16_t sampleRate, uint8_t bitsPerSample,
						    LC3FrameSize_t frameSize, uint8_t *buffer,
						    uint16_t *bufferSize, int32_t *result);
int32_t fake_LC3EncodeSessionData__succeeds(LC3EncoderHandle_t encodeHandle,
					    LC3EncodeInput_t *encodeInput,
					    LC3EncodeOutput_t *encodeOutput);
int32_t fake_LC3EncodeSessionData__fails(LC3EncoderHandle_t encodeHandle,
					 LC3EncodeInput_t *encodeInput,
					 LC3EncodeOutput_t *encodeOutput);
void fake_LC3EncodeSessionClose__succeeds(LC3EncoderHandle_t encodeHandle);
void fake_LC3EncodeSessionClose__fails(LC3EncoderHandle_t encodeHandle);
LC3DecoderHandle_t fake_LC3DecodeSessionOpen__succeeds(uint16_t sampleRate, uint8_t bitsPerSample,
						       LC3FrameSize_t frameSize, uint8_t *buffer,
						       uint16_t *bufferSize, int32_t *result);
LC3DecoderHandle_t fake_LC3DecodeSessionOpen__fails(uint16_t sampleRate, uint8_t bitsPerSample,
						    LC3FrameSize_t frameSize, uint8_t *buffer,
						    uint16_t *bufferSize, int32_t *result);
int32_t fake_LC3DecodeSessionData__succeeds(LC3DecoderHandle_t decodeHandle,
					    LC3DecodeInput_t *decodeInput,
					    LC3DecodeOutput_t *decodeOutput);
int32_t fake_LC3DecodeSessionData__fails(LC3DecoderHandle_t decodeHandle,
					 LC3DecodeInput_t *decodeInput,
					 LC3DecodeOutput_t *decodeOutput);
void fake_LC3DecodeSessionClose__succeeds(LC3DecoderHandle_t decodeHandle);
void fake_LC3DecodeSessionClose__fails(LC3DecoderHandle_t decodeHandle);
uint16_t fake_LC3BitstreamBuffersize__succeeds(uint16_t sampleRate, uint32_t maxBitRate,
					       LC3FrameSize_t frameSize, int32_t *result);
uint16_t fake_LC3BitstreamBuffersize__fails(uint16_t sampleRate, uint32_t maxBitRate,
					    LC3FrameSize_t frameSize, int32_t *result);
uint16_t fake_LC3PCMBuffersize__succeeds(uint16_t sampleRate, uint8_t bitDepth,
					 LC3FrameSize_t frameSize, int32_t *result);
uint16_t fake_LC3PCMBuffersize__fails(uint16_t sampleRate, uint8_t bitDepth,
				      LC3FrameSize_t frameSize, int32_t *result);

extern uint8_t *wav_ref;
extern int wav_ref_size;
extern int wav_ref_read_size;

#endif /* _FAKES_H_ */
