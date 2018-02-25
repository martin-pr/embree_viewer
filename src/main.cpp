#include <iostream>
#include <stdexcept>

#include <SDL/SDL.h>

#include "scene.h"
#include "maths.h"
#include "mesh.h"
#include "alembic.h"

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

int main(int argc, char* argv[]) {
	// SDL initialisation
	if(SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(SDL_GetError());

	// make the window
	SDL_Surface* screen = SDL_SetVideoMode(SCREEN_SIZE, SCREEN_SIZE, SCREEN_BPP, SDL_SWSURFACE);

	{
		// make the scene
		Scene scene;

		// add a bunch of spheres
		Scene mesh;
		if(argc == 1) {
			Mesh m = Mesh::makeSphere(Vec3{0,0,0}, 0.3);
			mesh.addMesh(std::move(m));
		}
		else
			mesh = loadAlembic(argv[1]);
		mesh.commit();

		{
			// // static const long TOTAL = 1e5;
			// static const long SQTOTAL = powf(TOTAL, 1.0/3.0);

			// for(long x=-SQTOTAL/2;x<SQTOTAL/2;++x)
			// 	for(long y=-SQTOTAL/2;y<SQTOTAL/2;++y)
			// 		for(long z=-SQTOTAL/2;z<SQTOTAL/2;++z)
			// 			scene.addInstance(mesh, Vec3{(float)x, (float)y, (float)z});

			scene.addInstance(mesh, Vec3(0,0,0));
		}

		scene.commit();

		///////////////////////////

		Camera cam;

		// the main loop
		unsigned ctr = 0;

		bool quit = false;

		while(!quit) {
			// render pixels
			for(int y=0;y<screen->h;++y)
				for(int x=0;x<screen->w;++x) {
					const float xf = ((float)x / (float)screen->w - 0.5f) * 2.0f;
					const float yf = ((float)y / (float)screen->h - 0.5f) * 2.0f;

					const Ray r = cam.makeRay(xf, -yf);

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

							dist = powf(dist, 1.0f+ydiff);

							cam.position = cam.target - tr * dist;
						}
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
