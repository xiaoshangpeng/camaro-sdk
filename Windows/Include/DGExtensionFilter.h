#pragma once

#include "ExtensionFilterBase.h"

namespace TopGear
{
	namespace Win
	{
		class DGExtensionFilter : public ExtensionFilterBase
		{
		public:
			explicit DGExtensionFilter(IUnknown *pBase);
			virtual ~DGExtensionFilter() {}
			virtual std::string GetDeviceInfo() override;
		private:
			static const int DeviceInfoCode = 2;
			std::string deviceInfo;
		};
	}
}