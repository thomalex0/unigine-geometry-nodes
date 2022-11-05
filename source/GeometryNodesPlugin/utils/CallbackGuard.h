#pragma once

#include <functional>
#include <memory>
#include <cassert>

namespace GeometryNodes
{

	// Forward declaration at the top to pay the reader attention on it.
	class CallbackGuard;
	using CallbackGuardUPtr = std::unique_ptr<CallbackGuard>;

	/*
	 * This class corresponds to RAII idiom. It receives a callback id (void*)
	 * by using the creator-function and destroys it by using the destroyer-function.
	 */
	class CallbackGuard
	{
	public:
		CallbackGuard(const CallbackGuard&) = delete;
		CallbackGuard& operator=(const CallbackGuard&) = delete;
		CallbackGuard(CallbackGuard&&) = delete;
		CallbackGuard& operator=(CallbackGuard&&) = delete;

		using CreatorFn = std::function<void* ()>;
		using DestroyerFn = std::function<void(void*)>;

		explicit CallbackGuard(const CreatorFn& creator, DestroyerFn destoyer)
			: CallbackGuard(creator(), std::move(destoyer))
		{
		}
		explicit CallbackGuard(void* callback_id, DestroyerFn destoyer)
			: callback_id_{ callback_id }
			, destroyer_{ std::move(destoyer) }
		{
			assert(callback_id_ && destroyer_);
		}
		~CallbackGuard()
		{
			destroyer_(callback_id_);
		}

	private:
		void* const callback_id_;
		const DestroyerFn destroyer_;
	};

} // namespace GeometryNodes

