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
  + Instruction Set: Save & Load | Arithmetic & Logical Operations | Control | Native-Call