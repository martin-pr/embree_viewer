#include <iostream>
#include <stdexcept>
#include <fstream>

#include <SDL/SDL.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "json.hpp"

#include "scene.h"
#include "maths.h"
#include "mesh.h"

#include "alembic.h"
#include "obj.h"

#define SCREEN_SIZE	512
#define SCREEN_BPP	32

namespace {
void set_pixel(SDL_Surface *surface, int x, int y, const Vec3& pixel) {
	Uint32 *target_pixel = (Uint32*)((Uint8 *)surface->pixels + y * surface->pitch +
	                                 x * sizeof(* target_pixel));

	Uint32 value = ((Uint32)(pixel.x * 255) << 16) + ((Uint32)(pixel.y * 255) << 8) +
	               ((Uint32)(pixel.z * 255) << 0) + (255 << 24);

	*target_pixel = value;
}
}

namespace po = boost::program_options;

namespace {
	Scene loadMesh(const boost::filesystem::path& p) {
		Scene result;

		if(p.extension() == ".abc")
			result = loadAlembic(p);

		else if(p.extension() == ".obj")
			result = loadObj(p);

		else
			throw std::runtime_error("unknown mesh file format - " + p.string());

		result.commit();

		return result;
	}

	Mat4 parseMat4(const nlohmann::json& source) {
		assert(source.is_array() && source.size() == 16);

		Mat4 tr;
		for(std::size_t i=0;i<16;++i)
			tr.m[i] = source[i].get<float>();
		return tr;
	}

	Scene parseScene(const nlohmann::json& source, const boost::filesystem::path& scene_root);

	Scene parseObject(const nlohmann::json& source, const boost::filesystem::path& scene_root) {
		Scene scene;

		auto path = source.find("path");
		auto transform = source.find("transform");

		auto objects = source.find("objects");
		auto instances = source.find("instances");

		// object = a single instance, most likely :)
		if(source.is_object() && path != source.end() && path->is_string() && transform != source.end() && transform->is_array() && transform->size() == 16) {
			boost::filesystem::path p = path->get<std::string>();
			if(p.is_relative())
				p = scene_root / p;

			if(!boost::filesystem::exists(p))
				throw std::runtime_error("file not found - " + p.string());

			Scene item = loadMesh(p);
			scene.addInstance(item, parseMat4(*transform));
		}

		// a subscene
		else if(source.is_object() && objects != source.end() && objects->is_array()) {
			// without instancing
			if(instances == source.end())
				for(auto& o : *objects) {
					Scene item = parseObject(o, scene_root);
					scene.addInstance(item);
				}

			// with instancing
			else {
				std::vector<Scene> items;
				for(auto& o : *objects)
					items.push_back(parseObject(o, scene_root));

				for(auto& i : *instances) {
					auto id = i.find("id");
					auto transform = i.find("transform");
					assert(id != i.end() && transform != i.end());
					assert(id->is_number() && id->get<std::size_t>() < items.size());
					assert(transform->is_array() && transform->size() == 16);

					scene.addInstance(items[id->get<std::size_t>()], parseMat4(*transform));
				}
			}
		}

		// a list of items as a subscene
		else if(source.is_array()) {
			Scene item = parseScene(source, scene_root);
			scene.addInstance(item);
		}


		// something else is an error
		else {
			std::stringstream err;
			err << "invalid syntax in scene file - " << source;
			throw std::runtime_error(err.str());
		}

		scene.commit();

		return scene;
	}

	Scene parseScene(const nlohmann::json& source, const boost::filesystem::path& scene_root) {
		Scene scene;

		for(const auto& m : source)
			scene.addInstance(parseObject(m, scene_root));

		scene.commit();

		return scene;
	}
}

int main(int argc, char* argv[]) {
	po::options_description desc("Allowed options");

	desc.add_options()
	("help", "produce help message")
	("mesh", po::value<std::string>(), "load mesh file (Alembic)")
	("scene", po::value<std::string>(), "load a JSON scene file")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if(vm.count("help")) {
		std::cout << desc << std::endl;
		return 1;
	}

	// SDL initialisation
	if(SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(SDL_GetError());

	// make the window
	SDL_Surface* screen = SDL_SetVideoMode(SCREEN_SIZE, SCREEN_SIZE, SCREEN_BPP, SDL_SWSURFACE | SDL_RESIZABLE);

	{
		// make the scene
		Scene scene;
		if(vm.count("mesh"))
			scene = loadMesh(vm["mesh"].as<std::string>());

		else if(vm.count("scene")) {
			nlohmann::json source;
			{
				std::ifstream file(vm["scene"].as<std::string>());
				file >> source;
			}
			assert(source.is_array());

			const boost::filesystem::path scene_root = boost::filesystem::path(vm["scene"].as<std::string>()).parent_path();

			scene = parseScene(source, scene_root);
		}

		else {
			Mesh m = Mesh::makeSphere(Vec3{0, 0, 0}, 100);
			scene.addMesh(std::move(m));
		}

		scene.commit();

		///////////////////////////

		Camera cam;

		// the main loop
		unsigned ctr = 0;

		bool quit = false;

		while(!quit) {
			// render pixels
			for(int y = 0; y < screen->h; ++y)
				for(int x = 0; x < screen->w; ++x) {
					const float xf = ((float)x / (float)screen->w - 0.5f) * 2.0f;
					const float yf = ((float)y / (float)screen->h - 0.5f) * 2.0f;
					const float aspect = (float)screen->w / (float)screen->h;

					const Ray r = cam.makeRay(xf, -yf / aspect);

					const Vec3 color = scene.renderPixel(r);

					set_pixel(screen, x, y, color);
				}

			// show the result by flipping the double buffer
			SDL_Flip(screen);

			// exit condition
			{
				SDL_Event event;
				while(SDL_PollEvent(&event)) {
					// quit
					if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
						quit = true;

					// camera motion
					else if(event.type == SDL_MOUSEMOTION) {
						if(event.motion.state & SDL_BUTTON_LMASK) {
							const float xangle = ((float)(event.motion.xrel) / (float)screen->w) * M_PI;
							const float yangle = ((float)(event.motion.yrel) / (float)screen->h) * M_PI;

							cam.rotate(xangle, -yangle);
						}

						else if(event.motion.state & SDL_BUTTON_RMASK) {
							const float ydiff = ((float)(event.motion.yrel) / (float)screen->h);

							Vec3 tr = cam.target - cam.position;
							float dist = tr.length();
							tr.normalize();

							dist = powf(dist, 1.0f + ydiff);

							cam.position = cam.target - tr * dist;
						}
					}

					// window resizing
					else if(event.type == SDL_VIDEORESIZE) {
						screen = SDL_SetVideoMode(event.resize.w, event.resize.h, SCREEN_BPP, SDL_SWSURFACE | SDL_RESIZABLE);
					}
				}
			}

			++ctr;
		}
	}

	// clean up
	SDL_FreeSurface(screen);
	SDL_Quit();

	return 0;
}
