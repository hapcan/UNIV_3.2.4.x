    LIST
;=============================================================================
;   MIT License
;
;   Copyright (c) 2022 HAPCAN Home Automation Project (http://hapcan.com)
;
;   Permission is hereby granted, free of charge, to any person obtaining a copy
;   of this software and associated documentation files (the "Software"), to deal
;   in the Software without restriction, including without limitation the rights
;   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;   copies of the Software, and to permit persons to whom the Software is
;   furnished to do so, subject to the following conditions:
;
;   The above copyright notice and this permission notice shall be included in all
;   copies or substantial portions of the Software.
;
;   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;   SOFTWARE.
;==============================================================================
;   Filename:              univ_3-routines-rev9.inc
;   Associated diagram:    none
;   Author:                Jacek Siwilo                          
;   Note:                  UNIV 3 CPU general routines
;==============================================================================
;   Revision History
;   Rev:  Date:     Details:
;   0     05.2013   Initial version
;   1     07.2013   New routines added
;   2     08.2013   CanReinitialization modifications, New routines added
;   3     11.2013   Minor changes in ReceiveProcedure, TransmitProcedure, 
;                   UpdateDelayTimers, new ComputedGoto
;   4     07.2015   Added CheckLastMemoryAddress routine
;   5     10.2015   CAN FIFO buffer sizes declared outside
;   6     08.2016   Corrected in lines 1012, 1016, 1020
;   7     08.2017   Modified EepromSaveWREG procedure and new ReEnAllInt
;   8     03.2018   Added channel regs in UpdateDelayTimers and <, > operators
;                   in CheckAgainstBoxes procedure
;   9     10.2022   Channel name request added
;==============================================================================



;==============================================================================
;==============================================================================
;       I  N  I  T  I  A  L  I  Z  A  T  I  O  N
;==============================================================================
;==============================================================================

;------------------------------------------------------------------------------
; Routine:          GENERAL INITIALIZATION
;------------------------------------------------------------------------------
; Overview:         Initializes the node
;------------------------------------------------------------------------------
GeneralInitialization
        banksel CANINTCNT
        clrf    CANINTCNT                   ;CAN init counter

        call    CheckLastMemoryAddress      ;check last used config memory address
        call    ReadEepromConfig            ;read eeprom config to RAM
        call    ClearChannelRegs            ;clear timer channel regs
        call    ClearUptimeRegs             ;clear uptime regs
    return

;------------------------------------------------------------------------------
; Routine:          CHECK LAST CONFIG MEMORY ADDRESS
;------------------------------------------------------------------------------
; Overview:         Checks eeprom and flash for last used address in the config
;                   section and saves
;                   last eeprom addresses in eeprom at 0xF00002
;                   last flash address in eeprom at 0xF00005
;------------------------------------------------------------------------------
CheckLastMemoryAddress
        rcall   CheckLastEepromAddress
        rcall   CheckLastFlashAddress
    return

;------------------------------------------------------------------------------
CheckLastEepromAddress
    ;read memory and save last address in ram
        banksel EEPROM_U                    ;set first eeprom address in ram 0xF0004F
        movlw   0xF0
        movwf   EEPROM_U
        clrf    EEPROM_H
        movlw   0x4F
        movwf   EEPROM_L
        clrf    EEADRH                      ;load addres to EEADRR 0xF0004F
        movlw   0x4F   
        movwf   EEADR
        bcf     EECON1,EEPGD                ;access data EEPROM memory
        bcf     EECON1,CFGS                 ;access data FLASH or EEPROM
CLEALoop                                    ;read eeprom loop
        rcall   ReadEepromLastAddress
        tstfsz  EEADR                       ;was last memory cell read? (when EEADR=EEADRH=0)
        bra     CLEALoop
        tstfsz  EEADRH
        bra     CLEALoop
    ;save last address in eeprom memory
        clrf    EEADRH                      ;take eeprom address
        movlw   0x02
        movwf   EEADR
        ;EEPROM_U
        call    EepromReadToWREG            ;read current value in eeprom
        xorwf   EEPROM_U,W
        bz      $ + .10                     ;skip if the same
        movff   EEPROM_U,WREG               ;set data for EepromSaveWREG routine
        call    EepromSaveWREG
        ;EEPROM_H
        incf    EEADR
        call    EepromReadToWREG
        xorwf   EEPROM_H,W
        bz      $ + .10
        movff   EEPROM_H,WREG
        call    EepromSaveWREG
        ;EEPROM_L
        incf    EEADR
        call    EepromReadToWREG
        xorwf   EEPROM_L,W
        bz      $ + .10
        movff   EEPROM_L,WREG
        call    EepromSaveWREG
    return
;--------------------------
ReadEepromLastAddress        
        bsf     EECON1,RD                   ;read data from EEPROM
        movlw   0xFF
        xorwf   EEDATA,W
        bz      $ + .10                     ;skip if EEDATA = 0xFF
        movff   EEADRH,EEPROM_H             ;save read address as last used
        movff   EEADR,EEPROM_L
        infsnz  EEADR                       ;increment address
        incfsz  EEADRH
    return

;------------------------------------------------------------------------------
CheckLastFlashAddress
    ;read memory and save last address in ram
        banksel FLASH_U                     ;set first flash address in ram 0x008000
        clrf    FLASH_U
        movlw   0x80
        movwf   FLASH_H
        clrf    FLASH_L
        clrf    TBLPTRU                     ;load addres to TBLPTR 0x008000
        movlw   0x80   
        movwf   TBLPTRH        
        clrf    TBLPTRL
CLFALoop                                    ;read flash loop
        rcall   ReadFlashLastAddress
        movlw   0x01                        ;was last memory cell read? (0x010000 current address?)
        xorwf   TBLPTRU,W
        bnz     CLFALoop
    ;save last address in eeprom memory
        clrf    EEADRH                      ;take eeprom address
        movlw   0x05
        movwf   EEADR
        ;FLASH_U
        call    EepromReadToWREG            ;read current value in eeprom
        xorwf   FLASH_U,W
        bz      $ + .10                     ;skip if the same
        movff   FLASH_U,WREG                ;set data for EepromSaveWREG routine
        call    EepromSaveWREG
        ;FLASH_H
        call    EepromReadToWREG
        xorwf   FLASH_H,W
        bz      $ + .10
        incf    EEADR
        movff   FLASH_H,WREG
        call    EepromSaveWREG
        ;FLASH_L
        call    EepromReadToWREG
        xorwf   FLASH_L,W
        bz      $ + .10
        incf    EEADR
        movff   FLASH_L,WREG
        call    EepromSaveWREG
    return
;-----------------------
ReadFlashLastAddress
        tblrd*                              ;read and move to TABLAT
        movlw   0xFF
        xorwf   TABLAT,W
        bz      $ + .14                     ;skip if TABLAT not 0xFF
        movff   TBLPTRU,FLASH_U             ;save read address as last used
        movff   TBLPTRH,FLASH_H
        movff   TBLPTRL,FLASH_L
        tblrd*+                             ;increment address
    return

;------------------------------------------------------------------------------
; Routine:          READ CONFIG EEPROM
;------------------------------------------------------------------------------
; Overview:         Reads config from eeprom
;------------------------------------------------------------------------------
ReadEepromConfig
        bcf     EECON1,EEPGD                ;access data EEPROM memory
        bcf     EECON1,CFGS                 ;access data FLASH or EEPROM
        clrf    EEADRH                      ;start reading from 0000h EEPROM address
        clrf    EEADR
        lfsr    FSR0,0x200                  ;start writing to address
        movlw   0x00                        ;256 successive registers
ReadConfig
        bsf     EECON1,RD                   ;read data from EEPROM
        movff   EEDATA,POSTINC0             ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                       ;increment address of EEPROM
        decfsz  WREG                        ;all bytes read?
        bra     ReadConfig                  ;not yet - read more
    return

;------------------------------------------------------------------------------
; Routine:          INIT CHANNEL REGISTERS
;------------------------------------------------------------------------------
; Overview:         It clears all channel registers
;------------------------------------------------------------------------------
ClearChannelRegs                            ;clear the whole field
        banksel Instr1Ch1
        lfsr    FSR1,Instr1Ch1              ;starting from Instr1Ch1
        movlw   .110                        ;clear 110 successive registers
ClearLoop1:
        clrf    POSTINC1                    ;set data
        decfsz  WREG                        ;all bytes read?
        bra     ClearLoop1

        setf    Instr1Ch1                   ;clear channel instructions
        setf    Instr1Ch2
        setf    Instr1Ch3
        setf    Instr1Ch4
        setf    Instr1Ch5
        setf    Instr1Ch6
        setf    Instr1Ch7
        setf    Instr1Ch8
        setf    Instr1Ch9
        setf    Instr1Ch10
    return

;------------------------------------------------------------------------------
; Routine:          INIT UPTIME REGISTERS
;------------------------------------------------------------------------------
; Overview:         It clears all uptime registers
;------------------------------------------------------------------------------
ClearUptimeRegs
        banksel UPTIME0
        clrf    UPTIME0
        clrf    UPTIME1
        clrf    UPTIME2
        clrf    UPTIME3
    return

;------------------------------------------------------------------------------
; Routine:          CAN REINITIALIZATION
;------------------------------------------------------------------------------
; Overview:         It sets up CAN engine again
;------------------------------------------------------------------------------
CanReinitialization
        movlw   b'10000000'                 ;abort transmission and request configuration mode
        movwf   CANCON
ConfigModeOn:
        btfss   CANSTAT,7                   ;check if configuration mode is on
        bra     ConfigModeOn

        clrf    ECANCON                     ;Mode 0
        movlw   b'00100001'                 ;CANTX2 is digital pin, CANTX pin will drive VDD, oscillator as the source for CAN
        movwf   CIOCON        

    ;setting can baud rate (125kbps, Tbit=16TQ for 8MHz clock)
        banksel BRGCON1                     ;choose bank 14
        movlw   b'00000001'                 ;set SJW=1xTQ & BRP=0 (TQ=4xTosc)
        movwf   BRGCON1
        movlw   b'10111100'                 ;set SAM=1 SEG1PH=8xTQ PRSEG=5xTQ
        movwf   BRGCON2
        movlw   b'00000001'                 ;set WAKFIL=0 & SEG2PH=2xTQ
        movwf   BRGCON3

    ;setting Masks & Filters
        clrf    RXM0SIDH                    ;clear Mask 0 - all message receives
        clrf    RXM0SIDL                    ;don't care about filters
        clrf    RXM0EIDH
        clrf    RXM0EIDL
        clrf    RXM1SIDH                    ;clear Mask 1 - all message receives
        clrf    RXM1SIDL                    ;don't care about filters
        clrf    RXM1EIDH
        clrf    RXM1EIDL
        bsf     RXF0SIDL,3                  ;set EXIDEN bit in Filter 0
        
    ;setting configuration of receive buffers
        movlw   b'01000100'                 ;receive only valid extended messages
        movwf   RXB0CON                     ;and overflow RX0 will write to RX1
        banksel RXB1CON                     ;choose bank 15
        movwf   RXB1CON

    ;setting can interrupt
        movlw   b'00100011'
        movwf   PIE5                        ;enable interrupts
        clrf    IPR5                        ;set low priority for all

    ;end of can configuration
        bsf     TRISB,3                     ;set can input
        bcf     TRISB,2                     ;set can output
        movlw   b'00000000'                 ;request normal mode
        movwf   CANCON

ConfigModeOff:
        btfsc   CANSTAT,7                   ;check if configuration mode is off
        bra     ConfigModeOff               ;note: masks & filters read as 00h, but in fact they are set

    ;flag for main firmware
        banksel CANFULL                     ;choose right bank
        clrf    CANFULL                     ;clear CAN buffer full flag

    ;update CAN initiaization counter
        banksel CANINTCNT                   ;choose right bank               
        infsnz  CANINTCNT                   ;skip if not zero
        decf    CANINTCNT                   ;go back to 0xFF if was zero
    ;update transmit timer
        banksel TRSMTTIMER                  ;choose right bank 
        clrf    TRSMTTIMER                  ;clear transmit timer
    return



;==============================================================================
;==============================================================================
;       C  A  N    F  I  F  O    B  U  F  F  E  R  S
;==============================================================================
;==============================================================================

;------------------------------------------------------------------------------
; Routine:          CAN FIFO INITIALIZE
;------------------------------------------------------------------------------
; Overview:         Initialization of CAN Receive & Transmit FIFO buffers
;
;    Input:         RXFIFOSIZE - size of CAN RX FIFO buffer
;                   TXFIFOSIZE - size of CAN TX FIFO buffer
;                   RXFIFORAM - begining of RX FIFO in RAM
;                   TXFIFORAM - begining of TX FIFO in RAM
;------------------------------------------------------------------------------
FIFOInitialization
        banksel RXTOP                       ;choose the right bank
        clrf    RXTOP                       ;clear stack top 
        clrf    RXCNT                       ;clear stack counter
        clrf    RXCNTMX                     ;clear max stack counter
        banksel TXTOP                       ;choose the right bank
        clrf    TXTOP                       ;clear stack top 
        clrf    TXCNT                       ;clear stack counter
        clrf    TXCNTMX                     ;clear max stack counter
    return

;------------------------------------------------------------------------------
; Routine:          PROCESS CAN MSG IN RECEIVE FIFO
;------------------------------------------------------------------------------
; Overview:         Checks CAN FIFO RX buffer and processes the msg if any is
;                   waiting
;    Input:         none
;   Output:         WREG = 0 if message was read from FIFO
;                   WREG = 1 if no message in FIFO
;------------------------------------------------------------------------------
ReceiveProcedure
        banksel RXCNT
        tstfsz  RXCNT                       ;any msg received?
        bra     $ + 4                       ;yes
    retlw   0x01                            ;no
        ;make sure there was not writing to RX FIFO during reading
        bcf     INTCON,GIEL                 ;low interrupt off
        call    ReadFromCanRxFIFO
        bsf     INTCON,GIEL                 ;low interrupt on
        call    ProcessMessage              ;handle received message
    retlw   0x00

;------------------------------------------------------------------------------
; Routine:          PROCESS CAN MSG IN TRANSMIT FIFO
;------------------------------------------------------------------------------
; Overview:         Checks CAN FIFO TX buffer and processes the msg if any is
;                   waiting
;    Input:         none
;   Output:         WREG = 0 if message from FIFO was sent to CAN
;                   WREG = 1 if msg in FIFO was not sent (no message in FIFO)
;                   WREG = 2 if msg in FIFO was not sent (CAN bus is busy)
;------------------------------------------------------------------------------
TransmitProcedure
        banksel TXCNT
        tstfsz  TXCNT                       ;any msg to transmit
        bra     $ + 4                       ;yes
    retlw   0x01                            ;no
        ;check if transmit buffer is free
        banksel TXB2CON
        btfss   TXB2CON,3                   ;check if TXB2 is free skip if busy
        bra     TransmitProcNow
        btfss   TXB1CON,3                   ;check if TXB1 is free skip if busy
        bra     TransmitProcNow
        call    CheckTransmitTimer          ;check for how long buffers are busy
    retlw   0x02
TransmitProcNow
        call    ReadFromCanTxFIFO
        call    CanTransmit
    retlw   0x00

;------------------------------------------------------------------------------
; Routine:          COPY CAN RX msg to CAN RX FIFO INPUT buffer
;------------------------------------------------------------------------------
; Overview:         Copies received CAN message (12 bytes beginning from
;                   CANFRAME1) to CAN receive FIFO input buffer (12 bytes begining
;                   from RXFIFOIN0)
;------------------------------------------------------------------------------
Copy_RXB_RXFIFOIN
        banksel RXFIFOR0                    ;choose the right bank
        lfsr    FSR0,CANFRAME1              ;point at source
        lfsr    FSR1,RXFIFOIN0              ;point at destination
        movlw   .12                         ;read 12 successive registers
        movwf   RXFIFOR0                      
Copy_RXB_RXFIFOIN_Loop:
        movlw   .8
        xorwf   RXFIFOR0,W                  ;first 4 regs passed?
        bnz     $ + .4                      ;no
        incf    FSR0L                       ;yes, so pass RXBDLC reg
        movff   POSTINC0,POSTINC1           ;move data from receive buffer to destination buffer
        decfsz  RXFIFOR0                    ;all 12 bytes read?
        bra     Copy_RXB_RXFIFOIN_Loop      ;not yet - read more
        banksel CANFULL                     ;choose the right bank
        clrf    CANFULL                     ;clear "buffer full" flag
    return

;------------------------------------------------------------------------------
; Routine:          COPY CAN TX FIFO INPUT msg to CAN RX FIFO INPUT buffer
;------------------------------------------------------------------------------
; Overview:         Copies CAN transmit FIFO input buffer (12 bytes beginning
;                   from TXFIFOIN0) to CAN receive FIFO input buffer (12 bytes
;                   begining from RXFIFOIN0)
;------------------------------------------------------------------------------
Copy_TXFIFOIN_RXFIFOIN
        banksel TXFIFOR0                    ;choose the right bank
        lfsr    FSR0,TXFIFOIN0              ;point at source
        lfsr    FSR1,RXFIFOIN0              ;point at destination
        movlw   .12                         ;read 12 successive registers
        movwf   TXFIFOR0                      
Copy_TXFIFOIN_RXFIFOIN_Loop:
        movff   POSTINC0,POSTINC1           ;move data from receive buffer to destination buffer
        decfsz  TXFIFOR0                    ;all 12 bytes read?
        bra     Copy_TXFIFOIN_RXFIFOIN_Loop ;not yet - read more
    return

;------------------------------------------------------------------------------
; Routine:          WRITE MESSAGE TO CAN RX FIFO
;------------------------------------------------------------------------------
; Overview:         Writes message saved in CAN RX FIFO input buffer (12 bytes
;                   beginning from RXFIFOIN0) into CAN RX FIFO
;------------------------------------------------------------------------------
WriteToCanRxFIFO
        banksel RXTOP                       ;choose the right bank
    ;point at destination address (top of RX FIFO)
        lfsr    FSR1,RXFIFORAM-.12          ;point at first reg in RX FIF0 (RXFIFORAM-12)  
        movff   RXTOP,RXFIFOR0              ;copy stack top
        incf    RXFIFOR0                      
        movlw   .12
AddRXFIFOAdr:
        addwf   FSR1L                       ;add multiply 32 to FSR1
        bnc     $ + .4                      ;go to next ram bank if overflow
        incf    FSR1H
        decfsz  RXFIFOR0
        bra     AddRXFIFOAdr

    ;copy to FIFO
        lfsr    FSR0,RXFIFOIN0              ;point at source
        movlw   .12                         ;read 12 successive registers
        movwf   RXFIFOR0                      
CopyToRXFIFOLoop:
        movff   POSTINC0,POSTINC1           ;move data from receive buffer to destination buffer
        decfsz  RXFIFOR0                    ;all 12 bytes read?
        bra     CopyToRXFIFOLoop            ;not yet - read more

    ;update stack top
        incf    RXTOP                       ;increment top of stack
        movlw   RXFIFOSIZE                  ;stack depth
        xorwf   RXTOP,W                     ;overflow
        bnz     $ + .4                      ;no
        clrf    RXTOP                       ;yes, so set top to zero
    ;update stack counter
        movlw   RXFIFOSIZE                  ;stack depth
        xorwf   RXCNT,W                     ;overflow?
        bz      $ + .6                      ;yes
        incf    RXCNT                       ;no, so increment stack counter
        bra     $ + .6
    ;update stack counter max value
        infsnz  RXCNTMX                     ;indicate overflow ;over 0xFF?
        decf    RXCNTMX                     ;yes, so go back to 0xFF
        movf    RXCNT,W
        cpfsgt  RXCNTMX                     ;compare, skip if RXCNTMX>RXCNT
        movwf   RXCNTMX                     ;save cnt as max value

EndWriteToCanRxFIFO
    return                              ;end of Save msg to FIFO procedure

;------------------------------------------------------------------------------
; Routine:          READ MESSAGE FROM CAN RX FIFO
;------------------------------------------------------------------------------
; Overview:         Reads next message in queue from CAN RX FIFO and saves in
;                   CAN RX FIFO output buffer (12 bytes beginning from RXFIFO0)
;------------------------------------------------------------------------------
ReadFromCanRxFIFO
        banksel RXCNT                       ;choose the right bank
        tstfsz  RXCNT                       ;any on stack?
        bra     $ + .4                      ;yes
    return                                  ;stack empty
    ;find first reg in FIFO              
        movf    RXCNT,W                       
        subwf   RXTOP,W                     ;if(RXTOP-RXCNT>0)
        bnn     $ + .4                      ;Adr = RXTOP-RXCNT      
        addlw   RXFIFOSIZE                  ;else: RXTOP-RXCNT+RXFIFOSIZE
    ;get address of first FIFO reg
        movwf   RXFIFOR1          
        lfsr    FSR1,RXFIFORAM-.12          ;point at first reg in RX FIF0 (RXFIFORAM-12)  
        incf    RXFIFOR1
        movlw   .12  
GetCRXFIFOAdr:
        addwf   FSR1L                       ;add multiply 32 to FSR1
        bnc     $ + .4                      ;go to next ram bank if overflow
        incf    FSR1H
        decfsz  RXFIFOR1
        bra     GetCRXFIFOAdr

    ;copy from FIFO
        lfsr    FSR0,RXFIFO0                ;point at first register of CAN RX FIFO output buffer
        movlw   .12                         ;read 12 successive registers           
CopyFromCRXFIFOLoop:
        movff   POSTINC1,POSTINC0           ;move data from receive buffer to destination buffer
        decfsz  WREG                        ;all 5 bytes read?
        bra     CopyFromCRXFIFOLoop         ;not yet - read more

    ;update FIFO stack top
        ;top stack stays unchanged
    ;update FIFO stack counter
        decf    RXCNT

EndReadFromRXFIFO
    return                              ;end of read msg from FIFO procedure

;------------------------------------------------------------------------------
; Routine:          WRITE MESSAGE TO CAN TX FIFO
;------------------------------------------------------------------------------
; Overview:         Writes message saved in CAN TX FIFO input buffer (12 bytes
;                   beginning from TXFIFOIN0) into CAN TX FIFO
;------------------------------------------------------------------------------
WriteToCanTxFIFO
        banksel TXTOP                       ;choose the right bank
    ;point at destination address (top of TX FIFO)
        lfsr    FSR1,TXFIFORAM-.12          ;point at first reg in TX FIF0 (TXFIFORAM-.12)
        movff   TXTOP,TXFIFOR0              ;copy stack top
        incf    TXFIFOR0                      
        movlw   .12
AddTXFIFOAdr:
        addwf   FSR1L                       ;add multiply 32 to FSR1
        bnc     $ + .4                      ;go to next ram bank if overflow
        incf    FSR1H
        decfsz  TXFIFOR0
        bra     AddTXFIFOAdr

    ;copy to FIFO
        lfsr    FSR0,TXFIFOIN0              ;point at source (first register in transmit buffer TXBSIDH)
        movlw   .12                         ;read 12 successive registers
        movwf   TXFIFOR0                      
CopyToTXFIFOLoop:
        movff   POSTINC0,POSTINC1           ;move data from receive buffer to destination buffer
        decfsz  TXFIFOR0                    ;all 5 bytes read?
        bra     CopyToTXFIFOLoop            ;not yet - read more

    ;update stack top
        incf    TXTOP                       ;increment top of stack
        movlw   TXFIFOSIZE                  ;stack depth
        xorwf   TXTOP,W                     ;overflow
        bnz     $ + .4                      ;no
        clrf    TXTOP                       ;yes, so set top to zero
    ;update stack counter
        movlw   TXFIFOSIZE                  ;stack depth
        xorwf   TXCNT,W                     ;overflow?
        bz      $ + .6                      ;yes
        incf    TXCNT                       ;no, so increment stack counter
        bra     $ + .6
    ;update stack counter max value
        infsnz  TXCNTMX                     ;indicate overflow ;over 0xFF?
        decf    TXCNTMX                     ;yes, so go back to 0xFF
        movf    TXCNT,W
        cpfsgt  TXCNTMX                     ;compare, skip if TXCNTMX>TXCNT
        movwf   TXCNTMX                     ;save cnt as max value

EndWriteToCanTxFIFO
    return                              ;end of Save msg to FIFO procedure

;------------------------------------------------------------------------------
; Routine:          READ MESSAGE FROM CAN TX FIFO
;------------------------------------------------------------------------------
; Overview:         Reads next message in queue from CAN TX FIFO and saves in
;                   CAN TX FIFO output buffer (12 bytes beginning from TXFIFO0)
;------------------------------------------------------------------------------
ReadFromCanTxFIFO
        banksel TXCNT                       ;choose the right bank
        tstfsz  TXCNT                       ;any on stack?
        bra     $ + .4                      ;yes
    return                                  ;stack empty
    ;find first reg in FIFO              
        movf    TXCNT,W                       
        subwf   TXTOP,W                     ;if(TXTOP-TXCNT>0)
        bnn     $ + .4                      ;Adr = TXTOP-TXCNT      
        addlw   TXFIFOSIZE                  ;else: TXTOP-TXCNT+TXFIFOSIZE
    ;get address of first FIFO reg
        movwf   TXFIFOR1          
        lfsr    FSR1,TXFIFORAM-.12          ;point at first reg in TX FIF0 (TXFIFORAM-.12)
        incf    TXFIFOR1
        movlw   .12   
GetTXFIFOAdr:
        addwf   FSR1L                       ;add multiply 12 to FSR1
        bnc     $ + .4                      ;go to next ram bank if overflow
        incf    FSR1H
        decfsz  TXFIFOR1
        bra     GetTXFIFOAdr

    ;copy from FIFO
        lfsr    FSR0,TXFIFO0                ;point at first register of FIFO transmit buffer TXBFIFOSH
        movlw   .12                         ;read 12 successive registers  
CopyFromTXFIFOLoop:
        movff   POSTINC1,POSTINC0           ;move data from receive buffer to destination buffer
        decfsz  WREG                        ;all 12 bytes read?
        bra     CopyFromTXFIFOLoop          ;not yet - read more

    ;update FIFO stack top
        ;top stack stays unchanged
    ;update FIFO stack counter
        decf    TXCNT

EndReadFromCanTxFIFO
    return                              ;end of read msg from FIFO procedure



;==============================================================================
;==============================================================================
;       T   R   A   N   S   M   I   T   T   I   N   G       M  S  G.
;==============================================================================
;==============================================================================

;------------------------------------------------------------------------------
; Routine:          CAN TRANSMIT ROUTINE
;------------------------------------------------------------------------------
; Overview:         Moves message from CAN TX FIFO output buffer (12 bytes
;                   beginning from TXFIFO0) into available CAN transmit buffer
;                   (TXB2 or TXB1) and request transmission
;------------------------------------------------------------------------------
CanTransmit
        banksel TXFIFO0                     ;point at right bank
    ;prepare bits in TXFIFO1
        btfsc   TXFIFO1,4                   ;what is the fourth bit?
        bra     $ + .6                      ;it is 1, so set it in fourth position
        bcf     TXFIFO1,1                   ;it is 0, so clear it in fourth position
        bra     $ + .4
        bsf     TXFIFO1,1
        bsf     TXFIFO1,3                   ;set EXIDE bit
TXB2
        movf    CANCON,W                    ;take register bits
        andlw   b'11110001'                 ;clear WIN bits
        iorlw   b'00000100'                 ;access TXB2 buffer
        movwf   CANCON                      ;apply changes
        btfsc   RXB0CON,3                   ;check if TXB2 is free skip if free
        bra     TXB1
        call    WriteBuffer
        bsf     RXB0CON,3                   ;request trasmition & clear TXABT, TXLARB, TXERR
        bra     ExitCanTransmit             ;now wait for TXB2IF (transmit successful)
TXB1
        movf    CANCON,W                    ;take register bits
        andlw   b'11110001'                 ;clear WIN bits
        iorlw   b'00000110'                 ;access TXB1 buffer
        movwf   CANCON                      ;apply changes
        btfsc   RXB0CON,3                   ;check if TXB1 is free skip if free
        bra     TXB2
        call    WriteBuffer
        bsf     RXB0CON,3                   ;request trasmition & clear TXABT, TXLARB, TXERR
        bra     ExitCanTransmit             ;now wait for TXB1IF (transmit successful)
ExitCanTransmit
        return

;--------------------------
WriteBuffer:
        banksel ROUT0                       ;point at right bank
        lfsr    FSR0,0xF60                  ;point at first register of transmit buffer RXB0
        lfsr    FSR1,TXFIFO0                ;point at first register of source buffer TXFIFOSH
        clrf    POSTINC0                    ;clear TXxCON
        movlw   .12                         ;read 12 successive registers
        movwf   ROUT0
WriteBufferLoop:
        movlw   .8
        xorwf   ROUT0,W                     ;first 4 regs passed?
        bnz     $ + .4                      ;no
        incf    FSR0L                       ;yes, so pass TXBDLC reg
        movff   POSTINC1,POSTINC0           ;move data from transmit buffer to destination buffer
        decfsz  ROUT0                       ;all 13 bytes read?
        bra     WriteBufferLoop             ;not yet - read more
        movlw   0x08                        ;move 8 to DLC reg
        movwf   RXB0DLC
        clrf    TRSMTTIMER                  ;clear transmit timer
    return



;==============================================================================
;==============================================================================
;       R   E   C   E   I   V   I   N   G       M  S  G.
;==============================================================================
;==============================================================================

;------------------------------------------------------------------------------
; Routine:          PROCESS RECEIVED CAN MESSAGE
;------------------------------------------------------------------------------
; Overview:         It handles message received from CAN bus
;------------------------------------------------------------------------------
ProcessMessage
        ;recognize the message
        movlw   0x1F                        ;check if RXFIFO0 > 1Fh
        cpfsgt  RXFIFO0                     ;skip if yes
        bra     $ + .8
        call    ProcessNormalMessage
    return
        call    ProcessSystemMessage
    return

;------------------------------------------------------------------------------
; PROCESS SYSTEM MESSAGE
;------------------------------------------------------------------------------
ProcessSystemMessage
        banksel RXFIFO0
        ;check is system message
        movlw   0x20                        ;check if RXFIFO0 < 20h
        cpfslt  RXFIFO0                     ;skip if yes
        bra     EndProcessSystemMessages    ;it is not system message

SystemMessageNodeCheck                      ;check if message is for this node (NODE,GROUP)
        movff   GROUPNR,WREG                ;check group
        xorwf   RXFIFO7,W
        bnz     SystemMessageGroupCheck     ;if it is not right group, go and check if it is for all
        movff   NODENR,WREG
        xorwf   RXFIFO6,W
        bnz     SystemMessageGroupCheck     ;if it is right group, but not right node, check if it is for all nodes in this group
        bra     SystemMessageIndividual     ;process message for individual node

SystemMessageGroupCheck
        clrf    WREG                        ;check if message is for all nodes (0,0)
        xorwf   RXFIFO7,W
        bz      $ + .10                     ;group nr = 0, now check if node nr = 0
        movff   GROUPNR,WREG                ;check if message is for this group
        xorwf   RXFIFO7,W
        bnz     EndProcessSystemMessages    ;it is not for this group
        clrf    WREG                        ;check if message is for all nodes in this group (0,GROUP)
        xorwf   RXFIFO6,W
        bnz     EndProcessSystemMessages
        bra     SystemMessageGroup          ;process group message

EndProcessSystemMessages
    return                                  ;end of ProcessSystemMessage

;------------------------------------------------------------------------------
SystemMessageIndividual                     ;can be executed only after direct addressing to node
        ;SM_100_10F_IND
        movlw   0x10                        ;is this 100h to 10Fh message?
        xorwf   RXFIFO0,W
        bz      SM_100_10F_IND
        ;SM_110_11F_IND
        movlw   0x11                        ;is this 110h to 11Fh message?
        xorwf   RXFIFO0,W
        bz      SM_110_11F_IND
    bra     EndProcessSystemMessages

SM_100_10F_IND                              ;messages from 0x100 to 0x10F
        ;SM108                              ;is this group Status request?
        movlw   0x80                    
        xorwf   RXFIFO1,W
        bz      SM108_StatusAsk
        ;SM109                              ;is this Status request?
        movlw   0x90                    
        xorwf   RXFIFO1,W
        bz      SM109_StatusAsk
        ;SM10A                              ;is this Instruction in message?
        movlw   0xA0                    
        xorwf   RXFIFO1,W
        bz      SM10A_InstructionInMessage
    bra     EndProcessSystemMessages

SM_110_11F_IND                              ;messages from 0x110 to 0x11F
        ;SM112                              ;is this group uptime request?
        movlw   0x20                    
        xorwf   RXFIFO1,W
        bz      SM112_UptimeAsk
        ;SM113                              ;is this node uptime request?
        movlw   0x30                    
        xorwf   RXFIFO1,W
        bz      SM113_UptimeAsk
        ;SM114                              ;is this group health request? 
        movlw   0x40                    
        xorwf   RXFIFO1,W
        bz      SM114_HealthAsk
        ;SM115                              ;is this node health request? 
        movlw   0x50                    
        xorwf   RXFIFO1,W
        bz      SM115_HealthAsk
        ;SM116                              ;is this group channel name request? 
        movlw   0x60                    
        xorwf   RXFIFO1,W
        bnz     $ + 4
        bra     SM116_ChannelName
        ;SM117                              ;is this node channel name request? 
        movlw   0x70                    
        xorwf   RXFIFO1,W
        bnz     $ + 4
        bra     SM117_ChannelName
    bra     EndProcessSystemMessages

;------------------------------------------------------------------------------
SystemMessageGroup                          ;can be executed after group addressing, too    
        ;SM_100_10F_GROUP 
        movlw   0x10                        ;is this 100h to 10Fh message?
        xorwf   RXFIFO0,W
        bz      SM_100_10F_GROUP
        ;SSM_110_11F_GROUP
        movlw   0x11                        ;is this 110h to 11Fh message?
        xorwf   RXFIFO0,W
        bz      SM_110_11F_GROUP
    bra     EndProcessSystemMessages

SM_100_10F_GROUP                            ;messages from 0x100 to 0x10F
        ;SM108                              ;is this group Status request?
        movlw   0x80                    
        xorwf   RXFIFO1,W
        bz      SM108_StatusAsk
    bra     EndProcessSystemMessages        ;if unknown command just exit

SM_110_11F_GROUP                            ;messages from 0x110 to 0x11F
        ;SM112                              ;is this group uptime request?
        movlw   0x20                    
        xorwf   RXFIFO1,W
        bz      SM112_UptimeAsk
        ;SM114                              ;is this group health request? 
        movlw   0x40                    
        xorwf   RXFIFO1,W
        bz      SM114_HealthAsk
        ;SM116                              ;is this group channel name request? 
        movlw   0x60                    
        xorwf   RXFIFO1,W
        bz      SM116_ChannelName
    bra        EndProcessSystemMessages     ;if unknown command just exit

;------------------------------------------------------------------------------
SM108_StatusAsk
SM109_StatusAsk
        call    NodeStatusRequest
    bra        EndProcessSystemMessages

;-------------------------------------
SM10A_InstructionInMessage                  ;do instr. which is in the message
        movff   RXFIFO4,INSTR1              ;move data from message to INSTRx regs
        movff   RXFIFO5,INSTR2
        movff   RXFIFO8,INSTR3
        movff   RXFIFO9,INSTR4
        movff   RXFIFO10,INSTR5
        movff   RXFIFO11,INSTR6
        call    DoInstructionRequest        ;call DoInstruction in main program
    bra        EndProcessSystemMessages

;-------------------------------------
SM112_UptimeAsk
SM113_UptimeAsk
        movff   RXFIFO0,TXFIFOIN0           ;set frame type
        movff   RXFIFO1,TXFIFOIN1
        bsf     TXFIFOIN1,0                 ;response bit
        movff   NODENR,TXFIFOIN2            ;node id
        movff   GROUPNR,TXFIFOIN3
        setf    TXFIFOIN4                   ;unused
        setf    TXFIFOIN5                   ;unused
        setf    TXFIFOIN6                   ;unused
        setf    TXFIFOIN7                   ;unused
        movff   UPTIME3,TXFIFOIN8
        movff   UPTIME2,TXFIFOIN9
        movff   UPTIME1,TXFIFOIN10
        movff   UPTIME0,TXFIFOIN11
        call    WriteToCanTxFIFO
    bra        EndProcessSystemMessages

;-------------------------------------
SM114_HealthAsk
SM115_HealthAsk
        movlw   0x01                    
        xorwf   RXFIFO4,W
        bnz     $ + .6
        rcall   HealthAskRead
    bra    EndHealthAsk
        movlw   0x02                    
        xorwf   RXFIFO4,W
        bnz     $ + .6
        rcall   HealthAskClear
    bra    EndHealthAsk

EndHealthAsk
    bra        EndProcessSystemMessages

HealthAskRead                               ;read current & saved in eeprom values
    ;frame with current values
        movff   RXFIFO0,TXFIFOIN0           ;set frame type
        movff   RXFIFO1,TXFIFOIN1
        bsf     TXFIFOIN1,0                 ;response bit
        movff   NODENR,TXFIFOIN2            ;node id
        movff   GROUPNR,TXFIFOIN3
        movlw   0x01                        ;frame no 1 - current values
        movwf   TXFIFOIN4
        movff   RXCNT,TXFIFOIN5             ;rx fifo counter
        movff   TXCNT,TXFIFOIN6             ;tx fifo counter
        movff   RXCNTMX,TXFIFOIN7           ;rx fifo counter max
        movff   TXCNTMX,TXFIFOIN8           ;tx fifo counter max
        movff   CANINTCNT,TXFIFOIN9         ;CAN reinit counter
        movff   RXERRCNT,TXFIFOIN10         ;rx error counter
        movff   TXERRCNT,TXFIFOIN11         ;tx error counter
        call    WriteToCanTxFIFO
    ;frame with values from eeprom
        movlw   0x02                        ;frame no 2 - values from eeprom
        movwf   TXFIFOIN4
        setf    TXFIFOIN5                   ;not used
        setf    TXFIFOIN6                   ;not used
        lfsr    FSR0,TXFIFOIN7              ;start writing to register
        bcf     EECON1,EEPGD                ;access data EEPROM memory
        bcf     EECON1, CFGS                ;access data FLASH or EEPROM not Config Bits
        clrf    EEADRH                      ;set address
        movlw   0x2B                        ;access registers in eeprom
        movwf   EEADR                       ;move box address to EEADR
        movlw   0x05                
HealthAskLoop1
        bsf     EECON1,RD                   ;read data from EEPROM
        movff   EEDATA,POSTINC0             ;move EEPROM DATA to register indicated by FSR0 and increment FSR0
        incf    EEADR                       ;increment EEPROM address
        decfsz  WREG                        ;5 bytes read?
        bra     HealthAskLoop1
        call    WriteToCanTxFIFO
    return

HealthAskClear                              ;clear saved in eeprom values
        clrf    EEADRH                      ;set address
        movlw   0x2B                        ;access registers in eeprom
        movwf   EEADR                       ;move box address to EEADR
        banksel ROUT1
        movlw   0x05
        movwf   ROUT1
HealthToEepromLoop2
        clrf    WREG                        ;clear data
        call    EepromSaveWREG
        incf    EEADR                       ;increment address of EEPROM
        decfsz  ROUT1
        bra     HealthToEepromLoop2
        ;clear ram
        banksel RXCNTMXEE
        clrf    RXCNTMXEE
        clrf    TXCNTMXEE
        clrf    CANINTCNTEE
        clrf    RXERRCNTEE
    return

;-------------------------------------
SM116_ChannelName
SM117_ChannelName
    ;beginning of frame
        movff   RXFIFO0,TXFIFOIN0           ;set frame type
        movff   RXFIFO1,TXFIFOIN1
        bsf     TXFIFOIN1,0                 ;response bit
        movff   NODENR,TXFIFOIN2            ;node id
        movff   GROUPNR,TXFIFOIN3
    ;all channels
        movlw   0x00                        ;RXFIFO4 == 0x00?                   
        xorwf   RXFIFO4,W
        bnz     $ + .6                      ;no
        rcall   ChannelNameAll              ;yes
    bra    EndChannelName
    ;one channel
        movlw   CHANNELSNR                  ;RXFIFO4 > CHANNELSNR? 
        cpfsgt  RXFIFO4                                         
        rcall   ChannelNameOne              ;no
    bra    EndChannelName                   ;yes

EndChannelName
    bra        EndProcessSystemMessages


ChannelNameAll
    ;init start addess 0x008400
        clrf    TBLPTRU                 
        movlw   0x84
        movwf   TBLPTRH
        clrf    TBLPTRL
    ;init number of channels
        movlw   CHANNELSNR
        banksel ROUT4
        movwf   ROUT4
    ;init channel nr
        banksel TXFIFOIN4
        clrf    TXFIFOIN4                   ;channel 0
        rcall   ChNameChannelLoop
    return

ChannelNameOne
    ;init start addess
        movf    RXFIFO4,W                   ;channel -1
        decf    WREG
        mullw   .32                         ;(channel-1)*32
        clrf    TBLPTRU      
        movff   PRODH,TBLPTRH  
        movlw   0x84        
        addwf   TBLPTRH
        movff   PRODL,TBLPTRL
    ;init number of channels
        movlw   1                           ;only one channel
        banksel ROUT4
        movwf   ROUT4
    ;init channel nr and frame nr
        banksel RXFIFO4                     ;channel = RXFIFO-1 * 8
        decf    RXFIFO4,W
        banksel TXFIFOIN4
        movwf   TXFIFOIN4
        rlncf   TXFIFOIN4                   ;channel << 3                   
        rlncf   TXFIFOIN4
        rlncf   TXFIFOIN4
        rcall   ChNameChannelLoop
    return

ChNameChannelLoop                           ;sends 5 frames of channel name
    ;init number of frames per channel
        movlw   5
        banksel ROUT5
        movwf   ROUT5
    ;clear frame bits
        banksel TXFIFOIN4
        movlw   b'11111000'
        andwf   TXFIFOIN4    
    ;increment channel number
        movlw   8
        addwf   TXFIFOIN4
ChNameFrameLoop
    ;increment frame number
        banksel TXFIFOIN4
        incf    TXFIFOIN4
    ;read channel name
        rcall   ChNameReadFrameBytes
        call    WriteToCanTxFIFO
    ;frames done?
        banksel ROUT5
        decfsz  ROUT5
        bra     ChNameFrameLoop
    ;channels done?
        banksel ROUT4
        decfsz  ROUT4
        bra     ChNameChannelLoop
    return

ChNameReadFrameBytes                        ;prepares 7 or 4 bytes of single frame
    ;point at destination
        lfsr    FSR0,TXFIFOIN5              
    ;last frame of channel?
        movlw   1                       
        banksel ROUT5
        xorwf   ROUT5,W
        bz      $ + 6
        movlw   7                           ;no, so read 7 bytes
        bra     $ + .12
        movlw   4                           ;yes, so read only 4 bytes
        banksel TXFIFOIN9                   ;and clear last 3 bytes
        clrf    TXFIFOIN9
        clrf    TXFIFOIN10
        clrf    TXFIFOIN11
        ;read WREG bytes
        tblrd*+
        movff   TABLAT,POSTINC0
        decfsz  WREG
        bra     $ - 8
    return

;------------------------------------------------------------------------------
; PROCESS NORMAL MESSAGE
;------------------------------------------------------------------------------
ProcessNormalMessage

;------------------------------------------------------------------------------
; Routine:          CHECK AGAINST BOXES
;------------------------------------------------------------------------------
; Overview:         It compares received message to message saved in boxes
;------------------------------------------------------------------------------
CheckAgainstBoxes
        banksel BOXADR                      ;select bank
        setf    BOXADR                      ;first box minus 1 (range 0-127)

;get BOX by BOX
NextBOX    
        banksel BOXADR                      ;select bank
        incf    BOXADR,F                    ;point at next BOX
        movlw   .128                        ;check if it is last box
        cpfseq  BOXADR                      ;skip if BOXADR is euqal WREG    
        bra     $ + .4
    bra        EndCheckAgainstBoxes

;-----------------------
;check if BOX is enabled
        bcf     STATUS,C                    ;divide by .8 and add 0x40 to get adequate enable byte form copied eeprom in ram
        rrcf    BOXADR,W
        bcf     STATUS,C
        rrcf    WREG,W
        bcf     STATUS,C
        rrcf    WREG,W
        addlw   0x40
        lfsr    FSR0,0x200                  ;read value of condition register
        addwf   FSR0L                       ;add ofset in WREG
        movff   INDF0,BOXEN
        ;get bit of BOXEN
        movff   BOXADR,ROUT2
        movlw   0x8                         ;8 bits in enable reg for 8 boxes
        addwf   ROUT2,F                     ;beacuse next is subwf
SubEnBit
        subwf   ROUT2,F                     ;ROUTx has got bit position
        cpfslt  ROUT2                       ;is ROUTx less than 8?
        bra     SubEnBit                    ;no
        ;check bit
        rlncf   BOXEN,F                     ;to cancel first rotate right
        incf    ROUT2                       ;to cancel first decrementing
RotEnBit                                    ;totate BOXEN byte ROUTx times
        rrncf   BOXEN,F
        decfsz  ROUT2,F
        bra     RotEnBit
        btfss   BOXEN,0                     ;skip if box enabled
        bra     NextBOX                     ;box disabled

;-----------------------
;read 32 regs from flash
        clrf    TBLPTRU                     ;set address of flash
        movlw   .32                         ;start writing form first register of BOXxx
        mulwf   BOXADR                      ;get TBLPTR value 32xBOXADR (range 0x008800 - 0x0097E0)
        movff   PRODH,TBLPTRH
        movff   PRODL,TBLPTRL  
        movlw   0x88                        ;add 0x008800 to TBLPTR
        addwf   TBLPTRH
        lfsr    FSR0,BOXFIL1                ;start writing to first reg in BOX temp buffer
        movlw   .32                         ;32 registers            
ReadBox
        tblrd*+                             ;read and move to TABLAT and increment address
        movff   TABLAT,POSTINC0             ;move flash data to register indicated by FSR0 and increment FSR0
        decfsz  WREG                        ;all BOX bytes read?
        bra     ReadBox

;-----------------------
;compare box to received msg
        lfsr    FSR0,RXFIFO0                ;get first byte of RECEIVED MESSAGE (in temp FIFO buffer)
        lfsr    FSR1,BOXFIL1                ;get first byte of BOX FILTER
        lfsr    FSR2,BOXFIL1C               ;get first byte of BOX FILTER CONDITION
        movlw   .12                         ;compare 12 bytes
        movwf   ROUT2
CompareMsgsLoop
        movlw   0x00                        ;filter condition 0x00: "x" - byte doesn't need checking
        xorwf   INDF2,W
        bnz     $ + .4
        bra     EndCompareMsgsLoop 
        movlw   0x01                        ;filter condition 0x01: "=" - RXFIFO0 = BOXFIL1
        xorwf   INDF2,W
        bnz     $ + .4
        bra     CompareMsgs01
        movlw   0x02                        ;filter condition 0x02: "!" - RXFIFO0 != BOXFIL1
        xorwf   INDF2,W
        bnz     $ + .4
        bra     CompareMsgs02
        movlw   0x03                        ;filter condition 0x03: "<" - RXFIFO0 <= BOXFIL1
        xorwf   INDF2,W
        bnz     $ + .4
        bra     CompareMsgs03
        movlw   0x04                        ;filter condition 0x04: "<" - RXFIFO0 >= BOXFIL1
        xorwf   INDF2,W
        bnz     $ + .4
        bra     CompareMsgs04
        bra     NextBOX                     ;unknown condition
       
CompareMsgs01                               ;RXFIFO0 = BOXFIL1?
        movf    INDF0,W
        xorwf   INDF1,W
        bnz     NextBOX                     ;not truth
        bra     EndCompareMsgsLoop          ;truth
CompareMsgs02                               ;RXFIFO0 != BOXFIL1?
        movf    INDF0,W
        xorwf   INDF1,W
        bz      NextBOX                     ;not truth
        bra     EndCompareMsgsLoop          ;truth
CompareMsgs03                               ;RXFIFO0 <= BOXFIL1?
        movf    INDF0,W
        cpfslt  INDF1
        bra     EndCompareMsgsLoop          ;truth
        bra     NextBOX                     ;not truth
CompareMsgs04                               ;RXFIFO0 >= BOXFIL1?
        movf    INDF0,W
        cpfsgt  INDF1
        bra     EndCompareMsgsLoop          ;truth
        bra     NextBOX                     ;not truth

EndCompareMsgsLoop
        incf    FSR0L                       ;point at next bytes in BOX
        incf    FSR1L
        incf    FSR2L
        decfsz  ROUT2                 
        bra     CompareMsgsLoop

;-----------------------
;check if enable/disable instruction
CheckEnDis
        movlw   0xDC
        cpfsgt  BOXINSTR1
        bra     CopyINSTR
        call    EnableDisableBox
        banksel SAVEENBITS                  ;select bank
        setf    SAVEENBITS                  ;set flag "enable bits must be saved to EEPROM"
        bra     NextBOX                     ;check all boxes

CopyINSTR
        movff   BOXINSTR1,INSTR1            ;move instruction from box
        movff   BOXINSTR2,INSTR2
        movff   BOXINSTR3,INSTR3
        movff   BOXINSTR4,INSTR4
        movff   BOXINSTR5,INSTR5
        movff   BOXINSTR6,INSTR6
        movff   BOXINSTR7,INSTR7
        movff   BOXINSTR8,INSTR8

InstructionReady
        call    DoInstructionRequest        ;call do instruction in main program, and set delay if necessary
        bra     NextBOX

;-------------------------------------
EndCheckAgainstBoxes
        banksel SAVEENBITS
        tstfsz  SAVEENBITS                  ;skip if ENABLE BOXES don't need to be saved
        bra     SaveEnBits
        bra     ExitCheckAgainstBoxes
SaveEnBits
        lfsr    FSR0,ENBOX08                ;start writing from register ENBOX08
        clrf    EEADRH
        movlw   0x40                        ;start writing to reg 0x40 of EEPROM
        movwf   EEADR
        bcf     EECON1,EEPGD                ;access EEPROM
        bcf     EECON1,CFGS                 ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN                 ;enable writes
        banksel ROUT2
        movlw   .16                         ;16 bytes
        movwf   ROUT2
        bcf     INTCON,GIEH                 ;disable high interrupt
        bcf     INTCON,GIEL                 ;disable low interrupt
SaveEnBitsLoop
        movff   POSTINC0,EEDATA             ;move data indicated by FSR0 to EEDATA and increment FSR0
        movlw   0x55                        ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR                   ;write EEPROM
        btfss   PIR4,EEIF                   ;check if write is complited
        bra     $ - .2
        bcf     PIR4,EEIF                   ;clear flag
        incf    EEADR                       ;increment address of EEPROM
        banksel ROUT2
        decfsz  ROUT2
    bra        SaveEnBitsLoop
        bcf     EECON1,WREN                 ;disable writes    
        bsf     INTCON,GIEH                 ;enable high interrupt
        bsf     INTCON,GIEL                 ;enable low interrupt
        banksel SAVEENBITS
        clrf    SAVEENBITS                  ;clear flag "enable bits must be saved to EEPROM"

ExitCheckAgainstBoxes
    return

;------------------------------------------------------------------------------
; Routine:          ENABLE / DISABLE BOX
;------------------------------------------------------------------------------
; Overview:         It enables or disables boxes
;------------------------------------------------------------------------------
EnableDisableBox
        banksel BOXINSTR1
;Recognize instruction
        movlw   0xDD                        ;is this Instruction DD?
        xorwf   BOXINSTR1,W,BANKED
        bnz     $ + .8
        call    InstrDD
    return
        movlw   0xDE                        ;is this Instruction DE?
        xorwf   BOXINSTR1,W,BANKED
        bnz     $ + .8
        call    InstrDE
    return
        movlw   0xDF                        ;is this Instruction DF?
        xorwf   BOXINSTR1,W,BANKED
        bnz     $ + .6
        call    InstrDF
    return

;-------------------------------------
;Instruction execution
InstrDD                                     ;enable box
        call    IdentifyBOXES               ;get mask of boxes to change
        movff   ENBOX08,ROUT2               ;ENBOX08
        movf    BOXMASK1,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX08
        movff   ENBOX16,ROUT2               ;ENBOX16
        movf    BOXMASK2,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX16
        movff   ENBOX24,ROUT2               ;ENBOX24
        movf    BOXMASK3,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX24
        movff   ENBOX32,ROUT2               ;ENBOX32
        movf    BOXMASK4,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX32
        movff   ENBOX40,ROUT2               ;ENBOX40
        movf    BOXMASK5,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX40
        movff   ENBOX48,ROUT2               ;ENBOX48
        movf    BOXMASK6,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX48
        movff   ENBOX56,ROUT2               ;ENBOX56
        movf    BOXMASK7,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX56
        movff   ENBOX64,ROUT2               ;ENBOX64
        movf    BOXMASK8,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX64
        movff   ENBOX72,ROUT2               ;ENBOX72
        movf    BOXMASK9,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX72
        movff   ENBOX80,ROUT2               ;ENBOX80
        movf    BOXMASK10,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX80
        movff   ENBOX88,ROUT2               ;ENBOX88
        movf    BOXMASK11,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX88
        movff   ENBOX96,ROUT2               ;ENBOX96
        movf    BOXMASK12,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX96
        movff   ENBOX104,ROUT2              ;ENBOX104
        movf    BOXMASK13,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX104
        movff   ENBOX112,ROUT2              ;ENBOX112
        movf    BOXMASK14,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX112
        movff   ENBOX120,ROUT2              ;ENBOX120
        movf    BOXMASK15,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX120
        movff   ENBOX128,ROUT2              ;ENBOX128
        movf    BOXMASK16,W,BANKED
        iorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX128
    return
InstrDE                                     ;disable box
        call    IdentifyBOXES               ;get mask of boxes to change
        movf    BOXMASK1,W,BANKED           ;ENBOX08
        comf    WREG,A            
        movff   ENBOX08,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX08
        movf    BOXMASK2,W,BANKED           ;ENBOX16
        comf    WREG,A
        movff   ENBOX16,ROUT2
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX16
        movf    BOXMASK3,W,BANKED           ;ENBOX24
        comf    WREG,A
        movff   ENBOX24,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX24
        movf    BOXMASK4,W,BANKED           ;ENBOX32
        comf    WREG,A
        movff   ENBOX32,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX32
        movf    BOXMASK5,W,BANKED           ;ENBOX40
        comf    WREG,A
        movff   ENBOX40,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX40
        movf    BOXMASK6,W,BANKED           ;ENBOX48
        comf    WREG,A
        movff   ENBOX48,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX48
        movf    BOXMASK7,W,BANKED           ;ENBOX56
        comf    WREG,A
        movff   ENBOX56,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX56
        movf    BOXMASK8,W,BANKED           ;ENBOX64
        comf    WREG,A
        movff   ENBOX64,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX64
        movf    BOXMASK9,W,BANKED           ;ENBOX72
        comf    WREG,A
        movff   ENBOX72,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX72
        movf    BOXMASK10,W,BANKED          ;ENBOX80
        comf    WREG,A
        movff   ENBOX80,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX80
        movf    BOXMASK11,W,BANKED          ;ENBOX88
        comf    WREG,A
        movff   ENBOX88,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX88
        movf    BOXMASK12,W,BANKED          ;ENBOX96
        comf    WREG,A
        movff   ENBOX96,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX96
        movf    BOXMASK13,W,BANKED          ;ENBOX104
        comf    WREG,A
        movff   ENBOX104,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX104
        movf    BOXMASK14,W,BANKED          ;ENBOX112
        comf    WREG,A
        movff   ENBOX112,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX112
        movf    BOXMASK15,W,BANKED          ;ENBOX120
        comf    WREG,A
        movff   ENBOX120,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX120
        movf    BOXMASK16,W,BANKED          ;ENBOX128
        comf    WREG,A
        movff   ENBOX128,ROUT2    
        andwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX128
    return
InstrDF                                     ;toggle box
        call    IdentifyBOXES               ;get mask of boxes to change
        movff   ENBOX08,ROUT2               ;ENBOX08
        movf    BOXMASK1,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX08
        movff   ENBOX16,ROUT2               ;ENBOX16
        movf    BOXMASK2,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX16
        movff   ENBOX24,ROUT2               ;ENBOX24
        movf    BOXMASK3,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX24
        movff   ENBOX32,ROUT2               ;ENBOX32
        movf    BOXMASK4,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX32
        movff   ENBOX40,ROUT2               ;ENBOX40
        movf    BOXMASK5,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX40
        movff   ENBOX48,ROUT2               ;ENBOX48
        movf    BOXMASK6,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX48
        movff   ENBOX56,ROUT2               ;ENBOX56
        movf    BOXMASK7,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX56
        movff   ENBOX64,ROUT2               ;ENBOX64
        movf    BOXMASK8,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX64
        movff   ENBOX72,ROUT2               ;ENBOX72
        movf    BOXMASK9,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX72
        movff   ENBOX80,ROUT2               ;ENBOX80
        movf    BOXMASK10,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX80
        movff   ENBOX88,ROUT2               ;ENBOX88
        movf    BOXMASK11,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX88
        movff   ENBOX96,ROUT2               ;ENBOX96
        movf    BOXMASK12,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX96
        movff   ENBOX104,ROUT2              ;ENBOX104
        movf    BOXMASK13,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX104
        movff   ENBOX112,ROUT2              ;ENBOX112
        movf    BOXMASK14,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX112
        movff   ENBOX120,ROUT2              ;ENBOX120
        movf    BOXMASK15,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX120
        movff   ENBOX128,ROUT2              ;ENBOX128
        movf    BOXMASK16,W,BANKED
        xorwf   ROUT2,F,BANKED
        movff   ROUT2,ENBOX128
    return
;-------------------------------------
;  Identify boxes - creates masks in BOXMASK8 - BOXMASK128 (value '1' for boxes which need to be changed & value '0' for others)
IdentifyBOXES
        incf    BOXINSTR3                   ;increment start value
        movlw   .128                        ;128 bits to rotate
        movwf   ROUT2,BANKED
RotateMASK
        movlw   .120                        ;first 8 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK1          ;yes
        movlw   .112                        ;first 16 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK2          ;yes
        movlw   .104                        ;first 24 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK3          ;yes
        movlw   .96                         ;first 32 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK4          ;yes
        movlw   .88                         ;first 40 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK5          ;yes
        movlw   .80                         ;first 48 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK6          ;yes
        movlw   .72                         ;first 56 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK7          ;yes
        movlw   .64                         ;first 64 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK8          ;yes
        movlw   .56                         ;first 72 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK9          ;yes
        movlw   .48                         ;first 80 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK10         ;yes
        movlw   .40                         ;first 88 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK11         ;yes
        movlw   .32                         ;first 96 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK12         ;yes
        movlw   .24                         ;first 104 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK13         ;yes
        movlw   .16                         ;first 112 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK14         ;yes
        movlw   .8                          ;first 120 bits passed?
        cpfseq  ROUT2,BANKED
        bra     $+4
        movff   BOXMASK16,BOXMASK15         ;yes
        rcall   FillMASK
        decfsz  ROUT2,BANKED
        bra     RotateMASK
    return

;rotate subroutine
FillMASK
        tstfsz  BOXINSTR2
        bra     $+4        
        bra     BetweenBOXINSTR2and3
        ;rotate '0'
        bcf     STATUS,C,A                  ;below value of BOXINSTR2 all bits are '0'
        rrcf    BOXMASK16,BANKED
        decf    BOXINSTR2
    return
BetweenBOXINSTR2and3
        tstfsz  BOXINSTR3
        bra     $+4
        bra     AfterBOXINSTR3
        ;rotate '1'
        bsf     STATUS,C,A                  ;from value of BOXINSTR2 all bits are '1'
        rrcf    BOXMASK16,BANKED
        decf    BOXINSTR3
    return
AfterBOXINSTR3
        bcf     STATUS,C,A                  ;above value of BOXINSTR2+BOXINSTR3 all bits are '0'
        rrcf    BOXMASK16,BANKED
    return


;==============================================================================
;==============================================================================
;       P  E  R  I  O  D  I  C  A  L    1 0 0 0  ms    R  O  U  T  I  N  E  S
;==============================================================================
;==============================================================================

;------------------------------------------------------------------------------
; Routine:          INTERRUPT TIMER INITIALIZATION 8MHz
;------------------------------------------------------------------------------
; Overview:         It initializes 1000ms periodical interrupt for 8MHz clock
;------------------------------------------------------------------------------
Timer0Initialization8MHz:
        banksel PMD1
        bcf     PMD1,TMR0MD                 ;enable timer 0
        movlw   0x0B                        ;set 999.994 ms for 8MHz clock
        movwf   TMR0H
        movlw   0xDE
        movwf   TMR0L
        movlw   b'10000100'                 ;timer on, 16bit, prescaler 32
        movwf   T0CON
        bsf     INTCON2,TMR0IP              ;high priority for interrupt
        bcf     INTCON,TMR0IF               ;clear timer's flag
        bsf     INTCON,TMR0IE               ;interrupt on
    return

;------------------------------------------------------------------------------
; Routine:          INTERRUPT TIMER INITIALIZATION 32MHz
;------------------------------------------------------------------------------
; Overview:         It initializes 1000ms periodical interrupt for 32MHz clock
;------------------------------------------------------------------------------
Timer0Initialization32MHz:
        banksel PMD1
        bcf     PMD1,TMR0MD                 ;enable timer 0
        movlw   0x0B                        ;set 1000.006 ms for 32MHz clock
        movwf   TMR0H
        movlw   0xDC
        movwf   TMR0L
        movlw   b'10000110'                 ;timer on, 16bit, prescaler 128
        movwf   T0CON
        bsf     INTCON2,TMR0IP              ;high priority for interrupt
        bcf     INTCON,TMR0IF               ;clear timer's flag
        bsf     INTCON,TMR0IE               ;interrupt on
    return

;------------------------------------------------------------------------------
; Routine:          UPDATE TIMER REGISTERS
;------------------------------------------------------------------------------
; Overview:         It decrements TIMER register values for each channel.
;                   Routine must be run every 1s. If timer decrements to ZERO
;                   then DoInstructionRequest is called
;------------------------------------------------------------------------------
UpdateDelayTimers
    ;channel 1
        lfsr    FSR0,Instr1Ch1              ;get FROM channel 1 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer              ;update SUBITMERs and TIMER registers
        lfsr    FSR1,TimerCh1               ;set TO channel 1 registers (only timer regs)
        call    CopyToChannelRegisters
    ;channel 2
        lfsr    FSR0,Instr1Ch2              ;channel 2 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh2
        call    CopyToChannelRegisters
    ;channel 3
        lfsr    FSR0,Instr1Ch3              ;channel 3 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh3 
        call    CopyToChannelRegisters
    ;channel 4
        lfsr    FSR0,Instr1Ch4              ;channel 4 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer 
        lfsr    FSR1,TimerCh4
        call    CopyToChannelRegisters
    ;channel 5
        lfsr    FSR0,Instr1Ch5              ;channel 5 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh5
        call    CopyToChannelRegisters
    ;channel 6
        lfsr    FSR0,Instr1Ch6              ;channel 6 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer 
        lfsr    FSR1,TimerCh6
        call    CopyToChannelRegisters
    ;channel 7
        lfsr    FSR0,Instr1Ch7              ;channel 7 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh7
        call    CopyToChannelRegisters
    ;channel 8
        lfsr    FSR0,Instr1Ch8              ;channel 8 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh8
        call    CopyToChannelRegisters
    ;channel 9
        lfsr    FSR0,Instr1Ch9              ;channel 9 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh9
        call    CopyToChannelRegisters
    ;channel 10
        lfsr    FSR0,Instr1Ch10             ;channel 10 registers
        call    CopyFromChannelRegisters
        call    DecrementTimer
        lfsr    FSR1,TimerCh10
        call    CopyToChannelRegisters
ExitUpdateDelayTimers
    return

;-------------------------------
CopyFromChannelRegisters:
        lfsr    FSR1,INSTR1                 ;TO reg starting from INSTR1
        movlw   .11                         ;read 11 successive registers
CFChRLoop:
        movff   POSTINC0,POSTINC1           ;move data from receive buffer to destination buffer
        decfsz  WREG                        ;all 11 bytes read?
        bra     CFChRLoop
    return

;-------------------------------
CopyToChannelRegisters:
        lfsr    FSR0,TIMER                  ;FROM reg starting from INSTR1
        movlw   .3                          ;read 3 successive registers
CTChRLoop:
        movff   POSTINC0,POSTINC1           ;move data from receive buffer to destination buffer
        decfsz  WREG                        ;all 3 bytes read?
        bra     CTChRLoop
    return

;-------------------------------
DecrementTimer
        banksel TIMER                       ;choose right bank
    ;DecrementTimeCheckSUBTIMER1
        tstfsz  SUBTIMER1
        bra     $ + .4
        bra     $ + .6
        decf    SUBTIMER1
        bra     ExitDecrementTimer
    ;DecrementTimeCheckSUBTIMER2
        tstfsz  SUBTIMER2
        bra     $ + .4
        bra     $ + .8        
        decf    SUBTIMER2                
        setf    SUBTIMER1                   ;set another 256 steps of sub1
        bra     ExitDecrementTimer
    ;DecrementTimeCheckTIMER
        tstfsz  TIMER
        bra     $ + .4
        bra     ExitDecrementTimer    
        dcfsnz  TIMER                       ;dec delay time register
        bra     $ + .6
        rcall   SetTimer
        bra     ExitDecrementTimer
        call    DoInstructionRequest
ExitDecrementTimer
    return

;------------------------------------------------------------------------------
; Routine:          SET TIMER
;------------------------------------------------------------------------------
; Overview:         It sets SUBTIMERx registers according to value of TIMER
;------------------------------------------------------------------------------
SetTimer
        banksel TIMER                       ;choose right bank
        movlw   .60                         ;1-60s, step 1s
        cpfsgt  TIMER                       ;skip if f>W
        bra     Delay_1s
        movlw   .108                        ;1-5min, step 5s
        cpfsgt  TIMER                       ;skip if f>W
        bra     Delay_5s
        movlw   .163                        ;5-60min, step 1min
        cpfsgt  TIMER                       ;skip if f>W
        bra     Delay_1m
        bra     Delay_15m                   ;1-20h, step 15min
Delay_1s
        clrf    SUBTIMER1
        clrf    SUBTIMER2
        movf    TMR0L,W                     ;read TMR0L to refresh TMR0H for the trick
        movlw   0x85                        ;trick - if first load is when timer is <0,5s to next overflow
        cpfslt  TMR0H                       ;then increment SUBTIMER1 (it gives delay = 1.5s not 0.5s)
        incf    SUBTIMER1
        bra     ExitSetTimer
Delay_5s
        movlw   .4                          ;5s
        movwf   SUBTIMER1
        clrf    SUBTIMER2
        bra     ExitSetTimer
Delay_1m
        movlw   .59                         ;60s
        movwf   SUBTIMER1
        clrf    SUBTIMER2
        bra     ExitSetTimer
Delay_15m
        movlw   .131                        ;900s
        movwf   SUBTIMER1
        movlw   .3
        movwf   SUBTIMER2
        bra     ExitSetTimer
ExitSetTimer
    return

;------------------------------------------------------------------------------
; Routine:          UPDATE UPTIME REGISTERS
;------------------------------------------------------------------------------
; Overview:         Increments up time counter
;------------------------------------------------------------------------------
UpdateUpTime
        banksel UPTIME0
        incfsz  UPTIME0
        bra     $ + .4 
        incfsz  UPTIME1                     ;equal 0, so increment reg 1
        bra     $ + .4 
        incfsz  UPTIME2                     ;equal 0, so increment reg 2
        bra     $ + .4 
        incf    UPTIME3                     ;equal 0, so increment reg 3
ExitUpTime
    return

;------------------------------------------------------------------------------
; Routine:          TRANSMISSION TIMER
;------------------------------------------------------------------------------
; Overview:         It does CAN engine reinitialization if 2 messages are  
;                   waiting in transmit buffers for free bus longer than 10s
;                   and another message is in the queue to be transmitted.
;------------------------------------------------------------------------------
UpdateTransmitTimer                         ;increments timer which counts time from last transmission
        banksel TRSMTTIMER
        infsnz  TRSMTTIMER                  ;skip if not zero
        decf    TRSMTTIMER                  ;go back to 0xFF if was zero
    return
;-------------------------------
CheckTransmitTimer                          ;checks for how long TX buffers are busy
        banksel TRSMTTIMER
        movlw   .10                         ;check if TransmitTimer < 10 means TX buffers are busy less then 10s
        cpfslt  TRSMTTIMER                  ;skip if yes
        call    CanReinitialization         ;the msg couldn't been sent for 9s, so reinit
    return

;==============================================================================
;==============================================================================
;       O   T   H   E   R      R   O   U   T   I   N   E   S
;==============================================================================
;==============================================================================

;------------------------------------------------------------------------------
; Routine:          UPDATE HEALTH CHECK REGISTERS
;------------------------------------------------------------------------------
; Overview:         Saves maximum values of health regs into eeprom
;------------------------------------------------------------------------------
UpdateHealthRegs
        banksel RXCNTMXEE                   ;eeprom bank
        ;RXCNTMXEE                          ;can rx fifo buffer level
        movff   RXCNTMX,WREG                ;take current value
        cpfslt  RXCNTMXEE                   ;compare to saved in eeprom, skip if RXCNTMXEE<RXCNTMX
        bra     $ + .14                     ;do not save
        movwf   RXCNTMXEE                   ;save as max value
        movlw   0x2B                        ;set address
        movwf   EEADR
        movf    RXCNTMXEE,W                 ;take max value
        rcall   HealthToEepromSaveNow
    bra     ExitUpdateHealthRegs    
        ;TXCNTMXEE                          ;can tx fifo buffer level
        movff   TXCNTMX,WREG                ;take current value
        cpfslt  TXCNTMXEE                   ;compare saved in eeprom, skip if TXCNTMXEE>TXCNTMX
        bra     $ + .14                     ;do not save
        movwf   TXCNTMXEE                   ;save as max value
        movlw   0x2C                        ;set address
        movwf   EEADR
        movf    TXCNTMXEE,W                 ;take max value
        rcall   HealthToEepromSaveNow    
    bra     ExitUpdateHealthRegs    
        ;CANIntCntEE                        ;can reinitialization
        movff   CANINTCNT,WREG              ;take current value
        cpfslt  CANINTCNTEE                 ;compare saved in eeprom, skip if CANINTCNTEE>CANINTCNT
        bra     $ + .14                     ;do not save
        movwf   CANINTCNTEE                 ;save as max value
        movlw   0x2D                        ;set address
        movwf   EEADR
        movf    CANINTCNTEE,W               ;take max value
        rcall   HealthToEepromSaveNow    
    bra     ExitUpdateHealthRegs    
        ;RXERRCNTEE                         ;can rx errors
        movff   RXERRCNT,WREG               ;take current value
        cpfslt  RXERRCNTEE                  ;compare saved in eeprom, skip if RXERRCNTEE>RXERRCNT
        bra     $ + .14                     ;do not save
        movwf   RXERRCNTEE                  ;save as max value
        movlw   0x2E                        ;set address
        movwf   EEADR
        movf    RXERRCNTEE,W                ;take max value
        rcall   HealthToEepromSaveNow
    bra     ExitUpdateHealthRegs    
        ;TXERRCNTEE                         ;can tx errors
        movff   TXERRCNT,WREG               ;take current value
        cpfslt  TXERRCNTEE                  ;compare saved in eeprom, skip if TXERRCNTEE>TXERRCNT
        bra     $ + .14                     ;do not save
        movwf   TXERRCNTEE                  ;save as max value
        movlw   0x2F                        ;set address
        movwf   EEADR
        movf    TXERRCNTEE,W                ;take max value
        rcall   HealthToEepromSaveNow
    bra     ExitUpdateHealthRegs

ExitUpdateHealthRegs
    return

;-------------------------------
HealthToEepromSaveNow
        clrf    EEADRH                      ;point at address
        rcall   EepromSaveWREG              ;save to eeprom
    return

;------------------------------------------------------------------------------
; Routine:          ENABLE / DISABLE INTERRUPTS
;------------------------------------------------------------------------------
; Overview:         Enables or disables high and low interrupts
;------------------------------------------------------------------------------
DisAllInt
        movff   INTCON,INTCON_SHD           ;save status to shadow register
        bcf     INTCON,GIEH                 ;disable high interrupt
        bcf     INTCON,GIEL                 ;disable low interrupt
    return
;-------------------------------
ReEnAllInt
        banksel INTCON_SHD
        btfsc   INTCON_SHD,GIEH             ;was high interrupt enabled?
        bsf     INTCON,GIEH                 ;yes, so enable it
        btfsc   INTCON_SHD,GIEL             ;was low interrupt enabled?
        bsf     INTCON,GIEL                 ;yes, so enable it
    return
;-------------------------------
EnAllInt
        bsf     INTCON,GIEH                 ;enable high interrupt
        bsf     INTCON,GIEL                 ;enable low interrupt
    return

;------------------------------------------------------------------------------
; Routine:          SAVE TO EEPROM ROUTINE
;------------------------------------------------------------------------------
; Overview:         It saves data held in WREG into previously set EEPROM
;                   address
;
;   Intput:         EEADRH & EEADR - address of eeprom memory cell to be written
;                   WREG - data to be saved
;------------------------------------------------------------------------------
EepromSetADRH                               ;set eeprom high address
        movwf   EEADRH
    return
;-------------------------------
EepromSetADRL                               ;set eeprom low address
        movwf   EEADR
    return
;-------------------------------
EepromSaveWREG    
        movwf   EEDATA                      ;set data
        bcf     EECON1,EEPGD                ;access EEPROM
        bcf     EECON1,CFGS                 ;access EEPROM and FLASH, not CONF_BIT
        bsf     EECON1,WREN                 ;enable writes
        call    DisAllInt                   ;disable interrupts
        movlw   0x55                        ;required sequence
        movwf   EECON2
        movlw   0xAA
        movwf   EECON2
        bsf     EECON1,WR                   ;write EEPROM
        btfsc   EECON1,WR                   ;wait for write cycle to complete
        bra     $ - .2
        call    ReEnAllInt                  ;reenable interrupts
        btfss   PIR4,EEIF                   ;wait for write operation to complete
        bra     $ - .2
        bcf     PIR4,EEIF                   ;clear flag
        bcf     EECON1,WREN                 ;disable writes
    return

;------------------------------------------------------------------------------
; Routine:          READ FROM EEPROM ROUTINE
;------------------------------------------------------------------------------
; Overview:         Reads eeprom cell
;
;    Input:         EEADRH and EEADR must be set as required before calling this
;                   routine
;   Output:         WREG - value of read eeprom register
;------------------------------------------------------------------------------
EepromReadToWREG
        bcf     EECON1,EEPGD                ;access data EEPROM memory
        bcf     EECON1,CFGS                 ;access data FLASH or EEPROM
        bsf     EECON1,RD                   ;read data from EEPROM
        movff   EEDATA,WREG                 ;move EEPROM DATA to WREG
    return

;------------------------------------------------------------------------------
; Routine:          COMPUTED GOTO
;------------------------------------------------------------------------------
; Overview:         It changes program counter according to input value of WREG
;                   using this calculation PC = PCin + 2*WREG
;
; Example:          movlw   0x01
;                   call    ComputedGoto
;                   bra     Jump00          (only single-word instruction)
;                   bra     Jump01
;
;    Input:         WREG (0-255) - jump index
;   Output:         PC - program counter changed according to jump index
;------------------------------------------------------------------------------
ComputedGoto
        addwf   TOSL                        ;add WREG to stack
        btfsc   STATUS,C                    ;overflow?    
        incf    TOSH                        ;yes, so increment stack high byte
        btfsc   STATUS,C                    ;overflow?
        incf    TOSU                        ;yes, so increment stack upper byte
        addwf   TOSL                        ;add WREG again to stack for single-word instruction (2 byte)
        btfsc   STATUS,C
        incf    TOSH
        btfsc   STATUS,C
        incf    TOSU    
    return
