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


| Message | Description |
| --------- | ----------- |
| [0x00, 0x00, 0x00, 0x00, 0x00] | reset | 
| [0xC1, 0x00, 0x00, 0x84, 0x80] | Set Trigger Values. Falling channels: 0, 5 Rising channels:  0 |
| [0xC1, 0x00, 0x00, 0x84, 0x80] | Set Trigger Values. Falling channels: 0, 5 Rising channels:  0 |
| [0x80, 0xE7, 0x03, 0x00, 0x00] | Set Divisor value: divisor=15,139,584, but out of order |
| **Large Break** | Probably due to a function or scope change in libsigrok |
| [0x81, 0xFF, 0x1F, 0xFF, 0x1F] | Read delay. read counter=4,280,287,007, delay counter=0 |
| [0x82, 0x3A, 0x00, 0x00, 0x00] | I'm guessing 0x82 is a continuation of 0x81 |
| [0x01, 0x01, 0x01, 0x01, 0x01] | Arm the fucking trigger |
| **Capture Begins** | |

