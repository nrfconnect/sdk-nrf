/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_ENUM_MISSING_MEMBER_H_
#define CI_DOXYGEN_REVIEWER_TEST_ENUM_MISSING_MEMBER_H_

/**
 * @file enum_missing_member.h
 * @ingroup ci_doxygen_reviewer_test
 */

/** @brief Reviewer test fixture states. */
enum ci_doxy_reviewer_fixture_state {
	/** Fixture is uninitialized. */
	CI_DOXY_REVIEWER_FIXTURE_STATE_IDLE = 0,
	/** Fixture is running a scenario. */
	CI_DOXY_REVIEWER_FIXTURE_STATE_ACTIVE,
	CI_DOXY_REVIEWER_FIXTURE_STATE_DONE,
};

#endif /* CI_DOXYGEN_REVIEWER_TEST_ENUM_MISSING_MEMBER_H_ */
