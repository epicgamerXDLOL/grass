#pragma once
#include "text.h"
#include "cursor.h"
#include "common.h"
#include <memory>


namespace gui
{
    enum class EntryMode
    {
        NORMAL,
        HIGHLIGHT
    };

    class TextEntry
    {
    public:
        TextEntry(SDL_Rect rect, SDL_Color bg_color, const Cursor& cursor, const Text& text);

        void render(SDL_Renderer* rend, bool show_cursor = false);

        bool check_clicked(int mx, int my);

        // adds a character to where the cursor currently is
        void insert_char(char c);
        // removes a character where the cursor currently is
        void remove_char();

        void move_cursor_characters(int x, int y);
        // moves cursor to end of line, returns true if cursor has gone out of bounds
        bool jump_to_eol();
        // jump to eol and moves bounds if cursor is too far right
        bool conditional_jump_to_eol();
        // move bounds by (x, y) characters if the cursor is out of bounds
        void conditional_move_bounds_characters(int x, int y);
        
        // moves bounds by (x, y) characters
        void move_bounds_characters(int x, int y);

        void reset_bounds_x();
        void reset_bounds_y();

        bool out_of_bounds();
        bool out_of_bounds_x();
        bool out_of_bounds_y();

        void clear_cache();
        // marks m_cached_textures[i] to be re rendered
        void placeholder_at_cache(int i);
        // resize and clear the cache to fit current text size
        void update_cache();
        // move the cache vertically by y characters
        void shift_cache(int y);


        void mouse_down(int mx, int my);
        void mouse_up();

        void move_cursor_to_click(int mx, int my);
        // x and y measured in characters
        void set_cursor_pos_characters(int x, int y);
        
        void stop_highlight();
        // stop highlight if nothing is highlighted
        void conditional_stop_highlight();

        void draw_highlighted_areas(SDL_Renderer* rend);
        // y_index measured in chars
        void highlight_line(SDL_Renderer* rend, int y_index);
        // y_index measured in chars, x2 and x2 measured in pixels
        void highlight_section(SDL_Renderer* rend, int y_index, int x1, int x2);
        
        void erase_highlighted_section();


        void resize_to(int w, int h);

        void scroll(int y);


        void hide() { m_hidden = true; }
        void show() { m_hidden = false; }


        Text* text() { return &m_text; }
        SDL_Rect rect() { return m_rect; }
        Cursor cursor() { return m_cursor; }
        int move_bounds_by() { return m_move_bounds_by; }
        bool hidden() { return m_hidden; }
        SDL_Point min_bounds() { return m_min_bounds; }
        SDL_Point max_bounds() { return m_max_bounds; }

        void set_bounds_movement(int amount) { m_move_bounds_by = amount; }

    private:
        SDL_Rect m_rect;
        Cursor m_cursor;
        SDL_Color m_bg_color;

        // measured in characters
        SDL_Point m_min_bounds, m_max_bounds;

        Text m_text;

        std::vector<std::unique_ptr<SDL_Texture, common::TextureDeleter>> m_cached_textures;

        int m_move_bounds_by{ 5 };

        // not rendered
        Cursor m_highlight_start;
        EntryMode m_mode{ EntryMode::NORMAL };

        bool m_hidden{ false };
    };
}
