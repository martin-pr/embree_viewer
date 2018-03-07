#pragma once

#include <boost/noncopyable.hpp>

#include <SDL2/SDL.h>

class Texture : public boost::noncopyable {
	public:
		Texture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h);
		~Texture();

		bool isLocked() const;
		void lock();
		void unlock();

		int width() const;
		int height() const;
		Uint32 format() const;

		Uint32* pixels();
		int pitch() const;

		SDL_Texture* texture();

	private:
		SDL_Texture* m_texture;

		Uint32* m_pixels;
		int m_pitch, m_width, m_height;
		Uint32 m_format;
};
