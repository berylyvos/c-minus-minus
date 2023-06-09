# c-minus-minus

## How It Works
- Front & Back end in one
  + NO OPTIMIZATION
- One-pass Parser
  + source code stream -> parse & codegen -> VM code
- Minimalist Virtual Machine
  + Calculations based on stack and registers
  + Register: `pc` | `sp` | `bp` | `ax`
  + Memory: `code` | `data` | `stack`
  + Instruction Set
    - Save & Load: `IMM` | `LEA` | `LC` | `SC` | `LI` | `SI` | `PUSH`
    - Arithmetic & Logical Operations: `ADD` | `SUB` | `MUL` | `DIV` | `MOD` | `OR` | `XOR` | `AND` | `SHL` | `SHR` | `EQ` | `NE` | `LT` | `LE` | `GT` | `GE`
    - Control: `JMP` | `JZ` | `JNZ` | `CALL` | `NVAR` | `DARG` | `RET`
    - Native-Call: `OPEN` | `CLOS` | `READ` | `PRTF` | `MALC` | `FREE` | `MSET` | `MCMP` | `EXIT`