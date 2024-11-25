#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MqttifyFArchiveEncodeDecode.h"

namespace Mqttify
{
	BEGIN_DEFINE_SPEC(
		MqttifyDataEncodingDecodingSpec,
		"Mqttify.Automation.MqttifyDataEncodingDecoding",
		EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::ServerContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::
		ProgramContext)
	END_DEFINE_SPEC(MqttifyDataEncodingDecodingSpec)

	void MqttifyDataEncodingDecodingSpec::Define()
	{
		// Testing DecodeVariableByteInteger and EncodeVariableByteInteger
		Describe(
			"VariableByteInteger",
			[this]
			{
				It(
					"Should encode and decode to the same value",
					[this]
					{
						FBufferArchive OutArchive;
						Data::EncodeVariableByteInteger(300, OutArchive);

						FArrayReader InArchive;
						InArchive.Append(OutArchive);
						SIZE_T DecodedValue = Data::DecodeVariableByteInteger(InArchive);

						TestEqual("Decoded value should match original", DecodedValue, 300);
					});
			});

		// Testing EncodeString and DecodeString
		Describe(
			"StringEncodingDecoding",
			[this]
			{
				It(
					"Should encode and decode to the same string",
					[this]
					{
						FString OriginalString = TEXT("MqttifyTest");

						FBufferArchive OutArchive;
						Data::EncodeString(OriginalString, OutArchive);

						FArrayReader InArchive;
						InArchive.Append(OutArchive);
						FString DecodedString = Data::DecodeString(InArchive);

						TestEqual("Decoded string should match original", DecodedString, OriginalString);
					});

				It(
					"Should fail gracefully if archive is not in saving mode for EncodeString",
					[this]
					{
						FArrayReader InArchive{};
						const FString OriginalString = TEXT("MqttifyTest");

						AddExpectedError(Data::ArchiveNotSaving, EAutomationExpectedErrorFlags::Contains, 1);
						Data::EncodeString(OriginalString, InArchive);
					});

				It(
					"Should fail gracefully if archive is not in loading mode for DecodeString",
					[this]
					{
						// Create an FBufferArchive and encode a string into it
						FBufferArchive OutArchive;
						const FString OriginalString = TEXT("MqttifyTest");
						Data::EncodeString(OriginalString, OutArchive);

						// Since we're using OutArchive, which is in saving mode, this should trigger the error.
						AddExpectedError(Data::ArchiveNotLoading, EAutomationExpectedErrorFlags::Contains, 1);
						Data::DecodeString(OutArchive);
					});
			});
	}
} // namespace Mqttify
#endif // WITH_DEV_AUTOMATION_TESTS
