; This test is the same as oam_and_hblank, except instead
; of counting interrupts it writes the value of STAT. This
; should expose which STAT interrupt fired at each line.

.incdir "common"
.include "common.s"

   di

   ; Clear 144 bytes starting at C400
   ld h, $C4
   ld l, $00
   ld b, $99
   xor a
-  ld (hl+), a
   dec b
   jr nz, -

   ; Starting with LYC = 1, wait for LYC interrupt, change STAT to OAM + HBLANK, and
   ; count the number of STAT interrupts until VBLANK. Increment LYC and
   ; repeat until LYC == 90.
   ld b, $FF 
   
   ; Enable VBLANK interrupt
   xor a
   ldh (<IF), a
   ld a, %00000001
   ldh (<IE), a
   ei

wait:
   nop
   jp wait ; Wait for interrupt to fire

test_finish:
   di
   print_results test_callback
   halt_execution

test_callback:
   ; Loop through to print each memory value
   ld d, $91
   ld e, 10 
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

   call print_newline
   print_string_literal "PASS IF 00, AC,"
   call print_newline
   print_string_literal "A8 A8 A8 A8 ..."
   ret

newline:
   call print_newline
   jp print_loop - 2

stat_callback:
   ; Store STAT in $C400 + LYC
   ldh a, (<STAT)
   ld h, $C4
   ld l, b
   ld (hl), a

   xor a
   or d
   jr nz, stat_done 

   ; The next time we hit this interrupt, we'll take the other branch
   inc d 
   ld a, $28 ; OAM + HBLANK
   ldh (<STAT), a
  
stat_done:
   ; Clear raiased interrupt flags
   xor a
   ldh (<IF), a
   reti


vblank_callback:

   ; Increment LYC
   inc b ; This will overflow to 0 on our first loop
   ld a, b
   ldh (<LYC), a
   ld a, $40 ; Enable LYC interrupt
   ldh (<STAT), a

   ld d, 0 ; Register D is a flag to indicate if we caught LYC (0) or OAM (1)

   ; Enable STAT interrupt and VBLANK interrupt
   ld a, 3
   ldh (<IE), a

   add sp, +2 ; We're jumping out, not returning, so lets not overflow
   
   ld a, b
   cp $90
   jp z, test_finish

   ; Clear raised interrupts
   xor a
   ldh (<IF), a
   ei
   jp wait

.org INTR_VEC_STAT
   jp stat_callback

.org INTR_VEC_VBLANK
   jp vblank_callback
