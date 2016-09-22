#include <SDL/SDL.h>
#include <stdio.h>

#include "gpu.h"
#include "z80.h"

#define INPUT_POLL_RATE 8 // Poll for input every 8 ms (120 times a second)
#define SCALE_FACTOR 2 

int main(int argc, char* args[]) {
   if (argc < 2) {
      printf("USAGE: %s <binary> [-i]\n", args[0]);
      exit(0);
   }

   if (argc >= 3 && strcmp(args[2], "-i") == 0) {
      mem_init();
      mem_load_image(args[1]);
      mem_get_rom_info();
      mem_print_rom_info();
      mem_free();
      fflush(stdout);
      exit(0);
   }

   uint32_t screenFlags = SDL_HWSURFACE | SDL_DOUBLEBUF;
   SDL_Init(SDL_INIT_EVERYTHING);
   SDL_WM_SetCaption("Danger Boy", "Danger Boy");
   SDL_Surface* screen = SDL_SetVideoMode(160 * SCALE_FACTOR,
                                          144 * SCALE_FACTOR,
                                          32, screenFlags);
   SDL_Surface* gb_screen =
      SDL_CreateRGBSurface(screenFlags,
                           160 * SCALE_FACTOR,
                           144 * SCALE_FACTOR,
                           32,
                           0x00FF0000, 0x0000FF00,
                           0x000000FF, 0xFF000000);
   
   bool  is_running   = true;
   int   t_prev       = SDL_GetTicks();
   int   i_prev       = SDL_GetTicks();
   char* file         = args[1];
   z80_init(file);
   gpu_init(gb_screen);
   while (is_running && !check_error()) {
      int t = SDL_GetTicks();
      if (t - i_prev > INPUT_POLL_RATE) {
         i_prev = t;
         SDL_Event event;
         while (SDL_PollEvent(&event)) {
            switch (event.type) {
               case SDL_KEYUP:
                  if (event.key.keysym.sym == SDLK_ESCAPE) {
                     is_running = false;
                     break;
                  }
                  if (event.key.keysym.sym == SDLK_LEFT) {
                     mem_dpad |= 0x02;
                     DEBUG("DPAD LEFT RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_UP) {
                     mem_dpad |= 0x04;
                     DEBUG("DPAD UP RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_RIGHT) {
                     mem_dpad |= 0x01;
                     DEBUG("DPAD RIGHT RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_DOWN) {
                     mem_dpad |= 0x08;
                     DEBUG("DPAD DOWN RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_z) { // A
                     mem_buttons |= 0x01;
                     DEBUG("BUTTON A RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_x) { // B
                     mem_buttons |= 0x2;
                     DEBUG("BUTTON B RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_RETURN) { // Start
                     mem_buttons |= 0x08;
                     DEBUG("BUTTON START RELEASED\n");
                  }
                  if (event.key.keysym.sym == SDLK_RSHIFT) { // Select
                     mem_buttons |= 0x04;
                     DEBUG("BUTTON SELECT RELEASED\n");
                  }
                  break;

               case SDL_KEYDOWN:
                  if (event.key.keysym.sym == SDLK_LEFT) {
                     mem_dpad &= 0xFD;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("DPAD LEFT PRESSED\n");
                  }
                  if (event.key.keysym.sym == SDLK_UP) {
                     mem_dpad &= 0xFB;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("DPAD UP PRESSED\n");
                  }
                  if (event.key.keysym.sym == SDLK_RIGHT) {
                     mem_dpad &= 0xFE;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("DPAD RIGHT PRESSED\n");

                  }
                  if (event.key.keysym.sym == SDLK_DOWN) {
                     mem_dpad &= 0xF7;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("DPAD DOWN PRESSED\n");
                  }
                  if (event.key.keysym.sym == SDLK_z) { // A
                     mem_buttons &= 0xFE;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("BUTTON A PRESSED\n");
                  }
                  if (event.key.keysym.sym == SDLK_x) { // B
                     mem_buttons &= 0xFD;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("BUTTON B PRESSED\n");
                  }
                  if (event.key.keysym.sym == SDLK_RETURN) { // Start
                     mem_buttons &= 0xF7;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("BUTTON START PRESSED\n");
                  }
                  if (event.key.keysym.sym == SDLK_RSHIFT) { // Select
                     mem_buttons &= 0xFB;
                     mem_wb(INT_FLAG_ADDR, mem_rb(INT_FLAG_ADDR) | INT_INPUT);
                     DEBUG("BUTTON SELECT PRESSED\n");
                  }
                  break;
               case SDL_QUIT: is_running = false; break;
               default: break;
            }
         }
      }
      // We pause execution when the screen is ready to
      // be flipped to prevent emulating faster than 60 fps
      if (gpu_ready_to_draw == false) {
         z80_execute_step();
         inc_debug_time();
      } else {
         if (t - t_prev > 16) { // 60 fps
            t_prev = t;

            DEBUG("SCREEN REFRESH\n");

            SDL_FillRect(screen, NULL, 0xF00000);
            SDL_LockSurface(gb_screen);

            // Copy the display over one row at a time.
            // This avoids memory alignment issues on some platforms.
            for (int y = 0; y < gb_screen->h; ++y) {
               for (int x = 0; x < gb_screen->w; ++x) {
                  *(uint8_t*)(gb_screen->pixels
                           + (y * gb_screen->pitch
                           +  x * gb_screen->format->BytesPerPixel + 3)
                           ) = 255;
                  for (int c = 0; c < 3; ++c) {
                     int small_x = x / SCALE_FACTOR;
                     int small_y = y / SCALE_FACTOR;
                     *(uint8_t*)(gb_screen->pixels
                                 + (y * gb_screen->pitch
                                 +  x * gb_screen->format->BytesPerPixel + c)
                                 ) = gpu_vram[small_y * 160*3 + small_x*3 + c];
                  }
               }
            }

            SDL_UnlockSurface(gb_screen);
            SDL_BlitSurface(gb_screen, NULL, screen, NULL);
            SDL_Flip(screen);

            gpu_ready_to_draw = false;
         } else {
            SDL_Delay(1);
         }
      }
   }

   fflush(stderr);

   if (check_error()) {
      printf("An error occured. Exiting.\n");
      fflush(stdout);
   }

   mem_free();
   SDL_FreeSurface(screen);
   SDL_FreeSurface(gb_screen);
   SDL_Quit();
   return 0;
}
