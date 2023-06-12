# c-minus-minus

## How It Works
### Front & Back end in one
  + NO OPTIMIZATION
### One-pass Parser
  + source code -> |Lexer| -> token stream -> |Parser| -> VM code
#### CMM Grammer
- program: {global_decl}
- global_decl: var_decl | enum_decl | func_decl
- var_decl: type [`*`] Id [`,` [`*`] Id] `;`
- enum_decl: `enum` [Id] `{` Id [`=` `num`] [`,` Id [`=` `num`]] `}` `;`
- func_decl: type [`*`] Id `(` param_decl `)` `{` {var_decl}, {stmt} `}`
- param_decl: type [`*`] Id [`,` type [`*`] Id]
- stmt: if_stmt | while_stmt | return_stmt | empty_stmt | normal_stmt
- normal_stmt: expression `;`
- type: `char` | `int`

### Minimalist Virtual Machine
  + Register
    - `pc` | `sp` | `bp` | `ax`
  + Memory
    - `code` | `data` | `stack`
  + Instruction Set
    - Save & Load: `IMM` | `LEA` | `LC` | `SC` | `LI` | `SI` | `PUSH`
    - Arithmetic & Logical Operations: `ADD` | `SUB` | `MUL` | `DIV` | `MOD` | `OR` | `XOR` | `AND` | `SHL` | `SHR` | `EQ` | `NE` | `LT` | `LE` | `GT` | `GE`
    - Control: `JMP` | `JZ` | `JNZ` | `CALL` | `NVAR` | `DARG` | `RET`
    - Native-Call: `OPEN` | `CLOS` | `READ` | `PRTF` | `MALC` | `FREE` | `MSET` | `MCMP` | `EXIT`