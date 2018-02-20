#pragma once

#include <cmath>

struct Vec3 {
	Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {
	}

	float x, y, z;

	void normalize() {
		const float n = std::sqrt(x*x + y*y + z*z);
		x /= n;
		y /= n;
		z /= n;
	}

	float dot(const Vec3& v) const {
		return x*v.x + y*v.y + z*v.z;
	}
} __attribute__ ((aligned (16)));

typedef Vec3 Vertex;

struct Triangle {
	unsigned v0, v1, v2;
} __attribute__ ((aligned (32)));
