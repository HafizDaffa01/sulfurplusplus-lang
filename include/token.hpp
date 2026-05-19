#pragma once
#include <string>

enum class TokenType {
    // Literals
    INT_LIT, FLOAT_LIT, STRING_LIT, CHAR_LIT, PSSTRING_LIT, BOOL_LIT, NULL_LIT,

    // Identifiers
    IDENT,

    // Keywords
    LET, VAR, AUTO, DYN,
    FN, RETURN, CLASS, STRUCT, INTERFACE,
    IF, ELSE, WHILE, FOR, IN, BREAK, CONTINUE,
    IMPORT, AS, EXPORT, THIS_KW,
    UNSAFE, DEFER,
    PTR, REF,
    NULL_KW, TRUE_KW, FALSE_KW,
    NEW, DELETE,

    // Types
    TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
    TYPE_UINT8, TYPE_UINT16, TYPE_UINT32, TYPE_UINT64,
    TYPE_FLOAT32, TYPE_FLOAT64,
    TYPE_BOOL, TYPE_CHAR, TYPE_STR, TYPE_VOID,
    TYPE_LIST, TYPE_SET, TYPE_DICT, TYPE_MATRIX,

    // Arithmetic
    PLUS, MINUS, STAR, SLASH, PERCENT, POWER,

    // Comparison
    EQ, NEQ, LT, GT, LTE, GTE,

    // Logical
    AND, OR, NOT, BANG,

    // Bitwise
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LSHIFT, RSHIFT,

    // Assignment
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,

    // Special operators
    ARROW,         // ->  (pipeline)
    FAT_ARROW,     // =>
    NULL_COAL,     // ??
    OPT_CHAIN,     // ?.
    QUESTION,      // ?
    LSHIFT_OUT,    // << (stream output)
    RSHIFT_IN,     // >> (stream input)
    TILDE,         // ~ (destructor prefix)
    PLUS_NUM,      // +N> (constructor order) -- lexed as PLUS_NUM with value

    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    SEMICOLON, COLON, DOUBLE_COLON, COMMA, DOT, AT,

    // Annotations
    HASH, // #[...]

    // Lifecycle special tokens
    CONSTRUCTOR_ORDER, // +N>
    DESTRUCTOR_ORDER,  // ~N>

    // End of file
    EOF_T,
    INVALID
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), col(c) {}

    std::string typeStr() const;
};
