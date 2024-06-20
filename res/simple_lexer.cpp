#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>

// std::string::starts_with and std::string::contains

using Str = std::string_view;
using DynaStr = std::string;
template <typename T>
using Vec = std::vector<T>;

size_t starts_with(Str in, Str sequence)
{
    if (in.starts_with(sequence)) /* since C++20 */
    {
        return sequence.size();
    }

    return 0;
}

size_t contains(Str char_set, Str::value_type c)
{
    return char_set.find(c) != Str::npos;
}

size_t parse_num(Str in)
{
    size_t i = 0;

    for (; i < in.size() && contains("0123456789", in[i]); i++);

    return i;
}

size_t dispose_space(Str in)
{
    size_t i = 0;

    for (; i < in.size() && in[i] == ' '; i++);

    return i;
}

size_t parse_equals(Str in)
{
    return starts_with(in, "=");
}

bool recycle_var(Str leftovers)
{
    return leftovers == "var";
}

bool recycle_identifier(Str leftovers)
{
    return true;
}

#define DISPOSER(X) \
    X(dispose_space)

#define PARSER(X) \
    X(Number, parse_num) \
    X(Equals, parse_equals)

#define RECYCLER(X) \
    X(Var, recycle_var) \
    X(Identifier, recycle_identifier)

class Token
{
public:
#define PARSER_enum(IDENT, FUNC) IDENT,
#define RECYCLER_enum(IDENT, FUNC) IDENT,
    class Type
    {
    private:
        enum class EType
        {
            PARSER(PARSER_enum)
            RECYCLER(RECYCLER_enum)
            Error
        };

    private:
        Type(EType type)
        :
        _type(type)
        {
        }

    public:
        bool operator==(Type type) const
        {
            return _type == type._type;
        }

        bool operator!=(Type type) const
        {
            return _type != type._type;
        }

#define PARSER_strings(IDENT, FUNC) #IDENT,
#define RECYLCLER_strings(IDENT, FUNC) #IDENT,

        Str to_string() const
        {
            static Str strings[] =
            {
                PARSER(PARSER_strings)
                RECYCLER(RECYLCLER_strings)
                "Error"
            };

            return (size_t)_type < (sizeof(strings) / sizeof(strings[0])) ?
                    strings[(size_t)_type] : "Unknown";
        }

        static Type from_index(size_t index)
        {
            return Type((EType)index);
        }
    private:
        friend class Token;

        EType  _type;
    };

private:
    Type _type;
    Str _data;
public:

#define PARSER_constants(IDENT, FUNC) static inline const Type IDENT { Type::EType::IDENT };
#define RECYCLER_constants(IDENT, FUNC) static inline const Type IDENT { Type::EType::IDENT }; 
    PARSER(PARSER_constants)
    RECYCLER(RECYCLER_constants)

    static inline const Type Error { Type::EType::Error };

    Token(Type type, Str data)
    :
    _type(type),
    _data(data)
    {
    }

    DynaStr to_string() const
    {
        return (DynaStr)_type.to_string() + "('" + (DynaStr)_data + "')";
    }

    operator Type() const
    {
        return _type;
    }
};

#define PARSER_funcs(IDENT, FUNC) FUNC,
static size_t (*parser_funcs[])(Str in) = {
    PARSER(PARSER_funcs)
};

static const size_t parser_func_n = sizeof(parser_funcs) / sizeof(parser_funcs[0]);

#define RECYCLER_funcs(IDENT, FUNC) FUNC,
static bool (*recycle_funcs[])(Str leftovers) = {
    RECYCLER(RECYCLER_funcs)
};

static const size_t recycle_func_n = sizeof(recycle_funcs) / sizeof(recycle_funcs[0]);

size_t eat_token(Str in, Token::Type& type)
{
    size_t best_token_index = 0;
    size_t highest_consumption = 0;

    for (size_t tok_index = 0; tok_index < parser_func_n; tok_index++)
    {
        size_t consumption = parser_funcs[tok_index](in);

        if (consumption > highest_consumption)
        {
            best_token_index = tok_index;
            highest_consumption = consumption;
        }
    }

    if (highest_consumption)
    {
        type = Token::Type::from_index(best_token_index);
    }

    return highest_consumption;
}

size_t dispose_token(Str in)
{
    #define DISPOSER_funcs(FUNC) FUNC,
    static size_t (*dispose_funcs[])(Str in) = {
        DISPOSER(DISPOSER_funcs)
    };

    static const size_t dispose_func_n = sizeof(dispose_funcs) / sizeof(dispose_funcs[0]);

    for (size_t i = 0; i < dispose_func_n; i++)
    {
        size_t disposed = dispose_funcs[i](in);

        if (disposed)
        {
            return disposed;
        }
    }

    return 0;
}

Token recycle_leftovers(Str leftovers)
{
    for (size_t i = 0; i < recycle_func_n; i++)
    {
        if (recycle_funcs[i](leftovers))
        {
            return Token(Token::Type::from_index(parser_func_n + i), leftovers);
        }
    }

    return Token(Token::Error, leftovers);
}

size_t parse_token(Str in, Vec<Token>& tokens)
{
    Token::Type type = Token::Error;
    size_t disposed = 0;
    size_t consumed = 0;
    size_t leftovers = 0;

    for (size_t i = 0; i < in.length(); i++)
    {
        disposed = dispose_token(in.substr(i));

        if (disposed)
        {
            break;
        }

        consumed = eat_token(in.substr(i), type);

        if (type != Token::Error) break;

        leftovers++;
    }

    if (leftovers)
    {
        tokens.push_back(recycle_leftovers(in.substr(0, leftovers)));
    }

    if (disposed)
    {
        return leftovers + disposed;
    }

    if (consumed)
    {
        tokens.push_back(Token(type, in.substr(leftovers, consumed)));
    }

    return leftovers + consumed;
}

Vec<Token> tokenise(Str in)
{
    Vec<Token> tokens;
    size_t i = 0;

    while (i < in.length())
    {
        i += parse_token(in.substr(i), tokens);
    }

    return tokens;
}
/*
keep eating characters nobody wants to eat, accumulate Leftovers
when there is a good token add it
*/

int main(int argc, const char* argv[])
{
    DynaStr str;

    std::cout << "Enter string: ";
    std::getline(std::cin, str);

    std::cout << "input string: " << str << std::endl;

    Vec<Token> tokens = tokenise(str);

    for (const Token& tok : tokens)
    {
        std::cout << tok.to_string() << std::endl;
    }

    return 0;
}
