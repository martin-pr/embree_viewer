#include "scene.h"

#include <cmath>
#include <iostream>
#include <limits>

#include <embree3/rtcore_ray.h>

#include "mesh.h"

Scene::SceneHandle::SceneHandle(Device& device) {
	m_scene = rtcNewScene(device);

	rtcSetSceneFlags(m_scene, RTC_SCENE_FLAG_COMPACT);
}

Scene::SceneHandle::~SceneHandle() {
	rtcReleaseScene(m_scene);
}

Scene::SceneHandle::operator RTCScene& () {
	return m_scene;
}

Scene::SceneHandle::operator const RTCScene& () const {
	return m_scene;
}

////////////

Scene::Scene() : m_scene(new SceneHandle(m_device)) {
}

Scene::~Scene() {
}

Scene::Scene(Scene&& s) : m_device(s.m_device), m_scene(std::move(s.m_scene)) {
}

Scene& Scene::operator = (Scene&& s) {
	if(&s != this) {
		m_device = s.m_device;
		m_scene = std::move(s.m_scene);
	}

	return *this;
}

unsigned Scene::addMesh(Mesh&& geom) {
	unsigned int geomID = rtcAttachGeometry(*m_scene, geom.geom());

	rtcCommitGeometry(geom.geom());

	return geomID;
}

unsigned Scene::addInstance(const Scene& s, const Mat4& tr) {
	RTCGeometry instance = rtcNewGeometry(m_device, RTC_GEOMETRY_TYPE_INSTANCE);
	rtcSetGeometryInstancedScene(instance, *s.m_scene);
	unsigned int geomID = rtcAttachGeometry(*m_scene, instance);
	rtcReleaseGeometry(instance);

	rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, tr.m);

	rtcCommitGeometry(instance);

	return geomID;
}

void Scene::commit() {
	rtcCommitScene(*m_scene);
}

Vec3 Scene::renderPixel(const Ray& r) const {
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);

	RTCRayHit rayhit;

	rayhit.ray.org_x = r.origin.x;
	rayhit.ray.org_y = r.origin.y;
	rayhit.ray.org_z = r.origin.z;

	rayhit.ray.tnear = 0;
	rayhit.ray.tfar = std::numeric_limits<float>::infinity();

	rayhit.ray.dir_x = r.direction.x;
	rayhit.ray.dir_y = r.direction.y;
	rayhit.ray.dir_z = r.direction.z;

	rayhit.ray.time = 0;

	rayhit.ray.mask = 0xFFFFFFFF;
	rayhit.ray.id = 0;
	rayhit.ray.flags = 0;

	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect1(*m_scene, &context, &rayhit);

	Vec3 color{1, 1, 1};

	if(rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		//color = Vec3{(float)(rayhit.hit.geomID & 1), (float)((rayhit.hit.geomID >> 1) & 1), (float)((rayhit.hit.geomID >> 2) & 1)};
		color = Vec3(1, 0, 0);

		Vec3 norm(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
		norm.normalize();

		const float d = std::abs(r.direction.dot(norm));

		color.x *= d;
		color.y *= d;
		color.z *= d;
	}

	return color;
}
