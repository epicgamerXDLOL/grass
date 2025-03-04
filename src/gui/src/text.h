#pragma once
#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>


namespace gui
{
    // not renderable, only contains information helpful in rendering text that spans over multiple lines
    class Text
    {
    public:
        Text() = default;

        Text(TTF_Font* font, SDL_Point pos, const std::string& contents, SDL_Point char_dimensions, SDL_Color col);

        /* Inserts a character at m_contents[y][x]. */
        void insert(int x, int y, char c);

        /* Erases a character at m_contents[y][x].
        * Erases a new line like a normal character by default, but if erase_nl is set to false it will not erase new lines.
        */
        void erase(int x, int y, bool erase_nl = true);

        void remove_line(int i) { m_contents.erase(m_contents.begin() + i); }


        // getters and setters

        // Get m_contents in string form.
        std::string str();

        std::string get_line(int i) const { return i >= m_contents.size() ? "" : m_contents[i]; }
        std::string& get_line_ref(int i) { return m_contents[i]; }

        std::vector<std::string> contents() const { return m_contents; }
        void set_contents(const std::vector<std::string>& contents) { m_contents = contents; }

        void set_line(int i, const std::string& text) { m_contents[i] = text; }
        void insert_line(int i) { m_contents.insert(m_contents.begin() + i, ""); }

        SDL_Point char_dim() const { return m_char_dim; }

        TTF_Font* font() { return m_font; }
        SDL_Color color() const { return m_color; }

    private:
        SDL_Rect m_rect;

        // character dimensions
        SDL_Point m_char_dim;
        std::vector<std::string> m_contents;
        SDL_Color m_color;

        // non owning, dont free
        TTF_Font* m_font;
    };
}
