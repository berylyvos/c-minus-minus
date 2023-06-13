#include <stdint.h>
#include <string.h>

#define int int64_t

int MAX_SIZE;

int  *code,  // code segment
     *stack; // stack segment

char *data;  // data segment

int  *pc,    // program counter
     *sp,    // rsp register
     *bp;    // rbp register

int   ax,    // common register
      ibp,
      cycle;

// instruction set: copy from c4, change JSR/ENT/ADJ/LEV/BZ/BNZ to CALL/NVAR/DARG/RET/JZ/JNZ.
enum {IMM, LEA, JMP, JZ, JNZ, CALL, NVAR, DARG, RET, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, FREE, MSET, MCMP, EXIT};

// classes or keywords
enum {Num = 0x80, Fun, Sys, Glb, Lcl, Id, Char, Int, Enum, If, Else, Return, Sizeof, While,
    // operators in precedence order
    Assign, Cond, Lor, Land, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge,
    Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak};

// fields of sym_tbl
enum {Token, Hash, Name, Class, Type, Value, GClass, GType, GValue, SymSize};

// types of var & func in sym_tbl
enum {CHAR, INT, PTR};

// source code
char *src;

// symbol table, pointers
int *sym_tbl, *sym_ptr;

int token, token_val, line;

void tokenize() {
    char *ch_ptr;
    while ((token = *src++)) {
        if (token == '\n') line++;
        // skip marco
        else if (token == '#') while (*src != 0 && *src != '\n') src++;
        // handle symbol
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
            ch_ptr = src - 1;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_'))
                token = token * 147 + *src++;
            token = (token << 6) + (src - ch_ptr);
            sym_ptr = sym_tbl;
            // search same symbol in sym_tbl
            while (sym_ptr[Token]) {
                if (token == sym_ptr[Hash] && !memcmp((char*)sym_ptr[Name], ch_ptr, src - ch_ptr)) {
                    token = sym_ptr[Token];
                    return;
                }
                sym_ptr = sym_ptr + SymSize;
            }
            // add new symbol
            sym_ptr[Hash] = token;
            sym_ptr[Name] = (int)ch_ptr;
            token = sym_ptr[Token] = Id;
            return;
        }
        // handle number
        else if (token >= '0' && token <= '9') {
            // DEC, 1-9
            if ((token_val = token - '0'))
                while (*src >= '0' && *src <= '9') token_val = token_val * 10 + *src++ - '0';
            // HEX, 0x
            else if (*src == 'x' || *src == 'X')
                while ((token = *++src) && ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')))
                    token_val = token_val * 16 + (token & 0xF) + (token >= 'A' ? 9 : 0);
            // OCT, 0
            else while (*src >= '0' && *src <= '7') token_val = token_val * 8 + *src++ - '0';
            token = Num;
            return;
        }
        // handle string, char
        else if (token == '"' || token == '\'') {
            ch_ptr = data;
            while (*src != 0 && *src != token) {
                if ((token_val = *src++) == '\\') {
                    if ((token_val = *src++) == 'n') token_val = '\n';
                }
                // store string to data segment
                if (token == '"') *data++ = token_val;
            }
            src++;
            if (token == '"') token_val = (int)ch_ptr;
            else token = Num; // char
            return;
        }
        // handle '//' or '/'
        else if (token == '/') {
            if (*src == '/') {
                while (*src != 0 && *src != '\n') src++;
            } else {
                token = Div;
                return;
            }
        }
        // handle all kinds of operators
        else if (token == '=') {if (*src == '=') {src++; token = Eq;} else token = Assign; return;}
        else if (token == '+') {if (*src == '+') {src++; token = Inc;} else token = Add; return;}
        else if (token == '-') {if (*src == '-') {src++; token = Dec;} else token = Sub; return;}
        else if (token == '!') {if (*src == '=') {src++; token = Ne;} return;}
        else if (token == '<') {if (*src == '=') {src++; token = Le;} else if (*src == '<') {src++; token = Shl;} else token = Lt; return;}
        else if (token == '>') {if (*src == '=') {src++; token = Ge;} else if (*src == '>') {src++; token = Shr;} else token = Gt; return;}
        else if (token == '|') {if (*src == '|') {src++; token = Lor;} else token = Or; return;}
        else if (token == '&') {if (*src == '&') {src++; token = Land;} else token = And; return;}
        else if (token == '^') {token = Xor; return;}
        else if (token == '%') {token = Mod; return;}
        else if (token == '*') {token = Mul; return;}
        else if (token == '[') {token = Brak; return;}
        else if (token == '?') {token = Cond; return;}
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') return;
    }
}

void assert(int tk) {
    if (token != tk) {
        printf("line %lld: expect token: %lld(%c), get: %lld(%c)\n", line, tk, (char)tk, token, (char)token);
        exit(-1);
    }
    tokenize();
}

void check_local_id() {
    if (token != Id) {printf("line %lld: invalid identifer\n", line); exit(-1);}
    if (sym_ptr[Class] == Lcl) {
        printf("line %lld: duplicate declaration\n", line);
        exit(-1);
    }
}

void check_new_id() {
    if (token != Id) {printf("line %lld: invalid identifer\n", line); exit(-1);}
    if (sym_ptr[Class]) {
        printf("line %lld: duplicate declaration\n", line);
        exit(-1);
    }
}

void hide_global() {
    sym_ptr[GClass] = sym_ptr[Class];
    sym_ptr[GType] = sym_ptr[Type];
    sym_ptr[GValue] = sym_ptr[Value];
}

void recover_global() {
    sym_ptr[Class] = sym_ptr[GClass];
    sym_ptr[Type] = sym_ptr[GType];
    sym_ptr[Value] = sym_ptr[GValue];
}

int parse_base_type() {
    if (token == Char) {assert(Char); return CHAR;}
    else {assert(Int); return INT;}
}

void parse_enum() {
    int i;
    i = 0;
    while (token != '}') {
        check_new_id();
        assert(Id);
        // handle custom enum index
        if (token == Assign) {assert(Assign); assert(Num); i = token_val;}
        sym_ptr[Class] = Num;
        sym_ptr[Type] = INT;
        sym_ptr[Value] = i++;
        if (token == ',') tokenize();
    }
}

void parse_param();
void parse_expr();

void parse_stmt() {
    int *a;
    int *b;
    //     <cond>
    //     JZ a
    //     <true-stmt>
    //     JMP b
    //     a:
    //     <false-stmt>
    //     b:
    if (token == If) {
        assert(If); assert('('); parse_expr(Assign); assert(')');
        *++code = JZ; a = ++code; // JZ to false
        parse_stmt(); // true stmt
        if (token == Else) {
            assert(Else);
            *a = (int)(code + 3); // false point
            *++code = JMP; b = ++code; // JMP to endif
            parse_stmt(); // false stmt
        }
        *b = (int)(code + 1); // endif point
    }
    //     a:
    //     <cond>
    //     JZ b
    //     <loop-stmt>
    //     JMP a
    //     b:
    else if (token == While) {
        assert(While);
        a = code + 1; // entry point
        assert('('); parse_expr(Assign); assert(')');
        *++code = JZ; b = ++code; // JZ to endloop
        parse_stmt();
        *++code = JMP; *++code = (int)a; // JMP to entry point
        *b = (int)(code + 1); // endloop   
    }
    else if (token == Return) {
        assert(Return);
        if (token != ';') parse_expr(Assign);
        assert(';');
        *++code = RET;
    }
    else if (token == '{') {
        assert('{');
        while (token != '}') parse_stmt(Assign);
        assert('}');
    }
    else if (token == ';') assert(';');
    else {parse_expr(Assign); assert(';');}
}

void parse_func() {
    int type, i;
    i = ibp;
    // local vars must be declare in advance
    while (token == Char || token == Int) {
        type = parse_base_type();
        while (token != ';') {
            while (token == Mul) {assert(Mul); type = type + PTR;}
            check_local_id(); assert(Id);
            hide_global();
            sym_ptr[Class] = Lcl;
            sym_ptr[Type] = type;
            sym_ptr[Value] = ++i;
            if (token == ',') assert(',');
        }
        assert(';');
    }
    // new stack frames for local vars
    *++code = NVAR;
    *++code = i - ibp;
    // parse statements
    while (token != '}') parse_stmt();
    // if void, add return
    if (*code != RET) *++code = RET;
    // recover global vars from local vars
    sym_ptr = sym_tbl;
    while (sym_ptr[Token]) {
        if (sym_ptr[Class] == Lcl) recover_global();
        sym_ptr = sym_ptr + SymSize;
    }
}

void parse() {
    int type, base_type;
    line = 1; token = 1;
    while (token > 0) {
        tokenize();
        // parse enum
        if (token == Enum) {
            assert(Enum);
            if (token != '{') assert(Id);
            assert('{'); parse_enum(); assert('}');
        } else if (token == Int || token == Char) {
            base_type = parse_base_type();
            // parse var or func definition
            while (token != ';' && token != '}') {
                // parse pointer's star
                type = base_type;
                while (token == Mul) {assert(Mul); type = type + PTR;}
                check_new_id();
                assert(Id);
                sym_ptr[Type] = type;
                if (token == '(') {
                    // func
                    sym_ptr[Class] = Fun;
                    sym_ptr[Value] = (int)(code + 1);
                    assert('('); parse_param(); assert(')'); assert('{');
                    parse_func();
                } else {
                    // var
                    sym_ptr[Class] = Glb;
                    sym_ptr[Value] = (int)data;
                    data = data + 8;
                }
                // handle int a, b,...;
                if (token == ',') assert(',');
            }
            
        }
    }
}

int init_vm() {
    // allocate memory for vm
    if (!(code = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for code segment\n", MAX_SIZE);
        return -1;
    }
    if (!(data = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for data segment\n", MAX_SIZE);
        return -1;
    }
    if (!(stack = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for stack segment\n", MAX_SIZE);
        return -1;
    }
    if (!(sym_tbl = malloc(MAX_SIZE / 16))) {
        printf("could not malloc(%lld) for symbol table\n", MAX_SIZE);
        return -1;
    }
    memset(code, 0, MAX_SIZE);
    memset(data, 0, MAX_SIZE);
    memset(stack, 0, MAX_SIZE);
    memset(sym_tbl, 0, MAX_SIZE / 16);
    return 0;
}

int run_vm(int argc, char** argv) {
    int op;
    int *tmp;
    cycle = 0;
    while (1) {
        // read next instruction
        cycle++; op = *pc++;
        // load & save
        if (op == IMM)          ax = *pc++;                     // load immediate(or global addr)
        else if (op == LEA)     ax = (int)(bp + *pc++);         // load local addr
        else if (op == LC)      ax = *(char*)ax;                // load char
        else if (op == LI)      ax = *(int*)ax;                 // load int
        else if (op == SC)      *(char*)*sp = ax;               // save char to sp point to
        else if (op == SI)      *(int*)*sp = ax;                // save int to sp point to
        else if (op == PUSH)    *--sp = ax;                     // push ax to sp
        // jump
        else if (op == JMP)     pc = (int*)*pc;
        else if (op == JZ)      pc = ax ? pc + 1 : (int*)*pc;
        else if (op == JNZ)     pc = ax ? (int*)*pc : pc + 1;
        // arithmetic & bit
        else if (op == OR)      ax = *sp++ |  ax;
        else if (op == XOR)     ax = *sp++ ^  ax;
        else if (op == AND)     ax = *sp++ &  ax;
        else if (op == EQ)      ax = *sp++ == ax;
        else if (op == NE)      ax = *sp++ != ax;
        else if (op == LT)      ax = *sp++ <  ax;
        else if (op == LE)      ax = *sp++ <= ax;
        else if (op == GT)      ax = *sp++ >  ax;
        else if (op == GE)      ax = *sp++ >= ax;
        else if (op == SHL)     ax = *sp++ << ax;
        else if (op == SHR)     ax = *sp++ >> ax;
        else if (op == ADD)     ax = *sp++ +  ax;
        else if (op == SUB)     ax = *sp++ -  ax;
        else if (op == MUL)     ax = *sp++ *  ax;
        else if (op == DIV)     ax = *sp++ /  ax;
        else if (op == MOD)     ax = *sp++ %  ax;
        // some complicate instructions for function call
        // call function: push pc + 1 to stack, pc jump to func addr(pc point to)
        else if (op == CALL)    {*--sp = (int)(pc+1); pc = (int*)*pc;}
        // new stack frame for vars: save caller's bp, callee's sp as the new bp, add stack frames for vars 
        else if (op == NVAR)    {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}
        // delete stack frame for args: same as x86: add esp, <size>
        else if (op == DARG)    {sp = sp + *pc++;}
        // return to caller: restore stack and caller's bp, pc point to caller's next code addr
        else if (op == RET)     {sp = bp; bp = (int*)*sp++; pc = (int*)*sp++;}
        // native call
        else if (op == OPEN)    {ax = open((char*)sp[1], sp[0]);}
        else if (op == CLOS)    {ax = close(*sp);}
        else if (op == READ)    {ax = read(sp[2], (char*)sp[1], *sp);}
        else if (op == PRTF)    {tmp = sp + pc[1] - 1; ax = printf((char*)tmp[0], tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5]);}
        else if (op == MALC)    {ax = (int)malloc(*sp);}
        else if (op == FREE)    {free((void*)*sp);}
        else if (op == MSET)    {ax = (int)memset((char*)sp[2], sp[1], *sp);}
        else if (op == MCMP)    {ax = memcmp((char*)sp[2], (char*)sp[1], *sp);}
        else if (op == EXIT)    {printf("exit(%lld)\n", *sp); return *sp;}
        else {printf("unkown instruction: %lld, cycle: %lld\n", op, cycle); return -1;}
    }
    return 0;
}

int main() {

    return 0;
}