# Lexical analysis
Imagine understanding a sentence by reading the individual characters alone. It would be quite difficult wouldn't it? 

Lexical analysis or tokenising is the process of grouping characters into words or so called 'tokens' using a predetermined grammar. It is used extensively in compilers, programming languages and natural language processing to transform a meaningless sequence of characters into a sequence of tokens which the computer can work with.

This article will attempt to explain the workings of lexical analysis and how to write a lexer of your own.

## EXAMPLE
Starting with a pratical example, the "Hello, World!" programme in C would typically look like this before tokenising:

```
#include <stdio.h>

int main()
{
    puts("Hello, World!");
}
```

However it may look like this after being tokenised by a compiler:

[`#`] [`include`] [` `] [`<`] [`stdio.h`] [`>`] [`\n`]

[`\n`]

[`int`] [` `] [`main`] [`(`] [`)`] [`\n`] [`{`] [`\n`]

[` `] [` `] [` `] [` `] [`puts`] [`(`] [`"Hello, World!"`] [`)`] [`;`] [`\n`] 

[`}`]

Do you notice that the characters have been chunked together. The programme has been converted from a sequence of characters to sequence of words or tokens (like a sentence). 

It is also in this stage that a few very basic syntax errors are caught. For example if you missed the closing `"` in the string literal `"Hello, World!"` then the compiler would likely give an error.

NOTE:
To avoid confusion whitespace and newline characters in the example above have been retained. However in reality many compilers will remove these tokens as they have little value in most programming languages.

## Grammar
Just like English has grammar so do programming languages. Grammar defines the rules for tokenising a language. 

There can be many different types of tokens. This is an outline of the five main types:

- Punctuation (`{`, `}`, `;`, `<` , `>`, `>=`, `+`, `-`, `.`, `,`, etc)
- Whitespace (`\t`, `\n`, `' '`)
    - in some programming languages whitespace has semantic meaning (for example, Python)
- Identifiers & keywords (`main`, `x`, `my_var`, `int`, `return`, `😎`?)
    - these are typically case sensitive, start with a letter followed by any combination of alphanumeric characters but may also support unicode characters and emojis using leftover characters (more on this later)
- Numerical tokens (`42`, `3.14`, `0xF00D`)
- String literals (`"Hello!"`, `"This is \"a string\""`)

## Writing a lexer
A lexer is the algorithm which preforms the tokenisation step, breaking source code into tokens. It can be quite challenging to write a good lexer. There are many edge cases and these need to be handled effectively in order to produce the correct output.

### The scanner
When performing lexical analysis it is important to keep track of the current position in the source code and to be able to match against certain patterns. A scanner provides this convenience and will assist greatly you during parsing. It operates on a buffer or source file and will typically have the ability to:

- Advance by `n` characters
- Peek at characters without advancing
- Skip a sequence of characters
    - first checks whether a given string (for example "cat") matches the next following characters
    - if it does the scanner advances its position in the buffer (by 3 characters in the case of "cat")
- Skip a character if it is present in a character set
    - for example a character set of "abcdefghijklmnopqrstuvwxyz" would match a single alphabetical letter
- Skip a character if it is NOT present in a character set
- Take a substring up to a certain point

These basic functionalities can be used in conjunction to parse tokens. For example this code could be used to parse numerical tokens:

```c
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
```

There could also be a parsing function for string literals or punctuation tokens, etc. It all builds on the same idea. Skip over the characters you are interested in and return whether or not the token was accepted. While it is possible to [write a lexer in other ways](/res/simple_lexer.cpp), using a scanner is one of the most easiest and least error-prone approaches.

I maintain a scanner implementation at https://github.com/masterneko/scanner

### Token declarations
Eventually you will implement quite a few token parsing functions. But in a source file with many different combinations of tokens how will a lexer know which function to call?

```c
bool parse_comment(Scanner& scanner);
bool parse_number(Scanner& scanner);
bool parse_string(Scanner& scanner);
bool parse_more_than_or_equal(Scanner& scanner);
bool parse_more_than(Scanner& scanner);
bool parse_plus(Scanner& scanner);
bool parse_minus(Scanner& scanner);
bool parse_times(Scanner& scanner);
bool parse_divide(Scanner& scanner);
bool parse_space(Scanner& scanner);
```

A lexer would typically use a list of token declarations. In this list each token would be assigned a type ID and an associated parsing function. While it is possible to define a separate enum and an array of function pointers, a cool C/C++ trick is [X macros](https://en.wikipedia.org/wiki/X_macro). These allow you to define token declarations in a tabular format, all in one place.

```c++
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
```

Additionally you should also define some recycling functions. These will be used in the case that the lexer found leftover characters - an unknown token. The lexer will then attempt to repurpose these unknown characters by trying to find a recycler which accepts these characters. In our language any leftover characters will be turned into identifiers. This also has the side effect of allowing emojis which is cool 😎.

```c++
bool recycle_identifier(std::string_view leftovers)
{
    return true; /* accept regardless of the leftovers' contents */
}

#define RECYCLERS(X) \
      /* Type */       /* Recycling func */ \
    X(Identifier,     recycle_identifier)
```

You will also need a Token class, this will store all information related to each individual token including its type and location within the source file as well as any text it contains:

```c++
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
```

You may have noticed that there is a token of type `Token::Error` and this is what the lexer creates when there is a partially digested or a non-recyclable token.

### The lexer
You are 75% there. The last step is the lexer itself which will use the token declarations as defined earlier to convert the source file into tokens. This can however be one of the most difficult parts to master.


#### *Worked example:*

Take the following source code. The scanner will initially start off at the beginning where the arrow is pointing:
```
2 + num >= 5
^
```

The lexer should check what the kind of token is at this position. So it does a search `parse_comment()`: no, `parse_number()`: Yes - it found a match. The lexer forms a token containing the text it matched along with the location within the source file (location includes index, line and column number). It adds this to a list and moves on.

```
2 + num >= 5
 ^
```

Token list:
```
Number(text: "2", index: 0, line: 1, col: 1)
```

From here the next character in the sequence is a space character. `parse_comment()`: no, `parse_number()`: no, `parse_string()`: no. Eventually it finds at the bottom of the token declarations is `parse_space()`: Yes. The lexer adds the space token to the list and moves on.

```
2 + num >= 5
  ^
```

Token list:
```
Number(text: "2", index: 0, line: 1, col: 1)
Space(text: " ", index: 1, line: 1, col: 2)
```

Note: that the lexer is not concerned about converting number tokens into integers or floats. This can be done later. The main purpose of a lexer is to group sequences of characters into categories. Type conversion is inherently context driven (eg. `float x = 42`, here `42` would depend on knownledge that `x` is a `float` to be converted properly). Deriving context and relationships between tokens is the role of a syntatical analyser not lexical analysis.

The next character is parsed using `parse_plus` and a `Plus` token is produced and the scanner moves on.

```
2 + num >= 5
   ^
```

Token list:
```
Number(text: "2", index: 0, line: 1, col: 1)
Space(text: " ", index: 1, line: 1, col: 2)
Plus(text: "+", index 2, line: 1, col: 3)
```

The next character is parsed by `parse_space` and another `Space` token is produced. The scanner is moved forward.

```
2 + num >= 5
    ^
```

Token list:
```
Number(text: "2", index: 0, line: 1, col: 1)
Space(text: " ", index: 1, line: 1, col: 2)
Plus(text: "+", index 2, line: 1, col: 3)
Space(text: " ", index: 3, line: 1, col: 4)
```

The character 'n' is next. Does `parse_comment()` work? no, how about `parse_number()`? no, `parse_string()`? no, `parse_more_than_or_equal`: no, `parse_more_than`: no, `parse_plus`: no, `parse_minus`: no, `parse_times`: no, `parse_divide`: no, `parse_space`: no. Oh it seems like this character has been rejected by all the parsing functions. We'll come back to this character later meanwhile we'll see if the next character can be parsed.

```
2 + num >= 5
    *^
```

'u' also got rejected. We'll mark it with another asterisk and come back to this character once we find a token that can be parsed. 

```
2 + num >= 5
    **^
```

'm' doesn't seem to be a match either. Again we mark this character for later and move on to the next character.

```
2 + num >= 5
    ***^
```

This time we are in luck. `' '` is recognised by `parse_space`. Now the lexer can recycle those old rejected leftover characters into a token. 

The one and only recycler `recycle_identifier` is called: `recycle_identifier("num")` and returns true indicating its been accepted so a new identifier token is created out of these leftover characters.

Both the recycled `Identifier` and `Space` tokens are added to the token list.

Token list:
```
Number(text: "2", index: 0, line: 1, col: 1)
Space(text: " ", index: 1, line: 1, col: 2)
Plus(text: "+", index 2, line: 1, col: 3)
Space(text: " ", index: 3, line: 1, col: 4)
Identifier(text: "num", index: 4, line: 1, col: 5)
Space(text: " ", index: 7, line: 1, col: 8)
```

The scanner proceeds and now the current character is '>'

```
2 + num >= 5
        ^
```

Now there is actually two possible parsing functions the lexer could use: `parse_more_than` ('`>`') or `parse_more_than_or_equal` ('`>=`') since '>' is a substring of '>='. Logically `parse_more_than_or_equal` would be the sensible solution, however without understanding the concept of greediness the lexer could interpret this as separate `>` and `=` tokens. This is why lexers are greedy algorithms meaning they try to consume as many characters as possible.

In the end the lexer finds that `parse_more_than_or_equal` yields the most characters matched. So a `MoreThanOrEqual` token is created. The scanner as usual moves forward.

```
2 + num >= 5
          ^
```

Token list:
```
Number(text: "2", index: 0, line: 1, col: 1)
Space(text: " ", index: 1, line: 1, col: 2)
Plus(text: "+", index 2, line: 1, col: 3)
Space(text: " ", index: 3, line: 1, col: 4)
Identifier(text: "num", index: 4, line: 1, col: 5)
Space(text: " ", index: 7, line: 1, col: 8)
MoreThanOrEqual(text: ">=", index: 8, line: 1, col: 9)
```

Before reaching the end of file (EOF) the lexer finds two more tokens. These are `Space` and `Number`.

```
2 + num >= 5
            ^ EOF
```

So the final token list is:
```
Number(text: "2", index: 0, line: 1, col: 1)
Space(text: " ", index: 1, line: 1, col: 2)
Plus(text: "+", index 2, line: 1, col: 3)
Space(text: " ", index: 3, line: 1, col: 4)
Identifier(text: "num", index: 4, line: 1, col: 5)
Space(text: " ", index: 7, line: 1, col: 8)
MoreThanOrEqual(text: ">=", index: 8, line: 1, col: 9)
Space(text: " ", index: 10, line: 1, col: 11)
Number(text: "5", index: 11, line: 1, col: 12)
```

That is the basic algorithm of a lexer. If you were concatenate the text from each token, it would be the same as the starting source:

```c
  "2", " ", "+", " ", "num", " ", ">=", " ", "5"

= "2 + num >= 5"
```

### Lexer code
A main part of the lexer alogrithm is determining the greediest token. As seen in the example above, greediness is an important aspect to lexical analysis. It can mean the difference between the code being interpreted as separate `>` and `=` tokens or as a single `>=` token. This function will try out each parser function to determine which token consumes the highest number of characters. It will store the type of token it is in `best_type` and return the end position, the point at which the token ends.

```c++
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
```

We will need to create tokens at some point, so here is a utility function to do that:


```c++
Token create_token(Scanner::iterator start_it, Token::Type type)
{
    Token tok;

    tok.type = type;
    // take the text present between start_it and the current character
    tok.text = start_it.slice();

    Scanner::iterator::location loc = start_it.get_location();

    tok.index = loc.index;
    tok.line = loc.line;
    tok.col = loc.column;

    return tok;
}
```

The `find_greediest_token` function will not always match a token, so it is important we collect the characters that weren't consumed so they can be recycled. This function is responsible for adding a token to the list and if requred will recycle leftover characters.

```c++
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
            // advance by one character (this will become a leftover character)
            scanner++;
        }
        else
        {
            if (leftover_start != token_start) /* if there are leftovers */
            {
                // recycle the token into a usable type
                tokens.push_back(recycle_token(leftover_start));
            }

            // skip forward to the token's end
            scanner = token_end;

            // add the parsed token to the list
            tokens.push_back(create_token(token_start, token_type));

            break;
        }
    }
}
```

The `recyle_token` function is quite simple, it will call each recylcler function to check whether the token can be recycled. If not it will create an error token.

```c++
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
```

### tokenise_text

This function puts everything together in order to produce a list of tokens.

```c++
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
```


## Conclusion
Hopefully you now know how lexical analysis works and how to write an effective lexer. The code shown in this guide is available in full [here](/res/lexer.cpp).

