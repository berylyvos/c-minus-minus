#include <stdint.h>
#include <string.h>

#define int int64_t

int  *code,  // code segment
     *stack; // stack segment

char *data;  // data segment

int  *pc,    // program counter
     *sp,    // rsp register
     *bp;    // rbp register

int   ax;    // common register

// classes or keywords
enum {Num = 128, Fun, Sys, Glb, Lcl, Id, Char, Int, Enum, If, Else, Return, Sizeof, While,
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

void parse_enum();
int parse_base_type();

void parse_param();
void parse_expr();
void parse_stmt();
void parse_func();

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

int main() {

    return 0;
}