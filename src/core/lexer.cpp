#include "../include/lexer.hpp"
#include "../include/error.hpp"
#include <cctype>
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"let",       TokenType::LET},
    {"var",       TokenType::VAR},
    {"auto",      TokenType::AUTO},
    // {"dyn",       TokenType::DYN}, // removed dynamic keyword, using auto for dynamic typing
    {"fn",        TokenType::FN},
    {"return",    TokenType::RETURN},
    {"class",     TokenType::CLASS},
    {"struct",    TokenType::STRUCT},
    {"interface", TokenType::INTERFACE},
    {"if",        TokenType::IF},
    {"else",      TokenType::ELSE},
    {"while",     TokenType::WHILE},
    {"for",       TokenType::FOR},
    {"in",        TokenType::IN},
    {"break",     TokenType::BREAK},
    {"continue",  TokenType::CONTINUE},
    {"import",    TokenType::IMPORT},
    {"export",    TokenType::EXPORT},
    {"expose",    TokenType::EXPOSE},
    {"overwrite", TokenType::OVERWRITE},
    {"this",      TokenType::THIS_KW},
    {"as",        TokenType::AS},
    {"null",      TokenType::NULL_KW},
    {"true",      TokenType::TRUE_KW},
    {"false",     TokenType::FALSE_KW},
    {"unsafe",    TokenType::UNSAFE},
    {"defer",     TokenType::DEFER},
    {"try",       TokenType::TRY},
    {"catch",     TokenType::CATCH},
    {"throw",     TokenType::THROW},
    {"ptr",       TokenType::PTR},
    {"new",       TokenType::NEW},
    {"delete",    TokenType::DELETE},
    {"match",     TokenType::MATCH},
    {"int_8",     TokenType::TYPE_INT8},
    {"int_16",    TokenType::TYPE_INT16},
    {"int_32",    TokenType::TYPE_INT32},
    {"int_64",    TokenType::TYPE_INT64},
    {"uint_8",    TokenType::TYPE_UINT8},
    {"uint_16",   TokenType::TYPE_UINT16},
    {"uint_32",   TokenType::TYPE_UINT32},
    {"uint_64",   TokenType::TYPE_UINT64},
    {"float_32",  TokenType::TYPE_FLOAT32},
    {"float_64",  TokenType::TYPE_FLOAT64},
    {"bool",      TokenType::TYPE_BOOL},
    {"char",      TokenType::TYPE_CHAR},
    {"str",       TokenType::TYPE_STR},
    {"void",      TokenType::TYPE_VOID},
    {"list",      TokenType::TYPE_LIST},
    {"set",       TokenType::TYPE_SET},
    {"dict",      TokenType::TYPE_DICT},
    {"matrix",    TokenType::TYPE_MATRIX},
};

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source_(source), filename_(filename) {}

char Lexer::peek(int offset) const {
    size_t i = pos_ + offset;
    return (i < source_.size()) ? source_[i] : '\0';
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { line_++; col_ = 1; }
    else col_++;
    return c;
}

bool Lexer::match(char c) {
    if (pos_ < source_.size() && source_[pos_] == c) {
        advance();
        return true;
    }
    return false;
}

bool Lexer::matchStr(const std::string& s) {
    if (pos_ + s.size() <= source_.size() &&
        source_.substr(pos_, s.size()) == s) {
        for (size_t i = 0; i < s.size(); i++) advance();
        return true;
    }
    return false;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < source_.size()) {
        char c = peek();
        if (std::isspace(c)) { advance(); continue; }
        // Single-line comment //
        if (c == '/' && peek(1) == '/') {
            while (pos_ < source_.size() && peek() != '\n') advance();
            continue;
        }
        // Multi-line comment /* */
        if (c == '/' && peek(1) == '*') {
            advance(); advance();
            while (pos_ < source_.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
            continue;
        }
        break;
    }
}

Token Lexer::makeToken(TokenType type, const std::string& val) {
    return Token(type, val, line_, col_);
}

Token Lexer::lexNumber() {
    int startLine = line_;
    std::string num;
    bool isFloat = false;

    // Hex
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
        num += advance(); num += advance();
        while (std::isxdigit(peek()) || peek() == '_') {
            char c = advance();
            if (c != '_') num += c;
        }
        return Token(TokenType::INT_LIT, num, startLine, col_);
    }

    while (std::isdigit(peek()) || peek() == '_') {
        char c = advance();
        if (c != '_') num += c;
    }
    if (peek() == '.' && std::isdigit(peek(1))) {
        isFloat = true;
        num += advance();
        while (std::isdigit(peek()) || peek() == '_') {
            char c = advance();
            if (c != '_') num += c;
        }
    }
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        num += advance();
        if (peek() == '+' || peek() == '-') num += advance();
        while (std::isdigit(peek())) num += advance();
    }
    double multiplier = 1.0;
    bool hasSuffix = false;

    if (peek() == 'm' && peek(1) == 's') {
        advance(); advance();
        multiplier = 1.0;
        hasSuffix = true;
    } else if (peek() == 'u' && peek(1) == 's') {
        advance(); advance();
        multiplier = 0.001;
        hasSuffix = true;
    } else if (peek() == 's') {
        advance();
        multiplier = 1000.0;
        hasSuffix = true;
    } else if (peek() == 'm') {
        advance();
        multiplier = 60000.0;
        hasSuffix = true;
    } else if (peek() == 'h') {
        advance();
        multiplier = 3600000.0;
        hasSuffix = true;
    } else if (peek() == 'd') {
        advance();
        multiplier = 86400000.0;
        hasSuffix = true;
    }

    if (hasSuffix) {
        double val = std::stod(num) * multiplier;
        num = std::to_string(val);
        isFloat = true;
    }

    return Token(isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, num, startLine, col_);
}

Token Lexer::lexString(char delim) {
    int startLine = line_;
    std::string s;
    while (pos_ < source_.size() && peek() != delim) {
        if (peek() == '\\') {
            advance();
            char esc = advance();
            switch(esc) {
                case 'n': s += '\n'; break;
                case 't': s += '\t'; break;
                case 'r': s += '\r'; break;
                case '"': s += '"'; break;
                case '\'': s += '\''; break;
                case '\\': s += '\\'; break;
                case '0': s += '\0'; break;
                default: s += '\\'; s += esc;
            }
        } else {
            s += advance();
        }
    }
    if (pos_ >= source_.size())
        throw LexError("Unterminated string literal", startLine);
    advance(); // closing quote
    return Token(TokenType::STRING_LIT, s, startLine, col_);
}

Token Lexer::lexChar() {
    int startLine = line_;
    std::string s;
    if (peek() == '\\') {
        advance();
        char esc = advance();
        switch(esc) {
            case 'n': s += '\n'; break;
            case 't': s += '\t'; break;
            case 'r': s += '\r'; break;
            case '\'': s += '\''; break;
            case '\\': s += '\\'; break;
            default: s += esc;
        }
    } else {
        s += advance();
    }
    if (!match('\''))
        throw LexError("Unterminated char literal", startLine);
    return Token(TokenType::CHAR_LIT, s, startLine, col_);
}

Token Lexer::lexPSString() {
    // ps"..." - read raw content, interpolation parsed later
    int startLine = line_;
    if (peek() != '"')
        throw LexError("Expected '\"' after 'ps'", startLine);
    advance(); // consume opening "
    std::string raw;
    int depth = 0;
    while (pos_ < source_.size()) {
        char c = peek();
        if (c == '"' && depth == 0) { advance(); break; }
        if (c == '{') depth++;
        if (c == '}') depth--;
        if (c == '\\') {
            raw += advance();
            raw += advance();
        } else {
            raw += advance();
        }
    }
    return Token(TokenType::PSSTRING_LIT, raw, startLine, col_);
}

Token Lexer::lexIdent() {
    int startLine = line_; int startCol = col_;
    std::string id;
    while (pos_ < source_.size() && (std::isalnum(peek()) || peek() == '_')) {
        id += advance();
    }
    // Check for ps"..." special case
    if (id == "ps" && peek() == '"') {
        return lexPSString();
    }
    auto it = KEYWORDS.find(id);
    if (it != KEYWORDS.end())
        return Token(it->second, id, startLine, startCol);
    return Token(TokenType::IDENT, id, startLine, startCol);
}

Token Lexer::lexLifecycle() {
    // Already consumed '+' or '~'
    // Now read digits and '>'
    // Called when we see e.g. +1> or ~1>
    return makeToken(TokenType::INVALID, "");
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();
        if (pos_ >= source_.size()) {
            tokens.push_back(makeToken(TokenType::EOF_T, ""));
            break;
        }

        int startLine = line_; int startCol = col_;
        char c = peek();

        // Numbers
        if (std::isdigit(c)) {
            tokens.push_back(lexNumber());
            continue;
        }

        // Identifiers / keywords
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(lexIdent());
            continue;
        }

        // Strings
        if (c == '"') { advance(); tokens.push_back(lexString('"')); continue; }
        if (c == '\'') { advance(); tokens.push_back(lexChar()); continue; }

        advance(); // consume current char

        switch(c) {
            case '+':
                // Check for +N> lifecycle pattern
                if (std::isdigit(peek())) {
                    std::string num;
                    while (std::isdigit(peek())) num += advance();
                    if (peek() == '>') {
                        advance();
                        tokens.push_back(Token(TokenType::CONSTRUCTOR_ORDER, num, startLine, startCol));
                    } else {
                        // Just a plus followed by number
                        tokens.push_back(Token(TokenType::PLUS, "+", startLine, startCol));
                        tokens.push_back(Token(TokenType::INT_LIT, num, startLine, startCol));
                    }
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::PLUS_ASSIGN, "+=", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::PLUS, "+", startLine, startCol));
                }
                break;
            case '-':
                if (match('>')) {
                    tokens.push_back(Token(TokenType::ARROW, "->", startLine, startCol));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::MINUS_ASSIGN, "-=", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::MINUS, "-", startLine, startCol));
                }
                break;
            case '*':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::STAR_ASSIGN, "*=", startLine, startCol));
                } else if (match('*')) {
                    tokens.push_back(Token(TokenType::POWER, "**", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::STAR, "*", startLine, startCol));
                }
                break;
            case '/':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::SLASH_ASSIGN, "/=", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::SLASH, "/", startLine, startCol));
                }
                break;
            case '%':
                tokens.push_back(Token(TokenType::PERCENT, "%", startLine, startCol));
                break;
            case '=':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::EQ, "==", startLine, startCol));
                } else if (match('>')) {
                    tokens.push_back(Token(TokenType::FAT_ARROW, "=>", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::ASSIGN, "=", startLine, startCol));
                }
                break;
            case '!':
                if (match('=')) {
                    tokens.push_back(Token(TokenType::NEQ, "!=", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::BANG, "!", startLine, startCol));
                }
                break;
            case '<':
                if (match('<')) {
                    tokens.push_back(Token(TokenType::LSHIFT_OUT, "<<", startLine, startCol));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::LTE, "<=", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::LT, "<", startLine, startCol));
                }
                break;
            case '>':
                if (match('>')) {
                    tokens.push_back(Token(TokenType::RSHIFT_IN, ">>", startLine, startCol));
                } else if (match('=')) {
                    tokens.push_back(Token(TokenType::GTE, ">=", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::GT, ">", startLine, startCol));
                }
                break;
            case '&':
                if (match('&')) {
                    tokens.push_back(Token(TokenType::AND, "&&", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::BIT_AND, "&", startLine, startCol));
                }
                break;
            case '|':
                if (match('|')) {
                    tokens.push_back(Token(TokenType::OR, "||", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::BIT_OR, "|", startLine, startCol));
                }
                break;
            case '?':
                if (match('?')) {
                    tokens.push_back(Token(TokenType::NULL_COAL, "??", startLine, startCol));
                } else if (match('.')) {
                    tokens.push_back(Token(TokenType::OPT_CHAIN, "?.", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::QUESTION, "?", startLine, startCol));
                }
                break;
            case '~':
                // Check for ~N> lifecycle destructor
                if (std::isdigit(peek())) {
                    std::string num;
                    while (std::isdigit(peek())) num += advance();
                    if (peek() == '>') {
                        advance();
                        tokens.push_back(Token(TokenType::DESTRUCTOR_ORDER, num, startLine, startCol));
                    } else {
                        tokens.push_back(Token(TokenType::TILDE, "~", startLine, startCol));
                    }
                } else {
                    tokens.push_back(Token(TokenType::TILDE, "~", startLine, startCol));
                }
                break;
            case '(':  tokens.push_back(Token(TokenType::LPAREN,   "(", startLine, startCol)); break;
            case ')':  tokens.push_back(Token(TokenType::RPAREN,   ")", startLine, startCol)); break;
            case '{':  tokens.push_back(Token(TokenType::LBRACE,   "{", startLine, startCol)); break;
            case '}':  tokens.push_back(Token(TokenType::RBRACE,   "}", startLine, startCol)); break;
            case '[':  tokens.push_back(Token(TokenType::LBRACKET, "[", startLine, startCol)); break;
            case ']':  tokens.push_back(Token(TokenType::RBRACKET, "]", startLine, startCol)); break;
            case ';':  tokens.push_back(Token(TokenType::SEMICOLON,";", startLine, startCol)); break;
            case ':':
                if (match(':')) {
                    tokens.push_back(Token(TokenType::DOUBLE_COLON, "::", startLine, startCol));
                } else {
                    tokens.push_back(Token(TokenType::COLON,    ":", startLine, startCol));
                }
                break;
            case ',':  tokens.push_back(Token(TokenType::COMMA,    ",", startLine, startCol)); break;
            case '.':  tokens.push_back(Token(TokenType::DOT,      ".", startLine, startCol)); break;
            case '@':  tokens.push_back(Token(TokenType::AT,       "@", startLine, startCol)); break;
            case '^':  tokens.push_back(Token(TokenType::BIT_XOR,  "^", startLine, startCol)); break;
            default:
                throw LexError(std::string("Unexpected character: '") + c + "'", startLine);
        }
    }

    return tokens;
}

std::string Token::typeStr() const {
    switch(type) {
        case TokenType::IDENT: return "IDENT(" + value + ")";
        case TokenType::INT_LIT: return "INT(" + value + ")";
        case TokenType::FLOAT_LIT: return "FLOAT(" + value + ")";
        case TokenType::STRING_LIT: return "STR(\"" + value + "\")";
        case TokenType::EOF_T: return "EOF";
        default: return "TOKEN(" + value + ")";
    }
}
