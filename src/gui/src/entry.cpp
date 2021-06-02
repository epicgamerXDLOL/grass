#include "entry.h"
#include "common.h"
#include <iostream>


gui::TextEntry::TextEntry(SDL_Rect rect, const Text& text, SDL_Color bg_col, SDL_Color cursor_col)
    : m_rect(rect), m_text(text), m_background_color(bg_col), m_cursor_color(cursor_col)
{
    m_display_cursor_pos = { m_rect.x, m_rect.y };
    m_real_cursor_pos = { m_rect.x, m_rect.y };

    m_min_visible_indexes = { 0, 0 };
    m_max_visible_indexes = { m_rect.w / m_text.char_dim().x, m_rect.h / m_text.char_dim().y };
}


void gui::TextEntry::render(SDL_Renderer* rend)
{
    SDL_SetRenderDrawColor(rend, m_background_color.r, m_background_color.g, m_background_color.b, 255);
    SDL_RenderFillRect(rend, &m_rect);
    
    int y = (m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y;

    placeholder_at_cache(y);
    placeholder_at_cache(y - 1);

    for (int i = 0; i < m_cached_textures.size(); ++i)
    {
        std::string line = m_text.get_line(i + m_min_visible_indexes.y);

        if (m_min_visible_indexes.x >= line.size())
            continue;

        std::string visible = line.substr(m_min_visible_indexes.x, std::min((int)line.size(), m_max_visible_indexes.x + 1) - m_min_visible_indexes.x);

        if (!m_cached_textures[i].get())
        {
            m_cached_textures[i] = std::unique_ptr<SDL_Texture, common::TextureDeleter>(common::render_text(rend, m_text.font(), visible.c_str(), m_text.color()));
        }

        SDL_Rect rect = {
            m_rect.x,
            m_rect.y + m_text.char_dim().y * i, 
            m_text.char_dim().x * (int)visible.size(), 
            m_text.char_dim().y
        };

        if (m_cached_textures[i].get())
        {
            SDL_RenderCopy(rend, m_cached_textures[i].get(), nullptr, &rect);
        }
    }

    // Convenient debug stuff, dont delete this
    /*std::cout << "real: " << real_to_char_pos(m_real_cursor_pos).x << " | " << real_to_char_pos(m_real_cursor_pos).y << "\n";
    std::cout << "display: " << m_display_cursor_pos.x << " | " << m_display_cursor_pos.y << "\n";
    std::cout << "min bound: " << m_min_visible_indexes.x << " | " << m_min_visible_indexes.y << "\n";
    std::cout << "max bound: " << m_max_visible_indexes.x << " | " << m_max_visible_indexes.y << "\n";*/
}


void gui::TextEntry::add_char(char c)
{
    SDL_Point coords = real_to_char_pos(m_real_cursor_pos);
    m_text.insert(coords.x, coords.y, c);

    if (c == '\n')
    {
        std::string line = m_text.get_line(coords.y);
        std::string copied;

        copied = line.substr(coords.x, line.size());
        m_text.get_line_ref(coords.y).erase(coords.x, line.size());

        // move cursor to beginning of line
        reset_bounds_x();

        // manually move the cursors because display cursor x is limited to the right side of the text box while real cursor is not
        move_real_cursor(-((m_real_cursor_pos.x - m_rect.x) / m_text.char_dim().x), 1);
        move_display_cursor(-((m_display_cursor_pos.x - m_rect.x) / m_text.char_dim().x), 1);

        if (check_bounds(0, 1))
            m_cached_textures.erase(m_cached_textures.begin());

        m_text.set_line(coords.y + 1, copied);
        m_cached_textures.emplace_back(nullptr);

        clear_cache();
    }
    else
    {
        move_cursor(1, 0);
    }
}


void gui::TextEntry::remove_char(int count)
{
    for (int i = 0; i < count; ++i)
    {
        bool nl = false;
        int bounds_diff = 0;

        if ((m_real_cursor_pos.x - m_rect.x) == 0)
        {
            if (real_to_char_pos(m_real_cursor_pos).y != 0) // only move up if not at top of text box
            {
                // nl variable to move the cursor down at the end of the function
                nl = true;
                int diff = m_text.contents()[(m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y - 1].size();
                bounds_diff = diff - m_rect.w / m_text.char_dim().x;

                // seemingly redundant cursor moving is for jump_to_eol to make sure the cursor moves to the end of the correct line, not the empty current one
                move_cursor(diff, -1, false);
                jump_to_eol(false);

                int y = (m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y;
                m_text.set_line(y, m_text.get_line(y) + m_text.get_line(y + 1));
                m_text.set_line(y + 1, "");

                move_cursor(0, 1, false);

                remove_texture_from_cache((m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y - m_min_visible_indexes.y);

                if (m_text.contents().size() > m_max_visible_indexes.y)
                    m_cached_textures.emplace_back(nullptr);

                clear_cache();
            }
            else
            {
                continue;
            }
        }
        else
        {
            move_cursor(-1, 0);
        }

        SDL_Point coords = real_to_char_pos(m_real_cursor_pos);
        m_text.erase(coords.x, coords.y);
        
        if (nl)
        {
            move_cursor(0, -1, false);
            check_bounds(bounds_diff, -1);
        }
    }
}


bool gui::TextEntry::check_clicked(int mx, int my)
{
    return common::within_rect(m_rect, mx, my);
}


void gui::TextEntry::move_cursor(int x, int y, bool check)
{
    int new_y = ((m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y) + y;

    if (new_y < m_text.contents().size() && new_y >= 0)
    {
        move_display_cursor(x, y);
        move_real_cursor(x, y);

        if (check)
            check_bounds(x, y);

        if (!cursor_visible())
        {
            SDL_Point cursor_coords = real_to_char_pos(m_real_cursor_pos);

            int x_diff = cursor_coords.x - m_min_visible_indexes.x;
            int y_diff;

            if (m_min_visible_indexes.y >= cursor_coords.y)
            {
                y_diff = cursor_coords.y - m_min_visible_indexes.y;
            }

            if (m_max_visible_indexes.y <= cursor_coords.y)
            {
                y_diff = cursor_coords.y - (m_max_visible_indexes.y - 1);
            }
            

            move_bounds(x_diff, y_diff);

            SDL_Point display_cursor_coords = real_to_char_pos(m_display_cursor_pos);
            move_display_cursor(-display_cursor_coords.x, 0);

            clear_cache();
        }
    }
}


SDL_Point gui::TextEntry::real_to_char_pos(SDL_Point pos)
{
    return {
        (pos.x - m_rect.x) / m_text.char_dim().x,
        (pos.y - m_rect.y) / m_text.char_dim().y
    };
}


bool gui::TextEntry::move_bounds(int x, int y)
{
    if (m_min_visible_indexes.x + x >= 0 && m_min_visible_indexes.y + y >= 0)
    {
        m_min_visible_indexes.x += x;
        m_min_visible_indexes.y += y;

        // dont want to move maximum if minimum is invalid, that would cause the bounds to shrink
        m_max_visible_indexes.x += x;
        m_max_visible_indexes.y += y;

        return true;
    }

    return false;
}


void gui::TextEntry::reset_bounds_x()
{
    m_min_visible_indexes.x = 0;
    m_max_visible_indexes.x = m_rect.w / m_text.char_dim().x;
}


void gui::TextEntry::reset_bounds_y()
{
    m_min_visible_indexes.y = 0;
    m_max_visible_indexes.y = m_rect.h / m_text.char_dim().y;
}


void gui::TextEntry::set_cursor_pos(int x, int y)
{
    m_display_cursor_pos = { x * m_text.char_dim().x + m_rect.x, y * m_text.char_dim().y + m_rect.y };
    m_real_cursor_pos = { x * m_text.char_dim().x + m_rect.x, y * m_text.char_dim().y + m_rect.y };
}


bool gui::TextEntry::check_bounds(int x, int y)
{
    int max_x = m_rect.x + ((int)((float)m_rect.w / (float)m_text.char_dim().x) * m_text.char_dim().x);
    if (m_display_cursor_pos.x < m_rect.x || m_display_cursor_pos.x > max_x)
    {
        bool moved = move_bounds(x, 0);
        m_display_cursor_pos.x = std::min(std::max(m_rect.x, m_display_cursor_pos.x), max_x);

        if (moved)
            clear_cache();

        return moved;
    }

    int max_y = m_rect.y + ((int)(m_rect.h / m_text.char_dim().y) * m_text.char_dim().y) - m_text.char_dim().y;
    if (m_display_cursor_pos.y < m_rect.y || m_display_cursor_pos.y > max_y)
    {
        bool moved = move_bounds(0, y);
        m_display_cursor_pos.y = std::min(std::max(m_rect.y, m_display_cursor_pos.y), max_y);

        if (moved)
            shift_cache(y);

        return moved;
    }

    return false;
}


void gui::TextEntry::move_real_cursor(int x, int y)
{
    m_real_cursor_pos.x += x * m_text.char_dim().x;
    m_real_cursor_pos.y += y * m_text.char_dim().y;

    m_real_cursor_pos = {
        std::max(m_rect.x, m_real_cursor_pos.x),
        std::max(m_rect.y, m_real_cursor_pos.y)
    };

    // make sure real cursor doesnt move farther than the amount of text in m_text.contents()
    if (m_real_cursor_pos.y >= m_rect.y + m_text.contents().size() * m_text.char_dim().y)
        m_real_cursor_pos.y = m_rect.y + (m_text.contents().size() - 1) * m_text.char_dim().y;
}


void gui::TextEntry::move_display_cursor(int x, int y)
{
    m_display_cursor_pos.x += x * m_text.char_dim().x;
    m_display_cursor_pos.y += y * m_text.char_dim().y;
}


void gui::TextEntry::jump_to_eol(bool check)
{
    // text box left position + string size * width of each character
    int real_end_of_line = (int)(m_rect.x + m_text.get_line(real_to_char_pos(m_real_cursor_pos).y).size() * m_text.char_dim().x);

    if (m_real_cursor_pos.x != real_end_of_line)
    {
        m_real_cursor_pos.x = real_end_of_line;

        // text box x position + current line size * character width
        m_display_cursor_pos.x = m_rect.x + (m_text.contents()[(m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y].size() - m_min_visible_indexes.x) * m_text.char_dim().x;

        // move bounds if cursor goes too far right
        bool moved = false;
        
        if (check)
        {
            moved = check_bounds((m_real_cursor_pos.x - m_rect.x - m_max_visible_indexes.x * m_text.char_dim().x) / m_text.char_dim().x, 0);
        }

        // move bounds if cursor goes too far left
        if (check && !moved)
        {
            int current_line_size = m_text.get_line((m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y).size();

            if (m_min_visible_indexes.x > current_line_size)
            {
                // make sure the cursor shifts if the line is off screen
                m_display_cursor_pos.x = -1;
            }

            check_bounds(m_text.get_line((m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y).size() - m_min_visible_indexes.x, 0);
        }
    }
}


void gui::TextEntry::draw_cursor(SDL_Renderer* rend)
{
    if (cursor_visible())
    {
        SDL_SetRenderDrawColor(rend, m_cursor_color.r, m_cursor_color.g, m_cursor_color.b, 255);

        SDL_Rect tmp = {
            std::max(m_display_cursor_pos.x, m_rect.x + 1) - 1,
            m_display_cursor_pos.y,
            1,
            m_text.char_dim().y
        };

        SDL_RenderFillRect(rend, &tmp);
    }
}


void gui::TextEntry::placeholder_at_cache(int index)
{
    if (index - m_min_visible_indexes.y < 0)
        return;

    if (index - m_min_visible_indexes.y >= (int)m_cached_textures.size())
    {
        m_cached_textures.emplace_back(nullptr);
    }
    else
    {
        m_cached_textures[index - m_min_visible_indexes.y] = nullptr;
    }
}


void gui::TextEntry::clear_cache()
{
    for (int i = 0; i < m_cached_textures.size(); ++i)
    {
        m_cached_textures[i] = nullptr;
    }
}


void gui::TextEntry::remove_texture_from_cache(int index)
{
    m_cached_textures.erase(m_cached_textures.begin() + index);
}


void gui::TextEntry::update_cache()
{
    m_cached_textures.clear();
    m_cached_textures = std::vector<std::unique_ptr<SDL_Texture, common::TextureDeleter>>(std::min(m_max_visible_indexes.y, (int)m_text.contents().size()) - m_min_visible_indexes.y);
}


void gui::TextEntry::shift_cache(int y)
{
    if (y > 0)
    {
        for (int i = 0; i < y; ++i)
        {
            m_cached_textures.erase(m_cached_textures.begin());
            m_cached_textures.emplace_back(nullptr);            
        }
    }
    else
    {
        for (int i = y; i < 0; ++i)
        {
            m_cached_textures.pop_back();
            m_cached_textures.insert(m_cached_textures.begin(), nullptr);
        }
    }
}


void gui::TextEntry::resize_to(int w, int h)
{
    m_rect.w = w - m_rect.x;
    m_rect.h = h - m_rect.y;

    m_max_visible_indexes = { 
        m_min_visible_indexes.x + (int)(m_rect.w / m_text.char_dim().x),
        m_min_visible_indexes.y + (int)(m_rect.h / m_text.char_dim().y)
    };

    m_display_cursor_pos = {
        std::min((m_max_visible_indexes.x - m_min_visible_indexes.x) * m_text.char_dim().x + m_rect.x, m_display_cursor_pos.x),
        std::min((m_max_visible_indexes.y - m_min_visible_indexes.y) * m_text.char_dim().y + m_rect.y - m_text.char_dim().y, m_display_cursor_pos.y)
    };

    m_real_cursor_pos = {
        std::min(m_max_visible_indexes.x * m_text.char_dim().x + m_rect.x, m_real_cursor_pos.x),
        std::min(m_max_visible_indexes.y * m_text.char_dim().y + m_rect.y - m_text.char_dim().y, m_real_cursor_pos.y)
    };

    if (m_text.get_line((m_real_cursor_pos.y - m_rect.y) / m_text.char_dim().y).size() <= (m_real_cursor_pos.x - m_rect.x) / m_text.char_dim().x)
        jump_to_eol();

    update_cache();
}


void gui::TextEntry::scroll(int y)
{
    m_min_visible_indexes.y += y;
    m_max_visible_indexes.y += y;

    shift_cache(y);
    move_display_cursor(0, -y);
}


bool gui::TextEntry::cursor_visible()
{
    SDL_Point coords = real_to_char_pos(m_real_cursor_pos);

    return coords.x >= m_min_visible_indexes.x && coords.x <= m_max_visible_indexes.x
        && coords.y >= m_min_visible_indexes.y && coords.y < m_max_visible_indexes.y;
}