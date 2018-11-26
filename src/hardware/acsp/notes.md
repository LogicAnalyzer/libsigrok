OPCODE | Description | Command (Following Four Bits)
--------------------- | ----------------------------- | -------------------------------------------------------------------------------------------
0x00 | Reset | Doesn't Matter
0x01 | Arm Trigger | Doesn't Matter
0x02 | Query ID | Doesn't Matter
0x04 | Query Metadata | Doesn't Matter
0x05 | Finish Now (TBD) | Doesn't Matter
0x07 | Poll/Query Analyzer State (TBD) | TBD
0x08 | Return Capture Data (TBD) | Doesn't Matter
0x80 | Set Sample Rate | 24 bit value for DIVIDER see https://github.com/LogicAnalyzer/documentation/blob/master/fpga/modules/sampler/sampler.md
0x81 | Set Read/Delay Count | "Command [31:16] is the number of READ samples to take, Command[15:0] is the number of delay samples."
0xC0 | Set Basic Trigger Mask(TBD) | Not yet implemented.
0xC1 | Set Basic Trigger Value | "command[15:8] enables channels 7-0 for falling edge detection |  command [7:0] enables channels 7-0 for rising edge detection |  both rising and falling supported."


Pretriger Capture Issue
|| Message || Description ||
| ---- | -------- |
| 00| RESET |
| C1,00,00,01,00 | Set channel 0 to falling edge triggered|
| C1,00,00,01,00 | Set channel 0 to falling edge triggered|
| 80,00,00,03,E7 | Set sample rate to 100KHz |
| 81,00,5E,00,83 | Read count = 94, Delay Count = 131 |
| 82,38,00,00,00 | Set flags |
| 01 | Start the fucking capture |