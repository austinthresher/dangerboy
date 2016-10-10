#include <SDL/SDL.h>
#include <stdio.h>

#include "cpu.h"
#include "debugger.h"
#include "lcd.h"

#define INPUT_POLL_RATE 12 // Poll for input every 12 ms
#define SCALE_FACTOR 2

int main(int argc, char* args[]) {
   if (argc < 2) {
      printf("USAGE: %s <binary> [-i]\n", args[0]);
      exit(0);
   }

   bool rand_input = false;
   bool debug_flag = false;
   if (argc > 2) {
      for (int a = 0; a < argc - 2; ++a) {
         if (strcmp(args[a + 2], "-i") == 0) {
            mem_init();
            mem_load_image(args[1]);
            mem_print_rom_info();
            mem_free();
            fflush(stdout);
            exit(0);
         }
         if (strcmp(args[a + 2], "-d") == 0) {
            debug_flag = true;
         }
         if (strcmp(args[a + 2], "-v") == 0) {
            printf("%s\n", args[1]);
         }
         if (strcmp(args[a + 2], "-r") == 0) {
            rand_input = true;
         }
      }
   }

   uint32_t screenFlags = SDL_HWSURFACE | SDL_DOUBLEBUF;
   SDL_Init(SDL_INIT_EVERYTHING);
   SDL_WM_SetCaption("Danger Boy", "Danger Boy");
   SDL_Surface* screen = SDL_SetVideoMode(
         160 * SCALE_FACTOR, 144 * SCALE_FACTOR, 32, screenFlags);
   SDL_Surface* gb_screen = SDL_CreateRGBSurface(SDL_HWSURFACE,
         160 * SCALE_FACTOR,
         144 * SCALE_FACTOR,
         32,
         0x00FF0000,
         0x0000FF00,
         0x000000FF,
         0xFF000000);

   bool is_running = true;
   bool turbo      = false;
   int turbo_skip  = 3;
   int turbo_count = 0;
   int t_prev      = SDL_GetTicks();
   int i_prev      = SDL_GetTicks();
   char* file      = args[1];
   bool break_next = false;

   mem_init();
   mem_load_image(file);
   debugger_init();
   cpu_init();
   lcd_reset();
   if (debug_flag) {
      debugger_break();
   }
   bool rand_press = false;
   byte rand_mask  = 0;
   int rand_timer  = 5;
   while (is_running) {
      int t = SDL_GetTicks();
      if (t - i_prev > INPUT_POLL_RATE) {
         if (rand_input) {
            if (rand_timer-- < 0) {
               rand_timer = rand() % 10;
               if (!rand_press) {
                  if (rand() % 2 > 0) {
                     rand_press = true;
                     if (rand() % 2 > 0) {
                        rand_mask = 1;
                     } else {
                        rand_mask = 8;
                     }
                     mem_buttons &= ~rand_mask;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
               } else {
                  mem_buttons |= rand_mask;
                  rand_press = false;
                  rand_mask  = 0;
               }
            }
            turbo = true;
         }
         i_prev = t;
         SDL_Event event;
         while (SDL_PollEvent(&event)) {
            switch (event.type) {
               case SDL_KEYUP:
                  if (event.key.keysym.sym == SDLK_ESCAPE) {
                     is_running = false;
                     break;
                  }
                  if (event.key.keysym.sym == SDLK_SPACE) {
                     turbo = false;
                  }
                  if (event.key.keysym.sym == SDLK_LEFT) {
                     mem_dpad |= 0x02;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_UP) {
                     mem_dpad |= 0x04;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_RIGHT) {
                     mem_dpad |= 0x01;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_DOWN) {
                     mem_dpad |= 0x08;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_z) { // A
                     mem_buttons |= 0x01;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_x) { // B
                     mem_buttons |= 0x2;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_RETURN) { // Start
                     mem_buttons |= 0x08;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  if (event.key.keysym.sym == SDLK_RSHIFT) { // Select
                     mem_buttons |= 0x04;
                     if (break_next) {
                        debugger_break();
                     }
                  }
                  break;

               case SDL_KEYDOWN:
                  if (event.key.keysym.sym == SDLK_d) {
                     debugger_break();
                  }
                  if (event.key.keysym.sym == SDLK_n) {
                     break_next = true; // Break after the next input is given
                  }
                  if (event.key.keysym.sym == SDLK_SPACE) {
                     turbo = true;
                  }
                  if (event.key.keysym.sym == SDLK_LEFT) {
                     mem_dpad &= 0xFD;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_UP) {
                     mem_dpad &= 0xFB;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_RIGHT) {
                     mem_dpad &= 0xFE;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_DOWN) {
                     mem_dpad &= 0xF7;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_z) { // A
                     mem_buttons &= 0xFE;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_x) { // B
                     mem_buttons &= 0xFD;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_RETURN) { // Start
                     mem_buttons &= 0xF7;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  if (event.key.keysym.sym == SDLK_RSHIFT) { // Select
                     mem_buttons &= 0xFB;
                     wbyte(IF, rbyte(IF) | INT_INPUT);
                  }
                  break;
               case SDL_QUIT: is_running = false; break;
               default: break;
            }
         }
      }
      // We pause execution when the screen is ready to
      // be flipped to prevent emulating faster than 60 fps
      if (!lcd_ready()) {
         // If we aren't ready to render, check if the debugger
         // wants to break. Otherwise, execute an opcode and
         // advance time.
         if (debugger_should_break()) {
            debugger_cli();
         }
         cpu_execute_step();
      } else {
         // If we're skipping frames, only draw every turbo_skip frame
         if (turbo) {
            if (turbo_count < turbo_skip) {
               turbo_count++;
               continue;
            }
            turbo_count = 0;
         }

         // Delay until we're rendering at 60fps
         while (t - t_prev < 16 && !turbo) {
            SDL_Delay(1);
            t = SDL_GetTicks();
         }
         t_prev = t;

         // If the LCD is off, just draw white to the screen
         if (lcd_disabled()) {
            SDL_FillRect(screen, NULL, 0xFEFEFEFF);
            SDL_Flip(screen);
            continue;
         }

         // Copy the LCD framebuffer to the display. We expand
         // the framebuffer from 1 value per pixel (greyscale)
         // to RGB here.
         SDL_LockSurface(gb_screen);

         uint8_t *framebuffer = lcd_get_framebuffer();
         uint8_t *display     = gb_screen->pixels;
         int pitch            = gb_screen->pitch;
         int bpp              = gb_screen->format->BytesPerPixel;
         for (int y = 0; y < gb_screen->h; ++y) {
            for (int x = 0; x < gb_screen->w; ++x) {
               // Set Alpha channel to 255
               display[y * pitch + x * bpp + 3] = 255;
               for (int c = 0; c < 3; ++c) {
                  int small_x = x / SCALE_FACTOR;
                  int small_y = y / SCALE_FACTOR;
                  display[y * pitch + x * bpp + c] =
                     framebuffer[small_y * 160 + small_x];
               }
            }
         }

         SDL_UnlockSurface(gb_screen);
         SDL_BlitSurface(gb_screen, NULL, screen, NULL);
         SDL_Flip(screen);
      }
   }

   SDL_Quit();
   debugger_free();
   mem_free();
   return 0;
}
