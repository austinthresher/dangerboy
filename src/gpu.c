#include "gpu.h"

void gpu_init() {
  gpu_mode = GPU_MODE_SCAN_OAM; // This mode needs to be written to 0xFF41
  gpu_timer = 0;
  gpu_scanline = 0;
  gpu_blankscreen = false;
  gpu_ready_to_draw = false;
  gpu_last_scanline = 0;
  gpu_draw_frame = false;
  gpu_hit_stat_interrupt = false;
  gpu_vram = NULL;
  gpu_window_scanline = 0;
  gpu_window_y = 0;
  gpu_reset();
}

void gpu_reset() {
  if (gpu_vram != NULL)
    free(gpu_vram);

  gpu_vram = (byte *)calloc(160 * 144, 4); // 4 bytes per pixel

  for (int i = 0, n = 0; i < 40; i++, n += 4) {
    mem_wb(0xFE00 + n + 0, 0);
    mem_wb(0xFE00 + n + 1, 0);
    mem_wb(0xFE00 + n + 2, 0);
    mem_wb(0xFE00 + n + 3, 0);
  }
}

void gpu_execute_step(tick ticks) {
  gpu_timer += ticks;

  if (mem_need_reset_scanline) {
    gpu_scanline = 0;
    gpu_window_scanline = 0;
    mem_need_reset_scanline = false;
  }

  byte last_mode = gpu_mode;

  if (gpu_timer >= 456) {
    gpu_timer -= 456;

    gpu_scanline++;
    if (mem_rb(0xFF41) & 0x40 && gpu_scanline == mem_rb(0xFF45)) {
      mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_stat);
    }

    // The screen has 144 scanlines, we've now completed 0-143
    if (gpu_scanline == 144) {
      gpu_mode = GPU_MODE_VBLANK;
    }
    if (gpu_scanline >= 154) {
      gpu_ready_to_draw = true;
      gpu_scanline = 0;
      gpu_window_scanline = 0;
    }
  }

  if (gpu_scanline < 144) {
    if (gpu_timer <= 204) {
      gpu_mode = GPU_MODE_HBLANK;
    } else if (gpu_timer <= 284) {
      gpu_mode = GPU_MODE_SCAN_OAM;
    } else {
      gpu_mode = GPU_MODE_SCAN_VRAM;
    }

    gpu_draw_frame = false;
  }

  // STAT interrupt
  if (gpu_mode != last_mode) {
    byte stat_reg = mem_rb(0xFF41);
    if ((gpu_mode == GPU_MODE_HBLANK && (stat_reg & 0x08) != 0) ||
        (gpu_mode == GPU_MODE_VBLANK && (stat_reg & 0x10) != 0) ||
        (gpu_mode == GPU_MODE_SCAN_OAM && (stat_reg & 0x20) != 0) ||
        (gpu_scanline == mem_rb(0xFF45) && (stat_reg & 0x40) != 0)) {
      mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_stat);
    }
  }

  if (gpu_mode == GPU_MODE_HBLANK) {
    // TODO: Does anything need to be done here?
  } else if (gpu_mode == GPU_MODE_VBLANK) {
    if (!gpu_draw_frame) {
      mem_wb(0xFF0F, mem_rb(0xFF0F) | z80_interrupt_vblank);
      if ((mem_rb(0xFF40) & 0x80) == 0) {
        gpu_blankscreen = true;
      } else {
        gpu_blankscreen = false;
      }
      gpu_draw_frame = true;
    }
  } else if (gpu_mode == GPU_MODE_SCAN_OAM) {
    // TODO: Does anything need to be done here?
  } else if (gpu_mode == GPU_MODE_SCAN_VRAM) {
    if (gpu_last_scanline != gpu_scanline) {
      gpu_do_scanline();
      gpu_last_scanline = gpu_scanline;
    }
  }

  // Now lets update our memory registers
  // TODO: 0xFF40 - LCD and GPU misc stuff

  // Writing here normally resets it, so we set it directly
  mem_direct_write(0xFF44, gpu_scanline & 0xFF);
  byte lyc = mem_rb(0xFF45);

  // TODO: Why was this commented out?
  // byte stat = (gpu_scanline == lyc ? 0x04 : 0) | gpu_mode & 0x03;
  // cpu->mem.wb(0xFF41, (cpu->mem.rb(0xFF41) & 0xF8) | stat); //Leave the
  // upper
  // 4 bits alone
}

void gpu_do_scanline() {
  bool using_window = false;
  int output_addr = gpu_scanline * 160 * 4;

  if (gpu_blankscreen) {
    for (int i = 0; i < 160; i++) {
      gpu_vram[output_addr++] = 0xCC;
      gpu_vram[output_addr++] = 0xFF;
      gpu_vram[output_addr++] = 0xCC;
      gpu_vram[output_addr++] = 0xFF;
    }
    return;
  }

  byte gpu_window_x = mem_rb(0xFF4B);

  if (gpu_scanline == 0) {
    gpu_window_y = mem_rb(0xFF4A);
  }

  byte scrollX = mem_rb(0xFF43);
  byte scrollY = mem_rb(0xFF42);
  bool which_win_tile_map = (mem_rb(0xFF40) & 0x40) != 0;
  bool which_bg_tile_map = (mem_rb(0xFF40) & 0x08) != 0;
  bool which_tile_data = (mem_rb(0xFF40) & 0x10) == 0;

  word bg_tile_map_location = 0x9800;
  if (which_bg_tile_map) {
    bg_tile_map_location += 0x0400;
  }

  word win_tile_map_location = 0x9800;
  if (which_win_tile_map) {
    win_tile_map_location += 0x0400;
  }

  word bg_tile_map_offset = (((gpu_scanline + scrollY) & 0xFF) >> 3) * 32;
  word win_tile_map_offset = ((gpu_window_scanline & 0xFF) >> 3) * 32;
  byte bg_x_tile_offset = (scrollX >> 3) & 0x1F;
  byte win_x_tile_offset = 0;
  byte window_x_px = 0;
  word bg_current_tile = mem_direct_read(bg_tile_map_location +
                                         bg_tile_map_offset + bg_x_tile_offset);

  if (which_tile_data && bg_current_tile < 128) {
    bg_current_tile += 256;
  }

  word win_current_tile = mem_direct_read(
      win_tile_map_location + win_tile_map_offset + win_x_tile_offset);

  if (which_tile_data && win_current_tile < 128) {
    win_current_tile += 256;
  }

  byte bg_x_px_offset = scrollX & 0x07;
  byte bg_y_px_offset = (scrollY + gpu_scanline) & 0x07;
  byte win_x_px_offset = 0;
  byte win_y_px_offset = gpu_window_scanline & 0x07;
  byte start_x_off = bg_x_px_offset;
  byte outcol = 0;

  for (int i = 0; i < 160; i++) {
    if ((mem_rb(0xFF40) & 0x20) != 0 && gpu_scanline >= gpu_window_y &&
        i >= gpu_window_x - 7 && gpu_window_x < 166) {
      using_window = true;
    }

    word tempaddr = 0x8000;
    if (using_window == false) {
      tempaddr += bg_current_tile * 16 + bg_y_px_offset * 2;
    } else {
      tempaddr += win_current_tile * 16 + win_y_px_offset * 2;
    }

    byte hi = mem_direct_read(tempaddr + 1);
    byte lo = mem_direct_read(tempaddr);
    byte mask = 1;

    if (using_window == false) {
      mask <<= (7 - ((i + start_x_off) & 0x07));
    } else {
      mask <<= (7 - ((window_x_px++) & 0x07));
    }

    byte col = 0;
    if (hi & mask) {
      col = 2;
    }
    if (lo & mask) {
      col += 1;
    }

    outcol = gpu_pick_color(col, mem_direct_read(0xFF47));

    if (using_window == false) {
      gpu_vram[output_addr++] = outcol;
      gpu_vram[output_addr++] = outcol;
      gpu_vram[output_addr++] = outcol;
      gpu_vram[output_addr++] = 0xFF; // Alpha channel

      bg_x_px_offset++;

      if (bg_x_px_offset >= 8) {
        bg_x_px_offset = 0;
        bg_x_tile_offset++;
        bg_x_tile_offset &= 0x1F;
        bg_current_tile = mem_direct_read(
            bg_tile_map_location + bg_tile_map_offset + bg_x_tile_offset);
        if (which_tile_data && bg_current_tile < 128) {
          bg_current_tile += 256;
        }
      }
    } else {
      gpu_vram[output_addr++] = outcol;
      gpu_vram[output_addr++] = outcol;
      gpu_vram[output_addr++] = outcol;
      gpu_vram[output_addr++] = 0xFF; // Alpha channel
      win_x_px_offset++;

      if (win_x_px_offset >= 8) {
        win_x_px_offset = 0;
        win_x_tile_offset++;
        win_x_tile_offset &= 0x1F;
        win_current_tile = mem_direct_read(
            win_tile_map_location + win_tile_map_offset + win_x_tile_offset);
        if (which_tile_data && win_current_tile < 128) {
          win_current_tile += 256;
        }
      }
    }
  }

  if (using_window) {
    gpu_window_scanline++;
  }

  bool big_sprites = (mem_direct_read(0xFF40) & 0x04) == 0x04;

  for (int spr = 0; spr < 40; spr++) {
    byte y = mem_direct_read(0xFE00 + spr * 4);
    byte x = mem_direct_read(0xFE00 + 1 + spr * 4);
    byte tile = mem_direct_read(0xFE00 + 2 + spr * 4);
    byte val = mem_direct_read(0xFE00 + 3 + spr * 4);
    byte palette = (val & 0x10) ? 1 : 0;
    bool xflip = (val & 0x20) ? true : false;
    bool yflip = (val & 0x40) ? true : false;
    byte priority = (val & 0x80) ? 1 : 0;

    if (x == 0 && y == 0) {
      continue;
    }

    x -= 8;
    y -= 16;

    if (y <= gpu_scanline && y + (big_sprites ? 16 : 8) > gpu_scanline) {
      byte temptile = tile;
      if (mem_rb(0xFF40) & 0x04) {
        temptile &= 0xFE;
      }
      outcol = 0;
      word stempaddr;
      byte y_mask = 0x07;

      if (big_sprites) {
        y_mask = 0x0F;
      }
      if (!yflip) {
        stempaddr = 0x8000 + temptile * 16 + ((gpu_scanline - y) & y_mask) * 2;
      } else {
        stempaddr =
            0x8000 + temptile * 16 +
            ((big_sprites ? 32 : 14) - ((gpu_scanline - y) & y_mask) * 2);
        // TODO: There's an off by 1 error somewhere with big sprites
      }

      byte shi = mem_direct_read(stempaddr + 1);
      byte slo = mem_direct_read(stempaddr);

      for (int sx = 0; sx < 8; sx++) {
        byte smask;
        if (!xflip) {
          smask = 0x80 >> sx;
        } else {
          smask = 0x01 << sx;
        }
        byte scol = 0;
        if (shi & smask) {
          scol = 2;
        }
        if (slo & smask) {
          scol++;
        }
        byte color = gpu_pick_color(scol, mem_direct_read(0xFF48 + palette));

        // TODO: This doesn't check priority with background. Fix.
        if ((x + sx) < 160 && scol) {
          outcol = color;
          int output_addr = (gpu_scanline * 160 + x + sx) * 4;
          if (!gpu_blankscreen) {
            gpu_vram[output_addr++] = outcol; // * 0.9;
            gpu_vram[output_addr++] = outcol;
            gpu_vram[output_addr++] = outcol; // * 0.6;
            gpu_vram[output_addr++] = 0xFF;
          } else {
            gpu_vram[output_addr++] = 0xFF;
            gpu_vram[output_addr++] = 0xFF;
            gpu_vram[output_addr++] = 0xFF;
            gpu_vram[output_addr++] = 0xFF;
          }
        }
      }
    }
  }
}

byte gpu_pick_color(byte col, byte pal) {
  byte outcol = 0;
  if (col == 0x03) {
    byte temp = (pal >> 6) & 0x03;
    if (temp == 3) {
      outcol = COLOR_BLACK;
    } else if (temp == 2) {
      outcol = COLOR_DARK;
    } else if (temp == 1) {
      outcol = COLOR_LITE;
    } else if (temp == 0) {
      outcol = COLOR_WHITE;
    }
  } else if (col == 0x02) {
    byte temp = (pal >> 4) & 0x03;
    if (temp == 3) {
      outcol = COLOR_BLACK;
    } else if (temp == 2) {
      outcol = COLOR_DARK;
    } else if (temp == 1) {
      outcol = COLOR_LITE;
    } else if (temp == 0) {
      outcol = COLOR_WHITE;
    }
  } else if (col == 0x01) {
    byte temp = (pal >> 2) & 0x03;
    if (temp == 3) {
      outcol = COLOR_BLACK;
    } else if (temp == 2) {
      outcol = COLOR_DARK;
    } else if (temp == 1) {
      outcol = COLOR_LITE;
    } else if (temp == 0) {
      outcol = COLOR_WHITE;
    }
  } else if (col == 0x00) {
    byte temp = pal & 0x03;
    if (temp == 3) {
      outcol = COLOR_BLACK;
    } else if (temp == 2) {
      outcol = COLOR_DARK;
    } else if (temp == 1) {
      outcol = COLOR_LITE;
    } else if (temp == 0) {
      outcol = COLOR_WHITE;
    }
  }
  return outcol;
}
