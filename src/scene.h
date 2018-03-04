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

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		Scene(Scene&& s);
		Scene& operator = (Scene&& s);

		unsigned addMesh(Mesh&& geom);
		unsigned addInstance(const Scene& scene, const Mat4& tr = Mat4());

		void commit();

		Vec3 renderPixel(const Ray& r) const;
		RTCRayHit trace(const Ray& r) const;

	private:
		class SceneHandle {
			public:
				SceneHandle(Device& device);
				~SceneHandle();

				SceneHandle(const SceneHandle& m) = delete;
				SceneHandle& operator=(const SceneHandle& m) = delete;

				operator RTCScene& ();
				operator const RTCScene& () const;

			private:
				RTCScene m_scene;
		};

		Device m_device;

		std::unique_ptr<SceneHandle> m_scene;
};
