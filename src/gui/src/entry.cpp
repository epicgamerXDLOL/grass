#include "entry.h"
#include "common.h"
#include <iostream>


gui::TextEntry::TextEntry(SDL_Rect rect, const Text& text, SDL_Color bg_col)
    : m_rect(rect), m_text(text), m_background_color(bg_col)
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

    m_visible_content = get_visible_content();

    Text tmp = m_text;
    tmp.set_contents(m_visible_content);

    tmp.render(rend);

    std::cout << real_to_char_pos(m_real_cursor_pos).x << " | " << real_to_char_pos(m_real_cursor_pos).y << "\n";
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

        reset_bounds_x();

        // move cursor to beginning of line
        move_display_cursor(-(m_display_cursor_pos.x / m_text.char_dim().x), 1);
        move_real_cursor(-(m_real_cursor_pos.x / m_text.char_dim().x), 1);
        
        m_text.set_line(coords.y + 1, copied);

        check_bounds(0, -1);
    }
    else
    {
        move_cursor(1, 0);
    }

    m_visible_content = get_visible_content();
}


void gui::TextEntry::remove_char(int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (m_text.get_line(real_to_char_pos(m_real_cursor_pos).y).size() == 0)
        {
            if (real_to_char_pos(m_real_cursor_pos).y != 0) // only move up if not at top of text box
            {
                int diff = m_text.contents()[m_text.contents().size() - 2].size();
                move_cursor(diff, -1);
                jump_to_eol();
            }
        }
        else
        {
            move_cursor(-1, 0);
        }

        SDL_Point coords = real_to_char_pos(m_real_cursor_pos);
        m_text.erase(coords.x, coords.y);
    }

    m_visible_content = get_visible_content();
}


bool gui::TextEntry::check_clicked(int mx, int my)
{
    return common::within_rect(m_rect, mx, my);
}


std::vector<std::string> gui::TextEntry::get_visible_content()
{
    std::vector<std::string> visible;

    for (int i = 0; i < m_text.contents().size(); ++i)
    {
        if (i < m_min_visible_indexes.y || i > m_max_visible_indexes.y)
            continue;

        std::string line;

        for (int j = 0; j < m_text.contents()[i].size(); ++j)
        {
            if (j >= m_min_visible_indexes.x && j < m_max_visible_indexes.x)
            {
                line += m_text.contents()[i][j];
            }
        }

        visible.emplace_back(line);
    }

    return visible;
}


void gui::TextEntry::move_cursor(int x, int y)
{
    m_real_cursor_pos.x += x * m_text.char_dim().x;
    m_real_cursor_pos.y += y * m_text.char_dim().y;

    m_display_cursor_pos.x += x * m_text.char_dim().x;
    m_display_cursor_pos.y += y * m_text.char_dim().y;

    if (m_display_cursor_pos.x < m_rect.x || m_display_cursor_pos.x > m_rect.x + m_rect.w)
        move_bounds(x, 0);

    if (m_display_cursor_pos.y < m_rect.y || m_display_cursor_pos.y >= m_rect.y + m_rect.h)
        move_bounds(0, y);
    
    //int real_end_of_line = (int)(m_rect.x + m_text.get_line(real_to_char_pos(m_real_cursor_pos).y).size() * m_text.char_dim().x);

    m_display_cursor_pos = {
        std::min(std::max(m_display_cursor_pos.x, m_rect.x), m_rect.x + m_rect.w),
        std::min(std::max(m_display_cursor_pos.y, m_rect.y), m_rect.y + m_rect.h - m_text.char_dim().y)
    };

    m_real_cursor_pos = {
        std::max(m_rect.x, m_real_cursor_pos.x),
        std::max(m_rect.y, m_real_cursor_pos.y)
    };

    /*m_visible_content = get_visible_content();

    if (m_real_cursor_pos.x > real_end_of_line && real_end_of_line != -1)
    {
        m_real_cursor_pos.x = real_end_of_line;
        m_display_cursor_pos.x = m_rect.x + m_visible_content[(m_display_cursor_pos.y - m_rect.y) / m_text.char_dim().y].size() * m_text.char_dim().x;
    }*/
}

SDL_Point gui::TextEntry::real_to_char_pos(SDL_Point pos)
{
    return {
        (pos.x - m_rect.x) / m_text.char_dim().x,
        (pos.y - m_rect.y) / m_text.char_dim().y
    };
}


void gui::TextEntry::move_bounds(int x, int y)
{
    m_min_visible_indexes.x += x;
    m_min_visible_indexes.y += y;

    m_max_visible_indexes.x += x;
    m_max_visible_indexes.y += y;
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


void gui::TextEntry::check_bounds(int x, int y)
{
    if (m_display_cursor_pos.x < m_rect.x || m_display_cursor_pos.x > m_rect.x + m_rect.w)
        move_bounds(x, 0);

    if (m_display_cursor_pos.y < m_rect.y || m_display_cursor_pos.y >= m_rect.y + m_rect.h)
        move_bounds(0, y);
}


void gui::TextEntry::move_real_cursor(int x, int y)
{
    m_real_cursor_pos.x += x * m_text.char_dim().x;
    m_real_cursor_pos.y += y * m_text.char_dim().y;

    //int real_end_of_line = (int)(m_rect.x + m_text.get_line(real_to_char_pos(m_real_cursor_pos).y).size() * m_text.char_dim().x);

    m_real_cursor_pos = {
        std::max(m_rect.x, m_real_cursor_pos.x),
        std::max(m_rect.y, m_real_cursor_pos.y)
    };

    /*m_visible_content = get_visible_content();

    if (m_real_cursor_pos.x > real_end_of_line)
    {
        m_real_cursor_pos.x = real_end_of_line;
        m_display_cursor_pos.x = m_rect.x + m_visible_content[(m_display_cursor_pos.y - m_rect.y) / m_text.char_dim().y].size() * m_text.char_dim().x;
    }*/
}


void gui::TextEntry::jump_to_eol()
{
    int real_end_of_line = (int)(m_rect.x + m_text.get_line(real_to_char_pos(m_real_cursor_pos).y).size() * m_text.char_dim().x);

    m_visible_content = get_visible_content();

    if (m_real_cursor_pos.x != real_end_of_line)
    {
        m_real_cursor_pos.x = real_end_of_line;
        m_display_cursor_pos.x = m_rect.x + m_visible_content[(m_display_cursor_pos.y - m_rect.y) / m_text.char_dim().y].size() * m_text.char_dim().x;
    }
}


void gui::TextEntry::move_display_cursor(int x, int y)
{
    m_display_cursor_pos.x += x * m_text.char_dim().x;
    m_display_cursor_pos.y += y * m_text.char_dim().y;

    if (m_display_cursor_pos.x < m_rect.x || m_display_cursor_pos.x > m_rect.x + m_rect.w)
        move_bounds(x, 0);

    if (m_display_cursor_pos.y < m_rect.y || m_display_cursor_pos.y >= m_rect.y + m_rect.h)
        move_bounds(0, y);

    m_display_cursor_pos = {
        std::min(std::max(m_display_cursor_pos.x, m_rect.x), m_rect.x + m_rect.w),
        std::min(std::max(m_display_cursor_pos.y, m_rect.y), m_rect.y + m_rect.h - m_text.char_dim().y)
    };
}


void gui::TextEntry::draw_cursor(SDL_Renderer* rend)
{
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderDrawLine(rend, m_display_cursor_pos.x, m_display_cursor_pos.y, m_display_cursor_pos.x, m_display_cursor_pos.y + m_text.char_dim().y);
}