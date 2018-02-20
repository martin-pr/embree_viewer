#include <iostream>
#include <stdexcept>

#include <SDL/SDL.h>

#define SCREEN_WIDTH	1024
#define SCREEN_HEIGHT	768
#define SCREEN_BPP	32

int main(int /*argc*/, char* /*argv*/[]) {
	// SDL initialisation
	if(SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(SDL_GetError());

	// make the window
	SDL_Surface* screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);

	// the main loop
	unsigned ctr = 0;
	while(1) {
		// draw something silly
		{
			SDL_Rect rect;
			rect.x = ctr;
			rect.y = ctr;
			rect.w = 10;
			rect.h = 10;

			SDL_FillRect(screen, &rect, 0xffffffff);
		}

		// show the result by flipping the double buffer
		SDL_Flip(screen);

		// exit condition
		{
			SDL_Event event;
			if(SDL_PollEvent(&event) && event.type == SDL_QUIT)
				break;
		}

		++ctr;
	}

	// clean up
	SDL_FreeSurface(screen);
	SDL_Quit();

	return 0;
}
