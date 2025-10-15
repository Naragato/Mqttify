#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyMessage.h"
#include "Mqtt/Delegates/OnMessage.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Mqtt/MqttifyQualityOfService.h"
#include "Mqtt/State/MqttifyClientContext.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
	FMqttifyClientDispatchSpec,
	"Mqttify.Automation.ClientDispatchWildcardRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FMqttifyClientDispatchSpec)

void FMqttifyClientDispatchSpec::Define()
{
	Describe("FMqttifyClientContext dispatch routes to exact and wildcard delegates", [this]
	{
		It("Dispatch should invoke exact, '+', and '#' delegates for matching topic", [this]
		{
			FMqttifyConnectionSettingsBuilder Builder(TEXT("mqtt://user:pass@localhost:1883"));
			const TSharedPtr<FMqttifyConnectionSettings> Settings = Builder.Build();
			TestTrue(TEXT("Settings should be valid"), Settings.IsValid());
			const FMqttifyConnectionSettingsRef SettingsRef = Settings.ToSharedRef();

			TSharedRef<FMqttifyClientContext> Context = MakeShared<FMqttifyClientContext>(SettingsRef);

			static const TCHAR* ExactTopic = TEXT("sensors/uk/temperatures");
			bool bExactCalled = false;
			Context->GetMessageDelegate(ExactTopic)->AddLambda([&](const FMqttifyMessage&){ bExactCalled = true; });

			bool bPlusCalled = false;
			Context->GetMessageDelegate(TEXT("sensors/+/temperatures"))->AddLambda([&](const FMqttifyMessage&){ bPlusCalled = true; });

			bool bHashCalled = false;
			Context->GetMessageDelegate(TEXT("sensors/#"))->AddLambda([&](const FMqttifyMessage&){ bHashCalled = true; });

			FMqttifyMessage Msg{FString{ExactTopic}, TArray<uint8>{}, false, EMqttifyQualityOfService::AtMostOnce};
			Context->CompleteMessage(MoveTemp(Msg));

			TestTrue(TEXT("Exact delegate fired"), bExactCalled);
			TestTrue(TEXT("'+' filter delegate fired"), bPlusCalled);
			TestTrue(TEXT("'#' filter delegate fired"), bHashCalled);

			bExactCalled = false;
			bPlusCalled = false;
			bHashCalled = false;
			FMqttifyMessage Msg2{FString{TEXT("other/topic")}, TArray<uint8>{}, false, EMqttifyQualityOfService::AtMostOnce};
			Context->CompleteMessage(MoveTemp(Msg2));
			TestFalse(TEXT("Exact delegate should not fire for other/topic"), bExactCalled);
			TestFalse(TEXT("'+' filter should not fire for other/topic"), bPlusCalled);
			// sensors/# will not match other/topic either, so remains false
			TestFalse(TEXT("'#' filter should not fire for other/topic"), bHashCalled);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
