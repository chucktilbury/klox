/**
 * @file scanner.c
 * @brief Classic scanner. This scanner is a stand-alone that simply returns
 * token structures that are derived from the input stream, which may or may
 * not be a complete file.
 *
 * @version 0.1
 * @date 2022-03-20
 *
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/**
 * @brief Structure for the scanner state.
 */
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

/**
 * @brief Global scanner var.
 */
Scanner scanner;

/**
 * @brief Initialize the scanner with a new text buffer.
 *
 * @param source - buffer for the scanner
 */
void initScanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

/**
 * @brief Return true if the character can be a part of a symbol.
 *
 * @param c - character to test
 * @return true - if it can be a part of a symbol
 *
 * @return false - if it cannot be part of a symbol
 */
static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

/**
 * @brief Return true if the character represents a numerical digit.
 *
 * @param c - character to test
 * @return true - if it's a digit
 *
 * @return false - if it's not a digit
 */
static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

/**
 * @brief Return true if the scanner has reached the end of the buffer.
 *
 * @return true
 * @return false
 */
static bool isAtEnd()
{
    return *scanner.current == '\0';
}

/**
 * @brief Move the position of the scanner in the buffer forward one
 * character position.
 *
 * @return char - the new current character
 */
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

/**
 * @brief Look at the current character in the buffer without moving the
 * position in the buffer.
 *
 * @return char - current character
 */
static char peek()
{
    return *scanner.current;
}

/**
 * @brief Return the next character in the buffer without moving the position
 * in the buffer.
 *
 * @return char - next character in the buffer
 */
static char peekNext()
{
    if(isAtEnd()) {
        return '\0';
    }
    return scanner.current[1];
}

/**
 * @brief If the current character in the buffer matches the expected
 * character then advance the buffer and return true, otherwise, return
 * false.
 *
 * @param expected - character that is to be expected
 *
 * @return true - if the expected character matches the current character
 * @return false - if it does not match
 */
static bool match(char expected)
{
    if(isAtEnd()) {
        return false;
    }
    if(*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

/**
 * @brief Create a token. Note that this returns a token that was allocated
 * upon the stack.
 *
 * @param type - type of the new token, according to the TokenType enum
 *
 * @return Token - the token that was created
 */
static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

/**
 * @brief Create an error token that has a message instead of text that was
 * read from the input. Note that the token is created on the stack and then
 * returned.
 *
 * @param message - error message created by caller
 *
 * @return Token - token object
 */
static Token errorToken(const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

/**
 * @brief Skip all of the white space, such that it is not made a part of a
 * token. This function also handled line counting and skips comments.
 */
static void skipWhitespace()
{
    for(;;) {
        char c = peek();
        switch(c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            // TODO: skip multi-line comments here as well.
            case '/':
                if(peekNext() == '/') {
                    // A comment goes until the end of the line.
                    while(peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                }
                else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/**
 * @brief Called by identifierType() to check to see if the rest of the token
 * matches a keyword. If it does, then return the type, else return the basic
 * TOKEN_IDENTIFIER.
 *
 * @param start - starting index into the buffer
 * @param length - number of characters to check in the buffer
 * @param rest - characters to check against the buffer
 * @param type - the type to return if the characters match
 *
 * @return TokenType - enum value to return
 */
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
{
    if(scanner.current - scanner.start == start + length &&
            memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

/**
 * @brief When this is called, an identifier has been detected, but it is
 * not know if it it is a keyword or a user defined name. This makes the
 * decision and returns the type.
 *
 * @return TokenType - token type
 */
static TokenType identifierType()
{
    switch(scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if(scanner.current - scanner.start > 1) {
                switch(scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if(scanner.current - scanner.start > 1) {
                switch(scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

/**
 * @brief When this is called the first character past a stop character is a
 * part of an identifier. This finishes reading the identifier.
 *
 * @return Token - token that was read
 */
static Token identifier()
{
    while(isAlpha(peek()) || isDigit(peek())) {
        advance();
    }
    return makeToken(identifierType());
}

/**
 * @brief When this is read, a digit was already read. This expects a number
 * in floating point format.
 *
 * @return Token - token that was read
 */
static Token number()
{
    while(isDigit(peek())) {
        advance();
    }

    // Look for a fractional part.
    if(peek() == '.' && isDigit(peekNext())) {
        // Consume the ".".
        advance();

        while(isDigit(peek())) {
            advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
}

/**
 * @brief When this is entered, a double quote has been read. This finishes
 * reading a string that may span lines.
 *
 * @return Token - token that was read
 */
static Token string()
{
    while(peek() != '"' && !isAtEnd()) {
        if(peek() == '\n') {
            scanner.line++;
        }
        advance();
    }

    if(isAtEnd()) {
        return errorToken("Unterminated string.");
    }

    // The closing quote.
    advance();
    return makeToken(TOKEN_STRING);
}

/**
 * @brief Main entry point to the scanner. This scans a token and returns it
 * to the caller.
 *
 * @return Token - the token to return
 */
Token scanToken()
{
    skipWhitespace();
    scanner.start = scanner.current;

    if(isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();
    if(isAlpha(c)) {
        return identifier();
    }
    if(isDigit(c)) {
        return number();
    }

    switch(c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL :
                             TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL :
                             TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL :
                             TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL :
                             TOKEN_GREATER);
        case '"':
            return string();
    }

    return errorToken("Unexpected character.");
}
