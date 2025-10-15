#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyTopicFilter.h"

BEGIN_DEFINE_SPEC(
	FMqttifyTopicFilterSpec,
	"Mqttify.Automation.MqttifyTopicFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FMqttifyTopicFilterSpec)

void FMqttifyTopicFilterSpec::Define()
{
	Describe("FMqttifyTopicFilter::MatchesWildcard", [this]
	{
		It("Basic single-level '+' matches exactly one level (including empty)", [this]
		{
			FMqttifyTopicFilter Filter1{TEXT("topic/+/data")};
			TestTrue(TEXT("topic/x/data matches"), Filter1.MatchesWildcard(TEXT("topic/x/data")));
			TestTrue(TEXT("topic//data matches empty level"), Filter1.MatchesWildcard(TEXT("topic//data")));
			TestTrue(TEXT("topic/123/data matches"), Filter1.MatchesWildcard(TEXT("topic/123/data")));
			TestFalse(TEXT("topic/x/data/extra does not match"), Filter1.MatchesWildcard(TEXT("topic/x/data/extra")));
			TestFalse(TEXT("topic/x does not match"), Filter1.MatchesWildcard(TEXT("topic/x")));
		});

		It("Multi-level '#' placement and matching", [this]
		{
			FMqttifyTopicFilter Filter2{TEXT("topic/#")};
			TestTrue(TEXT("topic matches topic/#"), Filter2.MatchesWildcard(TEXT("topic")));
			TestTrue(TEXT("topic/ matches topic/#"), Filter2.MatchesWildcard(TEXT("topic/")));
			TestTrue(TEXT("topic/a matches topic/#"), Filter2.MatchesWildcard(TEXT("topic/a")));
			TestTrue(TEXT("topic/a/b matches topic/#"), Filter2.MatchesWildcard(TEXT("topic/a/b")));

			FMqttifyTopicFilter Filter3{TEXT("#")};
			TestTrue(TEXT("# matches everything (empty not possible in topics, but any topic)"), Filter3.MatchesWildcard(TEXT("anything")));
		});

		It("Invalid wildcard placement should not match (treated as invalid filter)", [this]
		{
			TestFalse(TEXT("'a#' invalid placement"), FMqttifyTopicFilter{TEXT("a#")}.MatchesWildcard(TEXT("a")));
			TestFalse(TEXT("'a/#/b' invalid placement"), FMqttifyTopicFilter{TEXT("a/#/b")}.MatchesWildcard(TEXT("a/x/b")));
			TestFalse(TEXT("'a/+b' invalid placement"), FMqttifyTopicFilter{TEXT("a/+b")}.MatchesWildcard(TEXT("a/xb")));
			TestFalse(TEXT("'a+' invalid placement"), FMqttifyTopicFilter{TEXT("a+")}.MatchesWildcard(TEXT("a")));
			TestFalse(TEXT("'+/b+' invalid placement"), FMqttifyTopicFilter{TEXT("+/b+")}.MatchesWildcard(TEXT("x/by")));
			TestFalse(TEXT("'a/##' invalid placement"), FMqttifyTopicFilter{TEXT("a/##")}.MatchesWildcard(TEXT("a")));
			TestFalse(TEXT("'a/#b' invalid placement"), FMqttifyTopicFilter{TEXT("a/#b")}.MatchesWildcard(TEXT("a")));
		});

		It("Absolute vs relative leading slash must match", [this]
		{
			TestTrue(TEXT("/a/+ matches /a/b"), FMqttifyTopicFilter{TEXT("/a/+")}.MatchesWildcard(TEXT("/a/b")));
			TestTrue(TEXT("/a/+ matches /a/ (empty last level)"), FMqttifyTopicFilter{TEXT("/a/+")}.MatchesWildcard(TEXT("/a/")));
			TestFalse(TEXT("/a/+ does not match a/b"), FMqttifyTopicFilter{TEXT("/a/+")}.MatchesWildcard(TEXT("a/b")));

			TestTrue(TEXT("a/+ matches a/b"), FMqttifyTopicFilter{TEXT("a/+")}.MatchesWildcard(TEXT("a/b")));
			TestTrue(TEXT("a/+ matches a/ (empty last level)"), FMqttifyTopicFilter{TEXT("a/+")}.MatchesWildcard(TEXT("a/")));
			TestFalse(TEXT("a/+ does not match /a/b"), FMqttifyTopicFilter{TEXT("a/+")}.MatchesWildcard(TEXT("/a/b")));
		});

		It("Trailing slash semantics are strict", [this]
		{
			TestTrue(TEXT("a/b/ matches a/b/ only"), FMqttifyTopicFilter{TEXT("a/b/")}.MatchesWildcard(TEXT("a/b/")));
			TestFalse(TEXT("a/b/ does not match a/b"), FMqttifyTopicFilter{TEXT("a/b/")}.MatchesWildcard(TEXT("a/b")));

			TestTrue(TEXT("a/+/ matches a/x/"), FMqttifyTopicFilter{TEXT("a/+/")}.MatchesWildcard(TEXT("a/x/")));
			TestTrue(TEXT("a/+/ matches a// (empty level)"), FMqttifyTopicFilter{TEXT("a/+/")}.MatchesWildcard(TEXT("a//")));
			TestFalse(TEXT("a/+/ does not match a/x"), FMqttifyTopicFilter{TEXT("a/+/")}.MatchesWildcard(TEXT("a/x")));
		});

		It("Edge cases and empty levels", [this]
		{
			TestTrue(TEXT("+/+ matches x/y"), FMqttifyTopicFilter{TEXT("+/+")}.MatchesWildcard(TEXT("x/y")));
			TestTrue(TEXT("+/+ matches x/ (empty second level)"), FMqttifyTopicFilter{TEXT("+/+")}.MatchesWildcard(TEXT("x/")));
			TestTrue(TEXT("+/+ matches /y (empty first level)"), FMqttifyTopicFilter{TEXT("+/+")}.MatchesWildcard(TEXT("/y")));
			TestTrue(TEXT("+/+ matches / (two empty levels)"), FMqttifyTopicFilter{TEXT("+/+")}.MatchesWildcard(TEXT("/")));
			TestFalse(TEXT("+/+ does not match x"), FMqttifyTopicFilter{TEXT("+/+")}.MatchesWildcard(TEXT("x")));
			TestFalse(TEXT("+/+ does not match x/y/z"), FMqttifyTopicFilter{TEXT("+/+")}.MatchesWildcard(TEXT("x/y/z")));

			TestTrue(TEXT("home//temp matches itself"), FMqttifyTopicFilter{TEXT("home//temp")}.MatchesWildcard(TEXT("home//temp")));
			TestTrue(TEXT("home/+/temp must match empty middle level"), FMqttifyTopicFilter{TEXT("home/+/temp")}.MatchesWildcard(TEXT("home//temp")));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
