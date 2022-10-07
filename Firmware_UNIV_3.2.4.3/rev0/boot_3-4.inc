    LIST
;==============================================================================
;   HAPCAN - Home Automation Project Firmware (http://hapcan.com)
;   Copyright (C) 2013 hapcan.com
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
;    Filename:              boot_3-4.inc
;    Date:                  March 2013
;    Author:                Jacek Siwilo
;    Company:               hapcan.com
;    Description:           CAN & UART BOOTLOADER of UNIV 3 processor        
;==============================================================================
;===  Version History  ========================================================
;==============================================================================
;Ver:   Date:       Details:
;3.0    04.2012     Original version                                      
;3.1    08.2012     CAN rx msg (line 637) & UART rx msg (line 1268)       
;3.2    08.2012     Voltage rqst proc. in CAN (line 981) and UART (line 1476),
;                   PMD1 shadow regs in low interrupt (l. 538)
;3.3    09.2012     Self check (line 108)
;3.4    03.2013     Low interrupt improvments (line 650)                      
;==============================================================================
;===  LOCAL DEFINITION  =======================================================
;==============================================================================
#define     HARD1   0x30       ;hardware type (0x3000 - UNIV)
#define     HARD2   0x00       ;hardware type
#define     HVERS   .03        ;hardware version (UNIV 3)

#define     BVER    .03        ;bootloader software major version
#define     BREV    .04        ;bootloader software minor version
;==============================================================================
;===  BOOTLOADER CHECKSUM  ====================================================
;==============================================================================
BOOTCHKSM   code    0x000010
    DB      0x07, 0xAC, 0x2B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
;            |     |     |_______________________________________________ BSum0
;            |     |_____________________________________________________ BSum1
;            |___________________________________________________________ BSum2
;==============================================================================
;===  HARDWARE ID  ============================================================
;==============================================================================
HARDID      code    0x000020
    DB      HARD1,HARD2,HVERS,0xFF, ID0,  ID1,  ID2,  ID3
;            |     |     |     |     |_____|_____|_____|_________ serial number
;            |     |     |     |________________________________________ unused
;            |     |     |____________________________________ hardware version
;            |_____|_____________________________________________ hardware type  
;==============================================================================
;===  EEPROM DATA  ============================================================
;==============================================================================
EEPROMDATA  code    0xF00000
        DE      0x00                 ;Boot mode if FFh
            ORG     0xF00026
        DE      ID2,ID3              ;Identifier - last 2 bytes of S/N are used
;==============================================================================
;===  CONFIGURATION BITS  =====================================================
;==============================================================================
CONFIGCODE  CODE
    CONFIG  RETEN=OFF, INTOSCSEL=HIGH, SOSCSEL=DIG, XINST=OFF        ;CONFIG 1L
    CONFIG  FOSC=HS1, PLLCFG=OFF, FCMEN=OFF, IESO=OFF                ;CONFIG 1H
    CONFIG  PWRTEN=ON, BOREN=ON, BORV=1, BORPWR=MEDIUM               ;CONFIG 2L
    CONFIG  WDTEN=ON, WDTPS=2048                                     ;CONFIG 2H
    CONFIG  CANMX=PORTB, MSSPMSK=MSK7, MCLRE=ON                      ;CONFIG 3H
    CONFIG  STVREN=ON, BBSIZ=BB2K                                    ;CONFIG 4L
    CONFIG  CP0=ON, CP1=ON, CP2=ON, CP3=ON                           ;CONFIG 5L
    CONFIG  CPB=ON, CPD=ON                                           ;CONFIG 5H
    CONFIG  WRT0=OFF, WRT1=OFF, WRT2=OFF, WRT3=OFF                   ;CONFIG 6L
    CONFIG  WRTC=ON, WRTB=ON, WRTD=OFF                               ;CONFIG 6H
    CONFIG  EBTR0=OFF, EBTR1=OFF, EBTR2=OFF, EBTR3=OFF               ;CONFIG 7L
    CONFIG  EBTRB=ON                                                 ;CONFIG 7H

;==============================================================================
;               VECTORS
;==============================================================================
; Overview:     Reset, low and high interrupt vectors come here.                
;==============================================================================
;RESET VECTOR
ResetV  code    0x0000
        bra     ResetVector
;HIGH PRIORITY INTERRUPT VECTOR
HighV   code    0x0008
        bra     HighInt
;LOW PRIORITY INTERRUPT VECTOR
LowV    code    0x0018
        bra     LowInt

;==============================================================================
; Routine:      CHECK BOOTLOADER MODE ROUTINE
;==============================================================================
; Overview:     It checks after reset if node is in programming mode.                
;==============================================================================
BOOTLOADER      code    0x000028
ResetVector
        clrf    EEADRH                  ;EEPROM address
        clrf    EEADR
        bcf     EECON1,EEPGD            ;point to EEPROM
        bcf     EECON1,CFGS             ;access data FLASH or EEPROM not configuration bits
        bsf     EECON1,RD               ;read EEPROM
        incfsz  EEDATA,W                ;inrement ad move to WREG, skip if zero
        bra     NormalReset             ;normal reset
        goto    ProgrammingMode

NormalReset                             ;DO NOT CHANGE THE ORDER OF THESE PROCEDURES
        rcall   CheckBootloader         ;is bootloader ok?
        rcall   CheckFirmware           ;is firmware ok?
        rcall   CanConfiguration        ;initialize CAN
        rcall   GlobalConfiguration     ;global initialization
        rcall   CheckIfUART             ;is UART needed?
        rcall   BootWait                ;waits for about 3s in case firmware is wrong and programming mode is needed

        movlb   BootRAM
        btfss   bFLAGS1,FirmError       ;skip if firmware error
        goto    0x1020                  ;go to MAIN SOFTWARE (moved reset vector)

        sleep
        bra     $ - 2                   ;stay in loop

;==============================================================================
; Routine:      BOOT WAIT
;==============================================================================
; Overview:     It waits about 3s to allow to communicate with bootloader
;               in case firmware problems.
;               Drives RB6 pin to indicate status.
;               2 slow blinks (1s/1s) - normal mode 8MHz (no UART)
;               4 fast blinks (0,5s/0,5s) - normal mode 32MHz (UART)
;==============================================================================
BootWait
        bcf     TRISB,6                 ;RB6 as output
        bsf     LATB,6
        movlb   BootRAM
        tstfsz  bUART                   ;skip if 8MHz
        bra     Blink_05s_05s

Blink_1s_1s                             ;indicate 8MHz
        rcall   Wait1s_8MHz
        bcf     LATB,6
        rcall   Wait1s_8MHz
        bsf     LATB,6
        rcall   Wait1s_8MHz
        bcf     LATB,6
        bsf     TRISB,6                 ;RB7 as intput
    return

Blink_05s_05s                           ;indicate 32MHz
        rcall   Wait05s_32MHz
        bcf     LATB,6
        rcall   Wait05s_32MHz
        bsf     LATB,6
        rcall   Wait05s_32MHz
        bcf     LATB,6
        rcall   Wait05s_32MHz
        bsf     LATB,6
        rcall   Wait05s_32MHz
        bcf     LATB,6
        rcall   Wait05s_32MHz
        bsf     LATB,6
        rcall   Wait05s_32MHz
        bcf     LATB,6
        bsf     TRISB,6                 ;RB7 as intput
    return

Wait1s_8MHz                             ;routine takes 1s for 8MHz clock
        movlb   BootRAM
        movlw   0xB
        movwf   bR8        
T1L3    movlw   0xFA
        movwf   bR7    
T1L2    movlw   0xFA
        movwf   bR6
T1L1    decfsz  bR6
        bra     T1L1
        decfsz  bR7
        bra     T1L2
        decfsz  bR8
        bra     T1L3
    return

Wait05s_32MHz                           ;routine takes 0,5s for 32MHz clock
        movlb    BootRAM
        movlw    0x15
        movwf    bR8        
T2L3    movlw    0xFA
        movwf    bR7    
T2L2    movlw    0xFA
        movwf    bR6
T2L1    decfsz    bR6
        bra     T2L1
        decfsz    bR7
        bra     T2L2
        decfsz    bR8
        bra     T2L3
    return

;==============================================================================
; Routine:      CHECK IF UART IS REQUIRED
;==============================================================================
; Overview:     It checks if UART required, if so oscillator PLL is enabled
;==============================================================================
CheckIfUART
        movlb   BootRAM                 ;choose bootloader bank
        clrf    bUART                   ;clear UART enabled flag
        movlb   0xF                     ;bank 16
        bcf     ANCON0,ANSEL1           ;make AN1 digital pin
        bsf     TRISA,1                 ;port as input
        btfsc   PORTA,1                 ;skip if port connected to GND
        rcall   EnablePLL               ;enable PLL
        movlb   0xF                     ;bank 15
        bsf     ANCON0,ANSEL1           ;make AN1 analog pin
    return

;=======================
EnablePLL
        movlb   BootRAM                 ;choose bootloader bank
        setf    bUART                   ;set UART enabled flag
        bsf     OSCTUNE,PLLEN           ;enable PLL
        rcall   UARTConfiguration       ;initialize UART
        rcall   BTibboReset             ;reset for Tibbo      
    return

;=======================
BTibboReset:
        bcf     LATC,1                  ;reset active low
        bcf     TRISC,1                 ;start reset, output for RST
        rcall   Delay25000              ;wait 50ms
        rcall   Delay25000
        bsf     TRISC,1                 ;stop reset, input for RST (internal Tibbo pull up)
    return
;=======================
Delay25000:
        movlw    0x00
        movwf    bR4    
T40L2:  movlw    0x00
        movwf    bR5
T40L1:  decfsz   bR5
        bra      T40L1
        decfsz   bR4
        bra      T40L2
    return

;==============================================================================
; Routine:      UART CONFIGURATION ROUTINE
;==============================================================================
; Overview:     It sets configuration of UART
;==============================================================================
UARTConfiguration
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
        clrf    bRBOVF                  ;clear overflow flag
        clrf    bRBCNT                  ;clear byte counter
    ;interrupt
        bsf     IPR1,RC1IP              ;high interrupt for uart receiver
        bsf     PIE1,RC1IE              ;enable uart receiver interrupt
    return

;==============================================================================
; Routine:      BOOTLOADER CHECK ROUTINE
;==============================================================================
; Overview:     It makes checksum of the bootloader.                
;==============================================================================
CheckBootloader
        rcall   CheckFirmwareInit

;Count checksum of flash memory
        clrf    TBLPTRU                 ;load addres to TBLPTR 0x000028
        clrf    TBLPTRH        
        movlw   0x28
        movwf   TBLPTRL
BCheckSumLoop                           ;read flash loop
        rcall   CountCheckSum
        movlw   0x10                    ;was last memory cell read? (0x001000 current address?)
        xorwf   TBLPTRH,W
        bnz     BCheckSumLoop

;Copy checksum from firmware to bFSum
        clrf    TBLPTRU                 ;load addres to TBLPTR 0x000010
        clrf    TBLPTRH        
        movlw   0x10
        movwf   TBLPTRL
        rcall   ReadCheckSumFromFlash   ;read from TABLAT to bFSum registers

;checking if firmware checksum is ok
        rcall   CheckIfFirmOK
        tstfsz  WREG
        bra     ExitBootWrongChsm       ;WREG = 1
        rcall   CheckFirmwareInit       ;WREG = 0; bootloader checksum ok, do init for CheckFirmware routine
    return                              ;exit

;wrong bootloader loop
ExitBootWrongChsm                       ;blink diode on RB6
        bcf     TRISB,6                 ;RB6 as output
        btg     LATB,6                  ;toggle diode
        rcall   Wait1s_8MHz             ;wait 1s
        bra     ExitBootWrongChsm       ;stay in loop

;=======================
CheckFirmwareInit
        movlb   BootRAM
        clrf    bFIRMFLAG               ;clear firmware started completely flag
        clrf    bFLAGS1                 ;clear bootloader flags1 register
        clrf    bChSum0                 ;clear checksum registers
        clrf    bChSum1
        clrf    bChSum2
    return
;=======================
CountCheckSum
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,WREG             ;move flash data to register
        addwf   bChSum0,F
        bnc     $ + 8                   ;increment ChSum1 if carry after add operation
        incf    bChSum1
        bnc     $ + 4                   ;increment ChSum2 if carry after increment operation
        incf    bChSum2
    return
;=======================
ReadCheckSumFromFlash
        lfsr    FSR0,bFSum2             ;start writing to register
        movlw   0x03                
CopyCheckSumLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;4 bytes read?
        bra     CopyCheckSumLoop
    return
;=======================
CheckIfFirmOK
        movlb   BootRAM
        movf    bFSum2,W
        xorwf   bChSum2,W
        bnz     CheckIfFirmWrong
        movf    bFSum1,W
        xorwf   bChSum1,W
        bnz     CheckIfFirmWrong
        movf    bFSum0,W
        xorwf   bChSum0,W
        bnz     CheckIfFirmWrong
        retlw   0                       ;firmware ok, so WREG = 0
CheckIfFirmWrong
        retlw   1                       ;firmware wrong, so WREG = 1

;==============================================================================
; Routine:      FIRMWARE CHECK ROUTINE
;==============================================================================
; Overview:     It makes checksum of the firmware.                
;==============================================================================
CheckFirmware:
        ;rcall   CheckFirmwareInit      ;initialization done in checkbootloader routine

;Count checksum of flash memory
        clrf    TBLPTRU                 ;load addres to TBLPTR 0x001020
        movlw   0x10
        movwf   TBLPTRH        
        movlw   0x20
        movwf   TBLPTRL
CheckSumLoop                            ;read flash loop
        rcall   CountCheckSum
        movlw   0x80                    ;was last memory cell read 0x008000 (2 flash blocks - 32kB)?
        xorwf   TBLPTRH,W
        bnz     CheckSumLoop

;Copy checksum from firmware to bFSum
        clrf    TBLPTRU                 ;load addres to TBLPTR 0x001000
        movlw   0x10
        movwf   TBLPTRH        
        clrf    TBLPTRL
        rcall   ReadCheckSumFromFlash   ;read from TABLAT to bFSum registers

;checking if firmware checksum is ok
        rcall   CheckIfFirmOK
        tstfsz  WREG
        bra     ExitFirmWrongChsm       ;WREG = 1
                                        ;WREG = 0
;Checking firmware type and version
        ;copy firmware type and version from firmware to bFTyp
        clrf    TBLPTRU                 ;load addres to TBLPTR 0x001010
        movlw   0x10
        movwf   TBLPTRH        
        movlw   0x10
        movwf   TBLPTRL
        lfsr    FSR0,bFTyp3             ;start writing to register
        movlw   0x04                
CopyTypeLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;4 bytes read?
        bra     CopyTypeLoop
        ;copy firmware type from ID to bType
        clrf    TBLPTRU                 ;load addres to TBLPTR 0x000020
        clrf    TBLPTRH        
        movlw   0x20
        movwf   TBLPTRL
        lfsr    FSR0,bType3             ;start writing to register
        movlw   0x04                
CopyIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;4 bytes read?
        bra     CopyIDLoop

        movf    bFTyp3,W                ;is ID equal to this written at 0x001010?
        xorwf   bType3,W
        bnz     ExitFirmWrongType
        movf    bFTyp2,W                ;hardware type
        xorwf   bType2,W
        bnz     ExitFirmWrongType
        movf    bFTyp1,W                ;hardware version
        xorwf   bType1,W
        bnz     ExitFirmWrongType
    bra     ExitCheckFirmware

ExitFirmWrongType
        bsf     bFLAGS1,FirmError        ;firmware error
        bsf     bFLAGS1,FirmWrongType    ;firmware wrong type (type or hardware version)
        bra     ExitCheckFirmware
ExitFirmWrongChsm
        bsf     bFLAGS1,FirmError        ;firmware error
        bsf     bFLAGS1,FirmWrongChsm    ;firmware wrong

ExitCheckFirmware
    return

;==============================================================================
; Routine:      GLOBAL CONFIGURATION
;==============================================================================
; Overview:     It sets uart and interrupt
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
    ;disable perpheral
        movlb   0xF
        movlw   b'11111111'             ;The USART1 module is enabled only when needed in UARTConfiguration routine
        movwf   PMD0
        movlw   b'11111111'             ;The A/D module is enabled only when needed in VoltageRequest routines
        movwf   PMD1
        movlw   b'11111011'             ;The Enhanced CAN module is enabled
        movwf   PMD2
    ;interrupt
        bsf     RCON,IPEN               ;enable priority
        bsf     INTCON,GIEH             ;enable high interrupt
        bsf     INTCON,GIEL             ;enable low interrupt
    return


;==============================================================================
; Routine:      CAN CONFIGURATION ROUTINE
;==============================================================================
; Overview:     It sets configuration of bit timing, identifier,
;               masks and filters.
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

    ;setting Masks
        clrf    RXM0SIDH                ;clear Mask 0 - all message receives
        clrf    RXM0SIDL                ;don't care about filters
        clrf    RXM0EIDH
        clrf    RXM0EIDL
        clrf    RXM1SIDH                ;clear Mask 1 - all message receives
        clrf    RXM1SIDL                ;don't care about filters
        clrf    RXM1EIDH
        clrf    RXM1EIDL

        bsf     RXF0SIDL,3              ;set EXIDEN bit in Filter 0
        
    ;seting Identifier
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM
        clrf    EEADRH                  ;start reading from 26h EEPROM address
        movlw   0x26
        movwf   EEADR
        lfsr    FSR0,0x133              ;writing to address from 133h - bTXBEIDH
        movlw   0x02                    ;2 successive registers
ReadIdentifier:
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;all 4 bytes read?
        bra     ReadIdentifier          ;not yet - read more

    ;setting configuration of receive buffers
        movlw   b'01000100'             ;receive only valid extended messages
        movwf   RXB0CON                 ;and overflow RX0 will write to RX1
        movlb   0xF                     ;choose bank 15
        movwf   RXB1CON

    ;setting configuraion of transmit buffers
        movlb   BootRAM                 ;choose bootloader bank
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        clrf    bTXBCON                 ;clear buffer
    
    ;setting can interrupt
        movlw   b'00100011'             ;buf0 (RXB0IE), buf1 (RXB1IE) and CAN error (ERRIE) interrupts
        movwf   PIE5                    ;enable interrupts
        clrf    IPR5                    ;set low priority for all (default is FF)

    ;end of can configuration
        bsf     TRISB,3                 ;set can input
        bcf     TRISB,2                 ;set can output
        movlw   b'00000000'             ;request normal mode
        movwf   CANCON

    ;flag for main firmware
        clrf    bRXBFULL                ;clear CAN buffer full flag

ChkConModeOff:
        btfsc   CANSTAT,7               ;check if configuration mode is off
        bra     ChkConModeOff           ;note: masks & filters read as 00h, but in fact they are set
    return

;==============================================================================
; Routine:      CAN BOOTLOADER LOW INTERRUPT
;==============================================================================
; Overview:     Routine responses to CAN messages
;==============================================================================
LowInt
    ;save shadow registers (used in bootloader)
        movff   STATUS,bSTATUS_L        ;save STATUS register
        movff   WREG,bWREG_L            ;save working register
        movff   BSR,bBSR_L              ;save BSR register
        movff   PIR1,bPIR1_L            ;NOT CHANGED DURING LOW INTERRUPT
        movff   PIR4,bPIR4_L            ;NOT CHANGED DURING LOW INTERRUPT
        movff   PIR5,bPIR5_L            ;save CAN interrupt flags for use in functional firmware
        movff   CANSTAT,bCANSTAT_L      ;save read only reg for use in functional firmware
        movff   CANCON,bCANCON_L
        movff   FSR0L,bFSR0L_L            
        movff   FSR0H,bFSR0H_L
        movff   FSR1L,bFSR1L_L
        movff   FSR1H,bFSR1H_L
        movff   TABLAT,bTABLAT_L
        movff   TBLPTRL,bTBLPTRL_L
        movff   TBLPTRH,bTBLPTRH_L
        movff   TBLPTRU,bTBLPTRU_L
        movff   INTCON,bINTCON_L        ;NOT CHANGED DURING LOW INTERRUPT !!!DO NOT USE movff instr for interrupt config regs!!!
        movff   EECON1,bEECON1_L
        movff   EEDATA,bEEDATA_L
        movff   EEADRH,bEEADRH_L
        movff   EEADR,bEEADR_L
        movff   TRISA,bTRISA_L
        movff   ADCON2,bADCON2_L
        movff   ADCON1,bADCON1_L
        movff   ADCON0,bADCON0_L
        movff   ANCON0,bANCON0_L
        movff   ADRESL,bADRESL_L
        movff   ADRESH,bADRESH_L
        movff   PMD1,bPMD1_L
        movff   RCSTA1,bRCSTA1_L        ;NOT CHANGED DURING LOW INTERRUPT
        movff   TXSTA1,bTXSTA1_L        ;NOT CHANGED DURING LOW INTERRUPT
        movff   COMSTAT,bCOMSTAT_L      ;read only reg, but save it anyway for use in functional firmware
    ;call bootloader procedure
        rcall   CanInterrupt
    ;restore shadow registers (used in bootloader)
        movff   bSTATUS_L,STATUS        ;restore STATUS register
        movff   bWREG_L,WREG            ;restore working register
        movff   bCANCON_L,CANCON
        movff   bFSR0L_L,FSR0L            
        movff   bFSR0H_L,FSR0H
        movff   bFSR1L_L,FSR1L
        movff   bFSR1H_L,FSR1H
        movff   bTABLAT_L,TABLAT
        movff   bTBLPTRL_L,TBLPTRL
        movff   bTBLPTRH_L,TBLPTRH
        movff   bTBLPTRU_L,TBLPTRU
        movff   bEECON1_L,EECON1
        movff   bEEDATA_L,EEDATA
        movff   bEEADRH_L,EEADRH
        movff   bEEADR_L,EEADR
        movff   bTRISA_L,TRISA
        movff   bADCON2_L,ADCON2
        movff   bADCON1_L,ADCON1
        movff   bADCON0_L,ADCON0
        movff   bANCON0_L,ANCON0
        movff   bADRESL_L,ADRESL
        movff   bADRESH_L,ADRESH
        movff   bPMD1_L,PMD1
    ;check if firmware ok
        movlb   BootRAM
        btfsc   bFLAGS1,FirmError       ;skip if firmware ok
        bra     ExitLowInt              ;goto retfie
        movff   bBSR_L,BSR              ;restore BSR register
        goto    0x1040                  ;go to MAIN SOFTWARE (low interrupt)
ExitLowInt
        movff   bBSR_L,BSR              ;restore BSR register
    retfie

;==============================================================================
; Routine:      CAN INTERRUPT ROUTINE
;==============================================================================
; Overview:     It detects what kind of interrupt occured. If it was
;               "received message" interrupt, routine moves data from
;               receive buffer RXB0 or RXB1 to receive buffer bRXBSIDH
;               and then checks received message.
;==============================================================================
;    ORG    0x0370        ;because of addwf PCL,w instruction PCL can't be close to FFh
CanInterrupt    
        movf    PCL,W                   ;sequence needed to refresh PC

        movf    CANSTAT,W               ;retrieve ICODE bits
        andlw   b'00001110'
        addwf   PCL,F                   ;go to corresponding interrupt cause
        bra     EndOfCanInterrupt       ;000 = No interrupt             ;PC should never vector here
        bra     ErrorInterrupt          ;001 = Error interrupt
        bra     EndOfCanInterrupt       ;010 = TXB2 interrupt           ;not used here (can be handled in functional firmware)
        bra     EndOfCanInterrupt       ;011 = TXB1 interrupt           ;not used here (can be handled in functional firmware)
        bra     EndOfCanInterrupt       ;100 = TXB0 interrupt           ;not used here (can be handled in functional firmware)
        bra     RXB1Interrupt           ;101 = RXB1 interrupt
        bra     RXB0Interrupt           ;110 = RXB0 interrupt
        bra     EndOfCanInterrupt       ;111 = Wake-up on interrupt     ;not used here (can be handled in functional firmware)
;=======
ErrorInterrupt:
        btfss   COMSTAT,RXB1OVFL        ;buf1 overflow (buf0 never overflows, because overflow goes to buf1)
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
        movlw   b'11110001'             ;access RXB1 buffer
        andwf   CANCON                  ;clear WIN bits
        movlw   b'00001010'
        iorwf   CANCON                  ;set WIN bits
        btfsc   RXB0CON,RXFUL           ;check if buffer is full
        bra     $ + .6                  ;full, so read it
        bcf     PIR5,RXB1IF             ;clear the interrupt flag when buffer empty (acording to datasheet must be cleared after RXFUL flag)
        bra     EndOfCanInterrupt       ;end of CanInterrupt
        ;read buffer
        rcall   ReadBuffer
        bcf     PIR5,RXB1IF             ;clear the interrupt flag when buffer empty
        rcall   CANCheckMessage
        bra     EndOfCanInterrupt       ;end of CanInterrupt
;=======
RXB0Interrupt:
        movlw   b'11110001'             ;access RXB0 buffer
        andwf   CANCON
        btfsc   RXB0CON,RXFUL           ;check if buffer is full
        bra     $ + .6                  ;full, so read it
        bcf     PIR5,RXB0IF             ;clear the interrupt flag when buffer empty (acording to datasheet must be cleared after RXFUL flag)
        bra     EndOfCanInterrupt       ;end of CanInterrupt
        ;read buffer
        rcall   ReadBuffer
        bcf     PIR5,RXB0IF             ;clear the interrupt flag when buffer empty
        rcall   CANCheckMessage
        bra     EndOfCanInterrupt       ;end of CanInterrupt
;=======
EndOfCanInterrupt:
    return

;=======================
ReadBuffer:
        lfsr    FSR0,0xF60              ;point at first register in buffer 0
        lfsr    FSR1,0x100              ;point at first register of destination buffer bRXBCON
        movlw   0x0E                    ;read 14 successive registers
ReadBufferLoop:
        movff   POSTINC0,POSTINC1       ;move data from receive buffer to destination buffer
        decfsz  WREG                    ;all 14 bytes read?
        bra     ReadBufferLoop          ;not yet - read more

    ;Changing bits in bRXBSIDL
        movlb   BootRAM                 ;choose bootloader bank
        ;b1
        bcf     bRXBSIDL,4              ;clear bit in fourth position
        btfsc   bRXBSIDL,1              ;what is the first bit?
        bsf     bRXBSIDL,4              ;it is 1, so set it in fourth position
        ;RTR
        bcf     bRXBSIDL,1              ;clear RTR bit
        btfsc   bRXBCON,3               ;skip if Remote Transmission Request received
        bsf     bRXBSIDL,1              ;set RTR bit
        ;unused
        bcf     bRXBSIDL,2              ;clear bits <3,2> 
        bcf     bRXBSIDL,3

    ;allow receiving another message
        bcf     RXB0CON,RXFUL           ;clear buffer full flag        
    ;flag for main firmware
        setf    bRXBFULL                ;CAN buffer full flag
    return

;==============================================================================
; Routine:      CAN RECEIVED MESSAGE RECOGNITION
;==============================================================================
; Overview:     It checks if system message received and responses
;==============================================================================
CANCheckMessage
        movlb   BootRAM                 ;choose bootloader bank
    ;is this system message?
        btfsc   bRXBSIDL,0              ;check if it is response
    bra     ExitCANCheckMessage         ;yes, so exit
        movlw   0x20                    ;check if RXBSIDH < 20h
        cpfslt  bRXBSIDH                ;skip if yes
ExitCANCheckMessage
    return

;MESSAGE TO THIS NODE?
        movf    bTXBEIDL,W            
        xorwf   bRXBD3,W
        bnz     CheckGroup              ;go to check group matching
        movf    bTXBEIDH,W
        xorwf   bRXBD2,W
        bnz     CheckGroup              ;go to check group matching

    ;check if system message begins from 0x10 or 0x11
        movlw   0x10
        xorwf   bRXBSIDH,W
        bz      Messages0x10X
        movlw   0x11
        xorwf   bRXBSIDH,W
        bz      Messages0x11X
    bra     ExitCANCheckMessage         ;exit

Messages0x10X
    ;check if message is 100h (enter bootloader mode request)
        movlw   0x00
        xorwf   bRXBSIDL,W
        bnz     $ + 4    
        bra     EnterBootloader
    ;check if message is 101h (ask group for reset)
        movlw   0x10
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     NodeReset
    ;check if message is 102h (ask node for reset)
        movlw   0x20
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     NodeReset
    ;check if message is 103h (ask group for Node ID)
        movlw   0x30                    
        xorwf   bRXBSIDL,W
        bnz     $ + 4    
        bra     CANNodeID
    ;check if message is 104h (ask for Node ID)
        movlw   0x40
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANNodeID
    ;check if message is 105h (ask group for Firmware ID)
        movlw   0x50
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANFirmwareID
    ;check if message is 106h (ask for Node Firmware ID)
        movlw   0x60
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANFirmwareID
    ;check if message is 107h (change identifier)
        movlw   0x70
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANChangeIdentifier
    ;check if message is 10Bh (ask Group for Voltage)
        movlw   0xB0                    
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANVoltageAsk
    ;check if message is 10Ch (ask Node for Voltage)
        movlw   0xC0
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANVoltageAsk
    ;check if message is 10Dh (ask Group for Description)
        movlw   0xD0        
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANDescription
    ;check if mesage is 10Eh (ask Node for Description)
        movlw   0xE0
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANDescription
    ;check if message is 10Fh (ask Group for Procesor ID)
        movlw   0xF0
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANProcesorID
    bra     ExitCANCheckMessage         ;exit

Messages0x11X
    ;check if message is 110h (ask Node for Procesor ID)
        movlw   0x10
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANProcesorID
        bra     ExitCANCheckMessage     ;exit

;MESSAGE TO GROUP?
CheckGroup
        clrf    WREG                    ;check if message is for all nodes
        xorwf   bRXBD3,W
        bz      $ + .10                 ;check if RXBD2 is cleared
        movf    bTXBEIDL,W              ;check if message is for this group
        xorwf   bRXBD3,W
        bz      $ + 4    
    bra     ExitCANCheckMessage         ;it is not for this group
        clrf    WREG                    ;check if message is for all nodes in this group
        xorwf   bRXBD2
        bz      $ + 4
    bra     ExitCANCheckMessage         ;it is not for this group

    ;check if system message begins from 0x10
        movlw   0x10
        xorwf   bRXBSIDH,W
        bz      $ + 4
    bra     ExitCANCheckMessage         ;exit
    ;check if message is 101h (ask group for reset)
        movlw   0x10
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     NodeReset
    ;check if message is 103h (ask group for Node ID)
        movlw   0x30                    
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANNodeID
    ;check if message is 105h (ask group for Firmware ID)
        movlw   0x50
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANFirmwareID
    ;check if message is 10Bh (ask group for Voltage)
        movlw   0xB0
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANVoltageAsk
    ;check if message is 10Dh (ask Group for Description)
        movlw   0xD0        
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANDescription
    ;check if message is 10Fh (ask Group for Procesor ID)
        movlw   0xF0
        xorwf   bRXBSIDL,W
        bnz     $ + 4
        bra     CANProcesorID
        bra     ExitCANCheckMessage     ;exit

;==============================================================================
;EnterBootloader - in UART routine

;==============================================================================
;NodeReset - in UART routine

;==============================================================================
CANNodeID                               ;this sends 8 bytes ID saved in 200000h
        clrf    TBLPTRU                 ;load addres to TBLPTR
        clrf    TBLPTRH        
        movlw   0x20
        movwf   TBLPTRL
        lfsr    FSR0,bTXBD0             ;start writing to register bTXBD0
        movlw   0x08                
CANNodeIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     CANNodeIDLoop
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        rcall   CANTransmitBootloader
    bra     ExitCANCheckMessage         ;exit
;==============================================================================
CANFirmwareID                           ;this sends 8 bytes    software ID saved in 001010h or error (wrong firmware)
        btfss   bFLAGS1,FirmError
        bra     CANFirmwareIDOK
CANFirmwareIDWrong
        movlw   0x1F                    ;set frame type (error 0x1F1)
        movwf   bTXBSIDH                    
        movlw   0x10
        movwf   bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        movff   bFLAGS1,bTXBD0          ;description of error
        movff   bChSum2,bTXBD1          ;move result of counted checksum
        movff   bChSum1,bTXBD2
        movff   bChSum0,bTXBD3
        setf    bTXBD4
        setf    bTXBD5
        movlw   BVER                    ;bootloader version
        movwf   bTXBD6
        movlw   BREV
        movwf   bTXBD7    
        rcall   CANTransmitBootloader
    bra     ExitCANCheckMessage         ;exit
CANFirmwareIDOK
        clrf    TBLPTRU                 ;load addres to TBLPTR
        movlw   0x10
        movwf   TBLPTRH        
        movlw   0x10
        movwf   TBLPTRL
        lfsr    FSR0,bTXBD0             ;start writing to register bTXBD0
        movlw   0x06                
CANFirmwareIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     CANFirmwareIDLoop
        movlw   BVER                    ;bootloader version
        movwf   bTXBD6
        movlw   BREV                    ;bootloader revision
        movwf   bTXBD7
CANFirmwareIDSend
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        rcall   CANTransmitBootloader
    bra     ExitCANCheckMessage         ;exit
;==============================================================================
CANChangeIdentifier                     ;rescue function to use if two or more nodes have the same Identifiers set by mistake
        clrf    TBLPTRU                 ;load addres to TBLPTR
        clrf    TBLPTRH
        movlw   0x26    
        movwf   TBLPTRL
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,bTXBEIDH         ;move flash data to register
        tblrd*                          ;read and move to TABLAT
        movff   TABLAT,bTXBEIDL         ;move flash data to register
        ;write to eeprom
        bcf     INTCON,7                ;disable interrupt
        bcf     EECON1,EEPGD            ;access EEPROM
        bcf     EECON1,CFGS             ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN             ;enable writes
        clrf    EEADRH
        movlw   0x26                    ;point at Identifier in eeprom
        movwf   EEADR
        movff   bTXBEIDH,EEDATA         ;move data 
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        incf    EEADR                   ;increment address of EEPROM    
        movff   bTXBEIDL,EEDATA         ;move data 
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        bcf     EECON1,WREN             ;disable writes    
        bsf     INTCON,7                ;enable interrupt
        ;send response
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        setf    bTXBD0                  ;unused
        setf    bTXBD1                  ;unused
        setf    bTXBD2                  ;unused
        setf    bTXBD3                  ;unused
        setf    bTXBD4                  ;unused
        setf    bTXBD5                  ;unused
        setf    bTXBD6                  ;unused
        setf    bTXBD7                  ;unused
        rcall   CANTransmitBootloader
    bra     ExitCANCheckMessage         ;exit
;==============================================================================
CANVoltageAsk
        movlb   0xF                     ;bank 16
        bcf     PMD1,ADCMD              ;enable A/D module
        bsf     ANCON0,ANSEL0           ;make AN0 analog pin
        movlb   BootRAM
        bsf     TRISA,0                 ;inputs on port A pin 0
        movlw   b'00010010'             ;result left justified, convertion time = Tosc*32 Tad=1us, required acquisition time at least 3,72us for Rs=10kom
        movwf   ADCON2
        movlw   b'00000000'             ;Vref-=Vss, Vref+=Vdd, analog "-" =Vss
        movwf   ADCON1
        ;AN0
        movlw   b'00000001'             ;AN0 chanel, power up converter
        movwf   ADCON0
        bsf     ADCON0,1                ;start conversion
        btfsc   ADCON0,1                ;wait till conversion finished
        bra     $ - 2
        movff   ADRESH,bTXBD0           ;move result of AD converter
        movff   ADRESL,bTXBD1
        ;VddCore
        movff   WREG,WREG               ;wait 2xTad
        movff   WREG,WREG
        movff   WREG,WREG
        movff   WREG,WREG
        movff   WREG,WREG
        movlw   b'01111001'             ;VDDCORE chanel, power up converter
        movwf   ADCON0
        bsf     ADCON0,1                ;start conversion
        btfsc   ADCON0,1                ;wait till conversion finished
        bra     $ - 2
        movff   ADRESH,bTXBD2           ;move result of AD converter
        movff   ADRESL,bTXBD3
        ;format frame
        movlb   BootRAM
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        setf    bTXBD4                  ;unused
        setf    bTXBD5                  ;unused
        setf    bTXBD6                  ;unused
        setf    bTXBD7                  ;unused
        rcall   CANTransmitBootloader
    bra     ExitCANCheckMessage         ;exit    
;==============================================================================
CANDescription
        lfsr    FSR0,bTXBD0             ;start writing to register bTXBD0
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM not Config Bits
        clrf    EEADRH
        movlw   0x30                    ;access description in eeprom
        movwf   EEADR                   ;move box address to EEADR
        movlw   0x08                
CANDescriptionLoop1
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;8 bytes read?
        bra     CANDescriptionLoop1
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        rcall   CANTransmitBootloader
        ;wait for TXB0 to empty
        btfsc   RXB0CON,3               ;because CANCON is set RXB0CON is in fact TXB0CON
        bra     $ - 2
        ;next frame of 8 bytes
        lfsr    FSR0,bTXBD0             ;start writing to register bTXBD0
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM not Config Bits
        movlw   0x38                    ;access description in eeprom
        movwf   EEADR                   ;move box address to EEADR
        movlw   0x08                
CANDescriptionLoop2
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;8 bytes read?
        bra     CANDescriptionLoop2
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        rcall   CANTransmitBootloader    
    bra     ExitCANCheckMessage         ;exit
;==============================================================================
CANProcesorID                           ;this sends 8 bytes ID saved in 3FFFFEh
        movlw   0x3F                    ;load addres to TBLPTR
        movwf   TBLPTRU
        movlw   0xFF        
        movwf   TBLPTRH        
        movlw   0xFE
        movwf   TBLPTRL
        lfsr    FSR0,bTXBD0             ;start writing to register TXBD0
        movlw   0x02                
CANProcesorIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     CANProcesorIDLoop
        movff   bRXBSIDH,bTXBSIDH       ;set frame type
        movff   bRXBSIDL,bTXBSIDL
        bsf     bTXBSIDL,0              ;response bit
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   bTXBDLC
        setf    bTXBD2
        setf    bTXBD3
        setf    bTXBD4
        setf    bTXBD5
        setf    bTXBD6
        setf    bTXBD7
        rcall   CANTransmitBootloader
    bra     ExitCANCheckMessage         ;exit

;==========================================================================
; Routine:      BOOTLOADER TRANSMIT ROUTINE
;==========================================================================
; PreCondition: Data saved in transmit buffer TXBD0 - TXBD7
;==========================================================================
CANTransmitBootloader:
    ;prepare bits in TXBSIDL
        btfsc   bTXBSIDL,4              ;what is the fourth bit?
        bra     $ + 6                   ;it is 1, so set it in first position
        bcf     bTXBSIDL,1              ;it is 0, so clear it in first position
        bra     $ + 4
        bsf     bTXBSIDL,1
        bsf     bTXBSIDL,3              ;set EXIDE bit
    
    ;transmit using buffer 0
        movlw   b'00001000'             ;access TXB0 buffer
        movwf   CANCON
        bcf     RXB0CON,3               ;make TXB0 free if was busy
        lfsr    FSR0,0xF61              ;point at first register of transmit buffer 0 (TXBSIDH)
        rcall   bWriteBuffer    
        bsf     RXB0CON,3               ;request trasmition & clear TXABT, TXLARB, TXERR
        return                          ;now wait for TXB0IF (transmit successful)

bWriteBuffer:
        lfsr    FSR1,0x131              ;point at first register of source buffer bTXBSIDH
        movlw   0x0D                    ;read 13 successive registers
bWriteBufferLoop:
        movff   POSTINC1,POSTINC0       ;move data from transmit buffer to destination buffer
        decfsz  WREG                    ;all 13 bytes read?
        bra     bWriteBufferLoop        ;not yet - read more
    return

;==============================================================================
; Routine:      UART BOOTLOADER HIGH INTERRUPT
;==============================================================================
; Overview:
;    The bootloader responses to these messages:
;    1. ENTER PROGRAMMING MODE
;    <0xAA><0x10><0x00><FCHK><0xA5>
;    2. RESET
;    <0xAA><0x10><0x20><FCHK><0xA5>
;    3. ASK NODE FOR S/N
;    <0xAA><0x10><0x40><FCHK><0xA5>
;    4. ASK NODE FOR FIRMWARE VERSION
;    <0xAA><0x10><0x60><FCHK><0xA5>
;    5. ASK NODE FOR VOLTAGE
;    <0xAA><0x10><0xC0><FCHK><0xA5>
;    6. ASK NODE FOR DESCRIPTION
;    <0xAA><0x10><0xE0><FCHK><0xA5>
;    6. ASK NODE FOR PROCESOR ID
;    <0xAA><0x11><0x00><FCHK><0xA5>
;            <0xAA> - frame start
;            <FCHK> - frame checksum
;            <0xA5> - frame stop
;==============================================================================
HighInt
    ;check if UART enabled
        movff   BSR,bBSR_H              ;save BSR register
        movlb   BootRAM                 ;choose bootloader bank
        tstfsz  bUART                   ;uart enabled?
        bra     $ + 4                   ;yes
    bra     Goto_0x1030                 ;no - go to main program high interrupt if firmware ok
    ;save shadow registers (used in bootloader)
        movff   STATUS,bSTATUS_H        ;save STATUS register
        movff   WREG,bWREG_H            ;save working register
        movff   PIR1,bPIR1_H            ;NOT CHANGED IN HIGH INTERRUPT
        movff   PIR4,bPIR4_H            ;NOT CHANGED IN HIGH INTERRUPT
        movff   PIR5,bPIR5_H            ;NOT CHANGED DURING LOW INTERRUPT
        movff   CANSTAT,bCANSTAT_H      ;NOT USED IN HIGH INTERRUPT
        movff   CANCON,bCANCON_H        ;NOT USED IN HIGH INTERRUPT
        movff   FSR0L,bFSR0L_H            
        movff   FSR0H,bFSR0H_H
        movff   FSR1L,bFSR1L_H
        movff   FSR1H,bFSR1H_H
        movff   TABLAT,bTABLAT_H
        movff   TBLPTRL,bTBLPTRL_H
        movff   TBLPTRH,bTBLPTRH_H
        movff   TBLPTRU,bTBLPTRU_H
        movff   INTCON,bINTCON_H        ;NOT CHANGED IN HIGH INTERRUPT
        movff   EECON1,bEECON1_H
        movff   EEDATA,bEEDATA_H
        movff   EEADRH,bEEADRH_H
        movff   EEADR,bEEADR_H
        movff   TRISA,bTRISA_H
        movff   ADCON2,bADCON2_H
        movff   ADCON1,bADCON1_H
        movff   ADCON0,bADCON0_H
        movff   ANCON0,bANCON0_H
        movff   ADRESL,bADRESL_H
        movff   ADRESH,bADRESH_H
        movff   RCSTA1,bRCSTA1_H
        movff   TXSTA1,bTXSTA1_H
        movff   PMD1,bPMD1_H
        movff   COMSTAT,bCOMSTAT_H      ;NOT USED IN HIGH INTERRUPT
    ;call bootloader procedure
        rcall   ActiveUART
    ;restore shadow registers (used in bootloader)
        movff   bSTATUS_H,STATUS        ;restore STATUS register
        movff   bWREG_H,WREG            ;restore working register
        movff   bFSR0L_H,FSR0L            
        movff   bFSR0H_H,FSR0H
        movff   bFSR1L_H,FSR1L
        movff   bFSR1H_H,FSR1H
        movff   bTABLAT_H,TABLAT
        movff   bTBLPTRL_H,TBLPTRL
        movff   bTBLPTRH_H,TBLPTRH
        movff   bTBLPTRU_H,TBLPTRU
        movff   bEECON1_H,EECON1
        movff   bEEDATA_H,EEDATA
        movff   bEEADRH_H,EEADRH
        movff   bEEADR_H,EEADR
        movff   bTRISA_H,TRISA
        movff   bADCON2_H,ADCON2
        movff   bADCON1_H,ADCON1
        movff   bADCON0_H,ADCON0
        movff   bANCON0_H,ANCON0
        movff   bADRESL_H,ADRESL
        movff   bADRESH_H,ADRESH
        movff   bRCSTA1_H,RCSTA1
        movff   bTXSTA1_H,TXSTA1
        movff   bPMD1_H,PMD1
    ;check if firmware ok
Goto_0x1030
        movlb   BootRAM
        btfsc   bFLAGS1,FirmError       ;skip if firmware ok
        bra     ExitHighInt             ;goto retfie
        movff   bBSR_H,BSR              ;restore BSR register
        goto    0x1030                  ;go to MAIN SOFTWARE (high interrupt)
ExitHighInt
        movff   bBSR_H,BSR              ;restore BSR register
    retfie

;==============================================================================
; Routine:      UART RECEIVE MESSAGE
;==============================================================================
; Overview:     It receives bytes from UART            
;==============================================================================
ActiveUART
        btfss   PIR1,RC1IF              ;check if uart received anything
    bra     ExitActiveUART              ;not UART interrupt
        movlb   BootRAM
        clrf    bRBOVF                  ;clear receive buffer overflow flag
        clrf    bRBCNT                  ;clear receive buffer counter
        rcall   ResetIdleTimer          ;reset time between received bytes
        lfsr    FSR1,bRB0               ;point at first register of RXB buffer
ActiveUARTLp
    ;overrun error
        btfsc   RCSTA1,OERR             ;check if overrun error occured
        bra     OverrunError            ;restart receiver and exit
    ;check idle timer
        rcall   DecIdleTimer            ;measures time between received bytes, schould be about 90us (time of receiving another byte)
        tstfsz  WREG                    ;skip if timer not overflow (if WREG=0)
        bra     EndOfActiveUARTLp
    ;check if received byte
        btfss   PIR1,RC1IF              ;check if uart received anything
        bra     ActiveUARTLp            ;wait for message
    ;received
        rcall   IncRecCounter           ;increment receive buffer counter
        rcall   ResetIdleTimer          ;reset time between received bytes
        movff   RCREG1,POSTINC1         ;move data from uart receive buffer to can transmit buffer
        bra     ActiveUARTLp            ;read more
EndOfActiveUARTLp
        rcall   UARTCheckMessage        ;check if it is system message
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
        incf    bRBCNT                  ;then set overflow flag and start writting to begining of buffer
        movlw   .30
        cpfsgt  bRBCNT                  ;skip if CNT>30
    return
    ;buffer overflow
        movlw   .1
        movwf   bRBCNT                  ;clear receive buffer counter
        incf    bRBOVF                  ;set receive buffer overflow flag  
        lfsr    FSR1,bRB0               ;point again at first register of RXB buffer
    return

;==============================================================================
; Routine:      UART RECEIVED MESSAGE RECOGNITION
;==============================================================================
; Overview:     It checks if system message received
;==============================================================================
UARTCheckMessage
    ;check if system message received
        movlb   BootRAM
        movlw   0x05                    ;check if 5bytes received
        xorwf   bRBCNT,W
        bz      CheckingMessage         ;yes
UARTExitCheckMessage
    return

CheckingMessage
        movlw   0xAA                    ;is start byte ok?
        xorwf   bRB0,W
        bz      $ + 4                
    bra UARTExitCheckMessage            ;no

        movlw   0xA5                    ;is stop byte ok?
        xorwf   bRB4,W
        bz      $ + 4    
    bra UARTExitCheckMessage            ;no

        clrf    bR2                     ;checksum
        movf    bRB1,W
        addwf   bR2
        movf    bRB2,W
        addwf   bR2
        movf    bR2,W                   ;is checksum byte ok?
        xorwf   bRB3,W
        bz      $ + 4    
    bra UARTExitCheckMessage            ;no

        movlw   0x10
        xorwf   bRB1,W
        bz      UARTMessages0x10X
        movlw   0x11
        xorwf   bRB1,W
        bz      UARTMessages0x11X
    bra UARTExitCheckMessage            ;other

UARTMessages0x10X
    ;check if message is 100h (enter bootloader mode request)
        movlw   0x0
        xorwf   bRB2,W
        bnz     $ + 4    
        bra     EnterBootloader
    ;check if message is 102h (ask node for reset)
        movlw   0x20
        xorwf   bRB2,W
        bnz     $ + 4
        bra     NodeReset
    ;check if message is 104h (ask for Node ID)
        movlw   0x40
        xorwf   bRB2,W
        bnz     $ + 4
        bra     UARTNodeID
    ;check if message is 106h (ask for Node Firmware ID)
        movlw   0x60
        xorwf   bRB2,W
        bnz     $ + 4
        bra     UARTFirmwareID
    ;check if message is 10Ch (ask Node for Voltage)
        movlw   0xC0
        xorwf   bRB2,W
        bnz     $ + 4
        bra     UARTVoltageAsk
    ;check if mesage is 10Eh (ask Node for Description)
        movlw   0xE0
        xorwf   bRB2,W
        bnz     $ + 4
        bra     UARTDescription
    bra UARTExitCheckMessage            ;other

UARTMessages0x11X
    ;check if message is 110h (ask Node for Procesor ID)
        movlw   0x10
        xorwf   bRB2,W
        bnz     $ + 4
        bra     UARTProcesorID
    bra UARTExitCheckMessage            ;other

;==============================================================================
EnterBootloader                         ;ROUTINE USED BY CAN INTERRUPT TOO
        clrf    EEADRH                  ;EEPROM address
        clrf    EEADR                   ;point at Bootloader mode flag
        setf    EEDATA                  ;set flag
        bcf     INTCON,GIE              ;disable interrupt
        bcf     EECON1,EEPGD            ;access EEPROM
        bcf     EECON1,CFGS             ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN             ;enable writes
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        bcf     EECON1,WREN             ;disable writes
;        bsf     INTCON,GIE              ;enable interrupt not needed now because of reset
        bcf     TXSTA1,TXEN             ;disable uart transmission
    reset
;==============================================================================
NodeReset                               ;ROUTINE USED BY CAN INTERRUPT TOO
        bcf     TXSTA1,TXEN             ;disable uart transmission
        reset
;==============================================================================
UARTNodeID                              ;this sends 8 bytes ID saved in 000020h
        clrf    TBLPTRU                 ;load addres to TBLPTR
        clrf    TBLPTRH    
        movlw   0x20    
        movwf   TBLPTRL
        lfsr    FSR0,bTB3               ;start writing to register bTXBD0
        movlw   0x08                
NodeIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     NodeIDLoop
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0
        rcall   UARTTransmitBootloader
    bra UARTExitCheckMessage
;==============================================================================
UARTFirmwareID                          ;this sends 8 bytes    software ID saved in 001010h or error (wrong firmware)
        btfss   bFLAGS1,FirmError
        bra     FirmwareIDOK
FirmwareIDWrong
        movlw   0x1F                    ;set frame type (error 0x1F1)
        movwf   bTB1                    
        movlw   0x10
        movwf   bTB2
        bsf     bTB2,0                  ;response bit
        movff   bFLAGS1,bTB3            ;description of error
        movff   bChSum2,bTB4            ;move result of counted checksum
        movff   bChSum1,bTB5
        movff   bChSum0,bTB6
        setf    bTB7
        setf    bTB8
        movlw   BVER                    ;bootloader version
        movwf   bTB9
        movlw   BREV                    ;bootloader revision
        movwf   bTB10        
        rcall   UARTTransmitBootloader
    bra UARTExitCheckMessage
FirmwareIDOK
        clrf    TBLPTRU                 ;load addres to TBLPTR
        movlw   0x10
        movwf   TBLPTRH        
        movlw   0x10
        movwf   TBLPTRL
        lfsr    FSR0,bTB3               ;start writing to register bTB3
        movlw   0x06                
FirmwareIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;6 bytes read?
        bra     FirmwareIDLoop
        movlw   BVER                    ;bootloader version
        movwf   bTB9
        movlw   BREV                    ;bootloader revision
        movwf   bTB10
FirmwareIDSend
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0
        rcall   UARTTransmitBootloader
    bra UARTExitCheckMessage
;==============================================================================
UARTVoltageAsk
        movlb   0xF                     ;bank 16
        bcf     PMD1,ADCMD              ;enable A/D module
        bsf     TRISA,0                 ;inputs on port A pin 0
        bsf     ANCON0,ANSEL0           ;make AN0 analog pin
        movlb   BootRAM
        movlw   b'00010010'             ;result left justified, convertion time = Tosc*32 Tad=1us, required acquisition time at least 3,72us for Rs=10kom
        movwf   ADCON2
        movlw   b'00000000'             ;Vref-=Vss, Vref+=Vdd, analog "-" =Vss
        movwf   ADCON1
        ;AN0
        movlw   b'00000001'             ;AN0 chanel, power up converter
        movwf   ADCON0
        bsf     ADCON0,1                ;start conversion
        btfsc   ADCON0,1                ;wait till conversion finished
        bra     $ - 2
        movff   ADRESH,bTB3             ;move result of AD converter
        movff   ADRESL,bTB4
        ;VddCore
        movff   WREG,WREG               ;wait 2xTad
        movff   WREG,WREG
        movff   WREG,WREG
        movff   WREG,WREG
        movff   WREG,WREG
        movlw   b'01111001'             ;VDDCORE chanel, power up converter
        movwf   ADCON0
        bsf     ADCON0,1                ;start conversion
        btfsc   ADCON0,1                ;wait till conversion finished
        bra     $ - 2
        movff   ADRESH,bTB5             ;move result of AD converter
        movff   ADRESL,bTB6
        ;format frame
        setf    bTB7                    ;unused
        setf    bTB8                    ;unused
        setf    bTB9                    ;unused
        setf    bTB10                   ;unused
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0
        rcall   UARTTransmitBootloader
    bra UARTExitCheckMessage
;==============================================================================
UARTDescription
        lfsr    FSR0,bTB3               ;start writing to register bTB3
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM not Config Bits
        clrf    EEADRH                  ;EEPROM address
        movlw   0x30                    ;access description in eeprom
        movwf   EEADR                   ;move box address to EEADR
        movlw   0x08                
DescriptionLoop1
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;8 bytes read?
        bra     DescriptionLoop1
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0
        rcall   UARTTransmitBootloader
        ;next frame of 8 bytes
        btfss   PIR1,TX1IF              ;check if TXREG if empty and ready to be loaded
        bra     $ - 2    
        lfsr    FSR0,bTB3               ;start writing to register bTB3
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM not Config Bits
        movlw   0x38                    ;access description in eeprom
        movwf   EEADR                   ;move box address to EEADR
        movlw   0x08                
DescriptionLoop2
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;8 bytes read?
        bra     DescriptionLoop2
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0
        rcall   UARTTransmitBootloader    
    bra UARTExitCheckMessage
;==============================================================================
UARTProcesorID                          ;this sends 8 bytes ID saved in 3FFFFEh
        movlw   0x3F                    ;load addres to TBLPTR
        movwf   TBLPTRU
        movlw   0xFF        
        movwf   TBLPTRH        
        movlw   0xFE
        movwf   TBLPTRL
        lfsr    FSR0,bTB3               ;start writing to register bTB3
        movlw   0x02                
ProcesorIDLoop
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     ProcesorIDLoop
        setf    bTB5
        setf    bTB6
        setf    bTB7
        setf    bTB8
        setf    bTB9
        setf    bTB10
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0
        rcall   UARTTransmitBootloader
    bra UARTExitCheckMessage

;==============================================================================
; Routine:            UART Transmit Routine
;==============================================================================
; Overview:         It sends response to system messages
;==============================================================================
UARTTransmitBootloader
        movlb   BootRAM
        movlw   0xAA                    ;start byte
        movwf   bTB0    
        movlw   0xA5                    ;stop byte
        movwf   bTB12

        clrf    bTB11                   ;checksum
        lfsr    FSR1,bTB1               ;point at first register of TXB buffer
        movlw   0x0A                    ;read 10 successive registers
        movwf   bR3
ChecksumLp1
        movff   POSTINC1,WREG           ;move data from RXB buffer to uart transmit buffer
        addwf   bTB11
        decfsz  bR3                     ;all 10 bytes read?
        bra     ChecksumLp1             ;not yet - read more

        bsf     TXSTA1,TXEN             ;enable uart transmission

        lfsr    FSR1,bTB0               ;point at first register of TXB buffer
        movlw   0x0D                    ;read 13 successive registers
        movwf   bR3
UARTTransmitLp1
        btfss   PIR1,TX1IF              ;check if TXREG if empty and ready to be loaded
        bra     $ - 2                
        movff   POSTINC1,TXREG1         ;move data from RXB buffer to uart transmit buffer
        decfsz  bR3                     ;all 13 bytes read?
        bra     UARTTransmitLp1         ;not yet - read more

    ;wait to finish transmitting last byte
        btfss   TXSTA1,TRMT             ;check if TSR Transmit Shift Register i empty 
        bra     $ - 2    
        ;bcf     TXSTA1,TXEN             ;disable uart transmission  (must be commented otherwise does not sent second msg)
    return
    
;==============================================================================
;==============================================================================
; END OF BOOTLOADER NORMAL MODE
;==============================================================================
;==============================================================================





;==============================================================================
;==============================================================================
; Routine:      PROGRAMMING FUNCTIONS
;==============================================================================
;==============================================================================
ProgrammingMode
        rcall   ProgrammingIndication   ;indicate programming mode
        rcall   CANProgrammingMode      ;initialize CAN programming mode
        rcall   UARTCheckPin            ;check if UART needed

MainBootLoop
        clrwdt
        ;check if CAN received
        btfsc   RXB0CON,RXFUL           ;check if message received
        bra     CANReceived    
        ;check if UART enabled
        movlb   BootRAM                 ;choose bootloader bank
        tstfsz  bUART                   ;uart enabled?
        bra     $ + 4                   ;yes
    bra     MainBootLoop                ;no
        ;check if UART received
        btfsc   PIR1,RC1IF              ;check if uart received anything
        bra     UARTReceived
    bra     MainBootLoop

;==============================================================================
; Routine:      CHECK IF UART IS REQUIRED
;==============================================================================
UARTCheckPin
        movlb   BootRAM                 ;choose bootloader bank
        clrf    bUART                   ;clear UART enabled flag
        movlb   0xF                     ;bank 16
        bcf     ANCON0,ANSEL1           ;make AN1 digital pin
        bsf     TRISA,1                 ;port as input
        btfsc   PORTA,1                 ;skip if port connected to GND
        rcall   UARTEnabled             ;set enabled PLL flag
        movlb   0xF                     ;bank 16
        bsf     ANCON0,ANSEL1           ;make AN1 analog pin
    return
;=======================
UARTEnabled
        bsf     OSCTUNE,PLLEN           ;enable PLL
        movlb   BootRAM                 ;choose bootloader bank
        setf    bUART                   ;set UART enabled flag
        rcall   UARTTibboReset          ;reset for Tibbo   
        rcall   UARTProgrammingMode     ;initialize UART programming mode   
    return
;=======================
UARTTibboReset:
        btfss   RCON,RI                 ;skip if reset not after RESET instruction
    return                              ;RESET detected (RI=0)
        bcf     LATC,1                  ;reset active low
        bcf     TRISC,1                 ;start reset, output for RST
        rcall   Delay25                 ;wait 50ms
        rcall   Delay25
        bsf     TRISC,1                 ;stop reset, input for RST (internal Tibbo pull up)
    return
;=======================
Delay25
        movlw   0x00
        movwf   bR4    
T25L2:  movlw   0x00
        movwf   bR5
T25L1:  decfsz  bR5
        bra     T25L1
        decfsz  bR4
        bra     T25L2
    return

;==============================================================================
; Routine:      PROGRAMMING INDICATION
;==============================================================================
; Overview:     Drives RB6 pin to indicate status.
;               1 quick blink, same for 8MHz and 32MHz clock
;==============================================================================
ProgrammingIndication
        movlb   BootRAM
        bcf     TRISB,6                 ;RB6 as output
        bsf     LATB,6                  ;start blink
        rcall   Delay25     
        rcall   Delay25       
        bcf     LATB,6
        bsf     TRISB,6                 ;RB6 as intput
    return

;==============================================================================
; Routine:      CAN BOOTLOADER PROGRAMMING FUNCTIONS
;               (WRITING & READING EEPROM AND FLASH)
;==============================================================================
; Overview:
;    The bootloader can program FLASH and EEPROM    memory.
;    There are four commands which can be sent to bootloader in programming mode:
;    1. EXIT ALL - exit programming mode;
;    <0x01><0x00><0x00><0x00>< XX >< XX >< XX >< XX >< XX >< XX >< XX >< XX >
;    2. EXIT - exit programming mode;
;    <0x02><0x00><NODE><GROUP>< XX >< XX >< XX >< XX >< XX >< XX >< XX >< XX >
;    3. CONTROL FRAME is being sent
;    <0x03><0x00><NODE><GROUP><ADRU><ADRH><ADRL>< XX >< XX ><COMM>< XX >< XX >
;       <COMM> - command register:
;            01 - read data
;            02 - write data
;            03 - erase block (only for FLASH memory)
;    4. DATA FRAME is being sent
;    <0x04><0x00><NODE><GROUP><DAT0><DAT1><DAT2><DAT3><DAT4><DAT5><DAT6><DAT7>
;==============================================================================

;==============================================================================
; Routine:      CAN MOVE 8 BYTES OF DATA FUNCTION
;==============================================================================
CANMoveData
        movlw   0x08                    ;read 8 successive registers
CANMoveDataLoop:
        movff   POSTINC0,POSTINC1       ;move data from receive buffer to destination buffer
        decfsz  WREG                    ;all 8 bytes read?
        bra     CANMoveDataLoop
    return

;==============================================================================
; Routine:            CAN TRANSMIT ROUTINE
;==============================================================================
CANSendError
        movlb   0xF                     ;choose bank 15
        movlw   0x0F                    ;set frame "received message misunderstood by bootloader"
        movwf   TXB0SIDH
        setf    TXB0D0                  ;reserved
        setf    TXB0D1                  ;reserved
        movlw   BVER                    ;bootloader software version
        movwf   TXB0D2
        movlw   BREV                    ;revision number of software
        movwf   TXB0D3
        setf    TXB0D4                  ;unused
        setf    TXB0D5                  ;unused
        setf    TXB0D6                  ;unused
        setf    TXB0D7                  ;unused
        bsf     TXB0CON,TXREQ           ;request trasmition
        return                          ;now wait for TXB0IF (transmit successful)
CANSendACK:
        movlb   0xF                     ;choose bank 15
        setf    TXB0D0                  ;reserved
        setf    TXB0D1                  ;reserved
        movlw   BVER                    ;bootloader software version
        movwf   TXB0D2
        movlw   BREV                    ;revision number of software
        movwf   TXB0D3
        setf    TXB0D4                  ;unused
        setf    TXB0D5                  ;unused
        setf    TXB0D6                  ;unused
        setf    TXB0D7                  ;unused
CANSend:
        movlb   0xF                     ;choose bank 15
        movff   RXB0SIDH,TXB0SIDH       ;send the same frame type
        bsf     TXB0CON,TXREQ           ;request trasmition
        return                          ;now wait for TXB0IF (transmit successful)

;==============================================================================
; Routine:      CAN CODES ROUTINE
;==============================================================================
CAN_code_exit_bootloader_mode
        clrf    EEADRH                  ;point at Bootloader mode flag
        clrf    EEADR
        clrf    EEDATA                  ;clear flag
        bcf     EECON1,EEPGD            ;access EEPROM
        bcf     EECON1,CFGS             ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN             ;enable writes
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
    reset
;==============================================================================
CAN_code_control_frame
        lfsr    FSR0,0xF66              ;point at first register of source buffer - RXB0D0
        lfsr    FSR1,0x106              ;point at first register of destination buffer - bRXBD0
        rcall   CANMoveData             ;move data from received buffer to temp buffer
        lfsr    FSR0,0x106              ;point at first register of source buffer - bRXBD0
        lfsr    FSR1,0xF26              ;point at first register of destination buffer - TXB0D0
        rcall   CANMoveData             ;move data from temp buffer to transmit buffer
        rcall   CANSend                 ;send acknowledge frame (same as received)
    bra     CANBackToLoop               ;go back to main loop
;==============================================================================
CAN_code_data_frame
        movlb   BootRAM                 ;choose boot bank
        movlw   0xF0                    ;check if address begins from F0....h - EEPROM
        xorwf   bRXBD0,W                
        bz      CANProgramEEPROM
        bra     CANProgramFLASH        

;==============================================================================
; Routine:            CAN BOOTLOADER CONFIGURATION ROUTINE
;==============================================================================
; Overview:            It sets configuration of bit timing, identifier,
;                    masks and filters. Mask are set to match each bit of
;                    received messege against filters. Each filter has
;                    the same value equal the value of Identifier. This 
;                    allows receiving only one type of message with
;                    Identifier the same as Identifier of node.
;==============================================================================
CANProgrammingMode
        bsf     CANCON,7                ;request configuration mode
        btfss   CANSTAT,7               ;check if configuration mode is on
        bra     $ - 2

        clrf    ECANCON                 ;Mode 0
        movlw   b'00100001'             ;CANTX2 is digital pin, CANTX pin will drive VDD, oscillator as the source for CAN
        movwf   CIOCON    
    ;setting can baud rate (125kbps, Tbit=16TQ)
        movlb   0xE                     ;choose bank 14
        movlw   b'00000001'             ;set SJW=1xTQ & BRP=0 (TQ=4xTosc)
        movwf   BRGCON1
        movlw   b'10111100'             ;set SAM=1 SEG1PH=8xTQ PRSEG=5xTQ
        movwf   BRGCON2
        movlw   b'00000001'             ;set WAKFIL=0 & SEG2PH=2xTQ
        movwf   BRGCON3
    ;setting Masks
        clrf    RXM0SIDH                ;set Mask0 -check all bits in filters without 1 first byte
        setf    RXM0SIDL
        setf    RXM0EIDH
        setf    RXM0EIDL
    ;read identifier from EEPROM
        movlb   0xF                     ;choose bank 15
        clrf    EEADRH
        movlw   0x26                    ;EEPROM address
        movwf   EEADR
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1,CFGS             ;access data FLASH or EEPROM not configuration bits
BL1     bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,TXB0EIDH         ;move EEPROM DATA to register
        incf    EEADR                   ;increment address of EEPROM
        movlw   b'00001001'             ;set bits: EXTENDED IDENTIFIER, RESPONSE
        movwf   TXB0SIDL
        movlw   0x10                    ;set frame type as 0x100 - after Enter Bootloader Mode request
        movwf   RXB0SIDH
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,TXB0EIDL         ;move EEPROM DATA to register
    ;setting filters F0 and F1
        movlb   0xE                     ;choose bank 14
        movlw   b'00001000'             ;filter F0, set EXIDEN bit, receive messege with the same identifier
        movwf   RXF0SIDL            
        movff   TXB0EIDH,RXF0EIDH    
        movff   TXB0EIDL,RXF0EIDL
        movwf   RXF1SIDL                ;filter F1, set EXIDEN bit, receive messege with XXXX0000h identifier
        clrf    RXF1EIDH
        clrf    RXF1EIDL
    ;setting configuration of receive buffers
        movlb   0xF                     ;choose bank 15
        movlw   b'01000000'             ;receive only valid extended messages
        movwf   RXB0CON                
        bsf     RXB1CON,7               ;make buffer busy, because it won't be used
    ;setting configuraion of transmit buffers
        movlw   b'00001000'             ;length code = 8 bytes
        movwf   TXB0DLC
        clrf    TXB0CON                 ;clear buffer
    ;setting can interrupt
        bcf     INTCON,GIE              ;no interrupts
    ;end of can configuration
        bsf     TRISB,3                 ;set can input
        bcf     TRISB,2                 ;set can output
        clrf    CANCON                  ;request normal mode
        btfsc   CANSTAT,7               ;check if configuration mode is off
        bra     $ - 2                   ;note: masks & filters read as 00h, but in fact they are set

        rcall   CANSendACK              ;send acknowledge message
    return

;==============================================================================
; Routine:      CAN BOOTLOADER MAIN PROGRAM
;==============================================================================
CANBootLoop
        bra     MainBootLoop            ;goto main loop

CANBackWithError                        ;wrong command or address
        rcall   CANSendError
CANBackToLoop
        bcf     RXB0CON,RXFUL           ;allow receive next message
        bra     CANBootLoop

CANReceived
        movlw   0x01                    ;is this exit bootloader request for group?
        xorwf   RXB0SIDH,W
        bz      CAN_code_exit_bootloader_mode

        btfsc   RXB0CON,0               ;check if it is filter 0, skip if it is
    bra     CANBackToLoop               ;go back to main loop if other command to group

        movlw   0x02                    ;is this exit bootloader request?
        xorwf   RXB0SIDH,W
        bz      CAN_code_exit_bootloader_mode
        movlw   0x03                    ;is this control frame sent code?
        xorwf   RXB0SIDH,W
        bz      CAN_code_control_frame
        movlw   0x04                    ;is this data frame sent code?
        xorwf   RXB0SIDH,W
        bz      CAN_code_data_frame
    bra     CANBackWithError            ;go back to main loop if other command


;==============================================================================
; Routine:      CAN PROGRAM EEPROM ROUTINE
;==============================================================================
CANProgramEEPROM
        movlb   BootRAM                 ;choose boot bank
        movlw   0x04
        cpfslt  bRXBD1                  ;skip if address less then 0xF00400
    bra     CANBackWithError            ;go back to main loop if bootloader firm memory

        movlw   0x01                    ;check Command Byte, read data?
        xorwf   bRXBD5,W                
        bz      CANReadEEPROM
        movlw   0x02                    ;check Command Byte, write data?
        xorwf   bRXBD5,W                
        bz      CANWriteEEPROM
    bra     CANBackWithError            ;go back to main loop if other command

CANWriteEEPROM
        lfsr    FSR0,0xF66              ;start writing from register RXB0D0
        movff   bRXBD1,EEADRH           ;move high address to EEADRH
        movff   bRXBD2,EEADR            ;move low address to EEADR
        bcf     EECON1,EEPGD            ;access EEPROM
        bcf     EECON1,CFGS             ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN             ;enable writes
        movlw   0x08
        movwf   bR10
CANWritemoreEEPROM
        movff   POSTINC0,EEDATA         ;move data indicated by FSR0 to EEDATA and increment FSR0
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        incf    EEADR                   ;increment address of EEPROM
        decfsz  bR10
        bra     CANWritemoreEEPROM
        bcf     EECON1,WREN             ;disable writes    

CANReadEEPROM
        lfsr    FSR0,0xF26              ;start writing to register TXB0D0
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM not Config Bits
        movff   bRXBD1,EEADRH           ;move high address to EEADRH
        movff   bRXBD2,EEADR            ;move low address to EEADR
        movlw   0x08                
CANReadmoreEEPROM
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;8 bytes read?
        bra     CANReadmoreEEPROM
        rcall   CANSend

CANEndProgramEEPROM
    bra     CANBackToLoop               ;go back to main loop

;==============================================================================
; Routine:      CAN PROGRAM FLASH ROUTINE
;==============================================================================
CANProgramFLASH
        movlb   BootRAM                 ;choose boot bank
        movff   bRXBD0,TBLPTRU          ;load addres to TBLPTRU
        movff   bRXBD1,TBLPTRH        
        movff   bRXBD2,TBLPTRL
    ;do not read bootloader firmware
        tstfsz  bRXBD0                  ;skip if address begins from 0x00
        bra     $ + .10
        movlw   0x10
        cpfslt  bRXBD1                  ;skip if address less then 0x001000
        bra     $ + 4
    bra     CANBackWithError            ;go back to main loop if bootloader firm memory

        movlw   0x01                    ;check Command Byte, read data?
        xorwf   bRXBD5,W                
        bz      CANReadFLASH
        movlw   0x02                    ;check Command Byte, write data?
        xorwf   bRXBD5,W                
        bz      CANWriteFLASH
        movlw   0x03                    ;check Command Byte, erase data?
        xorwf   bRXBD5,W                
        bz      CANEraseFLASH
    bra     CANBackWithError            ;go back to main loop if other command

CANEraseFLASH
        movf    TBLPTRL,W               ;check for a valid 64 bytes border
        andlw   b'00111111'
    bnz     CANBackWithError
        bsf     EECON1,EEPGD            ;access data FLASH memory
        bcf     EECON1,CFGS             ;access data FLASH or EEPROM not Config Bits
        bsf     EECON1,WREN             ;enable writes
        bsf     EECON1,FREE             ;enable FLASH row erase operation
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;start erase
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        movff   bRXBD2,TBLPTRL          ;point again at first register in block
        bra     CANReadFLASH            ;read first 8 bytes and send them

CANWriteFLASH
        movf    TBLPTRL,W               ;check for a valid 8 bytes border
        andlw   b'00000111'
    bnz     CANBackWithError
        lfsr    FSR0,0xF66              ;start writing from register RXB0D0
        movlw   0x08                
CANWritemoreFLASH
        movff   POSTINC0,TABLAT         ;move data from register indicated by FSR0 to TABLAT and increment FSR0
        tblwt*+                         ;perform short write to holding register and increment TBLPTR
        decfsz  WREG                    ;8 bytes read?
        bra     CANWritemoreFLASH
        tblrd*-                         ;point back into the block
        bsf     EECON1,EEPGD            ;access data FLASH memory
        bcf     EECON1,CFGS             ;access data FLASH or EEPROM not Config Bits
        bsf     EECON1,WREN             ;enable writes
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;start write
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        bcf     EECON1,WREN             ;disable writes
        movff   bRXBD2,TBLPTRL          ;point again at first register in the block

CANReadFLASH
        lfsr    FSR0,0xF26              ;start writing to register TXB0D0
        movlw   0x08                
CANReadmoreFLASH
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     CANReadmoreFLASH
        rcall   CANSend
        
    bra     CANBackToLoop               ;go back to main loop

;==============================================================================
;==============================================================================
; END OF CAN BOOTLOADER PROGRAMMING FUNCTIONS
;==============================================================================
;==============================================================================






;==============================================================================
; Routine:      UART BOOTLOADER PROGRAMMING FUNCTIONS
;               (WRITING & READING EEPROM AND FLASH)
;==============================================================================
; Overview:
;    The bootloader can program FLASH and EEPROM    memory.
;    There are three commands which can be sent to bootloader in programming mode:
;    1. EXIT - exit programming mode;
;        <0xAA><0x02><0x00>< XX >< XX >< XX >< XX >< XX >< XX >< XX >< XX ><CHKM><0xA5>
;             <0xAA> - start byte
;             <CHKM> - checksum byte
;             <0xA5> - stop byte
;    2. CONTROL FRAME is being sent
;        <0xAA><0x03><0x00><ADRU><ADRH><ADRL>< XX >< XX ><COMM>< XX >< XX ><CHSM><0xA5>
;             <COMM> - command register:
;                01 - read data
;                02 - write data
;                03 - erase block (only for FLASH memory)
;    3. DATA FRAME is being sent
;        <0xAA><0x04><0x00><DAT0><DAT1><DAT2><DAT3><DAT4><DAT5><DAT6><DAT7><CHSM><0xA5>
;==============================================================================

;==========================================================================
; Routine:      UART BOOTLOADER CONFIGURATION ROUTINE
;==========================================================================
; Overview:     It sets uart configuration
;==========================================================================
UARTProgrammingMode
    ;enable PLL - 32MHz clock
        bsf     OSCTUNE,PLLEN           ;enable PLL
    ;initiate uart
        clrf    SPBRGH1
        movlw   .68                     ;32MHz/(4(68+1))=115942 bps; 0,6%
        movwf   SPBRG1
        movlw   b'00001000'             ;16bit baud rate register, no auto baud rate
        movwf   BAUDCON1
        movlw   b'00000100'             ;asynchronous mode, high speed
        movwf   TXSTA1
        movlw   b'10010000'             ;serial port enabled, enables continuous receive
        movwf   RCSTA1

        rcall   Delay25                 ;stabilize UART before send
        rcall   UARTSendACK             ;send acknowledge message
    return

;==============================================================================
; Routine:      MOVE 8 BYTES OF DATA FUNCTION
;==============================================================================
UARTMoveData
        movlw   0x08                    ;read 8 successive registers
UARTMoveDataLoop:
        movff   POSTINC0,POSTINC1       ;move data from receive buffer to destination buffer
        decfsz  WREG                    ;all 8 bytes read?
        bra     UARTMoveDataLoop
    return

;==============================================================================
; Routine:      TRANSMIT ROUTINE
;==============================================================================
UARTSendError
        movlb   BootRAM                 ;choose boot bank
        movlw   0x0F                    ;set frame "received message misunderstood by bootloader"
        movwf   bTB1
        movlw   0x01                    ;response bit
        movwf   bTB2
        setf    bTB3                    ;unused
        setf    bTB4                    ;unused
        movlw   BVER                    ;bootloader software version
        movwf   bTB5
        movlw   BREV                    ;revision number of software
        movwf   bTB6
        setf    bTB7                    ;unused
        setf    bTB8                    ;unused
        setf    bTB9                    ;unused
        setf    bTB10                   ;unused
        bra     UARTSendNoHeader

UARTSendACK
        movlb   BootRAM                 ;choose boot bank
        movlw   0x10                    ;set frame "node in programming mode"
        movwf   bTB1
        movlw   0x01                    ;response bit
        movwf   bTB2
        setf    bTB3                    ;unused
        setf    bTB4                    ;unused
        movlw   BVER                    ;bootloader software version
        movwf   bTB5
        movlw   BREV                    ;revision number of software
        movwf   bTB6
        setf    bTB7                    ;unused
        setf    bTB8                    ;unused
        setf    bTB9                    ;unused
        setf    bTB10                   ;unused
        bra     UARTSendNoHeader

UARTSend
        movlb   BootRAM                 ;choose boot bank
        movff   bRB1,bTB1               ;type frame the same as received plus response bit
        movff   bRB2,bTB2
        bsf     bTB2,0                  ;response bit
UARTSendNoHeader
        movlb   BootRAM                 ;choose boot bank
        movlw   0xAA                    ;start byte
        movwf   bTB0
        movlw   0xA5                    ;stop byte
        movwf   bTB12
    ;checksum
        clrf    bTB11                   ;checksum
        lfsr    FSR1,bTB1               ;point at first register of TXB buffer
        movlw   0x0A                    ;read 10 successive registers
        movwf   bR12
UARTChecksumLoop
        movff   POSTINC1,WREG           ;move data from RXB buffer to uart transmit buffer
        addwf   bTB11
        decfsz  bR12                    ;all 10 bytes read?
        bra     UARTChecksumLoop        ;not yet - read more
    ;send
        bsf     TXSTA,TXEN              ;enable uart transmission
        lfsr    FSR1,bTB0               ;point at first register of TXB buffer
        movlw   0x0D                    ;read 13 successive registers
        movwf   bR12
UARTSendLp1:
        btfss   PIR1,TXIF               ;check if TXREG if empty and ready to be loaded
        bra     $ - 2                
        movff   POSTINC1,TXREG          ;move data from RXB buffer to uart transmit buffer
        decfsz  bR12                    ;all 13 bytes read?
        bra     UARTSendLp1             ;not yet - read more
    return

;==========================================================================
; Routine:      CODES ROUTINE
;==========================================================================
UART_code_exit_bootloader_mode
        clrf    EEADRH                  ;point at Bootloader mode flag
        clrf    EEADR
        clrf    EEDATA                  ;clear flag
        bcf     EECON1,EEPGD            ;access EEPROM
        bcf     EECON1,CFGS             ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN             ;enable writes
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        bcf     TXSTA,TXEN              ;disable uart transmission
    reset
;==========================================================================
UART_code_control_frame
        lfsr    FSR0,bRB3               ;point at first register of source buffer - bRB3
        lfsr    FSR1,bRB19              ;point at first register of destination buffer - bRB19
        rcall   UARTMoveData            ;move data from received buffer to temp buffer
        lfsr    FSR0,bRB3               ;point at first register of source buffer - bRB3
        lfsr    FSR1,bTB3               ;point at first register of destination buffer - bTB3
        rcall   UARTMoveData            ;move data from temp buffer to transmit buffer
        rcall   UARTSend                ;send acknowledge frame (same as received)
    bra     UARTBootLoop                ;go back to main loop
;==========================================================================
UART_code_data_frame
        movlb   BootRAM                 ;choose boot bank
        movlw   0xF0                    ;check if address begins from F0....h - EEPROM
        xorwf   bRB19,W                
        bz      UARTProgramEEPROM
        bra     UARTProgramFLASH        

;==============================================================================
; Routine:      CAN BOOTLOADER MAIN PROGRAM
;==============================================================================
UARTBootLoop
        bra     MainBootLoop

UARTBackWithError
        rcall   UARTSendError
        bra     UARTBootLoop

UARTReceived
        movlb   BootRAM
        clrf    bRBOVF                  ;clear receive buffer overflow flag
        clrf    bRBCNT                  ;clear receive buffer counter
        rcall   ResetIdleTimer          ;reset time between received bytes
        lfsr    FSR1,bRB0               ;point at first register of RXB buffer
UARTReceivedLp
    ;overrun error
        btfsc   RCSTA,OERR              ;check if overrun error occured
        bra     UARTOverrunError        ;restart receiver and exit
    ;check idle timer
        rcall   UARTIncIdleTimer        ;measures time between received bytes, schould be about 90us (time of receiving another byte)
        tstfsz  WREG                    ;skip if timer not overflow (if WREG=0)
        bra     UARTCheckFrame
    ;check if received byte
        btfss   PIR1,RC1IF              ;check if uart received anything
        bra     UARTReceivedLp          ;wait for message
    ;received
        rcall   UARTIncRecCounter       ;increment receive buffer counter
        rcall   UARTResetIdleTimer      ;reset time between received bytes
        movff   RCREG1,POSTINC1         ;move data from uart receive buffer to can transmit buffer
        bra     UARTReceivedLp          ;read more
;=======================
UARTOverrunError
        bcf     RCSTA1,CREN             ;reset overrun error
        bsf     RCSTA1,CREN             ;enable again receiver
    bra UARTBackWithError
;=======================
UARTIncIdleTimer                        ;measures time between received bytes, schould be less than 70us    
        incfsz  bR13
    retlw   0                           ;timer not overflow
    retlw   1                           ;timer overflow
;=======================
UARTResetIdleTimer                      ;counts about 100us (1,5us ReceivedeLoop x 67 increments of bR13)
        movlw   0xBC
        movwf   bR13
    return
;=======================
UARTIncRecCounter                       ;increments receive buffer counter up to 13
        incf    bRBCNT                  ;then set overflow flag and start writting to begining of buffer
        movlw   .13
        cpfsgt  bRBCNT                  ;skip if CNT>13
    return
    ;buffer overflow
        movlw   .1
        movwf   bRBCNT                  ;clear receive buffer counter
        incf    bRBOVF                  ;set receive buffer overflow flag  
        lfsr    FSR1,bRB0               ;point again at first register of RXB buffer
    return

;=======================
UARTCheckFrame
        movlb   BootRAM
        movlw   0x00                    ;buffer overflow?
        xorwf   bRBOVF,W
        bnz     UARTBackWithError       ;yes
        movlw   .13                     ;check if 13 bytes received
        xorwf   bRBCNT,W
        bnz     UARTBackWithError       ;no
        movlw   0xAA                    ;is start byte ok?
        xorwf   bRB0,W
        bnz     UARTBackWithError       ;no
        movlw   0xA5                    ;is stop byte ok?
        xorwf   bRB12,W
        bnz     UARTBackWithError       ;no

        clrf    bR15                    ;temp checksum
        lfsr    FSR1,bRB1               ;point at first register of RX buffer
        movlw   0x0A                    ;read 10 successive registers
        movwf   bR14
UARTChksumBootLoop
        movff   POSTINC1,WREG           ;move data from RXB buffer to uart transmit buffer
        addwf   bR15
        decfsz  bR14                    ;all 10 bytes read?
        bra     UARTChksumBootLoop      ;not yet - read more

        movf    bR15,W                  ;is checksum byte ok?
        xorwf   bRB11,W
        bnz     UARTBackWithError       ;no

UARTReceiveMessage
        movlw   0x02                    ;is this exit bootloader request?
        xorwf   bRB1,W
        bz      UART_code_exit_bootloader_mode
        movlw   0x03                    ;is this control frame sent code?
        xorwf   bRB1,W
        bz      UART_code_control_frame
        movlw   0x04                    ;is this data frame sent code?
        xorwf   bRB1,W
        bz      UART_code_data_frame
    bra     UARTBootLoop                ;go back to main loop if other command

;==========================================================================
; Routine:      PROGRAM EEPROM ROUTINE
;==========================================================================
UARTProgramEEPROM
        movlb   BootRAM
        movlw   0x04
        cpfslt  bRB20                   ;skip if address less then 0xF00400
    bra     UARTBackWithError           ;go back to main loop if bootloader firm memory

        movlw   0x01                    ;check Command Byte, read data?
        xorwf   bRB24,W                
        bz      UARTReadEEPROM
        movlw   0x02                    ;check Command Byte, write data?
        xorwf   bRB24,W                
        bz      UARTWriteEEPROM
    bra     UARTBackWithError           ;go back to main loop if other command

UARTWriteEEPROM
        lfsr    FSR0,bRB3               ;start writing from register bRB3
        movff   bRB20,EEADRH            ;move high address to EEADRH
        movff   bRB21,EEADR             ;move low address to EEADR
        bcf     EECON1,EEPGD            ;access EEPROM
        bcf     EECON1,CFGS             ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN             ;enable writes
        movlw   0x08
        movwf   bR11
UARTWritemoreEEPROM
        movff   POSTINC0,EEDATA         ;move data indicated by FSR0 to EEDATA and increment FSR0
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;write EEPROM
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        incf    EEADR                   ;increment address of EEPROM
        decfsz  bR11
        bra     UARTWritemoreEEPROM
        bcf     EECON1,WREN             ;disable writes    

UARTReadEEPROM
        lfsr    FSR0,bTB3               ;start writing to register bTB3
        bcf     EECON1,EEPGD            ;access data EEPROM memory
        bcf     EECON1, CFGS            ;access data FLASH or EEPROM not Config Bits
        movff   bRB20,EEADRH            ;move high address to EEADRH
        movff   bRB21,EEADR             ;move low address to EEADR
        movlw   0x08                
UARTReadmoreEEPROM
        bsf     EECON1,RD               ;read data from EEPROM
        movff   EEDATA,POSTINC0         ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                   ;increment address of EEPROM
        decfsz  WREG                    ;8 bytes read?
        bra     UARTReadmoreEEPROM
        rcall   UARTSend

UARTEndProgramEEPROM
    bra        UARTBootLoop             ;go back to main loop

;==========================================================================
; Routine:      PROGRAM FLASH ROUTINE
;==========================================================================
UARTProgramFLASH
        movlb   BootRAM
        movff   bRB19,TBLPTRU           ;load addres to TBLPTRU
        movff   bRB20,TBLPTRH        
        movff   bRB21,TBLPTRL
    ;do not read bootloader firmware
        tstfsz  bRB19                   ;skip if address begins from 0x00
        bra     $ + .10
        movlw   0x10
        cpfslt  bRB20                   ;skip if address less then 0x001000
        bra     $ + 4
    bra     UARTBackWithError           ;go back to main loop if bootloader firm memory

        movlw   0x01                    ;check Command Byte, read data?
        xorwf   bRB24,W                
        bz      UARTReadFLASH
        movlw   0x02                    ;check Command Byte, write data?
        xorwf   bRB24,W                
        bz      UARTWriteFLASH
        movlw   0x03                    ;check Command Byte, erase data?
        xorwf   bRB24,W                
        bz      UARTEraseFLASH
    bra     UARTBackWithError           ;go back to main loop if other command

UARTEraseFLASH
        movf    TBLPTRL,W               ;check for a valid 64 bytes border
        andlw   b'00111111'
        bz      $ + 4
    bra     UARTBackWithError
        bsf     EECON1,EEPGD            ;access data FLASH memory
        bcf     EECON1,CFGS             ;access data FLASH or EEPROM not Config Bits
        bsf     EECON1,WREN             ;enable writes
        bsf     EECON1,FREE             ;enable FLASH row erase operation
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;start erase
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        movff   bRB21,TBLPTRL           ;point again at first register in block
        bra     UARTReadFLASH           ;read first 8 bytes and send them

UARTWriteFLASH
        movf    TBLPTRL,W               ;check for a valid 8 bytes border
        andlw   b'00000111'
        bz      $ + 4
    bra     UARTBackWithError
        lfsr    FSR0,bRB3               ;start writing from register bRB3
        movlw   0x08                
UARTWritemoreFLASH
        movff   POSTINC0,TABLAT         ;move data from register indicated by FSR0 to TABLAT and increment FSR0
        tblwt*+                         ;perform short write to holding register and increment TBLPTR
        decfsz  WREG                    ;8 bytes read?
        bra     UARTWritemoreFLASH
        tblrd*-                         ;point back into the block
        bsf     EECON1,EEPGD            ;access data FLASH memory
        bcf     EECON1,CFGS             ;access data FLASH or EEPROM not Config Bits
        bsf     EECON1,WREN             ;enable writes
        movlw   0x55                    ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR               ;start write
        btfss   PIR4,EEIF               ;check if write is complited
        bra     $ - 2
        bcf     PIR4,EEIF               ;clear flag
        bcf     EECON1,WREN             ;disable writes
        bcf     PIR4,EEIF               ;clear flag (it should not occur, but it does and does not allow to write eeprom as the next)
        movff   bRB21,TBLPTRL           ;point again at first register in the block

UARTReadFLASH
        lfsr    FSR0,bTB3               ;start writing to register bTB3
        movlw   0x08                
UARTReadmoreFLASH
        tblrd*+                         ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0         ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                    ;8 bytes read?
        bra     UARTReadmoreFLASH
        rcall   UARTSend
        
    bra     UARTBootLoop                ;go back to main loop

;==========================================================================
;==========================================================================
; END OF UART BOOTLOADER PROGRAMMING FUNCTIONS
;==========================================================================
;==========================================================================



;==============================================================================
;==============================================================================
; END OF BOOTLOADER
;==============================================================================
;==============================================================================
