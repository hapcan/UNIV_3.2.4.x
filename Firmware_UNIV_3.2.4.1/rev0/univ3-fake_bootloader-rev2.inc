	LIST
;==============================================================================
;   HAPCAN - Home Automation Project Firmware (http://hapcan.com)
;   Copyright (C) 2013 Jacek Siwilo
;
;   This program is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see <http://www.gnu.org/licenses/>.
;==============================================================================
;   Filename:              univ3-fake_bootloader-revX.inc
;   Associated diagram:    none
;   Author:                Jacek Siwilo                           
;   Description            This is a fake bootloader for UNIV 3 CPU firmware
;                          project template. It allows your firmware debugging.
;==============================================================================
;   Revision History
;   Rev:  Date:     Details:
;   0     01.2013   Original version
;   1     03.2013   CAN interrupt changes
;   2     06.2013   Node ID, minor changes
;==============================================================================
;   EEPROM DATA
;==============================================================================
EEPROMDATA  code    0xF00026
		DE		0x05,0x05                          ;Node ID (NODE No, GROUP No)

;==============================================================================
;   FAKE BOOTLOADDER REGISTER DEFINITIONS
;==============================================================================
BootRAM     EQU     H'0001'             ;RAM for bootloader (BANK 1)
bR1         EQU     H'016D'             ;general purpose register
bR2         EQU     H'016E'             ;general purpose register

;==============================================================================
;   CONFIGURATION BITS
;==============================================================================
CONFIGCODE  code
    CONFIG  RETEN=OFF, INTOSCSEL=HIGH, SOSCSEL=DIG, XINST=OFF        ;CONFIG 1L
    CONFIG  FOSC=HS1, PLLCFG=OFF, FCMEN=OFF, IESO=OFF                ;CONFIG 1H
    CONFIG  PWRTEN=ON, BOREN=ON, BORV=1, BORPWR=MEDIUM               ;CONFIG 2L
    CONFIG  WDTEN=ON, WDTPS=2048                                     ;CONFIG 2H
    CONFIG  CANMX=PORTB, MSSPMSK=MSK7, MCLRE=ON                      ;CONFIG 3H
    CONFIG  STVREN=ON, BBSIZ=BB2K                                    ;CONFIG 4L

;==============================================================================
;   VECTORS
;==============================================================================
; Overview:         Reset, low and high interrupt vectors come here.				
;==============================================================================
;RESET VECTOR
ResetV  code    0x000000
        goto    ResetVector
;HIGH PRIORITY INTERRUPT VECTOR
HighV   code    0x000008
        goto    HighInt
;LOW PRIORITY INTERRUPT VECTOR
LowV    code    0x000018
        goto    LowInt

;==============================================================================
; Routine:          START ROUTINE
;==============================================================================
; Overview:         It does general initialization of the node				
;==============================================================================
ResetVector
        rcall   GlobalConfiguration   ;global initialization
        rcall   CanConfiguration      ;initialize CAN
        rcall   CheckIfUART           ;is UART needed?
        goto    0x001020              ;go to MAIN SOFTWARE (moved reset vector)

;==============================================================================
; Routine:          GLOBAL CONFIGURATION
;==============================================================================
; Overview:         It sets main config
;==============================================================================
GlobalConfiguration:
    ;watchdog
        bsf     WDTCON,REGSLP           ;regulator goes into Low-Power mode when device’s Sleep mode is enabled
        bcf     WDTCON,SRETEN           ;regulator is on when device’s Sleep mode is enabled and the Low-Power mode is controlled by REGSLP
        bsf     WDTCON,SWDTEN           ;watchdog Timer is on
    ;osc bits
        bsf     OSCCON,IDLEN            ;device enters an Idle mode when a SLEEP instruction is executed
        bcf     OSCCON,SCS0             ;default primary oscillator
        bcf     OSCCON,SCS1
    ;bor
        bsf     RCON,SBOREN             ;BOR enabled
    ;disable peripheral
        movlb   0xF
        movlw   b'11111111'             ;The USART1 module is enabled only when needed in UARTConfiguration routine
        movwf   PMD0
        movwf   PMD1
        movlw   b'11111011'             ;The Enhanced CAN module is enabled
        movwf   PMD2
    ;firmware flag
        clrf    FIRMREADY               ;clear firmware started completely flag
    ;interrupt
        bsf     RCON,IPEN               ;enable priority
        bsf     INTCON,GIEH             ;enable high interrupt
        bsf     INTCON,GIEL             ;enable low interrupt
    return

;==============================================================================
; Routine:          CAN CONFIGURATION ROUTINE
;==============================================================================
; Overview:         It sets configuration of bit timing, masks and filters.
;==============================================================================
CanConfiguration
        movlw   b'10000000'             ;request configuration mode
        movwf   CANCON
ChkConModeOn:
        btfss   CANSTAT,7               ;check if configuration mode is on
        bra     ChkConModeOn

        clrf    ECANCON                 ;Mode 0
        movlw   b'00100001'             ;CANTX2 is digital pin, CANTX pin will drive VDD, oscillator as the source for CAN
        movwf   CIOCON		

    ;setting can baud rate (125kbps, Tbit=16TQ for 8MHz clock)
        movlb   0xE                     ;choose bank 14
        movlw   b'00000001'             ;set SJW=1xTQ & BRP=0 (TQ=4xTosc)
        movwf   BRGCON1
        movlw   b'10111100'             ;set SAM=1 SEG1PH=8xTQ PRSEG=5xTQ
        movwf   BRGCON2
        movlw   b'00000001'             ;set WAKFIL=0 & SEG2PH=2xTQ
        movwf   BRGCON3

    ;setting Masks & Filters
        clrf    RXM0SIDH                ;clear Mask 0 - all message receives
        clrf    RXM0SIDL                ;don't care about filters
        clrf    RXM0EIDH
        clrf    RXM0EIDL
        clrf    RXM1SIDH                ;clear Mask 1 - all message receives
        clrf    RXM1SIDL                ;don't care about filters
        clrf    RXM1EIDH
        clrf    RXM1EIDL
        bsf     RXF0SIDL,3              ;set EXIDEN bit in Filter 0
		
    ;setting configuration of receive buffers
        movlw   b'01000100'             ;receive only valid extended messages
        movwf   RXB0CON                 ;and overflow RX0 will write to RX1
        movlb   0xF                     ;choose bank 15
        movwf   RXB1CON

    ;setting can interrupt
        movlw   b'00100011'
        movwf   PIE5                    ;enable interrupts
        clrf    IPR5                    ;set low priority for all

    ;end of can configuration
        bsf     TRISB,3                 ;set can input
        bcf     TRISB,2                 ;set can output
        movlw   b'00000000'             ;request normal mode
        movwf   CANCON

    ;flag for main firmware
        movlb   BootRAM                 ;choose bootloader bank
        clrf    CANFULL                 ;clear CAN buffer full flag

ChkConModeOff:
        btfsc   CANSTAT,7               ;check if configuration mode is off
        bra     ChkConModeOff           ;note: masks & filters read as 00h, but in fact they are set
    return

;==============================================================================
; Routine:          CAN BOOTLOADER LOW INTERRUPT
;==============================================================================
; Overview:         Routine receives CAN messages
;==============================================================================
LowInt
    ;save shadow registers (used in bootloader)
        movff   STATUS,STATUS_L         ;save STATUS register
        movff   WREG,WREG_L             ;save working register
        movff   BSR,BSR_L               ;save BSR register
        movff   CANCON,CANCON_L         ;save other registers
        movff   FSR0L,FSR0L_L			
        movff   FSR0H,FSR0H_L
        movff   FSR1L,FSR1L_L
        movff   FSR1H,FSR1H_L
    ;call bootloader procedure
        call    CanInterrupt
    ;restore shadow registers (used in bootloader)
        movff   STATUS_L,STATUS         ;restore STATUS register
        movff   WREG_L,WREG             ;restore working register
        movff   BSR_L,BSR               ;restore BSR register
        movff   CANCON_L,CANCON         ;restore other registers
        movff   FSR0L_L,FSR0L			
        movff   FSR0H_L,FSR0H
        movff   FSR1L_L,FSR1L
        movff   FSR1H_L,FSR1H
    ;go to firmware interrupt
        goto    0x001040                ;go to MAIN SOFTWARE (low interrupt)

;==============================================================================
; Routine:          CAN INTERRUPT ROUTINE
;==============================================================================
CanInterrupt    
		movf	PCL,W                   ;sequence needed to refresh PC

		movf    CANSTAT,W               ;retrieve ICODE bits
		andlw   b'00001110'
		addwf   PCL,F                   ;go to corresponding interrupt cause
        bra     EndOfCanInterrupt       ;000 = No interrupt             ;PC should never vector here
		bra     ErrorInterrupt          ;001 = Error interrupt
        bra     EndOfCanInterrupt       ;010 = TXB2 interrupt
        bra     EndOfCanInterrupt       ;011 = TXB1 interrupt
        bra     EndOfCanInterrupt       ;100 = TXB0 interrupt
		bra     RXB1Interrupt           ;101 = RXB1 interrupt
		bra     RXB0Interrupt           ;110 = RXB0 interrupt
        bra     EndOfCanInterrupt       ;111 = Wake-up on interrupt
;=======
ErrorInterrupt:
        btfss   COMSTAT,RXB1OVFL        ;buf1 overflow? (buf0 never overflows, because overflow goes to buf1)
        bra     ErrorInterruptExit
        movlb   0xF        
        bcf     RXB1CON,RXFUL           ;clear buffer 1 full flag
        movlw   b'11110001'             ;access buffer 0
        andwf   CANCON
        bcf     RXB0CON,RXFUL           ;clear buffer 1 full flag
        bcf     COMSTAT,RXB1OVFL        ;clear overflow flag
ErrorInterruptExit
        bcf     PIR5,ERRIF
        bra     EndOfCanInterrupt       ;end of CanInterrupt
;=======
RXB1Interrupt:
        movlw	b'11110001'             ;access RXB1 buffer
        andwf	CANCON                  ;clear WIN bits
        movlw	b'00001010'
        iorwf	CANCON                  ;set WIN bits
        btfsc	RXB0CON,RXFUL           ;check if buffer is full
        bra     $ + .6                  ;full, so read it
        bcf		PIR5,RXB1IF				;clear the interrupt flag when buffer empty (acording to datasheet must be cleared after RXFUL flag)
        bra 	EndOfCanInterrupt       ;end of CanInterrupt
        ;read buffer
        rcall	ReadBuffer
        bcf		PIR5,RXB1IF				;clear the interrupt flag when buffer empty
        bra 	EndOfCanInterrupt       ;end of CanInterrupt
;=======
RXB0Interrupt:
        movlw	b'11110001'             ;access RXB0 buffer
        andwf	CANCON
        btfsc	RXB0CON,RXFUL           ;check if buffer is full
        bra     $ + .6                  ;full, so read it
        bcf		PIR5,RXB0IF				;clear the interrupt flag when buffer empty (acording to datasheet must be cleared after RXFUL flag)
        bra 	EndOfCanInterrupt       ;end of CanInterrupt
        ;read buffer
        rcall	ReadBuffer
        bcf		PIR5,RXB0IF				;clear the interrupt flag when buffer empty
        bra 	EndOfCanInterrupt       ;end of CanInterrupt


EndOfCanInterrupt:
    return

;=======================
ReadBuffer:
		lfsr	FSR0,0xF60              ;point at first register in buffer 0
		lfsr	FSR1,0x100              ;point at first register of destination buffer bRXBCON
		movlw	0x0E                    ;read 14 successive registers
ReadBufferLoop:
		movff	POSTINC0,POSTINC1       ;move data from receive buffer to destination buffer
		decfsz	WREG                    ;all 14 bytes read?
		bra		ReadBufferLoop          ;not yet - read more

	;Changing bits in bRXBSIDL
		movlb	BootRAM				    ;choose bootloader bank
		;b1
		bcf		CANFRAME2,4             ;clear bit in fourth position
		btfsc	CANFRAME2,1             ;what is the first bit?
		bsf		CANFRAME2,4             ;it is 1, so set it in fourth position
		;RTR
		bcf		CANFRAME2,1             ;clear RTR bit
		btfsc	RxBCON,3                ;skip if Remote Transmission Request received
		bsf		CANFRAME2,1             ;set RTR bit
		;unused
		bcf		CANFRAME2,2             ;clear bits <3,2> 
		bcf		CANFRAME2,3

    ;allow receiving another message
		bcf		RXB0CON,RXFUL		    ;clear buffer full flag		
    ;flag for main firmware
        setf    CANFULL                 ;CAN buffer full flag
    return

;==============================================================================
; Routine:          CHECK IF UART IS REQUIRED
;==============================================================================
; Overview:         It checks if UART required, if so UART is initialized
;==============================================================================
CheckIfUART
        movlb   BootRAM                 ;choose bootloader bank
        clrf    UARTON                  ;clear UART enabled flag
        movlb   0xF                     ;bank 16
        bcf     ANCON0,ANSEL1           ;make AN1 digital pin
        bsf     TRISA,1                 ;port as input
        btfsc   PORTA,1                 ;skip if port connected to GND
        rcall   UARTConfiguration       ;initialize UART  
        movlb   0xF                     ;bank 15
        bsf     ANCON0,ANSEL1           ;make AN1 analog pin
    return

;==============================================================================
; Routine:          UART CONFIGURATION ROUTINE
;==============================================================================
; Overview:         It sets configuration of UART
;==============================================================================
UARTConfiguration
    ;enable PLL
        movlb   BootRAM                 ;choose bootloader bank
        setf    UARTON                   ;set UART enabled flag
        bsf     OSCTUNE,PLLEN           ;enable PLL
    ;tibbo reset
		bcf		LATC,1					;reset active low
		bcf		TRISC,1					;start reset, output for RST
        rcall   Delay25000              ;wait 50ms
        rcall   Delay25000
		bsf		TRISC,1					;stop reset, input for RST (internal Tibbo pull up)
	;initiate uart
        movlb   0xF         
        bcf     PMD0,UART1MD            ;enable USART1 module
        clrf    SPBRGH1
        movlw   .68                     ;32MHz/(4(68+1))=115942 bps; 0,6%
        movwf   SPBRG1
        movlw   b'00001000'             ;16bit baud rate register, no auto baud rate
        movwf   BAUDCON1
        movlw   b'00000100'             ;asynchronous mode, high speed
        movwf   TXSTA1
        movlw   b'10010000'             ;serial port enabled, enables continuous receive
        movwf   RCSTA1
    ;other
        movlb   BootRAM                 ;choose bootloader bank
        clrf    UARTOVF                 ;clear overflow flag
        clrf    UARTCNT                 ;clear byte counter
    ;interrupt
        bsf     IPR1,RC1IP              ;high interrupt for uart receiver
        bsf     PIE1,RC1IE              ;enable uart receiver interrupt
    return
Delay25000:
		movlw	0x00
		movwf	bR1	
T40L2:	movlw	0x00
		movwf	bR2
T40L1:	decfsz	bR2
		bra		T40L1
		decfsz	bR1
		bra		T40L2
	return
;==============================================================================
; Routine:          UART BOOTLOADER HIGH INTERRUPT
;==============================================================================
HighInt
    ;check if UART enabled
        movff   BSR,BSR_H               ;save BSR register
        movlb   BootRAM                 ;choose bootloader bank
        tstfsz  UARTON                  ;uart enabled?
        bra     $ + 4                   ;yes
    bra     Goto_0x1030                 ;no - go to main program high interrupt
    ;save shadow registers (used in bootloader)
        movff   STATUS,STATUS_H         ;save STATUS register
        movff   WREG,WREG_H             ;save working register
        movff   FSR0L,FSR0L_H           ;save other registers			
        movff   FSR0H,FSR0H_H
        movff   FSR1L,FSR1L_H
        movff   FSR1H,FSR1H_H
        movff   RCSTA1,RCSTA1_H
    ;call bootloader procedure
        call    ActiveUART
    ;restore shadow registers (used in bootloader)
        movff   STATUS_H,STATUS         ;restore STATUS register
        movff   WREG_H,WREG             ;restore working register
        movff   FSR0L_H,FSR0L           ;restore other registers
        movff   FSR0H_H,FSR0H
        movff   FSR1L_H,FSR1L
        movff   FSR1H_H,FSR1H
        movff   RCSTA1_H,RCSTA1
    ;go to firmware interrupt
Goto_0x1030
        movff   BSR_H,BSR               ;restore BSR register
        goto    0x001030                ;go to MAIN SOFTWARE (high interrupt)

;==============================================================================
; Routine:          UART RECEIVE MESSAGE
;==============================================================================
; Overview:         It receives bytes from UART			
;==============================================================================
ActiveUART
        btfss   PIR1,RC1IF              ;check if uart received anything
    bra     ExitActiveUART              ;not UART interrupt
        movlb   BootRAM
        clrf    UARTOVF                 ;clear receive buffer overflow flag
        clrf    UARTCNT                 ;clear receive buffer counter
        rcall   ResetIdleTimer          ;reset time between received bytes
        lfsr    FSR1,UART0              ;point at first register of UART receive buffer
ActiveUARTLp
    ;overrun error
        btfsc   RCSTA1,OERR             ;check if overrun error occurred
        bra     OverrunError            ;restart receiver and exit
    ;check idle timer
        rcall   DecIdleTimer            ;measures time between received bytes, should be about 90us (time of receiving another byte)
        tstfsz  WREG                    ;skip if timer not overflow (if WREG=0)
        bra     ExitActiveUART
    ;check if received byte
        btfss   PIR1,RC1IF              ;check if uart received anything
        bra     ActiveUARTLp            ;wait for message
    ;received
        rcall   IncRecCounter           ;increment receive buffer counter
        rcall   ResetIdleTimer          ;reset time between received bytes
        movff   RCREG1,POSTINC1         ;move data from uart receive buffer to can transmit buffer
        bra     ActiveUARTLp            ;read more
ExitActiveUART
    return

;=======================
OverrunError
        bcf     RCSTA1,CREN             ;reset overrun error
        bsf     RCSTA1,CREN             ;enable again receiver
	bra ExitActiveUART
;=======================
DecIdleTimer                            ;measures time between received bytes, bR1*16+bR2 
        decfsz  bR2       
    retlw   0                           ;timer not overflow
        tstfsz  bR1
        bra $ + 4
    retlw   1                           ;timer overflow  
        decf    bR1
    retlw   0                           ;timer not overflow
;=======================
ResetIdleTimer                          ;sets (1,5us * bR1*16A+bR2)
        movff   SPBRGH1,bR1             ;these registers give 20% more time than needed
        movff   SPBRG1,bR2   
    return
;=======================
IncRecCounter                           ;increments receive buffer counter up to 30
        incf    UARTCNT                 ;then set overflow flag and start writing to beginning of buffer
        movlw   .30
        cpfsgt  UARTCNT                 ;skip if CNT>30
    return
    ;buffer overflow
        movlw   .1
        movwf   UARTCNT                 ;clear receive buffer counter
        incf    UARTOVF                 ;set receive buffer overflow flag  
        lfsr    FSR1,UART0              ;point again at first register of UART receive buffer
    return

;==============================================================================
;==============================================================================
; END OF FAKE BOOTLOADER
;==============================================================================
;==============================================================================
