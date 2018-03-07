#pragma once

#include <vector>
#include <memory>
#include <thread>

#include <SDL2/SDL.h>

#include "scene.h"
#include "texture.h"

class Renderer : public boost::noncopyable {
	public:
		Renderer(const Scene& scene, SDL_Window* window, SDL_Renderer* renderer);
		~Renderer();

		void setCamera(Camera& cam);
		Ray cameraRay(int x, int y, int w, int h) const;

		void resize(std::size_t /*w*/, std::size_t /*h*/);

		SDL_Texture* texture();
		int currentTexture() const;

	private:
		void startRenderThread();
		void stopRenderThread();

		void renderAll();
		void renderFrame(SDL_PixelFormat format);
		void renderTile(int xMin, int xMax, int yMin, int yMax, Uint32* pixels, int pitch, int w, int h, SDL_PixelFormat format);

		void initTextures();

		const Scene* m_scene;
		SDL_Window* m_window;
		SDL_Renderer* m_renderer;

		std::vector<std::unique_ptr<Texture>> m_textures;
		int m_currentTexture;

		Camera m_camera;

		bool m_rendering;
		std::unique_ptr<std::thread> m_thread;

		int m_width, m_height;
};
