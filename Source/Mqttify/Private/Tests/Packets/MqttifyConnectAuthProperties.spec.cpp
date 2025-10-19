#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/MqttifyConnectPacket.h"
#include "Packets/Properties/MqttifyProperty.h"

using namespace Mqttify;

BEGIN_DEFINE_SPEC(
    FMqttifyConnectAuthPropsSpec,
    "Mqttify.Automation.Mqtt5.Connect.AuthProperties",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
    EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FMqttifyConnectAuthPropsSpec)

void FMqttifyConnectAuthPropsSpec::Define()
{
    Describe("CONNECT Authentication Method/Data properties (MQTT 5)", [this]
    {
        It("Should encode/decode Authentication Method and Data in CONNECT properties", [this]
        {
            const FString ClientId = TEXT("client-1");
            const uint16 KeepAlive = 30;
            const FString Username = TEXT("user");
            const FString Password = TEXT("pass");

            TArray<FMqttifyProperty> Properties;
            Properties.Add({FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationMethod>(FString(TEXT("SCRAM-SHA-1"))) });
            Properties.Add({FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationData>(TArray<uint8>{1,2,3,4})});

            const auto Connect = MakeShared<TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>>(
                ClientId,
                KeepAlive,
                Username,
                Password,
                false,
                false,
                FString(),
                FString(),
                EMqttifyQualityOfService::AtMostOnce,
                FMqttifyProperties(),
                FMqttifyProperties(Properties));

            // Encode
            TArray<uint8> Buffer;
            FMemoryWriter Writer(Buffer, true);
            Writer.SetByteSwapping(true);
            Connect->Encode(Writer);

            // Decode
            FArrayReader Reader;
            Reader.SetByteSwapping(true);
            Reader.Append(Buffer.GetData(), Buffer.Num());

            const FMqttifyFixedHeader Header = FMqttifyFixedHeader::Create(Reader);
            auto Decoded = TMqttifyConnectPacket<EMqttifyProtocolVersion::Mqtt_5>(Reader, Header);

            // Verify
            bool bFoundMethod = false;
            bool bFoundData = false;
            for (const FMqttifyProperty& P : Decoded.GetProperties().GetProperties())
            {
                if (P.GetIdentifier() == EMqttifyPropertyIdentifier::AuthenticationMethod)
                {
                    FString Value;
                    TestTrue(TEXT("Auth Method present"), P.TryGetValue(Value));
                    TestEqual(TEXT("Auth Method value"), Value, FString(TEXT("SCRAM-SHA-1")));
                    bFoundMethod = true;
                }
                else if (P.GetIdentifier() == EMqttifyPropertyIdentifier::AuthenticationData)
                {
                    TArray<uint8> Value;
                    TestTrue(TEXT("Auth Data present"), P.TryGetValue(Value));
                    TestEqual(TEXT("Auth Data length"), Value.Num(), 4);
                    TestEqual(TEXT("Auth Data byte 0"), (int32)Value[0], 1);
                    TestEqual(TEXT("Auth Data byte 1"), (int32)Value[1], 2);
                    TestEqual(TEXT("Auth Data byte 2"), (int32)Value[2], 3);
                    TestEqual(TEXT("Auth Data byte 3"), (int32)Value[3], 4);
                    bFoundData = true;
                }
            }

            TestTrue(TEXT("Auth Method property found"), bFoundMethod);
            TestTrue(TEXT("Auth Data property found"), bFoundData);
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
