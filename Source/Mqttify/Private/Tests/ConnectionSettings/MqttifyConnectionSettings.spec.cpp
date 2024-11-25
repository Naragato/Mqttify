#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"

BEGIN_DEFINE_SPEC(MqttifyConnectionSettingsSpec,
					"Mqttify.Automation.MqttifyConnectionSettings",
					EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::
					ClientContext | EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext)
	END_DEFINE_SPEC(MqttifyConnectionSettingsSpec)

	void MqttifyConnectionSettingsSpec::Define()
	{
		Describe("MqttifyConnectionSettings FromString creates a valid connection settings object from a valid URL",
				[this] {

					It(TEXT("Test valid URL mqtt://username:password@localhost:1883"),
						[this] {
							const TSharedPtr<FMqttifyConnectionSettings> Settings = FMqttifyConnectionSettingsBuilder(
								TEXT("mqtt://username:password@localhost:1883")).Build();
							TestNotNull(TEXT("Settings should not be null"), Settings.Get());
							if (Settings.IsValid())
							{
								TestEqual(TEXT("SocketProtocol should be mqtt"),
										Settings->GetTransportProtocol(),
										EMqttifyConnectionProtocol::Mqtt);
								TestEqual(TEXT("Host should be localhost"), Settings->GetHost(), TEXT("localhost"));
								TestEqual(TEXT("Username should be username"),
										Settings->GetCredentialsProvider()->GetCredentials().Username,
										TEXT("username"));
								TestEqual(TEXT("Password should be password"),
										Settings->GetCredentialsProvider()->GetCredentials().Password,
										TEXT("password"));
								TestEqual(TEXT("Post should be 1883"), Settings->GetPort(), 1883);
							}
						});

					It(TEXT("Test valid URL mqtts://user@domain.com:8883/path"),
						[this] {
							const TSharedPtr<FMqttifyConnectionSettings> Settings = FMqttifyConnectionSettingsBuilder(
								TEXT("mqtts://user@domain.com:8883/path")).Build();
							TestNotNull(TEXT("Settings should not be null"), Settings.Get());
							if (Settings.IsValid())
							{
								TestEqual(TEXT("SocketProtocol should be mqtts"),
										Settings->GetTransportProtocol(),
										EMqttifyConnectionProtocol::Mqtts);
								TestEqual(TEXT("Host should be domain.com"), Settings->GetHost(), TEXT("domain.com"));
								TestEqual(TEXT("Username should be user"), Settings->GetCredentialsProvider()->GetCredentials().Username, TEXT("user"));
								TestEqual(TEXT("Password should be empty"), Settings->GetCredentialsProvider()->GetCredentials().Password, TEXT(""));
								TestEqual(TEXT("Path to be path"), Settings->GetPath(), TEXT("path"));
								TestEqual(TEXT("Post should be 8883"), Settings->GetPort(), 8883);
							}
						});
				});

		Describe("MqttifyConnectionSettings FromString creates a null settings object from an invalid URL",
				[this] {

					It(TEXT("Test valid URL http://invalid.url:3000, but invalid for this protocol"),
						[this] {
							const TSharedPtr<FMqttifyConnectionSettings> Settings = FMqttifyConnectionSettingsBuilder(
								TEXT("http://invalid.url:3000")).Build();
							TestNull(TEXT("Invalid URL should produce a null settings object"), Settings.Get());
						});

					It(TEXT("Test empty URL"),
						[this] {
							const TSharedPtr<FMqttifyConnectionSettings> Settings = FMqttifyConnectionSettingsBuilder(
								TEXT("")).Build();
							TestNull(TEXT("Empty URL should produce a null settings object"), Settings.Get());
						});
				});
	}
#endif // WITH_DEV_AUTOMATION_TESTS
