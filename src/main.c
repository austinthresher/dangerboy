#include <SDL/SDL.h>
#include <stdio.h>

#include "gpu.h"
#include "z80.h"

int main(int argc, char* args[]) {
   if (argc < 2) {
      printf("USAGE: %s romfile.gb\n", args[0]);
      exit(0);
   }

   SDL_Init(SDL_INIT_EVERYTHING);
   SDL_WM_SetCaption("Danger Boy", "Danger Boy");
   SDL_Surface* screen = SDL_SetVideoMode(160, 144, 32, SDL_HWSURFACE);
   SDL_Surface* gb_screen =
      SDL_CreateRGBSurface(SDL_HWSURFACE, 160, 144, 32, 0x00FF0000, 0x0000FF00,
                           0x000000FF, 0xFF000000);

   bool  isRunning = true;
   int   t_prev    = SDL_GetTicks();
   char* file      = args[1];
   z80_init(file);
   gpu_init(gb_screen);

   while (isRunning) {
      int       t = SDL_GetTicks();
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
            case SDL_QUIT: isRunning = false; break;
            default: break;
         }
      }

      if (isRunning) {
         gpu_execute_step(z80_execute_step());
         if (gpu_ready_to_draw == true) {
            if ((t - t_prev) > 16) { // 60 fps
               t_prev = t;
               SDL_FillRect(screen, NULL, 0x000000);
               SDL_LockSurface(screen);
               memcpy(screen->pixels, gpu_vram, 160 * 144 * 4);
               SDL_UnlockSurface(screen);
               SDL_BlitSurface(gb_screen, NULL, screen, NULL);
               SDL_Flip(screen);
               gpu_ready_to_draw = false;
            } else {
               SDL_Delay(1);
            }
         }
      }
   }

   SDL_FreeSurface(screen);
   SDL_FreeSurface(gb_screen);
   SDL_Quit();
   return 0;
}
