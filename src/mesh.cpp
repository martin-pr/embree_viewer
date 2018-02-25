#include "mesh.h"

Mesh::Triangles::Triangles(Triangle* ptr, std::size_t size) : m_triangles(ptr), m_size(size) {
}

Triangle& Mesh::Triangles::operator[](std::size_t index) {
	assert(index < m_size);
	return m_triangles[index];
}

const Triangle& Mesh::Triangles::operator[](std::size_t index) const {
	assert(index < m_size);
	return m_triangles[index];
}

Mesh::Triangles::iterator Mesh::Triangles::begin() {
	return m_triangles;
}

Mesh::Triangles::iterator Mesh::Triangles::end() {
	return m_triangles + m_size;
}

Mesh::Triangles::const_iterator Mesh::Triangles::begin() const {
	return m_triangles;
}

Mesh::Triangles::const_iterator Mesh::Triangles::end() const {
	return m_triangles + m_size;
}

std::size_t Mesh::Triangles::size() const {
	return m_size;
}

/////////////////

Mesh::Vertices::Vertices(Vertex* ptr, std::size_t size) : m_vertices(ptr), m_size(size) {
}

Vertex& Mesh::Vertices::operator[](std::size_t index) {
	assert(index < m_size);
	return m_vertices[index];
}

const Vertex& Mesh::Vertices::operator[](std::size_t index) const {
	assert(index < m_size);
	return m_vertices[index];
}

Mesh::Vertices::iterator Mesh::Vertices::begin() {
	return m_vertices;
}

Mesh::Vertices::iterator Mesh::Vertices::end() {
	return m_vertices + m_size;
}

Mesh::Vertices::const_iterator Mesh::Vertices::begin() const {
	return m_vertices;
}

Mesh::Vertices::const_iterator Mesh::Vertices::end() const {
	return m_vertices + m_size;
}

///////////////////

Mesh::GeometryHandle::GeometryHandle(const Device& device) : m_geometry(rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE)) {
}

Mesh::GeometryHandle::~GeometryHandle() {
	rtcReleaseGeometry(m_geometry);
}

Mesh::GeometryHandle::operator RTCGeometry& () {
	return m_geometry;
}

Mesh::GeometryHandle::operator const RTCGeometry& () const {
	return m_geometry;
}

///////////////////

Mesh::Mesh(std::size_t vertexCount, std::size_t triangleCount) : m_geom(new GeometryHandle(m_device)),
	m_vertices((Vertex *)rtcSetNewGeometryBuffer(*m_geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex),
	           vertexCount), vertexCount),
	m_triangles((Triangle *)rtcSetNewGeometryBuffer(*m_geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle),
	            triangleCount), triangleCount)
{

}

Mesh::~Mesh() {

}

Mesh::Vertices& Mesh::vertices() {
	return m_vertices;
}

const Mesh::Vertices& Mesh::vertices() const {
	return m_vertices;
}

Mesh::Triangles& Mesh::triangles() {
	return m_triangles;
}

const Mesh::Triangles& Mesh::triangles() const {
	return m_triangles;
}

Mesh::Mesh(Mesh&& m) : m_device(m.m_device), m_geom(std::move(m.m_geom)), m_vertices(m.m_vertices), m_triangles(m.m_triangles) {

}

Mesh& Mesh::operator=(Mesh&& m) {
	m_device = m.m_device;
	m_geom = std::move(m.m_geom);
	m_vertices = m.m_vertices;
	m_triangles = m.m_triangles;

	return *this;
}

const RTCGeometry& Mesh::geom() const {
	assert(m_geom != nullptr);
	return *m_geom;
}

Mesh Mesh::makeSphere(const Vec3& p, float r, int numPhi, int numTheta) {
	Mesh result(numTheta * (numPhi + 1), 2 * numTheta * (numPhi - 1));

	/* create sphere */
	int tri = 0;
	const float rcpNumTheta = 1.0f / (float)numTheta;
	const float rcpNumPhi = 1.0f / (float)numPhi;
	for(int phi = 0; phi <= numPhi; phi++) {
		for(int theta = 0; theta < numTheta; theta++) {
			const float phif = phi * float(M_PI) * rcpNumPhi;
			const float thetaf = theta * 2.0f * float(M_PI) * rcpNumTheta;

			Vertex &v = result.vertices()[phi * numTheta + theta];
			v.x = p.x + r * sin(phif) * sin(thetaf);
			v.y = p.y + r * cos(phif);
			v.z = p.z + r * sin(phif) * cos(thetaf);
		}
		if(phi == 0)
			continue;

		for(int theta = 1; theta <= numTheta; theta++) {
			int p00 = (phi - 1) * numTheta + theta - 1;
			int p01 = (phi - 1) * numTheta + theta % numTheta;
			int p10 = phi * numTheta + theta - 1;
			int p11 = phi * numTheta + theta % numTheta;

			if(phi > 1) {
				result.triangles()[tri].v0 = p10;
				result.triangles()[tri].v1 = p01;
				result.triangles()[tri].v2 = p00;
				tri++;
			}

			if(phi < numPhi) {
				result.triangles()[tri].v0 = p11;
				result.triangles()[tri].v1 = p01;
				result.triangles()[tri].v2 = p10;
				tri++;
			}
		}
	}

	return result;
}
