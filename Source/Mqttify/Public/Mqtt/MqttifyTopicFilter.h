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

	// TODO: Not sure if we will need this for our client but if we
	// create a server it will 100% need this.
	/**
	 * @brief Matches a given topic with the stored filter, considering MQTT-style wildcards (+ and #).
	 * @param InTopic - topic to check against the filter.
	 * @return True on successful match, false otherwise.
	*/
	bool MatchesWildcard(const FString& InTopic) const
	{
		int FilterPos = 0;
		int TopicPos  = 0;

		while (FilterPos < Filter.Len() && TopicPos < InTopic.Len())
		{
			switch (Filter[FilterPos])
			{
				case '+':
					// Skip until the next level in the topic.
					while (InTopic[TopicPos] != '/' && TopicPos < InTopic.Len())
					{
						++TopicPos;
					}
				// Advance FilterPos past the '+' character.
					++FilterPos;
					break;

				case '#':
					// '#' must be the last character in the filter string, so if it matches, we have a successful topic-filter match.
					return true;

				default:
					// Normal character, check character by character
					while (Filter[FilterPos] == InTopic[TopicPos])
					{
						++FilterPos;
						++TopicPos;
						if (FilterPos == Filter.Len() && TopicPos == InTopic.Len())
						{
							// Both strings ended at the same time, hence they match.
							return true;
						}
						if (TopicPos == InTopic.Len() || FilterPos == Filter.Len())
						{
							// One string ended before the other, hence they do not match.
							return false;
						}
					}
				// If the unmatched character is '/', move to the next level. Otherwise, return false.
					if (Filter[FilterPos] == '/' && InTopic[TopicPos] == '/')
					{
						++FilterPos;
						++TopicPos;
					}
					else
					{
						return false;
					}
					break;
			}
		}

		// If either the filter or the topic have remaining characters, but not both, they do not match
		if ((FilterPos < Filter.Len() && Filter[FilterPos] != '#') || TopicPos < InTopic.Len())
		{
			return false;
		}

		// If both strings have ended simultaneously, or remaining filter characters are all '/', they match.
		while (FilterPos < Filter.Len())
		{
			if (Filter[FilterPos] != '/')
			{
				// Any character remaining in filter other than '/' will break the match.
				return false;
			}
			++FilterPos;
		}

		return true;
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
								const bool bInNoLocal                                       = true,
								const bool bInRetainAsPublished                             = true,
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
