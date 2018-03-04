#include <iostream>
#include <stdexcept>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "json.hpp"

#include "scene.h"
#include "maths.h"
#include "mesh.h"

#include "scene_loading.h"

#define SCREEN_SIZE	512

namespace po = boost::program_options;

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
	SDL_Window* screen = SDL_CreateWindow("embree_viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_SIZE, SCREEN_SIZE, SDL_SWSURFACE | SDL_WINDOW_RESIZABLE);
	assert(screen != nullptr);

	SDL_Renderer* renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
	assert(renderer != nullptr);

	SDL_Surface* winSurface = SDL_GetWindowSurface(screen);
	if(winSurface == nullptr)
		throw std::runtime_error(SDL_GetError());

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_GetWindowSurface(screen)->format->format, SDL_TEXTUREACCESS_STREAMING, SCREEN_SIZE / 4, SCREEN_SIZE / 4);

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
			/////////////////////
			// EVENT LOOP
			/////////////////////
			{
				SDL_Event event;
				SDL_WaitEvent(&event);
				do {
					// quit
					if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
						quit = true;

					// camera motion
					else if(event.type == SDL_MOUSEMOTION) {
						int w, h;
						SDL_GetWindowSize(screen, &w, &h);

						if(event.motion.state & SDL_BUTTON_LMASK) {
							const float xangle = ((float)(event.motion.xrel) / (float)w) * M_PI;
							const float yangle = ((float)(event.motion.yrel) / (float)h) * M_PI;

							cam.rotate(xangle, -yangle);
						}

						else if(event.motion.state & SDL_BUTTON_RMASK) {
							const float ydiff = ((float)(event.motion.yrel) / (float)h);

							Vec3 tr = cam.target - cam.position;
							float dist = tr.length();
							tr.normalize();

							dist = powf(dist, 1.0f + ydiff);

							cam.position = cam.target - tr * dist;
						}
					}

					// window resizing
					else if(event.type == SDL_WINDOWEVENT) {
						if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
							SDL_DestroyTexture(texture);

							texture = SDL_CreateTexture(renderer, SDL_GetWindowSurface(screen)->format->format, SDL_TEXTUREACCESS_STREAMING, event.window.data1 / 4, event.window.data2 / 4);
						}
					}
				} while(SDL_PollEvent(&event));
			}

			/////////////////////
			// RENDERING
			/////////////////////
			{
				Uint32* pixels = nullptr;
				int pitch = 0;
				Uint32 format;

				int w, h;
				SDL_QueryTexture(texture, &format, NULL, &w, &h);

				if(SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch))
					throw std::runtime_error(SDL_GetError());

				for(int y = 0; y < h; ++y)
					for(int x = 0; x < w; ++x) {
						const float xf = ((float)x / (float)w - 0.5f) * 2.0f;
						const float yf = ((float)y / (float)h - 0.5f) * 2.0f;
						const float aspect = (float)w / (float)h;

						const Ray r = cam.makeRay(xf, -yf / aspect);

						const Vec3 color = scene.renderPixel(r);

						Uint32 rgb = SDL_MapRGBA(SDL_GetWindowSurface(screen)->format, (Uint8)(color.x * 255.0), (Uint8)(color.y * 255.0), (Uint8)(color.z * 255.0), 255);
						Uint32 pixelPosition = y * (pitch / sizeof(Uint32)) + x;
						pixels[pixelPosition] = rgb;
					}

				SDL_UnlockTexture(texture);

				// show the result by flipping the double buffer
				SDL_RenderCopy(renderer, texture, NULL, NULL);

				SDL_RenderPresent(renderer);
			}

			++ctr;
		}
	}

	// clean up
	SDL_DestroyRenderer(renderer);

	SDL_Quit();

	return 0;
}
