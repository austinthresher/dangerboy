#include <SDL/SDL.h>
#include <stdio.h>

#include "gpu.h"
#include "z80.h"

int main(int argc, char *args[]) {
  if (argc < 2) {
    printf("USAGE: %s romfile.gb\n", args[0]);
    exit(0);
  }

  uint32_t screenFlags = SDL_HWSURFACE | SDL_DOUBLEBUF;
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_WM_SetCaption("Danger Boy", "Danger Boy");
  SDL_Surface *screen = SDL_SetVideoMode(160, 144, 32, screenFlags);
  SDL_Surface *gb_screen =
      SDL_CreateRGBSurface(screenFlags, 160, 144, 32, 0x00FF0000, 0x0000FF00,
                           0x000000FF, 0xFF000000);
  SDL_Rect gb_screen_rect = {0, 0, 160, 144};

  bool isRunning = true;
  int t_prev = SDL_GetTicks();
  char *file = args[1];
  z80_init(file);
  gpu_init(gb_screen);

  while (isRunning) {
    int t = SDL_GetTicks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          isRunning = false;
          break;
        }
        if (event.key.keysym.sym == SDLK_LEFT) {
          mem_dpad |= 0x02;
        }
        if (event.key.keysym.sym == SDLK_UP) {
          mem_dpad |= 0x04;
        }
        if (event.key.keysym.sym == SDLK_RIGHT) {
          mem_dpad |= 0x01;
        }
        if (event.key.keysym.sym == SDLK_DOWN) {
          mem_dpad |= 0x08;
        }
        if (event.key.keysym.sym == SDLK_z) { // A
          mem_buttons |= 0x01;
        }
        if (event.key.keysym.sym == SDLK_x) { // B
          mem_buttons |= 0x2;
        }
        if (event.key.keysym.sym == SDLK_RETURN) { // Start
          mem_buttons |= 0x08;
        }
        if (event.key.keysym.sym == SDLK_RSHIFT) { // Select
          mem_buttons |= 0x04;
        }
        break;

      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_LEFT) {
          mem_dpad &= 0xFD;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_UP) {
          mem_dpad &= 0xFB;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_RIGHT) {
          mem_dpad &= 0xFE;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_DOWN) {
          mem_dpad &= 0xF7;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_z) { // A
          mem_buttons &= 0xFE;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_x) { // B
          mem_buttons &= 0xFD;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_RETURN) { // Start
          mem_buttons &= 0xF7;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        if (event.key.keysym.sym == SDLK_RSHIFT) { // Select
          mem_buttons &= 0xFB;
          mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_input);
        }
        break;
      case SDL_QUIT:
        isRunning = false;
        break;
      default:
        break;
      }
    }

    if (isRunning) {

      // We pause execution when the screen is ready to
      // be flipped to prevent emulating faster than 60 fps
      if (gpu_ready_to_draw == false) {
        gpu_execute_step(z80_execute_step());
      } else {
        if (t - t_prev > 16) { // 60 fps
          t_prev -= 16;

          SDL_FillRect(screen, NULL, 0xF00000);
          SDL_LockSurface(gb_screen);

          // Copy the display over one row at a time.
          // This avoids memory alignment issues on some platforms.
          for (int y = 0; y < gb_screen->h; ++y) {
            uint8_t *pixels = gb_screen->pixels + (y * gb_screen->pitch);
            memcpy(pixels, gpu_vram + y * 160 * 4, 160 * 4);
          }

          SDL_UnlockSurface(gb_screen);
          SDL_BlitSurface(gb_screen, NULL, screen, &gb_screen_rect);
          SDL_Flip(screen);

          gpu_ready_to_draw = false;
        }
      }
    }
  }

  SDL_FreeSurface(screen);
  SDL_FreeSurface(gb_screen);
  SDL_Quit();
  return 0;
}
