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

#include "scene_loading.h"

#define SCREEN_SIZE	512
#define TEXTURE_LEVELS 8

namespace po = boost::program_options;

namespace {

class Renderer : public boost::noncopyable {
	public:
		Renderer(const Scene& scene, SDL_Window* window, SDL_Renderer* renderer) : m_scene(&scene), m_window(window),
			m_renderer(renderer), m_currentTexture(0), m_rendering(false) {

			m_textures.resize(TEXTURE_LEVELS, nullptr);
			initTextures();
		}

		~Renderer() {
			m_rendering = false;
			if(m_thread.get() != nullptr)
				m_thread->join();
			m_thread.reset();

			for(auto& t : m_textures)
				SDL_DestroyTexture(t);
		}

		void setCamera(Camera& cam) {
			m_camera = cam;

			startRenderThread();
		}

		void resize(std::size_t /*w*/, std::size_t /*h*/) {
			initTextures();

			startRenderThread();
		}

		SDL_Texture* texture() {
			assert(m_currentTexture > 0);
			return m_textures[m_currentTexture-1];
		}

		int currentTexture() const {
			return m_currentTexture-1;
		}

	private:
		void startRenderThread() {
			m_rendering = false;
			if(m_thread.get() != nullptr)
				m_thread->join();

			m_currentTexture = 0;
			m_rendering = true;

			std::function<void()> renderFunctor(std::bind(&Renderer::renderAll, this));
			m_thread = std::unique_ptr<std::thread>(new std::thread(renderFunctor));
		}

		void renderAll() {
			while(m_rendering && m_currentTexture < (int)m_textures.size())
				renderFrame();
		}

		void renderFrame() {
			if(m_currentTexture < (int)m_textures.size() && m_rendering) {
				Uint32* pixels = nullptr;
				int pitch = 0;
				Uint32 format;

				int w, h;
				SDL_QueryTexture(m_textures[m_currentTexture], &format, NULL, &w, &h);

				if(SDL_LockTexture(m_textures[m_currentTexture], nullptr, (void**)&pixels, &pitch))
					throw std::runtime_error(SDL_GetError());

				for(int y = 0; y < h && m_rendering; ++y)
					for(int x = 0; x < w && m_rendering; ++x) {
						const float xf = ((float)x / (float)w - 0.5f) * 2.0f;
						const float yf = ((float)y / (float)h - 0.5f) * 2.0f;
						const float aspect = (float)w / (float)h;

						const Ray r = m_camera.makeRay(xf, -yf / aspect);

						const Vec3 color = m_scene->renderPixel(r);

						Uint32 rgb = SDL_MapRGBA(SDL_GetWindowSurface(m_window)->format, (Uint8)(color.x * 255.0), (Uint8)(color.y * 255.0),
						                         (Uint8)(color.z * 255.0), 255);
						Uint32 pixelPosition = y * (pitch / sizeof(Uint32)) + x;
						pixels[pixelPosition] = rgb;
					}

				SDL_UnlockTexture(m_textures[m_currentTexture]);

				if(m_rendering)
					++m_currentTexture;
			}
			else
				m_rendering = false;
		}

		void initTextures() {
			int w, h;
			SDL_GetWindowSize(m_window, &w, &h);

			for(std::vector<SDL_Texture*>::reverse_iterator it = m_textures.rbegin(); it != m_textures.rend(); ++it) {
				if(*it != nullptr)
					SDL_DestroyTexture(*it);

				*it = SDL_CreateTexture(m_renderer, SDL_GetWindowSurface(m_window)->format->format, SDL_TEXTUREACCESS_STREAMING,
				                        w, h);

				w /= 2;
				h /= 2;
			}
		}

		const Scene* m_scene;
		SDL_Window* m_window;
		SDL_Renderer* m_renderer;

		std::vector<SDL_Texture*> m_textures;
		int m_currentTexture;

		Camera m_camera;

		bool m_rendering;
		std::unique_ptr<std::thread> m_thread;
};

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

							renderer.setCamera(cam);

							currentTexture = -1;
						}

						else if(event.motion.state & SDL_BUTTON_RMASK) {
							const float ydiff = ((float)(event.motion.yrel) / (float)h);

							Vec3 tr = cam.target - cam.position;
							float dist = tr.length();
							tr.normalize();

							dist = powf(dist, 1.0f + ydiff);

							cam.position = cam.target - tr * dist;

							renderer.setCamera(cam);

							currentTexture = -1;
						}
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
