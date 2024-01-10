#pragma once

namespace Mqttify
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnect, const bool /* IsConnected */);
} // namespace Mqttify
