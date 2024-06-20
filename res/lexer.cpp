#include <iostream>
#include <iomanip>
#include <list>

// https://raw.githubusercontent.com/masterneko/scanner/master/scanner.h
#include "scanner.h"

bool parse_comment(Scanner& scanner)
{
    if (scanner.skip_sequence("//"))
    {
        // While the scanner is not at the End Of File and the current character is not a newline, advance a character
        while (scanner && scanner.current() != '\n') scanner++;
    }
    else if (scanner.skip_sequence("/*"))
    {
        // The lexer is continously advanced until it is either at EOF or the termination sequence (*/) has been found.
        while (scanner && scanner.current() != "*/") scanner++;

        // we should only accept if the comment was actually terminated with '*/'
        // otherwise it is an invalid multiline comment
        if (!scanner.skip_sequence("*/"))
        {
            return false; /* token rejected */
        }
    }

    return true; /* token accepted */
}

bool parse_number(Scanner& scanner)
{
    // skip over prefix and handle accordingly
    if (scanner.skip_sequence("0x"))
    {
        size_t n = 0;

        // keeps matching and advancing until skip_char reached a char not in the list
        while (scanner.skip_char("0123456789abcdefABCDEF")) n++;

        // if it didn't match at least once it should be rejected
        if (n < 1)
        {
            return false;
        }
    }
    else
    {
        size_t n = 0;

        while (scanner.skip_char("0123456789")) n++;

        if (n < 1) return false;

        if (scanner.skip_char('.'))
        {
            n = 0;

            while (scanner.skip_char("0123456789")) n++;

            if (n < 1) return false;
        }
    }

    return true; /* the token was accepted */
}

bool parse_string(Scanner& scanner)
{
    if (!scanner.skip_char('"'))
    {
        return false; /* clearly not a string */
    }

    while (scanner && scanner != '"')
    {
        scanner.skip_char('\\'); /* handle escape sequence */

        scanner++;
    }

    if (!scanner.skip_char('"'))
    {
        return false; /* unterminated */
    }

    return true;
}

bool parse_more_than_or_equal(Scanner& scanner)
{
    return scanner.skip_sequence(">=");
}

bool parse_more_than(Scanner& scanner)
{
    return scanner.skip_char('>');
}

bool parse_plus(Scanner& scanner)
{
    return scanner.skip_char('+');
}

bool parse_minus(Scanner& scanner)
{
    return scanner.skip_char('-');
}

bool parse_times(Scanner& scanner)
{
    return scanner.skip_char('*');
}

bool parse_divide(Scanner& scanner)
{
    return scanner.skip_char('/');
}

bool parse_space(Scanner& scanner)
{
    size_t n = 0;

    while (scanner.skip_char(' ')) n++;

    return n >= 1;
}

// The parser determines token type by selecting the greediest
// accepted token. Each parsing function will consume as many
// characters as possible and return a boolean value indicating
// whether or not it was accepted. If a parsing function consumes
// characters but inevitably rejects the token, this will be treated
// as an error and the parser will create an Error token. However a
// rejected token does not always mean an error and if it is found
// that the token does not fit any of these categories, the parser
// will attempt to recycle the token.
#define TOKENS(X) \
      /* Type */       /* Parsing func */        \
    X(Comment,         parse_comment)            \
    X(Number,          parse_number)             \
    X(String,          parse_string)             \
    X(MoreThanOrEqual, parse_more_than_or_equal) \
    X(MoreThan,        parse_more_than)          \
    X(Plus,            parse_plus)               \
    X(Minus,           parse_minus)              \
    X(Times,           parse_times)              \
    X(Divide,          parse_divide)             \
    X(Space,           parse_space)

bool recycle_identifier(std::string_view leftovers)
{
    return true; /* accept regardless of the leftovers' contents */
}

// The parser will call each recycling function in a linear order.
// If the recycler accepts the leftover characters then a token
// of that type will be created.
#define RECYCLERS(X) \
      /* Type */       /* Recycling func */ \
    X(Identifier,     recycle_identifier)

class Token
{
public:
    #define TOKENS_enum(TYPE_, PARSING_FUNC_) TYPE_,
    #define RECYCLERS_enum(TYPE_, RECYCLE_FUNC_) TYPE_,
    enum Type
    {
        TOKENS(TOKENS_enum)
        RECYCLERS(RECYCLERS_enum)
        Error
    };

    static std::string_view type_to_string(Type type)
    {
        #define TOKENS_strings(TYPE_, PARSING_FUNC_) #TYPE_,
        #define RECYCLERS_strings(TYPE_, RECYCLE_FUNC_) #TYPE_,

        const char* strings[] = {
            TOKENS(TOKENS_strings)
            RECYCLERS(RECYCLERS_strings)
        };

        return (size_t)type < (sizeof(strings) / sizeof(strings[0])) ? strings[(size_t)type] : "Error";
    }

    Type type;
    std::string_view text;
    size_t index;
    size_t line, col;
};

#define TOKENS_funcs(TYPE_, PARSING_FUNC_) PARSING_FUNC_,
static bool (*parser_funcs[]) (Scanner& scanner) = {
    TOKENS(TOKENS_funcs)
};
static size_t parser_funcs_n = sizeof(parser_funcs) / sizeof(parser_funcs[0]);

Scanner::iterator find_greediest_token(Scanner& scanner, Token::Type& best_type)
{
    const Scanner::iterator best_start = scanner.current();
    Scanner::iterator best_end = best_start;

    for (size_t i = 0; i < parser_funcs_n; i++)
    {
        bool was_accepted = parser_funcs[i](scanner);

        if (was_accepted && scanner.current() > best_end) /* if it is accepted and the greediest so far */
        {
            // record as the best match
            best_type = (Token::Type)i;
            best_end = scanner.current();
        }
        else if (!was_accepted && scanner.current() != best_start) /* else if it was rejected halfway through parsing */
        {
            // create an Error token
            best_type = Token::Type::Error;
            best_end = scanner.current();
        }

        // reset to the beginning
        scanner = best_start;
    }

    return best_end;
}

Token create_token(Scanner::iterator start_it, Token::Type type)
{
    Token tok;

    tok.type = type;
    tok.text = start_it.slice();

    Scanner::iterator::location loc = start_it.get_location();

    tok.index = loc.index;
    tok.line = loc.line;
    tok.col = loc.column;

    return tok;
}

Token recycle_token(Scanner::iterator start_it)
{
    #define RECYCLERS_funcs(TYPE_, RECYCLE_FUNC_) RECYCLE_FUNC_,
    static bool (*recycler_funcs[]) (std::string_view text) = {
        RECYCLERS(RECYCLERS_funcs)
    };
    static size_t recycler_funcs_n = sizeof(recycler_funcs) / sizeof(recycler_funcs[0]);

    Token tok = create_token(start_it, Token::Type::Error);

    for (size_t i = 0; i < recycler_funcs_n; i++)
    {
        if (recycler_funcs[i](tok.text))
        {
            tok.type = (Token::Type)(parser_funcs_n + i); /* recylers located directly after parsers */

            break;
        }
    }

    return tok;
}

void parse_token(Scanner& scanner, std::list<Token>& tokens)
{
    const Scanner::iterator leftover_start = scanner.current();

    while (scanner)
    {
        Token::Type token_type = Token::Error;
        const Scanner::iterator token_start = scanner.current();
        const Scanner::iterator token_end = find_greediest_token(scanner, token_type);

        if (token_start == token_end) /* if no character was consumed */
        {
            // advance by one character
            scanner++;
        }
        else
        {
            if (leftover_start != token_start) /* if there are leftovers */
            {
                // recycle the token into a usable type
                tokens.push_back(recycle_token(leftover_start));
            }


            // add the parsed token to the list
            scanner = token_end;

            tokens.push_back(create_token(token_start, token_type));

            break;
        }
    }
}

std::list<Token> tokenise_text(std::string_view text)
{
    Scanner scanner(text);
    std::list<Token> tokens;

    while (scanner)
    {
        parse_token(scanner, tokens);
    }

    return tokens;
}

int main()
{
    std::list<Token> tokens = tokenise_text("2 + num >= 5");

    for (const Token& token : tokens)
    {
        std::cout << Token::type_to_string(token.type) << "(text: " << std::quoted(token.text)
                  << ", index: " << token.index << ", line: " << token.line << ", col: " << token.col << ")\n";
    }

    std::cout.flush();
}
