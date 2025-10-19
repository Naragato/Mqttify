#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mqtt/MqttifyConnectionSettings.h"
#include "Mqtt/MqttifyConnectionSettingsBuilder.h"
#include "Mqtt/State/MqttifyClientContext.h"
#include "Mqtt/State/MqttifyClientConnectingState.h"
#include "Packets/MqttifyAuthPacket.h"
#include "Packets/MqttifyConnAckPacket.h"
#include "Packets/Properties/MqttifyProperty.h"
#include "Tests/Support/FakeTestSocket.h"
#include "Mqtt/Interface/IMqttifyCredentialsProvider.h"

using namespace Mqttify;

// Dummy credentials provider used in tests to drive enhanced auth
class FTestEnhancedAuthProvider final : public IMqttifyCredentialsProvider
{
public:
	virtual ~FTestEnhancedAuthProvider() override = default;

	virtual FMqttifyCredentials GetCredentials() override
	{
		return FMqttifyCredentials{TEXT("user"), TEXT("pass")};
	}
	virtual FString GetAuthMethod() override { return FString(TEXT("TEST-AUTH")); }
	virtual TArray<uint8> GetInitialAuthData() override { return TArray<uint8>{0xCA}; }
	virtual TArray<uint8> OnAuthChallenge(const TArray<uint8>& /*ServerData*/) override { return TArray<uint8>{0xFE}; }
};

static TSharedPtr<FArrayReader> EncodePacketToReader(const TSharedRef<IMqttifyControlPacket>& Packet)
{
	TArray<uint8> Bytes;
	FMemoryWriter W(Bytes);
	W.SetByteSwapping(true);
	Packet->Encode(W);
	TSharedPtr<FArrayReader> Reader = MakeShared<FArrayReader>();
	Reader->SetByteSwapping(true);
	Reader->Append(Bytes.GetData(), Bytes.Num());
	return Reader;
}

BEGIN_DEFINE_SPEC(
	FMqttifyConnectingEnhancedAuthSpec,
	"Mqttify.Automation.ClientConnecting.EnhancedAuth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ServerContext |
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FMqttifyConnectingEnhancedAuthSpec)

void FMqttifyConnectingEnhancedAuthSpec::Define()
{
	Describe("FMqttifyClientConnectingState enhanced auth flow", [this]
	{
		It("Responds to AUTH(Continue) with provider data and completes on CONNACK", [this]
		{
			// Settings with enhanced auth provider
			FMqttifyConnectionSettingsBuilder Builder(TEXT("mqtt://localhost:1883"));
			Builder.SetMqttProtocolVersion(EMqttifyProtocolVersion::Mqtt_5);
			TSharedRef<IMqttifyCredentialsProvider> Provider = MakeShared<FTestEnhancedAuthProvider>();
			Builder.SetCredentialsProvider(Provider);
			const TSharedPtr<FMqttifyConnectionSettings> Settings = Builder.Build();
			TestTrue(TEXT("Settings should be valid"), Settings.IsValid());
			const FMqttifyConnectionSettingsRef SettingsRef = Settings.ToSharedRef();

			TSharedRef<FMqttifyClientContext> Context = MakeShared<FMqttifyClientContext>(SettingsRef);
			TSharedRef<FFakeTestSocket> Socket = MakeShared<FFakeTestSocket>(SettingsRef);

			bool bBecameConnected = false;
			FMqttifyClientState::FOnStateChangedDelegate OnStateChanged = FMqttifyClientState::FOnStateChangedDelegate::CreateLambda(
				[&](const FMqttifyClientState* /*Prev*/, const TSharedPtr<FMqttifyClientState>& NewState)
				{
					bBecameConnected = NewState.IsValid() && NewState->GetState() == EMqttifyState::Connected;
				});

			TSharedRef<FMqttifyClientConnectingState> State = MakeShared<FMqttifyClientConnectingState>(OnStateChanged, Context, false, Socket);

			// Trigger TryConnect which will send CONNECT with auth props
			State->OnSocketConnect(true);

			// Simulate server AUTH Continue with method+data
			TArray<FMqttifyProperty> AuthProps;
			AuthProps.Add({FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationMethod>(FString(TEXT("TEST-AUTH")))});
			AuthProps.Add({FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationData>(TArray<uint8>{9,9})});
			TSharedRef<FMqttifyAuthPacket> ServerAuth = MakeShared<FMqttifyAuthPacket>(EMqttifyReasonCode::ContinueAuthentication, FMqttifyProperties(AuthProps));
			State->OnReceivePacket(EncodePacketToReader(ServerAuth));

			// Validate client sent AUTH Continue with provider response {0xFE} and echoed method
			{
				const TArray<uint8>& OutBytes = Socket->GetLastSentBytes();
				FArrayReader R; R.SetByteSwapping(true); R.Append(OutBytes.GetData(), OutBytes.Num());
				const FMqttifyFixedHeader H = FMqttifyFixedHeader::Create(R);
				TestEqual(TEXT("Outgoing packet type should be AUTH"), (int32)H.GetPacketType(), (int32)EMqttifyPacketType::Auth);
				FMqttifyAuthPacket ClientAuth(R, H);
				TestEqual(TEXT("Client AUTH rc"), (int32)ClientAuth.GetReasonCode(), (int32)EMqttifyReasonCode::ContinueAuthentication);
				FString Method; TArray<uint8> Data;
				for (const FMqttifyProperty& P : ClientAuth.GetProperties().GetProperties())
				{
					if (P.GetIdentifier() == EMqttifyPropertyIdentifier::AuthenticationMethod) { P.TryGetValue(Method); }
					if (P.GetIdentifier() == EMqttifyPropertyIdentifier::AuthenticationData) { P.TryGetValue(Data); }
				}
				TestEqual(TEXT("Echoed method"), Method, FString(TEXT("TEST-AUTH")));
				TestEqual(TEXT("Auth data length"), Data.Num(), 1);
				if (Data.Num() == 1) { TestEqual(TEXT("Auth data byte"), (int32)Data[0], 0xFE); }
			}

			// Now simulate successful CONNACK (server may include method too per spec)
			TArray<FMqttifyProperty> ConnAckProps;
			ConnAckProps.Add({FMqttifyProperty::Create<EMqttifyPropertyIdentifier::AuthenticationMethod>(FString(TEXT("TEST-AUTH")))});
			TSharedRef<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>> ServerConnAck = MakeShared<TMqttifyConnAckPacket<EMqttifyProtocolVersion::Mqtt_5>>(
				false,
				EMqttifyReasonCode::Success,
				FMqttifyProperties(ConnAckProps));
			State->OnReceivePacket(EncodePacketToReader(ServerConnAck));

			TestTrue(TEXT("Should transition to Connected after CONNACK"), bBecameConnected);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
