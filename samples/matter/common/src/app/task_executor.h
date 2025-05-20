/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <type_traits>

namespace Nrf
{

	/**
	 * @brief Task to be executed in the application thread.
	 *
	 * The Task encompasses a functor that returns void and takes no arguments.
	 *
	 * This class resembles \c std::function<void()> but it is trivially copyable
	 * and uses static storage only. This enables the task to be passed using
	 * a lightweight, type-agnostic message queue, but the task can only hold
	 * a trivially copyable functor of a limited size, configurable using
	 * \c CONFIG_NCS_SAMPLE_MATTER_APP_TASK_MAX_SIZE Kconfig option.
	 */
	class Task {
	public:
		Task() = default;

		template <typename Functor> Task(const Functor &functor)
		{
			// Ensure that the functor can be safely serialized
			static_assert(std::is_trivially_copyable_v<Functor>, "Task must be trivially copyable");
			static_assert(sizeof(functor) <= sizeof(mStorage), "Task storage too small");
			static_assert(alignof(Storage) % alignof(Functor) == 0, "Task storage has insufficient alignment");

			// Ensure that the functor's destructor can be ignored
			static_assert(std::is_trivially_destructible_v<Functor>, "Task must be trivially destructible");

			new (&mStorage) Functor(functor);
			mHandler = [](const Storage &storage) { (*reinterpret_cast<const Functor *>(&storage))(); };
		}

		void operator()() const { mHandler(mStorage); }

	private:
		using Storage = std::aligned_storage_t<CONFIG_NCS_SAMPLE_MATTER_APP_TASK_MAX_SIZE, alignof(void *)>;
		using Handler = void (*)(const Storage &storage);

		Storage mStorage;
		Handler mHandler;
	};

	/**
	 * @brief Post a task to the task queue.
	 *
	 * The Task encompasses a functor that returns void and takes no arguments.
	 * The task can be constructed with a lambda function with all the required arguments captured.
	 *
	 * For example to post a task within which you need to run the method defined as "MyMethod",
	 * and it has an argument defined as "uint32_t" see the following example:
	 *
	 * void MyMethod(uint32_t arg){ // Do something with arg }
	 *
	 * uint32_t myNumber;
	 * PostTask([myNumber]{ MyMethod(myNumber) };)
	 *
	 * @param task the Task to be posted to the application thread's task queue
	 */
	void PostTask(const Task &task);

	/**
	 * @brief Dispatch the next available task.
	 *
	 * This is a blocking method that should be called in an application thread.
	 * Dispatching events relies on constant waiting for the next event posted in the
	 * task queue.
	 *
	 */
	void DispatchNextTask();
} /* namespace Nrf */
