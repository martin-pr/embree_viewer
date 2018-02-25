#pragma once

#include <vector>
#include <memory>

#include <embree3/rtcore_geometry.h>

#include "maths.h"
#include "device.h"

/// A simple non-copyable (only movable) representation of a mesh.
/// The non-copyability follows the design of Embree's geometry.
class Mesh {
	public:
		class Triangles {
			public:
				Triangle& operator[](std::size_t index);
				const Triangle& operator[](std::size_t index) const;

				typedef Triangle* iterator;
				iterator begin();
				iterator end();

				typedef const Triangle* const_iterator;
				const_iterator begin() const;
				const_iterator end() const;

				std::size_t size() const;

			private:
				Triangles(Triangle* ptr, std::size_t size);

				Triangles(const Triangles& t) = default;
				Triangles& operator=(const Triangles& t) = default;

				Triangle* m_triangles;
				std::size_t m_size;

				friend class Mesh;
		};

		class Vertices {
			public:
				Vertex& operator[](std::size_t index);
				const Vertex& operator[](std::size_t index) const;

				typedef Vertex* iterator;
				iterator begin();
				iterator end();

				typedef const Vertex* const_iterator;
				const_iterator begin() const;
				const_iterator end() const;

			private:
				Vertices(Vertex* ptr, std::size_t size);

				Vertices(const Vertices& t) = default;
				Vertices& operator=(const Vertices& t) = default;

				Vertex* m_vertices;
				std::size_t m_size;

				friend class Mesh;
		};

		Mesh(std::size_t vertexCount, std::size_t triangleCount);
		~Mesh();

		Mesh(const Mesh& m) = delete;
		Mesh& operator=(const Mesh& m) = delete;

		Mesh(Mesh&& m);
		Mesh& operator=(Mesh&& m);

		Vertices& vertices();
		const Vertices& vertices() const;

		Triangles& triangles();
		const Triangles& triangles() const;

		const RTCGeometry& geom() const;

		static Mesh makeSphere(const Vec3& p, float r, int numPhi = 5, int numTheta = 10);

	private:
		class GeometryHandle {
			public:
				GeometryHandle(const Device& device);
				~GeometryHandle();

				GeometryHandle(const GeometryHandle& m) = delete;
				GeometryHandle& operator=(const GeometryHandle& m) = delete;

				operator RTCGeometry& ();
				operator const RTCGeometry& () const;

			private:
				RTCGeometry m_geometry;
		};

		Device m_device;
		std::unique_ptr<GeometryHandle> m_geom;

		Vertices m_vertices;
		Triangles m_triangles;
};
