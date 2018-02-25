#pragma once

#include <memory>

#include <boost/noncopyable.hpp>

#include <embree3/rtcore_device.h>

/// A simple wrapper of a shared device resource, which gets auto-released when no
/// longer needed
class Device {
	public:
		Device();
		~Device();

		operator RTCDevice& ();
		operator const RTCDevice& () const;

	private:
		struct DeviceHandle : public boost::noncopyable {
			DeviceHandle();
			~DeviceHandle();

			RTCDevice device;
		};

		/// returns a shared device pointer, or instantiates a new one if none exists yet
		static std::shared_ptr<DeviceHandle> sharedDevice();

		std::shared_ptr<DeviceHandle> m_device;
};
