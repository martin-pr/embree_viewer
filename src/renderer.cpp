#include "renderer.h"

#define TEXTURE_LEVELS 8
#define TILE_SUBDIV 8

Renderer::Renderer(const Scene& scene, SDL_Window* window, SDL_Renderer* renderer) : m_scene(&scene), m_window(window),
	m_renderer(renderer), m_currentTexture(0), m_rendering(false) {

	m_textures.resize(TEXTURE_LEVELS, nullptr);
	initTextures();
}

Renderer::~Renderer() {
	m_rendering = false;
	if(m_thread.get() != nullptr)
		m_thread->join();
	m_thread.reset();

	for(auto& t : m_textures)
		SDL_DestroyTexture(t);
}

void Renderer::setCamera(Camera& cam) {
	m_camera = cam;

	startRenderThread();
}

void Renderer::resize(std::size_t /*w*/, std::size_t /*h*/) {
	initTextures();

	startRenderThread();
}

SDL_Texture* Renderer::texture() {
	assert(m_currentTexture > 0);
	return m_textures[m_currentTexture - 1];
}

int Renderer::currentTexture() const {
	return m_currentTexture - 1;
}

void Renderer::startRenderThread() {
	m_rendering = false;
	if(m_thread.get() != nullptr)
		m_thread->join();

	m_currentTexture = 0;
	m_rendering = true;

	std::function<void()> renderFunctor(std::bind(&Renderer::renderAll, this));
	m_thread = std::unique_ptr<std::thread>(new std::thread(renderFunctor));
}

void Renderer::renderAll() {
	while(m_rendering && m_currentTexture < (int)m_textures.size())
		renderFrame();
}

Ray Renderer::cameraRay(int x, int y, int w, int h) const {
	const float xf = ((float)x / (float)w - 0.5f) * 2.0f;
	const float yf = ((float)y / (float)h - 0.5f) * 2.0f;
	const float aspect = (float)w / (float)h;

	return m_camera.makeRay(xf, -yf / aspect);
}

void Renderer::renderFrame() {
	if(m_currentTexture < (int)m_textures.size() && m_rendering) {
		Uint32* pixels = nullptr;
		int pitch = 0;
		Uint32 format;

		int w, h;
		SDL_QueryTexture(m_textures[m_currentTexture], &format, NULL, &w, &h);

		if(SDL_LockTexture(m_textures[m_currentTexture], nullptr, (void**)&pixels, &pitch))
			throw std::runtime_error(SDL_GetError());

		for(int tileId = 0; tileId < TILE_SUBDIV*TILE_SUBDIV; ++tileId) {
			const int xMin = ((tileId % TILE_SUBDIV) * w) / TILE_SUBDIV;
			const int xMax = ((tileId % TILE_SUBDIV + 1) * w) / TILE_SUBDIV;
			const int yMin = ((tileId / TILE_SUBDIV) * h) / TILE_SUBDIV;
			const int yMax = ((tileId / TILE_SUBDIV + 1) * h) / TILE_SUBDIV;

			renderTile(xMin, xMax, yMin, yMax, pixels, pitch, w, h);
		}

		SDL_UnlockTexture(m_textures[m_currentTexture]);

		if(m_rendering)
			++m_currentTexture;
	}
	else
		m_rendering = false;
}

void Renderer::renderTile(int xMin, int xMax, int yMin, int yMax, Uint32* pixels, int pitch, int w, int h) {
	for(int y = yMin; y < yMax && m_rendering; ++y)
		for(int x = xMin; x < xMax && m_rendering; ++x) {
			const Vec3 color = m_scene->renderPixel(cameraRay(x, y, w, h));

			Uint32 rgb = SDL_MapRGBA(SDL_GetWindowSurface(m_window)->format, (Uint8)(color.x * 255.0), (Uint8)(color.y * 255.0),
			                         (Uint8)(color.z * 255.0), 255);
			Uint32 pixelPosition = y * (pitch / sizeof(Uint32)) + x;
			pixels[pixelPosition] = rgb;
		}
}

void Renderer::initTextures() {
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
