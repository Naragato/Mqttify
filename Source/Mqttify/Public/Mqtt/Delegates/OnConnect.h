#pragma once

namespace Mqttify
{
	DECLARE_TS_MULTICAST_DELEGATE_OneParam(FOnConnect, const bool /* IsConnected */);
} // namespace Mqttify
