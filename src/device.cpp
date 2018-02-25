#include "device.h"

#include <iostream>

std::shared_ptr<Device::DeviceHandle> Device::sharedDevice() {
	static std::weak_ptr<Device::DeviceHandle> s_device;

	std::shared_ptr<Device::DeviceHandle> dev = s_device.lock();
	if(!dev) {
		dev = std::shared_ptr<Device::DeviceHandle>(new Device::DeviceHandle());
		s_device = dev;
	}

	return dev;
}

Device::Device() : m_device(sharedDevice()) {
}

Device::~Device() {
}

Device::operator RTCDevice& () {
	return m_device->device;
}

Device::operator const RTCDevice& () const {
	return m_device->device;
}

namespace {

void error_handler(void* /*userPtr*/, const RTCError, const char* str) {
	std::cerr << "Device error: " << str << std::endl;
}

}

Device::DeviceHandle::DeviceHandle() : device(rtcNewDevice(NULL)) {
	rtcSetDeviceErrorFunction(device, error_handler, NULL);
}

Device::DeviceHandle::~DeviceHandle() {
	rtcReleaseDevice(device);
}
