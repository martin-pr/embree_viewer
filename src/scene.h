#pragma once

#include <memory>

#include <boost/noncopyable.hpp>

#include <embree3/rtcore.h>
#include <embree3/rtcore_scene.h>

#include "maths.h"
#include "device.h"

class Mesh;

class Scene : public boost::noncopyable {
	public:
		Scene();
		~Scene();

		unsigned addMesh(Mesh&& geom);
		unsigned addInstance(const Scene& scene, const Vec3& tr);

		void commit();

		Vec3 renderPixel(const Ray& r);

	private:
		Device m_device;
		RTCScene m_scene;

};
