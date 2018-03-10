#include <iostream>
#include <stdexcept>
#include <fstream>
#include <thread>
#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "json.hpp"

#include "scene.h"
#include "maths.h"
#include "mesh.h"
#include "renderer.h"

#include "scene_loading.h"

#define SCREEN_SIZE	512

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
	po::options_description desc("Allowed options");

	desc.add_options()
	("help", "produce help message")
	("mesh", po::value<std::string>(), "load mesh file (.abc, .obj)")
	("scene", po::value<std::string>(), "load a scene file (.json)")
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
	SDL_Window* screen = SDL_CreateWindow("embree_viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_SIZE,
	                                      SCREEN_SIZE, SDL_SWSURFACE | SDL_WINDOW_RESIZABLE);
	assert(screen != nullptr);

	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
	assert(sdlRenderer != nullptr);

	SDL_Surface* winSurface = SDL_GetWindowSurface(screen);
	if(winSurface == nullptr)
		throw std::runtime_error(SDL_GetError());

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

		Renderer renderer(scene, screen, sdlRenderer);

		Camera cam;
		renderer.setCamera(cam);

		// the main loop
		int currentTexture = 0;

		bool quit = false;

		while(!quit) {
			/////////////////////
			// EVENT LOOP
			/////////////////////
			{
				SDL_Event event;
				while(SDL_PollEvent(&event)) {
					int w, h;
					SDL_GetWindowSize(screen, &w, &h);

					// quit
					if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
						quit = true;

					// camera motion
					else if(event.type == SDL_MOUSEMOTION) {
						if(event.motion.state & SDL_BUTTON_LMASK) {
							const float xangle = ((float)(event.motion.xrel) / (float)w) * M_PI;
							const float yangle = ((float)(event.motion.yrel) / (float)h) * M_PI;

							cam.rotate(xangle, -yangle);

							renderer.setCamera(cam);

							currentTexture = -1;
						}

						else if(event.motion.state & SDL_BUTTON_RMASK) {
							const float ydiff = ((float)(event.motion.yrel) / (float)h);

							Vec3 tr = cam.target - cam.position;
							float dist = tr.length();
							tr.normalize();

							dist *= 1.0f + (float)ydiff * 2.0;

							cam.position = cam.target - tr * dist;

							renderer.setCamera(cam);

							currentTexture = -1;
						}
					}

					else if(event.type == SDL_MOUSEBUTTONDOWN && event.button.clicks == 2) {
						RTCRayHit hit = scene.trace(renderer.cameraRay(event.button.x, event.button.y, w, h));

						Vec3 target{
							hit.ray.org_x + hit.ray.dir_x * hit.ray.tfar,
							hit.ray.org_y + hit.ray.dir_y * hit.ray.tfar,
							hit.ray.org_z + hit.ray.dir_z * hit.ray.tfar,
						};

						cam.target = target;

						renderer.setCamera(cam);

						currentTexture = -1;
					}

					// window resizing
					else if(event.type == SDL_WINDOWEVENT) {
						if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
							renderer.resize(event.window.data1 / 4, event.window.data2 / 4);

							currentTexture = -1;
						}
					}
				}
			}

			/////////////////////
			// RENDERING
			/////////////////////
			{
				const int textureId = renderer.currentTexture();
				if(currentTexture != textureId && textureId >= 0) {
					// show the result by flipping the double buffer
					SDL_RenderCopy(sdlRenderer, renderer.texture(), NULL, NULL);

					SDL_RenderPresent(sdlRenderer);

					currentTexture = textureId;
				}
			}

			usleep(5000);
		}
	}

	// clean up
	SDL_DestroyRenderer(sdlRenderer);

	SDL_Quit();

	return 0;
}
