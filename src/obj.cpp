#include "obj.h"

#include <fstream>
#include <sstream>
#include <string>

#include "maths.h"
#include "mesh.h"

namespace {
	struct Face {
		int v = -1;
		int vn = -1;
		int vt = -1;
	};

	std::istream& operator >> (std::istream& in, Face& f) {
		f.v = -1;
		f.vn = -1;
		f.vt = -1;

		in >> f.v;
		if(in.peek() == '/') {
			in.get();

			if(in.peek() != '/')
				in >> f.vt;

			assert(in.peek() == '/');
			in.get();

			if(in.peek() >= '0' && in.peek() <= '9')
				in >> f.vn;

			assert(in.peek() != '/');
		}

		return in;
	}

	Mesh makeMesh(const std::vector<Vec3>& v, const std::vector<std::vector<Face>>& f) {
		std::size_t triangleCount = 0;
		for(auto& t : f)
			triangleCount += t.size()-2;

		Mesh result(v.size(), triangleCount);

		std::copy(v.begin(), v.end(), result.vertices().begin());

		std::size_t triangleIndex = 0;
		for(auto& t : f) {
			for(std::size_t ti=1;ti < t.size()-1; ++ti) {
				Triangle tr;
				tr.v0 = t[0].v-1;
				tr.v1 = t[ti].v-1;
				tr.v2 = t[ti+1].v-1;

				result.triangles()[triangleIndex] = tr;
				++triangleIndex;
			}
		}
		assert(triangleIndex == triangleCount);

		return result;
	}
}

Scene loadObj(boost::filesystem::path path) {
	Scene scene;

	std::vector<Vec3> vertices, normals;
	std::vector<std::vector<Face>> faces;

	std::ifstream file(path.string());

	while(!file.eof()) {
		std::string line;
		std::getline(file, line);

		if(!line.empty() && line[0] != '#') {
			std::stringstream linestr(line);

			std::string id;
			linestr >> id;

			if(id == "mtllib")
				;
			else if(id == "o") {
				if(!vertices.empty()) {
					scene.addMesh(makeMesh(vertices, faces));

					vertices.clear();
					normals.clear();
					faces.clear();
				}
			}
			else if(id == "v") {
				Vec3 v;
				linestr >> v.x >> v.y >> v.z;

				vertices.push_back(v);
			}
			else if(id == "vn") {
				Vec3 n;
				linestr >> n.x >> n.y >> n.z;

				normals.push_back(n);
			}
			else if(id == "f") {
				faces.push_back(std::vector<Face>());
				while(!linestr.eof()) {
					Face f;
					linestr >> f;

					if(f.v > 0)
						faces.back().push_back(f);
				}

				assert(faces.back().size() >= 3);
			}
		}
	}

	if(!vertices.empty())
		scene.addMesh(makeMesh(vertices, faces));

	return scene;
}
