#include "alembic.h"

#include <Alembic/AbcCoreFactory/IFactory.h>

#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>

#include "mesh.h"

namespace {

void extractMeshes(std::vector<Mesh>& meshes, const Alembic::Abc::IObject& obj, const Alembic::Abc::M44d& current = Alembic::Abc::M44d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)) {
	if(Alembic::AbcGeom::IXform::matches(obj.getHeader())) {
		Alembic::AbcGeom::IXform xform(obj, Alembic::Abc::kWrapExisting);

		Alembic::AbcGeom::XformSample value = xform.getSchema().getValue();
		Alembic::Abc::M44d matrix = value.getMatrix() * current;

		for(std::size_t i = 0; i < obj.getNumChildren(); ++i)
			extractMeshes(meshes, obj.getChild(i), matrix);
	}

	else if(Alembic::AbcGeom::IPolyMesh::matches(obj.getHeader())) {
		Alembic::AbcGeom::IPolyMesh inmesh(obj, Alembic::Abc::kWrapExisting);

		Alembic::AbcGeom::IPolyMeshSchema::Sample value = inmesh.getSchema().getValue();

		Alembic::Abc::Int32ArraySamplePtr faceCounts = value.getFaceCounts();
		Alembic::Abc::Int32ArraySamplePtr faceIndices = value.getFaceIndices();
		Alembic::Abc::P3fArraySamplePtr positions = value.getPositions();

		// count the number of triangles
		std::size_t triangleCount = 0;
		for(std::size_t i = 0; i < faceCounts->size(); ++i)
			triangleCount += (*faceCounts)[i] - 2;

		// make the mesh instance and transfer the data
		Mesh mesh(positions->size(), triangleCount);

		for(std::size_t i = 0; i < positions->size(); ++i) {
			const Imath::V3f pf = (*positions)[i];

			Imath::V3f pd = Imath::V3d(pf.x, pf.y, pf.z) * current;

			mesh.vertices()[i] = Vec3(pd.x, pd.y, pd.z);
		}

		{
			std::size_t triangleIndex = 0;
			std::size_t vertexIndex = 0;
			for(std::size_t i = 0; i < faceCounts->size(); ++i) {
				const std::size_t faceCount = (*faceCounts)[i];
				assert(faceCount >= 3);
				for(std::size_t i = 1; i < faceCount - 1; ++i) {
					Triangle& tr = mesh.triangles()[triangleIndex];

					tr.v0 = (*faceIndices)[vertexIndex];
					tr.v1 = (*faceIndices)[vertexIndex + i];
					tr.v2 = (*faceIndices)[vertexIndex + i+1];

					++triangleIndex;
				}
				vertexIndex += faceCount;
			}

			assert(triangleIndex == mesh.triangles().size());
			assert(vertexIndex == faceIndices->size());
		}

		meshes.push_back(std::move(mesh));
	}

	else if(Alembic::AbcGeom::ISubD::matches(obj.getHeader())) {
		Alembic::AbcGeom::ISubD inmesh(obj, Alembic::Abc::kWrapExisting);

		Alembic::AbcGeom::ISubDSchema::Sample value = inmesh.getSchema().getValue();

		Alembic::Abc::Int32ArraySamplePtr faceCounts = value.getFaceCounts();
		Alembic::Abc::Int32ArraySamplePtr faceIndices = value.getFaceIndices();
		Alembic::Abc::P3fArraySamplePtr positions = value.getPositions();

		// count the number of triangles
		std::size_t triangleCount = 0;
		for(std::size_t i = 0; i < faceCounts->size(); ++i)
			triangleCount += (*faceCounts)[i] - 2;

		// make the mesh instance and transfer the data
		Mesh mesh(positions->size(), triangleCount);

		for(std::size_t i = 0; i < positions->size(); ++i) {
			const Imath::V3f pf = (*positions)[i];

			Imath::V3f pd = Imath::V3d(pf.x, pf.y, pf.z) * current;

			mesh.vertices()[i] = Vec3(pd.x, pd.y, pd.z);
		}

		{
			std::size_t triangleIndex = 0;
			std::size_t vertexIndex = 0;
			for(std::size_t i = 0; i < faceCounts->size(); ++i) {
				const std::size_t faceCount = (*faceCounts)[i];
				assert(faceCount >= 3);
				for(std::size_t i = 1; i < faceCount - 1; ++i) {
					Triangle& tr = mesh.triangles()[triangleIndex];

					tr.v0 = (*faceIndices)[vertexIndex];
					tr.v1 = (*faceIndices)[vertexIndex + i];
					tr.v2 = (*faceIndices)[vertexIndex + i+1];

					++triangleIndex;
				}
				vertexIndex += faceCount;
			}

			assert(triangleIndex == mesh.triangles().size());
			assert(vertexIndex == faceIndices->size());
		}

		meshes.push_back(std::move(mesh));
	}
	else
		for(std::size_t i = 0; i < obj.getNumChildren(); ++i)
			extractMeshes(meshes, obj.getChild(i), current);
}

}

Scene loadAlembic(boost::filesystem::path path) {
	static Alembic::AbcCoreFactory::IFactory s_factory;

	Alembic::Abc::IArchive archive = s_factory.getArchive(path.string());

	std::vector<Mesh> meshes;
	extractMeshes(meshes, archive.getTop());

	Scene result;

	while(!meshes.empty()) {
		Mesh m = std::move(meshes.back());
		meshes.pop_back();

		result.addMesh(std::move(m));
	}

	return result;
}
