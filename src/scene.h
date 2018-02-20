#pragma once

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

		void commit();

		Vec3 renderPixel(float x, float y);

	private:
		RTCDevice m_device;
		RTCScene m_scene;

};
