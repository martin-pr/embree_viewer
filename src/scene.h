#pragma once

#include <memory>

#include <boost/noncopyable.hpp>

#include <rtcore.h>
#include <rtcore_device.h>
#include <rtcore_scene.h>

#include "maths.h"

class Scene : public boost::noncopyable {
	public:
		Scene();
		~Scene();

		unsigned addSphere(const Vec3& p, float r);
		unsigned addInstance(const Scene& scene, const Vec3& tr);

		void commit();

		Vec3 renderPixel(const Ray& r);

	private:
		struct ScopedDevice : public boost::noncopyable {
			ScopedDevice();
			~ScopedDevice();

			operator RTCDevice& ();
			operator const RTCDevice& () const;

			RTCDevice device;
		};

		static std::shared_ptr<ScopedDevice> device();

		std::shared_ptr<ScopedDevice> m_device;
		RTCScene m_scene;

};
