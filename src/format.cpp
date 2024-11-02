#include <string_view>
#include <vector>

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <raylib.h>

#include "format.hpp"

#include <dbg.h>

const char *codepoints = "\u2227\u2228\u00ac "; // Last space apparently required
const std::array<Code, 3> codes = {
    Code{ ":a", "\u2227", "\\wedge",    0.075f },
    Code{ ":o", "\u2228", "\\vee",      0.075f },
    Code{ ":n", "\u00ac", "\\neg",      0.075f }
};

void TextFormatter::read(std::istream &in) {
    lines.clear();

    std::string buf;
    while (std::getline(in, buf)) {
        lines.push_back(format_line(buf));
    }
}

std::vector<Token> TextFormatter::format_line(std::string_view line) {
    std::vector<Token> res;

    size_t pos;
    while (line.contains(':')) {
        pos = line.find(':');

        res.push_back({ Token::Type::String, std::string(line.substr(0, pos)) });

        line = line.substr(pos);
        bool found = false;
        for (const auto &code : codes) {
            if (line.size() < code.code.size())
                continue;

            if (line.substr(0, code.code.size()) == code.code) {
                res.push_back({ Token::Type::Symbol, &code });
                line = line.substr(code.code.size());
                found = true;
                break;
            }
        }

        // We didn't actually use it, so add
        // the colon to the rendered string
        if (!found) {
            res.back().s.push_back(line[0]);
            line = line.substr(1);
        }
    }

    if (!line.empty())
        res.push_back({ Token::Type::String, std::string(line) });

#if 0
    for (auto t : res)
        t.print();
#endif

    return res;
}

void TextFormatter::render(Font font, Font math_font, int font_size)
{
    const int spacing = 1;

    float y = 0;
    for (const auto &line : lines) {
        float x = 0;
        bool need_space = false;
        bool ended_on_space = false;
        for (auto tok : line) {
            if (tok.type == Token::Type::String) {
                if (need_space && !std::isspace(tok.s.front())) {
                    x += MeasureTextEx(font, " ", font_size, spacing).x;
                }

                DrawTextEx(font, tok.s.c_str(), 
                        {x, y}, 
                        font_size, spacing, BLACK);
                x += MeasureTextEx(font, tok.s.c_str(), font_size, spacing).x;

                need_space = false;
                ended_on_space = std::isspace(tok.s.back());
            } else {
                if (!ended_on_space || need_space) {
                    x += MeasureTextEx(font, " ", font_size, spacing).x;
                }

                DrawTextEx(math_font, tok.code->unicode.data(), 
                        {x, y + (tok.code->y_off * font_size)}, 
                        font_size, spacing, BLACK);
                x += MeasureTextEx(math_font, tok.code->unicode.data(), font_size, spacing).x;

                need_space = true;
                ended_on_space = false;
            }
        }

        y += (float)font_size;
    }
}

void Token::print()
{
    if (type == Type::String)
        fmt::println("String \"{}\"", s);
    else
        fmt::println("Token \"{}\"", code->code);
}
