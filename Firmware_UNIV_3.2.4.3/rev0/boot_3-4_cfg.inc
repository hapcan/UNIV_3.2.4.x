	NOLIST
;==============================================================================
;
;    Filename:				boot_config_3-4.inc
;    Date:					March 2013
;
;    Author:                Jacek Siwilo
;    Company:               hapcan.com 
;    Description:			Bootloders's definitions for UNIV 3
;		
;==============================================================================


;==============================================================================
;===  BOOTLOADER REGISTERS in BANK 1  =========================================
;==============================================================================

BootRAM     EQU     0x01        ;RAM for bootloader
;==========================
;CAN RX buffer
bRXBCON		EQU		H'0100'
bRXBSIDH	EQU		H'0101'
bRXBSIDL	EQU		H'0102'
bRXBEIDH	EQU		H'0103'
bRXBEIDL	EQU		H'0104'
bRXBDLC		EQU		H'0105'
bRXBD0		EQU		H'0106'
bRXBD1		EQU		H'0107'
bRXBD2		EQU		H'0108'
bRXBD3		EQU		H'0109'
bRXBD4		EQU		H'010A'
bRXBD5		EQU		H'010B'
bRXBD6		EQU		H'010C'
bRXBD7		EQU		H'010D'
bRXBFULL    EQU     H'010E'     ;CAN buffer ful flag
bFIRMFLAG   EQU     H'010F'     ;firmware started completely flag
;==========================
;uart RX buffer
bRB0		EQU		H'0110'
bRB1		EQU		H'0111'
bRB2		EQU		H'0112'
bRB3		EQU		H'0113'
bRB4		EQU		H'0114'
bRB5		EQU		H'0115'
bRB6		EQU		H'0116'
bRB7		EQU		H'0117'
bRB8		EQU		H'0118'
bRB9		EQU		H'0119'
bRB10		EQU		H'011A'
bRB11		EQU		H'011B'
bRB12		EQU		H'011C'
bRB13		EQU		H'011D'
bRB14		EQU		H'011E'
bRB15		EQU		H'011F'
bRB16		EQU		H'0120'
bRB17		EQU		H'0121'
bRB18		EQU		H'0122'
bRB19		EQU		H'0123'
bRB20		EQU		H'0124'
bRB21		EQU		H'0125'
bRB22		EQU		H'0126'
bRB23		EQU		H'0127'
bRB24		EQU		H'0128'
bRB25		EQU		H'0129'
bRB26		EQU		H'012A'
bRB27		EQU		H'012B'
bRB28		EQU		H'012C'
bRB29		EQU		H'012D'
bRBOVF		EQU		H'012E'
bRBCNT		EQU		H'012F'
;==========================
;CAN TX buffer
bTXBCON		EQU		H'0130'
bTXBSIDH	EQU		H'0131'
bTXBSIDL	EQU		H'0132'
bTXBEIDH	EQU		H'0133'
bTXBEIDL	EQU		H'0134'
bTXBDLC		EQU		H'0135'
bTXBD0		EQU		H'0136'
bTXBD1		EQU		H'0137'
bTXBD2		EQU		H'0138'
bTXBD3		EQU		H'0139'
bTXBD4		EQU		H'013A'
bTXBD5		EQU		H'013B'
bTXBD6		EQU		H'013C'
bTXBD7		EQU		H'013D'
;==========================
;uart TX buffer
bTB0		EQU		H'0140'
bTB1		EQU		H'0141'
bTB2		EQU		H'0142'
bTB3		EQU		H'0143'
bTB4		EQU		H'0144'
bTB5		EQU		H'0145'
bTB6		EQU		H'0146'
bTB7		EQU		H'0147'
bTB8		EQU		H'0148'
bTB9		EQU		H'0149'
bTB10		EQU		H'014A'
bTB11		EQU		H'014B'
bTB12		EQU		H'014C'
bTB13		EQU		H'014D'
bTB14		EQU		H'014E'
bTB15		EQU		H'014F'
;==========================
;general use bootloader registers
bR0		    EQU		H'0150'     ;NORMAL check firmware
bR1		    EQU		H'0151'     ;NORMAL UART receive msg;
bR2		    EQU		H'0152'     ;NORMAL UART receive msg; UART receive msg recog; 
bR3		    EQU		H'0153'     ;NORMAL UART transmit
bR4		    EQU		H'0154'     ;NORMAL check if pll; PROGR check if pll
bR5		    EQU		H'0155'     ;NORMAL check if pll; PROGR check if pll
bR6		    EQU		H'0156'     ;NORMAL boot indicat; PROGR boot indicat
bR7		    EQU		H'0157'     ;NORMAL boot indicat; PROGR boot indicat
bR8		    EQU		H'0158'     ;NORMAL boot indicat; PROGR boot indicat
bR9		    EQU		H'0159'     ;                   
bR10	    EQU		H'015A'     ;                     PROGR CAN program eeprom
bR11	    EQU		H'015B'     ;                     PROGR UART program eeprom
bR12	    EQU		H'015C'     ;                     PROGR UART transmit
bR13	    EQU		H'015D'     ;                     PROGR UART receive msg
bR14	    EQU		H'015E'     ;                     PROGR UART receive msg reco
bR15	    EQU		H'015F'     ;                     PROGR UART receive msg reco
;==========================
;bFLAGS1 bits
FirmError	EQU		H'0000'		;firmware error
FirmWrongChsm EQU	H'0002'		;firmware wrong checksum
FirmWrongType EQU	H'0003'		;firmware wrong type
;other bootloader registers
bFLAGS1		EQU		H'0160'		
bChSum2		EQU		H'0161'		;counted checksum by bootloader
bChSum1		EQU		H'0162'
bChSum0		EQU		H'0163'
bFSum2		EQU		H'0164'		;checksum written in firmware
bFSum1		EQU		H'0165'
bFSum0		EQU		H'0166'
bFTyp3		EQU		H'0167'		;firmware type written in firmware
bFTyp2		EQU		H'0168'
bFTyp1		EQU		H'0169'
bFTyp0		EQU		H'016A'
bType3		EQU		H'016B'		;firmware type written in ID
bType2		EQU		H'016C'
bType1		EQU		H'016D'
bType0		EQU		H'016E'
bUART		EQU		H'016F'     ;uart enabled flag
;==========================
;bootloader shadow registers for high interrupt
bSTATUS_H   EQU     H'0170'
bWREG_H     EQU     H'0171'
bBSR_H      EQU     H'0172'
bPIR1_H     EQU     H'0173'
bPIR4_H     EQU     H'0174'
bPIR5_H     EQU     H'0175'
bCANSTAT_H  EQU     H'0176'
bCANCON_H   EQU     H'0177'
bFSR0L_H    EQU		H'0178'
bFSR0H_H    EQU		H'0179'
bFSR1L_H    EQU		H'017A'
bFSR1H_H    EQU		H'017B'
bTABLAT_H   EQU		H'017C'
bTBLPTRL_H  EQU		H'017D'
bTBLPTRH_H  EQU		H'017E'
bTBLPTRU_H  EQU		H'017F'
bINTCON_H   EQU		H'0180'
bEECON1_H   EQU		H'0181'
bEEDATA_H   EQU		H'0182'
bEEADRH_H   EQU		H'0183'
bEEADR_H    EQU		H'0184'
bTRISA_H    EQU		H'0185'
bADCON2_H   EQU     H'0186'
bADCON1_H   EQU     H'0187'
bADCON0_H   EQU     H'0188'
bANCON0_H   EQU     H'0189'
bADRESL_H	EQU     H'018A'
bADRESH_H	EQU     H'018B'
bRCSTA1_H	EQU     H'018C'
bTXSTA1_H	EQU     H'018D'
bPMD1_H     EQU     H'018E'
bCOMSTAT_H  EQU     H'018F'
;==========================
;bootloader shadow registers for low interrupt
bSTATUS_L   EQU     H'0190'
bWREG_L     EQU     H'0191'
bBSR_L      EQU     H'0192'
bPIR1_L     EQU     H'0193'
bPIR4_L     EQU     H'0194'
bPIR5_L     EQU     H'0195'
bCANSTAT_L  EQU     H'0196'
bCANCON_L   EQU     H'0197'
bFSR0L_L    EQU		H'0198'
bFSR0H_L    EQU		H'0199'
bFSR1L_L    EQU		H'019A'
bFSR1H_L    EQU		H'019B'
bTABLAT_L   EQU		H'019C'
bTBLPTRL_L  EQU		H'019D'
bTBLPTRH_L  EQU		H'019E'
bTBLPTRU_L  EQU		H'019F'
bINTCON_L   EQU		H'01A0'
bEECON1_L   EQU		H'01A1'
bEEDATA_L   EQU		H'01A2'
bEEADRH_L   EQU		H'01A3'
bEEADR_L    EQU		H'01A4'
bTRISA_L    EQU		H'01A5'
bADCON2_L   EQU     H'01A6'
bADCON1_L   EQU     H'01A7'
bADCON0_L   EQU     H'01A8'
bANCON0_L   EQU     H'01A9'
bADRESL_L	EQU     H'01AA'
bADRESH_L	EQU     H'01AB'
bRCSTA1_L	EQU     H'01AC'
bTXSTA1_L	EQU     H'01AD'
bPMD1_L     EQU     H'01AE'
bCOMSTAT_L  EQU     H'01AF'


;==============================================================================
;===  PROCESSOR ID FIELDS  ====================================================
;==============================================================================
	__idlocs _IDLOC0,	HARD1
	__idlocs _IDLOC1,	HARD2
	__idlocs _IDLOC2,	HVERS
	__idlocs _IDLOC3,	0xFF    ;unused
	__idlocs _IDLOC4,	ID0
	__idlocs _IDLOC5,	ID1
	__idlocs _IDLOC6,	ID2
	__idlocs _IDLOC7,	ID3
;==============================================================================
    LIST
