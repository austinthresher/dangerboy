; This test records the value of STAT before,
; during and after disabling the LCD in each mode.

; A version of this test that waits for OAM at 144 should also be written

.incdir "common"
.include "common.s"

   di ; This test won't use interrupts

   ; Clear 12 bytes starting at C400
   ld h, $C4
   ld l, $00
   ld b, 12
   xor a
-  ld (hl+), a
   dec b
   jr nz, -

   ld b, 0 ; B will contain the memory offset to store results

   ; Start test
   ld a, $A0 ; Set LYC to an impossible value
   ldh (<LYC), a
   xor a
   ldh (<STAT), a
   wait_ly $45   ; Wait for the middle of the screen

mode0:
   ldh a, (<STAT)
   and 3
   jp nz, mode0
   call test_lcd

mode1:
   ldh a, (<STAT)
   and 3
   cp 1
   jp nz, mode1
   call test_lcd

; Mode 2 is short enough that we're too late if we
; wait until STAT & 3 == 2. Instead, we wait for mode 0,
; then run a bunch of nops to catch mode 2.
mode2:
   ldh a, (<STAT)
   and 3
   jp nz, mode2
   nops 40 ; 40 * 4 = 160, +40 cycles for two CALLs = length of HBLANK
   call test_lcd

mode3:
   ldh a, (<STAT)
   and 3
   cp 3
   jp nz, mode3
   call test_lcd

   jp test_finish

test_lcd:
   call store_stat
   disable_lcd
   call store_stat
   enable_lcd
   call store_stat
   ret

store_stat:
   ldh a, (<STAT)
   ld h, $C4
   ld l, b
   ld (hl), a
   inc b
   ret


test_finish:
   di
   print_results test_callback
   halt_execution

test_callback:
   print_string_literal "    BEFORE"
   call print_newline
   print_string_literal "  DURING"
   call print_newline
   print_string_literal "AFTER"
   call print_newline
   ; Loop through to print each memory value
   ld d, 12 ; rows
   ld e, 3  ; cols

print_loop:
   push hl
   dec d
   ld h, $C4
   ld l, d
   ld a, (hl)
   pop hl
   call print_a
   dec e
   jp z, newline
   xor a
   or d
   jp nz, print_loop 

print_finished:
   call print_newline
   print_string_literal "DONE."
   call print_newline
   print_string_literal "DMG / AGB:"
   call print_newline
   print_string_literal "808083"
   call print_newline
   print_string_literal "808082"
   call print_newline
   print_string_literal "808081"
   call print_newline
   print_string_literal "808080"
   ret

newline:
   call print_newline
   xor a
   or d
   jp z, print_finished
   jp print_loop - 2

