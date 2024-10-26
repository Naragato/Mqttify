#include "Mqtt/MqttifyTopicFilter.h"

#include "Internationalization/Regex.h"

FMqttifyTopicFilter::FMqttifyTopicFilter()
	: InQualityOfService{ EMqttifyQualityOfService::AtMostOnce }
	, bNoLocal{ false }
	, bRetainAsPublished{ false }
	, RetainHandlingOptions{ EMqttifyRetainHandlingOptions::SendRetainedMessagesAtSubscribeTime } {}

bool FMqttifyTopicFilter::IsValid() const
{
	// check that the topic filter is valid
	if (Filter.IsEmpty())
	{
		return false;
	}

	if (Filter.Len() == 0 || Filter.Len() > 65535)
	{
		return false;
	}

	// regex to validate filter
	const FRegexPattern RegexPattern(
		TEXT(
			"^(?:(?:[^\\s/]+|/|\\$[^/\\s]+|\\$share/[^/\\s]+)(?:/(?:[^\\s/]+|\\+|\\#|\\$[^/\\s]+|\\$share/[^/\\s]+))*(/)?)?$"),
		ERegexPatternFlags::CaseInsensitive
		);

	// create matcher
	FRegexMatcher RegexMatcher(RegexPattern, Filter);

	// check if the filter matches the regex
	return RegexMatcher.FindNext();
} // namespace Mqttify
