@REM This batch file has been generated by the IAR Embedded Workbench
@REM C-SPY Debugger, as an aid to preparing a command line for running
@REM the cspybat command line utility using the appropriate settings.
@REM
@REM Note that this file is generated every time a new debug session
@REM is initialized, so you may want to move or rename the file before
@REM making changes.
@REM
@REM You can launch cspybat by typing the name of this batch file followed
@REM by the name of the debug file (usually an ELF/DWARF or UBROF file).
@REM
@REM Read about available command line parameters in the C-SPY Debugging
@REM Guide. Hints about additional command line parameters that may be
@REM useful in specific cases:
@REM   --download_only   Downloads a code image without starting a debug
@REM                     session afterwards.
@REM   --silent          Omits the sign-on message.
@REM   --timeout         Limits the maximum allowed execution time.
@REM 


"F:\a_hai\Program Files\IAR\9.10.1\common\bin\cspybat" "F:\a_hai\Program Files\IAR\9.10.1\8051\bin\8051proc.dll" "F:\a_hai\Program Files\IAR\9.10.1\8051\bin\8051emu_cc.dll"  %1 --plugin "F:\a_hai\Program Files\IAR\9.10.1\8051\bin\8051bat.dll" --backend -B "--proc_core" "plain" "--proc_code_model" "banked" "--proc_nr_virtual_regs" "16" "--proc_pdata_bank_reg_addr" "0x93" "--proc_dptr_nr_of" "1" "--proc_codebank_reg" "0x9F" "--proc_codebank_start" "0x8000" "--proc_codebank_end" "0xFFFF" "--proc_codebank_mask" "0xFF" "--proc_data_model" "large" "-p" "F:\a_hai\Program Files\IAR\9.10.1\8051\config\devices\Texas Instruments\ioCC2540F256.ddf" "--proc_exclude_exit_breakpoint" "--proc_driver" "chipcon" "--erase_flash" "--verify_download" "use_crc16" "--stack_overflow" "--number_of_banks" "4" 


