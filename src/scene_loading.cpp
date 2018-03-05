#include "scene_loading.h"

#include <fstream>

#include <boost/filesystem.hpp>

#include <SDL2/SDL.h>

#include "alembic.h"
#include "obj.h"

namespace {

std::shared_ptr<Scene> parseMesh(const boost::filesystem::path& p) {
	std::unique_ptr<Scene> result(new Scene());

	if(p.extension() == ".abc")
		*result = loadAlembic(p);

	else if(p.extension() == ".obj")
		*result = loadObj(p);

	else
		throw std::runtime_error("unknown mesh file format - " + p.string());

	result->commit();

	return std::shared_ptr<Scene>(result.release());
}
}

Scene loadMesh(const boost::filesystem::path& p) {
	std::shared_ptr<Scene> result = parseMesh(p);

	return std::move(*result);
}

namespace {
struct Instance {
	Uint32 id;
	Mat4 transform;
};

Mat4 parseMat4(const nlohmann::json& source) {
	assert(source.is_array() && source.size() == 16);

	Mat4 tr;
	for(std::size_t i = 0; i < 16; ++i)
		tr.m[i] = source[i].get<float>();
	return tr;
}

std::shared_ptr<Scene> parseObject(const nlohmann::json& source, const boost::filesystem::path& scene_root, std::map<std::string, std::shared_ptr<Scene>>& instances);

std::shared_ptr<Scene> parseSubScene(const nlohmann::json& source, const boost::filesystem::path& scene_root, std::map<std::string, std::shared_ptr<Scene>>& instances) {
	std::shared_ptr<Scene> scene(new Scene());

	for(const auto& m : source)
		scene->addInstance(std::move(*parseObject(m, scene_root, instances)));

	scene->commit();

	return scene;
}

std::shared_ptr<Scene> parseObject(const nlohmann::json& source, const boost::filesystem::path& scene_root, std::map<std::string, std::shared_ptr<Scene>>& instances) {
	std::shared_ptr<Scene> scene(new Scene());

	auto path = source.find("path");
	auto transform = source.find("transform");

	auto objects = source.find("objects");
	auto instancesAttr = source.find("instances");
	auto instance_file = source.find("instance_file");

	auto scene_path = source.find("scene_path");

	Mat4 parentTransform;
	if(transform != source.end()) {
		assert(transform->is_array() && transform->size() == 16);
		parentTransform = parseMat4(*transform);
	}

	// already seen instance
	if(scene_path != source.end() && scene_path->is_string() && instances.find(scene_path->get<std::string>()) != instances.end()) {
		auto it = instances.find(scene_path->get<std::string>());

		scene->addInstance(*it->second, Mat4());
	}

	// a new instance
	else {
		// object = a single instance, most likely :)
		if(source.is_object() && path != source.end() && path->is_string()) {
			boost::filesystem::path p = path->get<std::string>();
			if(p.is_relative())
				p = scene_root / p;

			if(!boost::filesystem::exists(p))
				throw std::runtime_error("file not found - " + p.string());

			std::shared_ptr<Scene> item = parseMesh(p);
			scene->addInstance(*item, parentTransform);
		}

		// a subscene
		else if(source.is_object() && objects != source.end() && objects->is_array()) {
			// inline instancing
			if(instancesAttr != source.end()) {
				std::vector<Scene> items;
				for(auto& o : *objects)
					items.push_back(std::move(*parseObject(o, scene_root, instances)));

				for(auto& i : *instancesAttr) {
					auto id = i.find("id");
					auto transform = i.find("transform");
					assert(id != i.end() && transform != i.end());
					assert(id->is_number() && id->get<std::size_t>() < items.size());
					assert(transform->is_array() && transform->size() == 16);

					scene->addInstance(items[id->get<std::size_t>()], parseMat4(*transform) * parentTransform);
				}
			}

			// binary instancing fun
			else if(instance_file != source.end()) {
				std::vector<Scene> items;
				for(auto& o : *objects)
					items.push_back(std::move(*parseObject(o, scene_root, instances)));

				boost::filesystem::path p = instance_file->get<std::string>();
				if(p.is_relative())
					p = scene_root / p;

				if(!boost::filesystem::exists(p))
					throw std::runtime_error("file not found - " + p.string());

				std::ifstream file(p.string(), std::ios_base::binary);

				Instance i;
				assert(sizeof(i) == 17 * 4); // id + 16 floats

				while(!file.eof()) {
					file.read((char*)&i, sizeof(Instance));

					if(!file.eof())
						scene->addInstance(items[i.id], i.transform * parentTransform);
				}

			}

			// without instancing
			else
				for(auto& o : *objects) {
					std::shared_ptr<Scene> item = parseObject(o, scene_root, instances);
					scene->addInstance(*item, parentTransform);
				}
		}

		// a list of items as a subscene
		else if(source.is_array()) {
			std::shared_ptr<Scene> item = parseSubScene(source, scene_root, instances);
			scene->addInstance(*item, parentTransform);
		}


		// something else is an error
		else {
			std::stringstream err;
			err << "invalid syntax in scene file - " << source;
			throw std::runtime_error(err.str());
		}

		// deduplication
		if(scene_path != source.end() && scene_path->is_string() && (instances.find(scene_path->get<std::string>()) == instances.end()))
			instances.insert(std::make_pair(scene_path->get<std::string>(), scene));
	}

	scene->commit();

	return scene;
}
}

Scene parseScene(const nlohmann::json& source, const boost::filesystem::path& scene_root) {
	Scene scene;

	std::map<std::string, std::shared_ptr<Scene>> instances;

	for(const auto& m : source)
		scene.addInstance(*parseObject(m, scene_root, instances));

	scene.commit();

	return scene;
}
