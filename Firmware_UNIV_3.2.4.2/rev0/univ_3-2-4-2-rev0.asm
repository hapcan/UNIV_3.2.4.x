;==============================================================================
;   HAPCAN - Home Automation Project Firmware (http://hapcan.com)
;   Copyright (C) 2017 hapcan.com
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
;   Filename:              univ_3-2-4-2.asm
;   Associated diagram:    univ_3-2-4-x.sch
;   Author:                Jacek Siwilo                          
;   Note:                  Bistable Relay
;==============================================================================
;   Revision History
;   Rev:  Date:     Details:
;   0     03.2017   Original version
;==============================================================================
;===  FIRMWARE DEFINITIONS  =================================================== 
;==============================================================================
    #define    ATYPE    .2                            ;application type [0-255]
    #define    AVERS    .4                         ;application version [0-255]
    #define    FVERS    .2                            ;firmware version [0-255]

    #define    FREV     .0                         ;firmware revision [0-65536]
;==============================================================================
;===  NEEDED FILES  ===========================================================
;==============================================================================
    LIST P=18F26K80                              ;directive to define processor
    #include <P18F26K80.INC>           ;processor specific variable definitions
    #include <univ_3-2-4-2-rev0.inc>                         ;project variables
INCLUDEDFILES   code
    #include <univ3-routines-rev8.inc>                     ;UNIV 3 CPU routines
;==============================================================================
;===  FIRMWARE CHECKSUM  ======================================================
;==============================================================================
FIRMCHKSM   code    0x001000
    DB      0x65, 0xDD, 0x1E
;==============================================================================
;===  FIRMWARE ID  ============================================================
;==============================================================================
FIRMID      code    0x001010
    DB      0x30, 0x00, 0x03,ATYPE,AVERS,FVERS,FREV>>8,FREV
;            |     |     |     |     |     |     |_____|_____ firmware revision
;            |     |     |     |     |     |__________________ firmware version
;            |     |     |     |     |_____________________ application version
;            |     |     |     |______________________________ application type
;            |     |     |________________________________ hardware version '3'
;            |_____|______________________________________ hardware type 'UNIV'
;==============================================================================
;===  MOVED VECTORS  ==========================================================
;==============================================================================
;PROGRAM RESET VECTOR
FIRMRESET   code    0x1020
        goto    Main
;PROGRAM HIGH PRIORITY INTERRUPT VECTOR
FIRMHIGHINT code    0x1030
        call    HighInterrupt
        retfie
;PROGRAM LOW PRIORITY INTERRUPT VECTOR
FIRMLOWINT  code    0x1040
        call    LowInterrupt
        retfie

;==============================================================================
;===  FIRMWARE STARTS  ========================================================
;==============================================================================
FIRMSTART   code    0x001050
;------------------------------------------------------------------------------
;---  LOW PRIORITY INTERRUPT  -------------------------------------------------
;------------------------------------------------------------------------------
LowInterrupt
        movff   STATUS,STATUS_LOW           ;save STATUS register
        movff   WREG,WREG_LOW               ;save working register
        movff   BSR,BSR_LOW                 ;save BSR register
        movff   FSR0L,FSR0L_LOW             ;save other registers used in high int
        movff   FSR0H,FSR0H_LOW
        movff   FSR1L,FSR1L_LOW
        movff   FSR1H,FSR1H_LOW

    ;main firmware ready flag
        banksel FIRMREADY
        btfss   FIRMREADY,0
        bra     ExitLowInterrupt            ;main firmware is not ready yet
    ;CAN buffer
        banksel CANFULL
        btfsc   CANFULL,0                   ;check if CAN received anything
        call    CANInterrupt                ;proceed with CAN interrupt

ExitLowInterrupt
        movff   BSR_LOW,BSR                 ;restore BSR register
        movff   WREG_LOW,WREG               ;restore working register
        movff   STATUS_LOW,STATUS           ;restore STATUS register
        movff   FSR0L_LOW,FSR0L             ;restore other registers used in high int
        movff   FSR0H_LOW,FSR0H
        movff   FSR1L_LOW,FSR1L
        movff   FSR1H_LOW,FSR1H
    return

;------------------------------------------------------------------------------
;---  HIGH PRIORITY INTERRUPT  ------------------------------------------------
;------------------------------------------------------------------------------
HighInterrupt
        movff   STATUS,STATUS_HIGH          ;save STATUS register
        movff   WREG,WREG_HIGH              ;save working register
        movff   BSR,BSR_HIGH                ;save BSR register
        movff   FSR0L,FSR0L_HIGH            ;save other registers used in high int
        movff   FSR0H,FSR0H_HIGH
        movff   FSR1L,FSR1L_HIGH
        movff   FSR1H,FSR1H_HIGH

    ;main firmware ready flag
        banksel FIRMREADY
        btfss   FIRMREADY,0
        bra     ExitHighInterrupt           ;main firmware is not ready yet
    ;Timer0
        btfsc   INTCON,TMR0IF               ;Timer0 interrupt? (1000ms)
        rcall   Timer0Interrupt
    ;Timer2    
        btfsc   PIR1,TMR2IF                 ;Timer2 interrupt? (20ms)
        rcall   Timer2Interrupt

ExitHighInterrupt
        movff   BSR_HIGH,BSR                ;restore BSR register
        movff   WREG_HIGH,WREG              ;restore working register
        movff   STATUS_HIGH,STATUS          ;restore STATUS register
        movff   FSR0L_HIGH,FSR0L            ;restore other registers used in high int
        movff   FSR0H_HIGH,FSR0H
        movff   FSR1L_HIGH,FSR1L
        movff   FSR1H_HIGH,FSR1H
    return

;------------------------------------------------------------------------------
; Routine:          CAN INTERRUPT
;------------------------------------------------------------------------------
; Overview:         Checks CAN message for response and RTR and saves to FIFO
;------------------------------------------------------------------------------
CANInterrupt
        banksel CANFRAME2
        btfsc   CANFRAME2,0                 ;response message?
    return                                  ;yes, so ignore it and exit
        btfsc   CANFRAME2,1                 ;RTR (Remote Transmit Request)?
    return                                  ;yes, so ignore it and exit
        call    Copy_RXB_RXFIFOIN           ;copies received message to CAN RX FIFO input buffer
        call    WriteToCanRxFIFO            ;saves message to FIFO
    return

;------------------------------------------------------------------------------
; Routine:          TIMER 0 INTERRUPT
;------------------------------------------------------------------------------
; Overview:         1000ms periodical interrupt
;------------------------------------------------------------------------------
Timer0Interrupt:
        call    Timer0Initialization8MHz    ;restart 1000ms Timer   
        call    UpdateUpTime                ;counts time from restart
        call    UpdateTransmitTimer         ;increment transmit timer (seconds after last transmission)
        banksel TIMER0_1000ms
        setf    TIMER0_1000ms               ;timer 0 interrupt occurred flag
    return

;------------------------------------------------------------------------------
; Routine:            TIMER 2 INTERRUPT
;------------------------------------------------------------------------------
; Overview:            20ms periodical interrupt
;------------------------------------------------------------------------------
Timer2Interrupt
        rcall   Timer2Initialization8MHz    ;restart timer
        banksel TIMER2_20ms
        setf    TIMER2_20ms                 ;timer 2 interrupt occurred flag
    return
;-------------------------------
Timer2Initialization8MHz
        movlb   0xF
        bcf     PMD1,TMR2MD                 ;enable timer 2
        movlw   0x3F          
        movwf   TMR2                        ;set 20ms (19.999500)
        movlw   b'01001111'                 ;start timer, prescaler=16, postscaler=10
        movwf   T2CON
        bsf     IPR1,TMR2IP                 ;high priority for interrupt
        bcf     PIR1,TMR2IF                 ;clear timer's flag
        bsf     PIE1,TMR2IE                 ;interrupt on
    return

;==============================================================================
;===  MAIN PROGRAM  ===========================================================
;==============================================================================
Main:
    ;disable global interrupts for startup
        call    DisAllInt                   ;disable all interrupt
    ;firmware initialization
        rcall   PortInitialization          ;prepare processor ports
        call    GeneralInitialization       ;read eeprom config, clear other registers
        call    FIFOInitialization          ;prepare FIFO buffers
        call    RelayPowerUpStates          ;set relay power up states
        call    Timer0Initialization8MHz    ;Timer 0 initialization for 1s periodical interrupt 
        call    Timer2Initialization8MHz    ;Timer 2 initialization for 20ms periodical interrupt
    ;firmware ready
        banksel FIRMREADY
        bsf     FIRMREADY,0                 ;set flag "firmware started and ready for interrupts"
    ;enable global interrupts
        call    EnAllInt                    ;enable all interrupts

;-------------------------------
Loop:                                       ;main loop
        clrwdt                              ;clear Watchdog timer
        call    ReceiveProcedure            ;check if any msg in RX FIFO and if so - process the msg
        call    TransmitProcedure           ;check if any msg in TX FIFO and if so - transmit it
        rcall   OnceA20ms                   ;do routines only after 20ms interrupt 
        rcall   OnceA1000ms                 ;do routines only after 1000ms interrupt
    bra     Loop

;-------------------------------
OnceA20ms                                   ;procedures executed once per 1000ms (flag set in interrupt)
        banksel TIMER2_20ms
        tstfsz  TIMER2_20ms                 ;flag set?
        bra     $ + 4
    return                                  ;no, so exit
        call    SetRelays                   ;set new relay states when needed
        call    ResetRelays                 ;remove power from bistable relay
        banksel TIMER2_20ms
        clrf    TIMER2_20ms
    return
;-------------------------------
OnceA1000ms                                 ;procedures executed once per 1000ms (flag set in interrupt)
        banksel TIMER0_1000ms
        tstfsz  TIMER0_1000ms               ;flag set?
        bra     $ + 4
    return                                  ;no, so exit
        call    UpdateDelayTimers           ;updates channel timers 
        call    SaveSateToEeprom            ;save relay states into eeprom memory when needed
        call    UpdateHealthRegs            ;saves health maximums to eeprom
        banksel TIMER0_1000ms
        clrf    TIMER0_1000ms
    return


;==============================================================================
;===  FIRMWARE ROUTINES  ======================================================
;==============================================================================
;------------------------------------------------------------------------------
; Routine:          PORT INITIALIZATION
;------------------------------------------------------------------------------
; Overview:         It sets processor pins. All unused pins should be set as
;                   outputs and driven low
;------------------------------------------------------------------------------
PortInitialization                          ;default all pins set as analog (portA,B) or digital (portB,C) inputs 
    ;PORT A
        banksel ANCON0                      ;select memory bank
        ;0-digital, 1-analog input
        movlw   b'00000011'                 ;(x,x,x,AN4,AN3,AN2,AN1-boot_mode,AN0-volt)
        movwf   ANCON0
        ;output level
        clrf    LATA                        ;all low
        ;0-output, 1-input
        movlw   b'00000011'                 ;all outputs except, bit<1>-boot_mode, bit<0>-volt
        movwf   TRISA        
    ;PORT B
        ;0-digital, 1-analog input
        movlw   b'00000000'                 ;(x,x,x,x,x,AN10,AN9,AN8)
        movwf   ANCON1
        ;output level
        clrf    LATB                        ;all low
        ;0-output, 1-input
        movlw   b'00001000'                 ;all output except CANRX
        movwf   TRISB
    ;PORT C
        ;output level
        clrf    LATC                        ;all low
        ;0-output, 1-input
        movlw   b'00000000'                 ;all output 
        movwf   TRISC
    return
;------------------------------------------------------------------------------
; Routine:          NODE STATUS
;------------------------------------------------------------------------------
; Overview:         It prepares status messages when status request was
;                   received
;------------------------------------------------------------------------------
NodeStatusRequest
        banksel TXFIFOIN0
        movlw   0x01                        ;this is K1
        movwf   TXFIFOIN6
        btfsc   RelayStates,0               ;is relay off?
        bra     $ + 6                       ;no, so set D3
        clrf    TXFIFOIN7                   ;yes, so clear D3
        bra     $ + 4
        setf    TXFIFOIN7
        movff   Instr1Ch1,TXFIFOIN9         ;info what instruction is waiting for execution
        movlw   0x01
        movwf   TXFIFOIN10                  ;info what instruction is waiting for execution (channel)
        movff   TimerCh1,TXFIFOIN11         ;value of channel timer
        rcall   SendRelayStatus
        ;------------------
        movlw   0x02                        ;this is K2
        movwf   TXFIFOIN6
        btfsc   RelayStates,1
        bra     $ + 6
        clrf    TXFIFOIN7
        bra     $ + 4
        setf    TXFIFOIN7
        movff   Instr1Ch2,TXFIFOIN9
        movlw   0x02
        movwf   TXFIFOIN10
        movff   TimerCh2,TXFIFOIN11
        rcall   SendRelayStatus
        ;------------------
        movlw   0x03                        ;this is K3
        movwf   TXFIFOIN6
        btfsc   RelayStates,2
        bra     $ + 6
        clrf    TXFIFOIN7
        bra     $ + 4
        setf    TXFIFOIN7
        movff   Instr1Ch3,TXFIFOIN9
        movlw   0x04
        movwf   TXFIFOIN10
        movff   TimerCh3,TXFIFOIN11
        rcall   SendRelayStatus
        ;------------------
        movlw   0x04                        ;this is K4
        movwf   TXFIFOIN6
        btfsc   RelayStates,3
        bra     $ + 6
        clrf    TXFIFOIN7
        bra     $ + 4
        setf    TXFIFOIN7
        movff   Instr1Ch4,TXFIFOIN9
        movlw   0x08
        movwf   TXFIFOIN10
        movff   TimerCh4,TXFIFOIN11
        rcall   SendRelayStatus
        ;------------------
        movlw   0x05                        ;this is K5
        movwf   TXFIFOIN6
        btfsc   RelayStates,4
        bra     $ + 6
        clrf    TXFIFOIN7
        bra     $ + 4
        setf    TXFIFOIN7
        movff   Instr1Ch5,TXFIFOIN9
        movlw   0x10
        movwf   TXFIFOIN10
        movff   TimerCh5,TXFIFOIN11
        rcall   SendRelayStatus
        ;------------------
        movlw   0x06                        ;this is K6
        movwf   TXFIFOIN6
        btfsc   RelayStates,5
        bra     $ + 6
        clrf    TXFIFOIN7
        bra     $ + 4
        setf    TXFIFOIN7
        movff   Instr1Ch6,TXFIFOIN9
        movlw   0x20
        movwf   TXFIFOIN10
        movff   TimerCh6,TXFIFOIN11
        rcall   SendRelayStatus
    return

SendRelayStatus
        movlw   0x30                        ;set relay frame
        movwf   TXFIFOIN0
        movlw   0x20
        movwf   TXFIFOIN1
        bsf     TXFIFOIN1,0                 ;response bit
        movff   NODENR,TXFIFOIN2            ;node id
        movff   GROUPNR,TXFIFOIN3
        setf    TXFIFOIN4                   ;unused
        setf    TXFIFOIN5                   ;unused
        setf    TXFIFOIN8                   ;unused
        call    WriteToCanTxFIFO
    return

;------------------------------------------------------------------------------
; Routine:          DO INSTRUCTION
;------------------------------------------------------------------------------
; Overview:         Executes instruction immediately or sets timer for later
;                   execution
;------------------------------------------------------------------------------
DoInstructionRequest
        banksel INSTR1

;Check if timer is needed
        movff   INSTR3,TIMER                ;timer is in INSTR3 for this firmware
        tstfsz  TIMER                       ;is timer = 0?
        bra     $ + 8                       ;no
        call    DoInstructionNow            ;yes
    return
        call    DoInstructionLater          ;save instruction for later execution
    return

;-------------------------------
;Recognize instruction
DoInstructionNow
        movlw   0x00                        ;instruction 00?
        xorwf   INSTR1,W
        bz      Instr00
        movlw   0x01                        ;instruction 01?
        xorwf   INSTR1,W
        bz      Instr01
        movlw   0x02                        ;instruction 02?
        xorwf   INSTR1,W
        bz      Instr02
    bra     ExitDoInstructionNow            ;exit if unknown instruction

;-------------------------------
;Instruction execution
Instr00                                     ;turn off
        movf    INSTR2,W                    ;get mask of channels to change
        comf    WREG                        ;modify mask
        andwf   RelayStatesNew,F
        bra     ExitDoInstructionNow 
Instr01                                     ;turn on
        movf    INSTR2,W                    ;get mask of channels to change
        iorwf   RelayStatesNew,F
        bra     ExitDoInstructionNow 
Instr02                                     ;toggle
        movf    INSTR2,W                    ;get mask of channels to change
        xorwf   RelayStatesNew,F
        bra     ExitDoInstructionNow 

ExitDoInstructionNow
        setf    INSTR1                        ;clear instruction
        clrf    TIMER                       ;clear timer
        call    DoInstructionLater          ;clear waiting instruction for channel indicated in INSTR2
    return                        

;------------------------------------------------------------------------------
; Routine:            DO INSTRUCTION LATER
;------------------------------------------------------------------------------
; Overview:            It saves instruction for particular channel for later
;                   execution
;------------------------------------------------------------------------------
DoInstructionLater
        call    SetTimer                    ;update SUBTIMER1 & SUBTIMER2 registers
        ;identify channels
        banksel INSTR2
        btfsc   INSTR2,0                    ;channel 1
        call    SetChanel1
        btfsc   INSTR2,1                    ;channel 2
        call    SetChanel2
        btfsc   INSTR2,2                    ;channel 3
        call    SetChanel3
        btfsc   INSTR2,3                    ;channel 4
        call    SetChanel4
        btfsc   INSTR2,4                    ;channel 5
        call    SetChanel5
        btfsc   INSTR2,5                    ;channel 6
        call    SetChanel6
ExitDoInstructionLater
    return

;-------------------------------
SetChanel1
        movff   INSTR1,Instr1Ch1            ;copy registers
        movlw   b'00000001'
        movff   WREG,Instr2Ch1
        movff   TIMER,TimerCh1
        movff   SUBTIMER1,SubTmr1Ch1
        movff   SUBTIMER2,SubTmr2Ch1
    return
SetChanel2
        movff   INSTR1,Instr1Ch2
        movlw   b'00000010'
        movff   WREG,Instr2Ch2
        movff   TIMER,TimerCh2
        movff   SUBTIMER1,SubTmr1Ch2
        movff   SUBTIMER2,SubTmr2Ch2
    return
SetChanel3
        movff   INSTR1,Instr1Ch3
        movlw   b'00000100'
        movff   WREG,Instr2Ch3
        movff   TIMER,TimerCh3
        movff   SUBTIMER1,SubTmr1Ch3
        movff   SUBTIMER2,SubTmr2Ch3
    return
SetChanel4
        movff   INSTR1,Instr1Ch4
        movlw   b'00001000'
        movff   WREG,Instr2Ch4
        movff   TIMER,TimerCh4
        movff   SUBTIMER1,SubTmr1Ch4
        movff   SUBTIMER2,SubTmr2Ch4
    return
SetChanel5
        movff   INSTR1,Instr1Ch5
        movlw   b'00010000'
        movff   WREG,Instr2Ch5
        movff   TIMER,TimerCh5
        movff   SUBTIMER1,SubTmr1Ch5
        movff   SUBTIMER2,SubTmr2Ch5
    return
SetChanel6
        movff   INSTR1,Instr1Ch6
        movlw   b'00100000'
        movff   WREG,Instr2Ch6
        movff   TIMER,TimerCh6
        movff   SUBTIMER1,SubTmr1Ch6
        movff   SUBTIMER2,SubTmr2Ch6
    return

;------------------------------------------------------------------------------
; Routine:          RELAY POWER UP STATES
;------------------------------------------------------------------------------
; Overview:         Sets power up states according to configuration
;------------------------------------------------------------------------------
RelayPowerUpStates
        banksel E_RELSOURCESTATE            ;if bit <x>='1' then power up state from "last saved"; if bit <x>='0 then power up states from "set power up values"
        movff   E_RELSETSTATE,RelayStatesNew;take "set power up states" from CONFIG
        comf    E_RELSOURCESTATE,W          ;take bits that will be taken from "last saved" - these bits are zeroes now in WREG 
        andwf   RelayStatesNew,F,ACCESS     ;clear bits that will be taken from "last saved"
        movf    E_RELSOURCESTATE,W          ;take bits that will be taken from "last saved" - these bits are ones now in WREG         
        andwf   E_RELSAVEDSTATE,W           ;remove unwanted bits
        iorwf   RelayStatesNew,F,ACCESS     ;take bits from last saved
        comf    RelayStatesNew,W                    
        movwf   RelayStates                 ;complement RelayStatesNew and move to RelayStates, to toggle all relays at the beginning so their states can be known
    return

;------------------------------------------------------------------------------
; Routine:          SET RELAYS
;------------------------------------------------------------------------------
; Overview:         It sets bistable relays according to RelayStatesNew reg.
;                   Only one relay is set at a time.
;------------------------------------------------------------------------------
SetRelays
        ;any relay is driven now?
        tstfsz  LATC
        bra     ExitSetRelays               ;some relays are driven at the moment, so exit
        btfsc   LATB,0
        bra     ExitSetRelays               ;K5 is driven at the moment, so exit 
        btfsc   LATB,1
        bra     ExitSetRelays               ;K5 is driven at the moment, so exit
        btfsc   LATB,4
        bra     ExitSetRelays               ;K6 is driven at the moment, so exit
        btfsc   LATB,5
        bra     ExitSetRelays               ;K6 is driven at the moment, so exit

        rcall   SetRelaysCh1                ;check if K1 needs to be driven
        tstfsz  WREG                        ;was driven?
        bra     ExitSetRelays               ;yes, so exit because only one relay must be driven at the time
        rcall   SetRelaysCh2
        tstfsz  WREG
        bra     ExitSetRelays
        rcall   SetRelaysCh3
        tstfsz  WREG
        bra     ExitSetRelays
        rcall   SetRelaysCh4
        tstfsz  WREG
        bra     ExitSetRelays
        rcall   SetRelaysCh5
        tstfsz  WREG
        bra     ExitSetRelays
        rcall   SetRelaysCh6
ExitSetRelays
    return

;-------------------------------
SetRelaysCh1
        movf    RelayStatesNew,W            ;new state
        xorwf   RelayStates,W               ;actual state
        btfss   WREG,0                      ;K1 changed?
    retlw   0x00                            ;no, so exit with zero
        banksel E_REPOLARITY
        btfss   E_REPOLARITY,0              ;reverse polarity in config?
        bra     SetRelaysCh1Nor             ;no, normal
SetRelaysCh1Rev
        btfsc   RelayStatesNew,0            ;should go off?
        bra     $ + 8                       ;no
        bsf     LATC,0                      ;relay goes OFF
        bcf     RelayStates,0               ;set previous state to OFF
        bra     $ + 6
        bsf     LATC,6                      ;relay goes ON
        bsf     RelayStates,0               ;set previous state to ON
        bra     ExitSetRelaysCh1
SetRelaysCh1Nor
        btfsc   RelayStatesNew,0            ;should go off?
        bra     $ + 8                       ;no
        bsf     LATC,6                      ;relay goes OFF
        bcf     RelayStates,0               ;set previous state to OFF
        bra     $ + 6
        bsf     LATC,0                      ;relay goes ON
        bsf     RelayStates,0               ;set previous state to ON
ExitSetRelaysCh1
        movlw   0x03                        ;timer will turn power off from relay in (WREG-1)*20ms to WREG*20ms
        movwf   Ch1BiRelTimer               ;set timer
        rcall   EepromToSave                ;indicate that new state needs to be saved in eeprom
        call    RelayStatesCh1              ;send new states
    retlw   0x01                            ;exit with 1 indicating that relay was driven relay

;-------------------------------
SetRelaysCh2
        movf    RelayStatesNew,W
        xorwf   RelayStates,W
        btfss   WREG,1
    retlw   0x00
        banksel E_REPOLARITY
        btfss   E_REPOLARITY,1
        bra     SetRelaysCh2Nor
SetRelaysCh2Rev
        btfsc   RelayStatesNew,1
        bra     $ + 8
        bsf     LATC,1
        bcf     RelayStates,1
        bra     $ + 6
        bsf     LATC,7
        bsf     RelayStates,1
        bra     ExitSetRelaysCh2
SetRelaysCh2Nor
        btfsc   RelayStatesNew,1
        bra     $ + 8
        bsf     LATC,7
        bcf     RelayStates,1
        bra     $ + 6
        bsf     LATC,1
        bsf     RelayStates,1
ExitSetRelaysCh2
        movlw   0x03
        movwf   Ch2BiRelTimer
        rcall   EepromToSave
        call    RelayStatesCh2
    retlw   0x01
;-------------------------------
SetRelaysCh3
        movf    RelayStatesNew,W
        xorwf   RelayStates,W
        btfss   WREG,2
    retlw   0x00
        banksel E_REPOLARITY
        btfss   E_REPOLARITY,2
        bra     SetRelaysCh3Nor
SetRelaysCh3Rev
        btfsc   RelayStatesNew,2
        bra     $ + 8
        bsf     LATC,2
        bcf     RelayStates,2
        bra     $ + 6
        bsf     LATB,0
        bsf     RelayStates,2
        bra     ExitSetRelaysCh3
SetRelaysCh3Nor
        btfsc   RelayStatesNew,2
        bra     $ + 8
        bsf     LATB,0
        bcf     RelayStates,2
        bra     $ + 6
        bsf     LATC,2
        bsf     RelayStates,2
ExitSetRelaysCh3
        movlw   0x03
        movwf   Ch3BiRelTimer
        rcall   EepromToSave
        call    RelayStatesCh3
    retlw   0x01
;-------------------------------
SetRelaysCh4
        movf    RelayStatesNew,W
        xorwf   RelayStates,W
        btfss   WREG,3
    retlw   0x00
        banksel E_REPOLARITY
        btfss   E_REPOLARITY,3
        bra     SetRelaysCh4Nor
SetRelaysCh4Rev
        btfsc   RelayStatesNew,3
        bra     $ + 8
        bsf     LATC,3
        bcf     RelayStates,3
        bra     $ + 6
        bsf     LATB,1
        bsf     RelayStates,3
        bra     ExitSetRelaysCh4
SetRelaysCh4Nor
        btfsc   RelayStatesNew,3
        bra     $ + 8
        bsf     LATB,1
        bcf     RelayStates,3
        bra     $ + 6
        bsf     LATC,3
        bsf     RelayStates,3
ExitSetRelaysCh4
        movlw   0x03
        movwf   Ch4BiRelTimer
        rcall   EepromToSave
        call    RelayStatesCh4
    retlw   0x01
;-------------------------------
SetRelaysCh5
        movf    RelayStatesNew,W
        xorwf   RelayStates,W
        btfss   WREG,4
    retlw   0x00
        banksel E_REPOLARITY
        btfss   E_REPOLARITY,4
        bra     SetRelaysCh5Nor
SetRelaysCh5Rev
        btfsc   RelayStatesNew,4
        bra     $ + 8
        bsf     LATC,4
        bcf     RelayStates,4
        bra     $ + 6
        bsf     LATB,4
        bsf     RelayStates,4
        bra     ExitSetRelaysCh5
SetRelaysCh5Nor
        btfsc   RelayStatesNew,4
        bra     $ + 8
        bsf     LATB,4
        bcf     RelayStates,4
        bra     $ + 6
        bsf     LATC,4
        bsf     RelayStates,4
ExitSetRelaysCh5
        movlw   0x03
        movwf   Ch5BiRelTimer
        rcall   EepromToSave
        call    RelayStatesCh5 
    retlw   0x01
;-------------------------------
SetRelaysCh6
        movf    RelayStatesNew,W
        xorwf   RelayStates,W
        btfss   WREG,5
    retlw   0x00
        banksel E_REPOLARITY
        btfss   E_REPOLARITY,5
        bra     SetRelaysCh6Nor
SetRelaysCh6Rev
        btfsc   RelayStatesNew,5
        bra     $ + 8
        bsf     LATC,5
        bcf     RelayStates,5
        bra     $ + 6
        bsf     LATB,5
        bsf     RelayStates,5
        bra     ExitSetRelaysCh6
SetRelaysCh6Nor
        btfsc   RelayStatesNew,5
        bra     $ + 8
        bsf     LATB,5
        bcf     RelayStates,5
        bra     $ + 6
        bsf     LATC,5
        bsf     RelayStates,5
ExitSetRelaysCh6
        movlw   0x03
        movwf   Ch6BiRelTimer
        rcall   EepromToSave
        call    RelayStatesCh6
    retlw   0x01

;-------------------------------
EepromToSave                                ;indicate that save to eeprom needed
        banksel EEPROMTIMER
        movlw   0x06                        ;wait 6s before saving to eeprom
        movwf   EEPROMTIMER
    return

;------------------------------------------------------------------------------
; Routine:          SEND RELAY STATES
;------------------------------------------------------------------------------
; Overview:         Sends relay new state after executing instruction
;------------------------------------------------------------------------------
RelayStatesCh1                              ;transmit state of K1
        banksel TXFIFOIN0
        movlw   0x01                        ;"K1"
        movwf   TXFIFOIN6
        clrf    TXFIFOIN7                   ;"0x00 - relay is OFF"
        btfsc   RelayStates,0               ;K1 off?
        setf    TXFIFOIN7                   ;no, "0xFF - relay is ON"        
        movff   Instr1Ch1,TXFIFOIN9         ;INSTR1 - waiting instruction
        movlw   0x01                        ;INSTR2
        movwf   TXFIFOIN10
        movff   TimerCh1,TXFIFOIN11         ;TIMER
        rcall   SendRelayState
    return
RelayStatesCh2                              ;transmit state of K2
        banksel TXFIFOIN0
        movlw   0x02
        movwf   TXFIFOIN6
        clrf    TXFIFOIN7
        btfsc   RelayStates,1
        setf    TXFIFOIN7    
        movff   Instr1Ch2,TXFIFOIN9
        movlw   0x02
        movwf   TXFIFOIN10
        movff   TimerCh2,TXFIFOIN11
        rcall   SendRelayState
    return
RelayStatesCh3                              ;transmit state of K3
        banksel TXFIFOIN0
        movlw   0x03
        movwf   TXFIFOIN6
        clrf    TXFIFOIN7
        btfsc   RelayStates,2
        setf    TXFIFOIN7     
        movff   Instr1Ch3,TXFIFOIN9
        movlw   0x04
        movwf   TXFIFOIN10
        movff   TimerCh3,TXFIFOIN11
        rcall   SendRelayState
    return
RelayStatesCh4                              ;transmit state of K4
        banksel TXFIFOIN0
        movlw   0x04 
        movwf   TXFIFOIN6
        clrf    TXFIFOIN7  
        btfsc   RelayStates,3 
        setf    TXFIFOIN7       
        movff   Instr1Ch4,TXFIFOIN9 
        movlw   0x08
        movwf   TXFIFOIN10
        movff   TimerCh4,TXFIFOIN11
        rcall   SendRelayState
    return
RelayStatesCh5                              ;transmit state of K5
        banksel TXFIFOIN0
        movlw   0x05
        movwf   TXFIFOIN6
        clrf    TXFIFOIN7
        btfsc   RelayStates,4 
        setf    TXFIFOIN7     
        movff   Instr1Ch5,TXFIFOIN9
        movlw   0x10
        movwf   TXFIFOIN10
        movff   TimerCh5,TXFIFOIN11
        rcall   SendRelayState
    return
RelayStatesCh6                              ;transmit state of K6
        banksel TXFIFOIN0
        movlw   0x06
        movwf   TXFIFOIN6
        clrf    TXFIFOIN7
        btfsc   RelayStates,5
        setf    TXFIFOIN7      
        movff   Instr1Ch6,TXFIFOIN9
        movlw   0x20
        movwf   TXFIFOIN10
        movff   TimerCh6,TXFIFOIN11
        rcall   SendRelayState
    return

SendRelayState
        movlw   0x30                        ;set relay frame
        movwf   TXFIFOIN0
        movlw   0x20
        movwf   TXFIFOIN1
        movff   NODENR,TXFIFOIN2            ;node id
        movff   GROUPNR,TXFIFOIN3
        setf    TXFIFOIN4                   ;unused
        setf    TXFIFOIN5                   ;unused
        setf    TXFIFOIN8                   ;unused
        call    WriteToCanTxFIFO
    ;node can respond to its own message
        bcf     INTCON,GIEL                 ;disable low priority intr to make sure RXFIFO buffer is not overwritten
        call    Copy_TXFIFOIN_RXFIFOIN
        call    WriteToCanRxFIFO
        bsf     INTCON,GIEL                 ;enable back interrupt
    return

;------------------------------------------------------------------------------
; Routine:          RESET RELAYS
;------------------------------------------------------------------------------
; Overview:         It takes power off from relay coil
;------------------------------------------------------------------------------
ResetRelays:                                ;wait ChxBiRelTimer before taking voltage off
        tstfsz  Ch1BiRelTimer               ;channel1
        bra     $ + 4
        bra     ResetRelaysCh2
        decfsz  Ch1BiRelTimer
        bra     ResetRelaysCh2
        bcf     LATC,0
        bcf     LATC,6
ResetRelaysCh2
        tstfsz  Ch2BiRelTimer               ;K2
        bra     $ + 4
        bra     ResetRelaysCh3
        decfsz  Ch2BiRelTimer
        bra     ResetRelaysCh3
        bcf     LATC,1
        bcf     LATC,7
ResetRelaysCh3
        tstfsz  Ch3BiRelTimer               ;K3
        bra     $ + 4
        bra     ResetRelaysCh4
        decfsz  Ch3BiRelTimer
        bra     ResetRelaysCh4
        bcf     LATC,2
        bcf     LATB,0
ResetRelaysCh4
        tstfsz  Ch4BiRelTimer               ;K4
        bra     $ + 4
        bra     ResetRelaysCh5
        decfsz  Ch4BiRelTimer
        bra     ResetRelaysCh5
        bcf     LATC,3
        bcf     LATB,1
ResetRelaysCh5
        tstfsz  Ch5BiRelTimer               ;K5
        bra     $ + 4
        bra     ResetRelaysCh6
        decfsz  Ch5BiRelTimer
        bra     ResetRelaysCh6
        bcf     LATC,4
        bcf     LATB,4
ResetRelaysCh6
        tstfsz  Ch6BiRelTimer               ;K6
        bra     $ + 4
        bra     EndResetRelays
        decfsz  Ch6BiRelTimer
        bra     EndResetRelays
        bcf     LATC,5
        bcf     LATB,5
EndResetRelays
    return

;------------------------------------------------------------------------------
; Routine:          SAVE STATES TO EEPROM
;------------------------------------------------------------------------------
; Overview:         It saves current relay states into EEPROM memory
;------------------------------------------------------------------------------
SaveSateToEeprom       
        banksel EEPROMTIMER
    ;wait 6s before saving
        tstfsz  EEPROMTIMER
        decfsz  EEPROMTIMER
        bra     ExitSaveSateToEeprom
    ;save to eeprom
        banksel E_RELSAVEDSTATE 
        clrf    EEADRH                      ;point at high address
        movlw   low E_RELSAVEDSTATE         ;point at low address    
        movwf   EEADR
        movf    E_RELSAVEDSTATE,W           ;values the same?
        xorwf   RelayStates,W
        bz      ExitSaveSateToEeprom        ;yes, so don't save
        movff   RelayStates,E_RELSAVEDSTATE ;update E_RELSAVEDSTATE register
        movf    RelayStates,W               ;set data for EepromSaveWREG routine
        call    EepromSaveWREG
ExitSaveSateToEeprom
    return

;==============================================================================
;===  END  OF  PROGRAM  =======================================================
;==============================================================================
    END