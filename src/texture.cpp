#include "texture.h"

#include <cassert>
#include <stdexcept>

Texture::Texture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h) : m_pixels(nullptr), m_pitch(0), m_width(w), m_height(h), m_format(format) {
	m_texture = SDL_CreateTexture(renderer, format, access, w, h);
}

Texture::~Texture() {
	if(isLocked())
		unlock();
	assert(!isLocked());

	SDL_DestroyTexture(m_texture);
}

bool Texture::isLocked() const {
	return m_pixels != nullptr;
}

void Texture::lock() {
	if(!isLocked()) {
		if(SDL_LockTexture(m_texture, nullptr, (void**)&m_pixels, &m_pitch))
			throw std::runtime_error(SDL_GetError());
	}

	assert(isLocked());
}

void Texture::unlock() {
	if(isLocked()) {
		SDL_UnlockTexture(m_texture);
		m_pixels = nullptr;
	}

	assert(!isLocked());
}

int Texture::width() const {
	return m_width;
}

int Texture::height() const {
	return m_height;
}

Uint32 Texture::format() const {
	return m_format;
}

Uint32* Texture::pixels() {
	assert(m_pixels != nullptr);
	return m_pixels;
}

int Texture::pitch() const {
	return m_pitch;
}

SDL_Texture* Texture::texture() {
	return m_texture;
}
