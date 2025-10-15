#pragma once

#include "CoreMinimal.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Mqtt/MqttifyRetainHandlingOptions.h"
#include "MqttifyTopicFilter.generated.h"

/**
 * @brief Represents an MQTT topic for publish/subscribe operations.
 * https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901241
 * http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718106
 */
USTRUCT(BlueprintType)
struct MQTTIFY_API FMqttifyTopicFilter final
{
	GENERATED_BODY()

	bool operator==(const FMqttifyTopicFilter& Other) const
	{
		return Filter == Other.Filter &&
			InQualityOfService == Other.InQualityOfService &&
			bNoLocal == Other.bNoLocal &&
			bRetainAsPublished == Other.bRetainAsPublished &&
			RetainHandlingOptions == Other.RetainHandlingOptions;
	}

	bool operator==(const FString& Other) const
	{
		return Filter == Other;
	}

	bool operator!=(const FMqttifyTopicFilter& Other) const
	{
		return !(*this == Other);
	}

	bool operator!=(const FString& Other) const
	{
		return !(*this == Other);
	}

	/**
	 * @brief Matches a topic against this filter using MQTT wildcards ('+' and '#').
	 */
	bool MatchesWildcard(const FString& InTopic) const
	{
		const int32 FilterLength = Filter.Len();
		const int32 TopicLength = InTopic.Len();

		// Iterate by levels separated by '/'. Supports empty leading/trailing levels.
		auto NextSegment = [](const FString& String,
		                     int32& Index,
		                     const int32 Length,
		                     bool& bEmittedTrailingEmptySegment) -> TTuple<const TCHAR*, int32>
		{
			if (Index == Length && !bEmittedTrailingEmptySegment)
			{
				if (Length > 0 && String[Length - 1] == TEXT('/'))
				{
					bEmittedTrailingEmptySegment = true;
					return MakeTuple(static_cast<const TCHAR*>(nullptr), 0);
				}
			}
			if (Index >= Length)
			{
				return MakeTuple(static_cast<const TCHAR*>(nullptr), -1);
			}

			const int32 Start = Index;
			int32 End = Start;
			while (End < Length && String[End] != TEXT('/')) { ++End; }
			const int32 SegmentLength = End - Start;
			Index = (End < Length) ? End + 1 : Length;
			return MakeTuple(SegmentLength > 0 ? &String[Start] : static_cast<const TCHAR*>(nullptr), SegmentLength);
		};

		auto IsPlusLevel = [](const TCHAR* Ptr, int32 Len) { return Len == 1 && Ptr && *Ptr == TEXT('+'); };
		auto IsHashLevel = [](const TCHAR* Ptr, int32 Len) { return Len == 1 && Ptr && *Ptr == TEXT('#'); };

		int32 FilterIndex = 0;
		int32 TopicIndex = 0;
		bool bFilterEmittedTrailingEmpty = false;
		bool bTopicEmittedTrailingEmpty = false;

		while (true)
		{
			const auto [FilterPtr, FilterSegLen] = NextSegment(Filter, FilterIndex, FilterLength, bFilterEmittedTrailingEmpty);
			const auto [TopicPtr, TopicSegLen] = NextSegment(InTopic, TopicIndex, TopicLength, bTopicEmittedTrailingEmpty);

			const bool bFilterDone = (FilterSegLen < 0);
			const bool bTopicDone = (TopicSegLen < 0);

			if (bFilterDone || bTopicDone)
			{
				if (!bFilterDone && IsHashLevel(FilterPtr, FilterSegLen))
				{
					const auto [NextPtr, NextLen] = NextSegment(Filter, FilterIndex, FilterLength, bFilterEmittedTrailingEmpty);
					return NextLen < 0; // '#' must be the last level
				}
				return bFilterDone && bTopicDone;
			}

			if (IsHashLevel(FilterPtr, FilterSegLen))
			{
				const auto [NextPtr, NextLen] = NextSegment(Filter, FilterIndex, FilterLength, bFilterEmittedTrailingEmpty);
				return NextLen < 0;
			}

			if (!IsPlusLevel(FilterPtr, FilterSegLen))
			{
				if (FilterSegLen != TopicSegLen) { return false; }
				for (int32 CharIndex = 0; CharIndex < FilterSegLen; ++CharIndex)
				{
					if (FilterPtr[CharIndex] != TopicPtr[CharIndex]) { return false; }
				}
			}
		}
	}

private:
	/**
	 * @brief Topic path string, e.g., "home/livingroom/temperature".
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Topic", meta=(AllowPrivateAccess="true"))
	FString Filter;

	/**
	 * @brief Quality of service. Default is AtMostOnce. This gives the maximum QoS level at which the Server can send Application Messages to the Client.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Quality of Service", meta=(AllowPrivateAccess="true"))
	EMqttifyQualityOfService InQualityOfService;

	/**
	 * @brief No Local flag. Default is true. If true, the Server will not forward Application Messages published to its own
	 * This option is only used for MQTT v5.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="No Local", meta=(AllowPrivateAccess="true"))
	bool bNoLocal;

	/**
	 * @brief Retain as Published flag. Default is true. If true, Application Messages forwarded using this Subscription
	 * This option is only used for MQTT v5.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Retain as Published", meta=(AllowPrivateAccess="true"))
	bool bRetainAsPublished;

	/**
	 * @brief Retain Handling Options. Default is SendRetainedMessagesAtSubscribeTime. This option is only used for MQTT v5.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Retain Handling Options", meta=(AllowPrivateAccess="true"))
	EMqttifyRetainHandlingOptions RetainHandlingOptions;

public:
	/// @brief U+002F (/): Separator character for different levels of the topic
	static constexpr TCHAR TopicLevelSeparatorChar = '/';

	/// @brief U+002B (+): Single-level wildcard character, must occupy an entire level of the filter if used
	static constexpr TCHAR SingleLevelWildcardChar = '+';

	/// @brief U+0023 (#): Multi-level wildcard character, must be the last character in the topic filter and either on its own or following a topic level separator
	static constexpr TCHAR MultiLevelWildcardChar = '#';

	/// @brief Not part of the MQTT specification, but widely used by servers for system topics
	static constexpr TCHAR SystemChar = '$';

	/**
	 * @brief Default constructor.
	 */
	FMqttifyTopicFilter();

	/**
	 * @brief Constructor from topic path.
	 * @param InFilter Topic path.
	 * @param InQualityOfService This gives the maximum QoS level at which the Server can send Application Messages to the Client.
	 * @param bInNoLocal If true, the Server will not forward Application Messages published to its own This option is only used for MQTT v5.
	 * @param bInRetainAsPublished If true, Application Messages forwarded using this Subscription This option is only used for MQTT v5.
	 * @param InRetainHandlingOptions This option is only used for MQTT v5.
	 * This option specifies whether retained messages are sent when the subscription is established.
	 * This does not affect the sending of retained messages at any point after the subscribe.
	 * If there are no retained messages matching the Topic Filter, all of these values act the same. The values are:
	 */
	explicit FMqttifyTopicFilter(const FString& InFilter,
	                             const EMqttifyQualityOfService InQualityOfService =
		                             EMqttifyQualityOfService::AtMostOnce,
	                             const bool bInNoLocal = true,
	                             const bool bInRetainAsPublished = true,
	                             const EMqttifyRetainHandlingOptions InRetainHandlingOptions =
		                             EMqttifyRetainHandlingOptions::SendRetainedMessagesAtSubscribeTime)
		: Filter(InFilter)
		, InQualityOfService(InQualityOfService)
		, bNoLocal(bInNoLocal)
		, bRetainAsPublished(bInRetainAsPublished)
		, RetainHandlingOptions(InRetainHandlingOptions) {}

	/**
	 * @brief Get topic filter
	 */
	const FString& GetFilter() const { return Filter; }

	/**
	 * @brief Get quality of service
	 */
	EMqttifyQualityOfService GetQualityOfService() const { return InQualityOfService; }

	/**
	 * @brief Get no local flag
	 */
	bool GetIsNoLocal() const { return bNoLocal; }

	/**
	 * @brief Get retain as published flag
	 */
	bool GetIsRetainAsPublished() const { return bRetainAsPublished; }

	/**
	 * @brief Get retain handling options
	 */
	EMqttifyRetainHandlingOptions GetRetainHandlingOptions() const { return RetainHandlingOptions; }

	/**
	 * @brief Destructor.
	 */
	~FMqttifyTopicFilter() = default;

	/**
	 * @brief Validates the topic.
	 * @return True if the topic is valid, false otherwise.
	 */
	bool IsValid() const;

	friend FORCEINLINE uint32 GetTypeHash(const FMqttifyTopicFilter& InTopicFilter)
	{
		uint32 Hash = 0;

		Hash = HashCombine(Hash, GetTypeHash(InTopicFilter.Filter));
		Hash = HashCombine(Hash, GetTypeHash(InTopicFilter.InQualityOfService));
		Hash = HashCombine(Hash, GetTypeHash(InTopicFilter.bNoLocal));
		Hash = HashCombine(Hash, GetTypeHash(InTopicFilter.bRetainAsPublished));
		Hash = HashCombine(Hash, GetTypeHash(InTopicFilter.RetainHandlingOptions));
		return Hash;
	}
};