#pragma once

/**
 * @brief A result type that can be used to return a value and whether the operation was successful
 */
template <typename TResultType>
class TMqttifyResult
{
private:
	TSharedPtr<TResultType> Result;
	bool bSuccess;

public:
	/**
	 * @brief Construct a result with a value
	 * @param bInSuccess Whether the operation was successful
	 */
	explicit TMqttifyResult(const bool bInSuccess)
		: Result{ nullptr }
		, bSuccess{ bInSuccess } {}

	/**
	 * @brief Construct a result with a value
	 * @param InResult The result value
	 * @param bInSuccess Whether the operation was successful
	 */
	explicit TMqttifyResult(const bool bInSuccess, TResultType&& InResult)
		: Result(MakeShared<TResultType>(MoveTemp(InResult)))
		, bSuccess{ bInSuccess } {}

	/**
	 *@brief Get the result value
	 * @return The result value
	 */
	TSharedPtr<TResultType> GetResult() const { return Result; }
	/**
	 * @brief Whether the operation was successful
	 * @return True if the operation was successful
	 */
	bool HasSucceeded() const { return bSuccess; }
};

/**
 * @brief A result type that can be used to return a value and whether the operation was successful
 * the void specialization is used to return a result without a value
 */
template <>
class TMqttifyResult<void>
{
private:
	bool bSuccess;

public:
	explicit TMqttifyResult(const bool bInSuccess)
		: bSuccess{ bInSuccess } {}

	/**
	 * @brief Whether the operation was successful
	 * @return True if the operation was successful
	 */
	bool HasSucceeded() const { return bSuccess; }
};
