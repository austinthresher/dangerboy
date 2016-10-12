# Danger Boy

Danger Boy is a Game Boy emulator written in C. It focuses on emulating the original DMG with cycle accurate timing. Compatibility is fairly high.


The following features are missing but planned:

 - Sound
 - Window support in per-pixel mode
 - SRAM saving
 - Save states
 - Configurable controls
 - Different rendering modes


### Screenshots

![Prehistorik Man](https://imgur.com/C15onj1.gif) ![Stunt Race 3D Demo](https://imgur.com/pg5gY8x.gif)

![Debugger](http://i.imgur.com/GrMGTcf.png)


### Building

Danger Boy requires:

```
 - cmake
 - libsdl1.2-dev
 - libncurses5-dev
```

To build:

```
git clone https://github.com/austinthresher/dangerboy
cd dangerboy
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Usage

```
dangerboy [filename] [ -d ]
```

The `-d` flag starts the debugger.


### Controls

```
  A Button  -  Z
  B Button  -  X
  Start     -  Enter
  Select    -  Right Shift
  Turbo     -  Space
  Debugger  -  D
```


#### Danger Boy currently passes the following tests:

 - add_sp_e_timing.gb
 - boot_regs-dmg.gb
 - call_cc_timing2.gb
 - call_cc_timing.gb
 - call_timing2.gb
 - call_timing.gb
 - cpu_instrs.gb
 - di_timing-GS.gb
 - div_timing.gb
 - div_write.gb
 - ei_timing.gb
 - halt_ime0_ei.gb
 - halt_ime0_nointr_timing.gb
 - halt_ime1_timing2-GS.gb
 - halt_ime1_timing.gb
 - hblank_count.gb
 - hblank_ly_scx_timing-GS.gb
 - if_ie_registers.gb
 - instr_timing.gb
 - intr_1_2_timing-GS.gb
 - intr_2_0_timing.gb
 - intr_2_mode0_timing.gb
 - intr_2_mode3_timing.gb
 - intr_2_oam_ok_timing.gb
 - intr_timing.gb
 - jp_cc_timing.gb
 - jp_timing.gb
 - ld_hl_sp_e_timing.gb
 - mbc1_rom_4banks.gb
 - mem_oam.gb
 - mem_timing.gb
 - oam_count.gb
 - oam_dma_restart.gb
 - oam_dma_start.gb
 - oam_dma_timing.gb
 - pop_timing.gb
 - push_timing.gb
 - rapid_di_ei.gb
 - reg_f.gb
 - ret_cc_timing.gb
 - reti_intr_timing.gb
 - reti_timing.gb
 - ret_timing.gb
 - rst_timing.gb
 - tim00_div_trigger.gb
 - tim00.gb
 - tim01_div_trigger.gb
 - tim01.gb
 - tim10_div_trigger.gb
 - tim10.gb
 - tim11_div_trigger.gb
 - tim11.gb
 - tima_reload.gb
 - vblank_stat_intr-GS.gb


#### The following tests fail:

 - boot_hwio-G.gb
 - halt_bug.gb
 - intr_2_mode0_timing_sprites.gb
 - lcd_clock.gb
 - rapid_toggle.gb
 - sprite_priority.gb
 - stat_irq_blocking.gb
 - tima_write_reloading.gb
 - tma_write_reloading.gb
 - unused_hwio-GS.gb
