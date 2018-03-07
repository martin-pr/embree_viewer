#include "renderer.h"

#include <tbb/parallel_for.h>

#define TEXTURE_LEVELS 8
#define TILE_SUBDIV 8

Renderer::Renderer(const Scene& scene, SDL_Window* window, SDL_Renderer* renderer) : m_scene(&scene), m_window(window),
	m_renderer(renderer), m_currentTexture(0), m_rendering(false) {

	m_textures.resize(TEXTURE_LEVELS);
	initTextures();
}

Renderer::~Renderer() {
	stopRenderThread();

	for(auto& t : m_textures)
		if(t->isLocked())
			t->unlock();
}

void Renderer::setCamera(Camera& cam) {
	stopRenderThread();

	m_camera = cam;

	startRenderThread();
}

void Renderer::resize(std::size_t /*w*/, std::size_t /*h*/) {
	initTextures();

	startRenderThread();
}

SDL_Texture* Renderer::texture() {
	assert(m_currentTexture > 0);

	const int current = m_currentTexture-1;

	if(m_textures[current]->isLocked())
		m_textures[current]->unlock();

	return m_textures[current]->texture();
}

int Renderer::currentTexture() const {
	return m_currentTexture - 1;
}

void Renderer::startRenderThread() {
	stopRenderThread();

	m_currentTexture = 0;
	m_rendering = true;

	for(auto& t : m_textures)
		t->lock();

	std::function<void()> renderFunctor(std::bind(&Renderer::renderAll, this));
	m_thread = std::unique_ptr<std::thread>(new std::thread(renderFunctor));
}

void Renderer::stopRenderThread() {
	m_rendering = false;
	if(m_thread.get() != nullptr)
		m_thread->join();
	m_thread.reset();
}

void Renderer::renderAll() {
	auto format = *SDL_GetWindowSurface(m_window)->format;
	while(m_rendering && m_currentTexture < (int)m_textures.size())
		renderFrame(format);
}

Ray Renderer::cameraRay(int x, int y, int w, int h) const {
	const float xf = ((float)x / (float)w - 0.5f) * 2.0f;
	const float yf = ((float)y / (float)h - 0.5f) * 2.0f;
	const float aspect = (float)w / (float)h;

	return m_camera.makeRay(xf, -yf / aspect);
}

void Renderer::renderFrame(SDL_PixelFormat format) {
	if(m_currentTexture < (int)m_textures.size() && m_rendering) {
		assert(m_textures[m_currentTexture]->isLocked());

		const int w = m_textures[m_currentTexture]->width();
		const int h = m_textures[m_currentTexture]->height();
		const int pitch = m_textures[m_currentTexture]->pitch();
		Uint32* pixels = m_textures[m_currentTexture]->pixels();

		//for(int tileId = 0; tileId < TILE_SUBDIV*TILE_SUBDIV; ++tileId) {
		tbb::parallel_for(0, TILE_SUBDIV * TILE_SUBDIV, [this, &w, &h, &pixels, &pitch, &format](int tileId) {
			const int xMin = ((tileId % TILE_SUBDIV) * w) / TILE_SUBDIV;
			const int xMax = ((tileId % TILE_SUBDIV + 1) * w) / TILE_SUBDIV;
			const int yMin = ((tileId / TILE_SUBDIV) * h) / TILE_SUBDIV;
			const int yMax = ((tileId / TILE_SUBDIV + 1) * h) / TILE_SUBDIV;

			renderTile(xMin, xMax, yMin, yMax, pixels, pitch, w, h, format);
		});
		//}

		if(m_rendering)
			++m_currentTexture;
	}
	else
		m_rendering = false;
}

void Renderer::renderTile(int xMin, int xMax, int yMin, int yMax, Uint32* pixels, int pitch, int w, int h, SDL_PixelFormat format) {
	for(int y = yMin; y < yMax && m_rendering; ++y)
		for(int x = xMin; x < xMax && m_rendering; ++x) {
			const Vec3 color = m_scene->renderPixel(cameraRay(x, y, w, h));

			Uint32 rgb = SDL_MapRGBA(&format, (Uint8)(color.x * 255.0), (Uint8)(color.y * 255.0),
			                         (Uint8)(color.z * 255.0), 255);
			Uint32 pixelPosition = y * (pitch / sizeof(Uint32)) + x;
			pixels[pixelPosition] = rgb;
		}
}

void Renderer::initTextures() {
	stopRenderThread();

	int w, h;
	SDL_GetWindowSize(m_window, &w, &h);

	for(std::vector<std::unique_ptr<Texture>>::reverse_iterator it = m_textures.rbegin(); it != m_textures.rend(); ++it) {
		*it = std::unique_ptr<Texture>(new Texture(m_renderer, SDL_GetWindowSurface(m_window)->format->format,
		                               SDL_TEXTUREACCESS_STREAMING, w, h));

		w /= 2;
		h /= 2;
	}
}
