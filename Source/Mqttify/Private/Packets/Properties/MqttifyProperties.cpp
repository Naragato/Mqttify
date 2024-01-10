#include "Packets/Properties/MqttifyProperties.h"
#include "Mqtt/MqttifyErrorStrings.h"
#include "Serialization/MqttifyFArchiveEncodeDecode.h"

void Mqttify::FMqttifyProperties::Decode(FArrayReader& InReader)
{
	InReader.SetByteSwapping(true);
	int RemainingLength = Data::DecodeVariableByteInteger(InReader);
	int32 ReaderPos     = InReader.Tell();
	while (RemainingLength > 0)
	{
		FMqttifyProperty Property{ InReader };
		if (ReaderPos == InReader.Tell())
		{
			LOG_MQTTIFY(
				Error,
				TEXT("[Properties] %s Expected: %d, Actual: %d."),
				ErrorStrings::ArchiveTooSmall,
				RemainingLength,
				InReader.Tell() - ReaderPos);
			return;
		}
		ReaderPos = InReader.Tell();
		Properties.Add(Property);
		RemainingLength -= Property.GetLength();
	}
}

void Mqttify::FMqttifyProperties::Encode(FMemoryWriter& InWriter)
{
	InWriter.SetByteSwapping(true);
	Data::EncodeVariableByteInteger(GetLength(false), InWriter);
	for (FMqttifyProperty& Property : Properties)
	{
		Property.Encode(InWriter);
	}
}

uint32 Mqttify::FMqttifyProperties::GetLength(const bool bWithLengthField) const
{
	uint32 TotalLength = 0;

	for (const FMqttifyProperty& Property : Properties)
	{
		TotalLength += Property.GetLength();
	}

	if (bWithLengthField)
	{
		if (TotalLength > 0)
		{
			TotalLength += Data::VariableByteIntegerSize(TotalLength);
		}
		else
		{
			TotalLength += 1; // Byte for Properties length being 0
		}
	}

	return TotalLength;
}
