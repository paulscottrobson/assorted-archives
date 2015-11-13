; **************************************************************************************************************
;									
;                                           Slow counter on LEDs
;
; **************************************************************************************************************

	cpu 	8008new
	org 	0h

Start:
    mov     a,l
    out     018h
    xri     255
    out     019h
    inr     l
    mvi     d,040h
Delay:
    dcr     e
    jnz     Delay
    dcr     d
    jnz     Delay
    jmp     Start