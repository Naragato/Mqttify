#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Packets/Properties/MqttifyProperty.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(MqttifyPropertySpec,
					"Mqttify.Automation.MqttifyProperty",
					EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

	END_DEFINE_SPEC(MqttifyPropertySpec)

	void MqttifyPropertySpec::Define()
	{

		Describe("For invalid Property Identifiers",
				[this] {
					TArray<uint8> InvalidIdentifiers =
						{ 0, 4, 5, 6, 7, 10, 12, 13, 14, 15, 16, 20, 27, 29, 30, 32, 43 };

					for (const uint8& Identifier : InvalidIdentifiers)
					{
						It(FString::Printf(TEXT("Should return unknown property for %d"), Identifier),
							[this, Identifier] {
								const TArray Bytes{ Identifier };
								FArrayReader ArrayReader;
								ArrayReader.Append(Bytes);
								const FMqttifyProperty Property(ArrayReader);
								TestEqual("Identifier should be unknown",
										Property.GetIdentifier(),
										EMqttifyPropertyIdentifier::Unknown);
							});
					}
				});

		// For uint8
		Describe("FMqttifyProperty for PayloadFormatIndicator",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint8 Data            = 1;
							const FMqttifyProperty Property =
								FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator>(
									Data);

							TestEqual("Code should be PayloadFormatIndicator",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::PayloadFormatIndicator);
						});

					It("Should serialize and deserialize correctly",
						[this] {
							uint8 Data                        = 1;
							FMqttifyProperty OriginalProperty =
								FMqttifyProperty::Create<EMqttifyPropertyIdentifier::PayloadFormatIndicator>(
									Data);

							TArray<uint8> Bytes;
							FMemoryWriter MemoryWriter(Bytes);
							OriginalProperty.Encode(MemoryWriter);

							FArrayReader ArrayReader;
							ArrayReader.Append(Bytes);
							FMqttifyProperty DeserializedProperty(ArrayReader);

							TestEqual("Code should be PayloadFormatIndicator after deserialization",
									DeserializedProperty.GetIdentifier(),
									EMqttifyPropertyIdentifier::PayloadFormatIndicator);

							uint8 DeserializedData;
							TestTrue("Data should be valid after deserialization",
									DeserializedProperty.TryGetValue(DeserializedData) && DeserializedData == Data);
						});
				});

		Describe("FMqttifyProperty for ContentType",
				[this] {
					It("Should have the correct code",
						[this] {
							const FString Data              = TEXT("application/json");
							const FMqttifyProperty Property =
								FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ContentType>(Data);

							TestEqual("Code should be ContentType",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::ContentType);
						});

					It("Should serialize and deserialize correctly",
						[this] {
							FString Data                      = TEXT("application/json");
							FMqttifyProperty OriginalProperty =
								FMqttifyProperty::Create<EMqttifyPropertyIdentifier::ContentType>(Data);

							TArray<uint8> Bytes;
							FMemoryWriter MemoryWriter(Bytes);
							OriginalProperty.Encode(MemoryWriter);

							FArrayReader ArrayReader;
							ArrayReader.Append(Bytes);
							FMqttifyProperty DeserializedProperty(ArrayReader);

							TestEqual("Code should be ContentType after deserialization",
									DeserializedProperty.GetIdentifier(),
									EMqttifyPropertyIdentifier::ContentType);

							FString DeserializedData;
							DeserializedProperty.TryGetValue(DeserializedData);
							TestEqual("Data should be valid after deserialization", DeserializedData, Data);
						});
				});

		Describe("FMqttifyProperty for MaximumQoS",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint8 Data            = 1;
							const FMqttifyProperty Property =
								FMqttifyProperty::Create<EMqttifyPropertyIdentifier::MaximumQoS>(Data);
							TestEqual("Code should be MaximumQoS",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::MaximumQoS);
						});
				});

		Describe("FMqttifyProperty for RetainAvailable",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint8 Data            = 1;
							const FMqttifyProperty Property = FMqttifyProperty::Create<
								EMqttifyPropertyIdentifier::RetainAvailable>(Data);
							TestEqual("Code should be RetainAvailable",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::RetainAvailable);
						});
				});

		Describe("FMqttifyProperty for UserProperty",
				[this] {
					It("Should have the correct code",
						[this] {
							const TTuple<FString, FString> Data = MakeTuple(TEXT("Key"), TEXT("Value"));
							const FMqttifyProperty Property     = FMqttifyProperty::Create<
								EMqttifyPropertyIdentifier::UserProperty>(Data);
							TestEqual("Code should be UserProperty",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::UserProperty);
						});
				});

		Describe("FMqttifyProperty for MaximumPacketSize",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint32 Data           = 1000;
							const FMqttifyProperty Property = FMqttifyProperty::Create<
								EMqttifyPropertyIdentifier::MaximumPacketSize>(Data);
							TestEqual("Code should be MaximumPacketSize",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::MaximumPacketSize);
						});
				});

		Describe("FMqttifyProperty for WildcardSubscriptionAvailable",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint8 Data            = 1;
							const FMqttifyProperty Property = FMqttifyProperty::Create<
								EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable>(Data);
							TestEqual("Code should be WildcardSubscriptionAvailable",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::WildcardSubscriptionAvailable);
						});
				});

		Describe("FMqttifyProperty for SubscriptionIdAvailable",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint8 Data            = 1;
							const FMqttifyProperty Property = FMqttifyProperty::Create<
								EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable>(Data);
							TestEqual("Code should be SubscriptionIdAvailable",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::SubscriptionIdentifierAvailable);
						});
				});

		Describe("FMqttifyProperty for SharedSubscriptionAvailable",
				[this] {
					It("Should have the correct code",
						[this] {
							constexpr uint8 Data            = 1;
							const FMqttifyProperty Property = FMqttifyProperty::Create<
								EMqttifyPropertyIdentifier::SharedSubscriptionAvailable>(Data);
							TestEqual("Code should be SharedSubscriptionAvailable",
									Property.GetIdentifier(),
									EMqttifyPropertyIdentifier::SharedSubscriptionAvailable);
						});
				});
	}
} // namespace Mqttify
#endif // WITH_AUTOMATION_TESTS
