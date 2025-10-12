#pragma once
#include "Interface/IMqttifyControlPacket.h"

namespace Mqttify
{
	class FMqttifyPacketCache final
	{
	public:
		explicit FMqttifyPacketCache(const int32 InMaxSize = 1024)
			: MaxSize(InMaxSize) {}

		void AddPacket(const int32 PacketId, const TSharedRef<IMqttifyControlPacket>& Packet)
		{
			FWriteScopeLock WriteLock(RWLock);

			if (MaxSize > 0 && PacketOrder.Num() >= MaxSize)
			{
				const int32 EvictId = PacketOrder[0];
				PacketOrder.RemoveAt(0);
				PacketMap.Remove(EvictId);
			}

			PacketMap.Add(PacketId, Packet);
			PacketOrder.Add(PacketId);
		}

		bool RemovePacket(const int32 PacketId)
		{
			FWriteScopeLock WriteLock(RWLock);

			const int32 Removed = PacketMap.Remove(PacketId);
			PacketOrder.Remove(PacketId);

			return Removed > 0;
		}

		TSharedPtr<IMqttifyControlPacket> GetPacket(const int32 PacketId) const
		{
			FReadScopeLock ReadLock(RWLock);

			return PacketMap.FindRef(PacketId);
		}

		TSharedPtr<IMqttifyControlPacket> GetAndRemovePacket(const int32 PacketId)
		{
			FWriteScopeLock WriteLock(RWLock);

			TSharedPtr<IMqttifyControlPacket> Packet = PacketMap.FindRef(PacketId);
			if (Packet.IsValid())
			{
				PacketMap.Remove(PacketId);
				PacketOrder.Remove(PacketId);
			}
			return Packet;
		}

	private:
		TMap<int32, TSharedRef<IMqttifyControlPacket>> PacketMap;
		TArray<int32> PacketOrder;
		int32 MaxSize;
		mutable FRWLock RWLock;
	};
} // namespace Mqttify