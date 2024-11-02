#pragma once

#include <iostream>
#include <string_view>

struct Code {
    std::string_view code;
    std::string_view unicode;
    std::string_view latex;

    float y_off;
};

struct Token {
    enum class Type {
        String,
        Symbol
    };

    Type type;
    std::string s;
    const Code *code;

    Token(Type t, std::string s) : type(t), s(s), code(nullptr)
    {}
    Token(Type t, const Code *c) : type(t), code(c)
    {}

    void print();
};

class TextFormatter {
public:

    void read(std::istream &in);

    void render(Font font, Font math_font, int font_size);
    void latex(std::ostream &out);
private:
    std::vector<Token> format_line(std::string_view line);

    std::vector<std::vector<Token>> lines;
};

extern const char *codepoints;
extern const std::array<Code, 3> codes;
