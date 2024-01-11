/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <functional>

namespace Nrf
{
	using Task = std::function<void()>;

	/**
	 * @brief Post a task to the task queue.
	 *
	 * The Task is a function that returns void and enters no arguments.
	 * The task can be posted as a lambda function with all the required arguments captured.
	 *
	 * For example to post a task within which you need to run the method defined as "MyMethod",
	 * and it has an argument defined as "uint32_t" see the following example:
	 *
	 * void MyMethod(uint32_t arg){ // Do something with arg }
	 *
	 * uint32_t myNumber;
	 * PostTask([myNumber]{ MyMethod(myNumber) };)
	 *
	 * @param task the Task function to be posted to the task queue
	 */
	void PostTask(const Task &task);

	/**
	 * @brief Dispatch an next available task.
	 *
	 * This is a blocking method that should be called in an application thread.
	 * Dispatching events relies on constant waiting for the next event posted in the
	 * task queue.
	 *
	 */
	void DispatchNextTask();
} /* namespace Nrf */
