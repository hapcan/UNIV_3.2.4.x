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
;    Filename:              boot_3-4.asm
;    Date:                  March 2013
;    Author:                Jacek Siwilo
;    Description:           ASM file for UNIV 3 bootloader
;==============================================================================
;===  NODE SERIAL NUMBER  =====================================================
;==============================================================================
    #define     ID0     0x00            ;node serial number MSB
    #define     ID1     0x00            ;node serial number
    #define     ID2     0x00            ;node serial number
    #define     ID3     0x01            ;node serial number LSB
;==============================================================================
;===  NEEDED FILES  ===========================================================
;==============================================================================
    LIST P=18F26K80                     ;directive to define processor
    #include <P18F26K80.INC>            ;processor specific variable definitions
    #include "boot_3-4.inc"             ;bootloader
    #include "boot_3-4_cfg.inc"         ;bootloader config file
;==============================================================================
    END