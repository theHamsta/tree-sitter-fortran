#include <tree_sitter/parser.h>
#include <string>
#include <cwctype>

namespace {

using std::wstring;
using std::iswalnum;
using std::iswdigit;
using std::iswxdigit;
using std::iswspace;

enum TokenType {
    _LINE_CONTINUATION,
    NUMBER_LITERAL,
};

struct Scanner {
    //  consume current character into current token and advance
    void advance(TSLexer *lexer) {
      lexer->advance(lexer, false);
    }

    // ignore current character and advanceWW
    void skip(TSLexer *lexer) {
      lexer->advance(lexer, true);
    }

    bool is_boz_sentinel(char c) {
        switch (c) {
        case 'B':
        case 'b':
        case 'O':
        case 'o':
        case 'Z':
        case 'z':
            return true;
        default:
            return false;
        }
    }

    bool is_exp_sentinel(char c) {
        switch (c) {
        case 'D':
        case 'd':
        case 'E':
        case 'e':
            return true;
        default:
            return false;
        }
    }

    bool scan(TSLexer *lexer, const bool *valid_symbols) {
        while (iswspace(lexer->lookahead)) {
            skip(lexer);
        }
        if (valid_symbols[NUMBER_LITERAL]) {
            // this whole block needs tested with line cont.
            // generic BOZ rule
            // /([bBoOzZ]["'][0-9a-fA-F]+["'])|(["'][0-9a-fA-F]+["'][bBoOzZ])/
            // generic decimal rule (fails for 1.and.2)
            // /(((\d*\.)?\d+)|(\d+(\.\d*)?))([eEdD][-+]?\d+)?(_[a-zA-Z_]+)?/
            lexer->result_symbol = NUMBER_LITERAL;
            bool decimal = false, leading_digits = false, trailing_digits = false;

            if (iswdigit(lexer->lookahead)) {
                // consume leading digits
                while (iswdigit(lexer->lookahead)) {
                    advance(lexer); // store all digits
                }
                lexer->mark_end(lexer);
                leading_digits = true;
            }
            if (lexer->lookahead == '.') {
                advance(lexer);
                // only allow 0-9dDeE_ after a decimal
                if (iswdigit(lexer->lookahead)) {decimal = true;}
                else if (is_exp_sentinel(lexer->lookahead)) {decimal = true;}
                else if (lexer->lookahead == '_') {decimal = true;}
                else if (iswspace(lexer->lookahead)) {decimal = true;}
                if (decimal) {
                    lexer->mark_end(lexer);
                }
            }
            if (decimal && iswdigit(lexer->lookahead)) {
                // consume trailing digits
                while (iswdigit(lexer->lookahead)) {
                    advance(lexer); // store all digits
                }
                lexer->mark_end(lexer);
                trailing_digits = true;
            }
            if (!leading_digits && !trailing_digits) {
                return false; // don't allow empty matches
            }

            // process exp notation
            if (is_exp_sentinel(lexer->lookahead)) {
                // TODO
            }
            // get size qualifer
            if (lexer->lookahead == '_') {
                // TODO
            }


            // only a limited set of non-whitespace characters are allowed
            // after a number, this might be too permissive
            if (iswalnum(lexer->lookahead)) {
                return false;
            }
            // TODO: figure out how to handle 1.and.2
            return true;
        }
        else {
            lexer->result_symbol = _LINE_CONTINUATION;

            // Consume '&' at the end of the line
            if (lexer->lookahead != '&') {
                return false;
            }
            advance(lexer);

            // Consume end of line characters, we allow '\n', '\r\n' and
            // '\r' to cover unix, MSDOS and old style Macintosh
            if (lexer->lookahead == '\r') {
              lexer->advance(lexer, false);
              if (lexer->lookahead == '\n') {
                  advance(lexer);
              }
            }
            else {
              if (lexer->lookahead != '\n') return false;
              advance(lexer);
            }

            // in some instances a second ampersand exists, on the following line
            // I assume this is to allow the code to compile properly as free form
            // or fixed form (i.e. put the first ampersand past column 72 and the
            // second in column 6)
            while (iswspace(lexer->lookahead)) {
              advance(lexer);
            }
            if (lexer->lookahead == '&') advance(lexer);
            return true;
        }
    }
};

}

extern "C" {

void *tree_sitter_fortran_external_scanner_create() {
  return new Scanner();
}

bool tree_sitter_fortran_external_scanner_scan(void *payload, TSLexer *lexer,
                                               const bool *valid_symbols) {
  Scanner *scanner = static_cast<Scanner *>(payload);
  return scanner->scan(lexer, valid_symbols);
}

unsigned tree_sitter_fortran_external_scanner_serialize(void *payload, char *buffer) {
  return 0;
}

void tree_sitter_fortran_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
}

void tree_sitter_fortran_external_scanner_destroy(void *payload) {
  Scanner *scanner = static_cast<Scanner *>(payload);
  delete scanner;
}

}
