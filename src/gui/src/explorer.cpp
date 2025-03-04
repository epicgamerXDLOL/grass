#include "explorer.h"
#include "button.h"
#include <iostream>
#include <chrono>

namespace chrono = std::chrono;


gui::Explorer::Explorer(const std::string& dir, ExplorerMode mode, SDL_Point pos)
    : m_current_dir(dir), m_mode(mode)
{
    m_window = SDL_CreateWindow((std::string("Select ") + (mode == ExplorerMode::DIR ? "directory" : "file")).c_str(), pos.x, pos.y, 600, 400, SDL_WINDOW_SHOWN);
    m_rend = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_RenderClear(m_rend);
    SDL_RenderPresent(m_rend);

    SDL_SetWindowGrab(m_window, SDL_TRUE);
}


gui::Explorer::~Explorer()
{
    SDL_DestroyRenderer(m_rend);
    SDL_DestroyWindow(m_window);
}


std::string gui::Explorer::get_path()
{
    bool running = true;
    SDL_Event evt;

    bool return_path = false;

    common::Font font_button("res/CascadiaCode.ttf", 14);
    
    SDL_Point window_size;
    SDL_GetWindowSize(m_window, &window_size.x, &window_size.y);

    SDL_Point button_pos = { window_size.x - 100, window_size.y - 25 };

    std::vector<Button*> buttons;
    buttons.emplace_back(new Button(m_rend, Text(font_button.font(), button_pos, "Select", font_button.char_dim(), { 255, 255, 255 }), { button_pos.x, button_pos.y, 95, 20 }, { 100, 100, 100 }, [&]() {
        running = false;
        return_path = true;
    }));

    button_pos.x -= 100;
    buttons.emplace_back(new Button(m_rend, Text(font_button.font(), button_pos, "Cancel", font_button.char_dim(), { 255, 255, 255 }), { button_pos.x, button_pos.y, 95, 20 }, { 100, 100, 100 }, [&]() {
        running = false;
    }));

    int entry_start = 50;
    int entry_width = 400;

    chrono::system_clock::time_point first_click_time = chrono::system_clock::now();
    chrono::system_clock::time_point second_click_time = chrono::system_clock::now();
    bool ready_for_first_click = true;
    std::string first_clicked_item;

    while (running)
    {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
            case SDL_MOUSEBUTTONDOWN:
            {
                bool clicked = false;

                for (auto& btn : buttons)
                {
                    if (btn->check_clicked(mx, my))
                        clicked = true;
                }

                if (!clicked)
                {
                    m_selected_item = elem_at_mouse_pos(my, font_button.char_dim().y);

                    // clicked a valid item
                    if (!m_selected_item.empty())
                    {
                        m_selected_item_highlight = {
                            entry_start,
                            (int)(my / font_button.char_dim().y) * font_button.char_dim().y,
                            entry_width,
                            font_button.char_dim().y
                        };

                        if (ready_for_first_click)
                        {
                            first_click_time = chrono::system_clock::now();
                            ready_for_first_click = false;
                            first_clicked_item = m_selected_item;
                        }
                        else if (chrono::duration_cast<chrono::milliseconds>(second_click_time - first_click_time).count() < 500)
                        {
                            if (m_selected_item != first_clicked_item)
                            {
                                ready_for_first_click = false;
                            }
                            else
                            {
                                m_current_dir += (m_selected_item.empty() ? "" : "/" + m_selected_item);
                                m_selected_item_highlight = { 0, 0, 0, 0 };
                                m_selected_item.clear();
                                ready_for_first_click = true;
                                first_clicked_item.clear();
                            }
                        }
                    }
                }

                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                for (auto& btn : buttons)
                {
                    btn->set_down(false);
                }
            }
            }
        }

        SDL_RenderClear(m_rend);

        second_click_time = chrono::system_clock::now();

        if (!ready_for_first_click && chrono::duration_cast<chrono::milliseconds>(second_click_time - first_click_time).count() > 500)
            ready_for_first_click = true;

        for (auto& btn : buttons)
        {
            if (btn)
            {
                btn->check_hover(mx, my);
                btn->render(m_rend);
            }
        }

        update_current_directory();
        render_current_directory(font_button, entry_start);
        highlight_elem_at_mouse(my, font_button.char_dim().y, entry_start, entry_width);

        SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_rend, 255, 255, 255, 100);
        SDL_RenderFillRect(m_rend, &m_selected_item_highlight);
        SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(m_rend, 50, 50, 50, 255);
        SDL_RenderPresent(m_rend);
        
        if (!running)
        {
            cleanup(buttons, font_button.font_ptr());

            if (return_path)
                return m_current_dir + (m_selected_item.empty() ? "" : '/' + m_selected_item);
            else
                return "";
        }
    }

    return "";
}


void gui::Explorer::cleanup(std::vector<Button*>& buttons, TTF_Font** font)
{
    for (auto& tex : m_current_textures)
    {
        if (tex)
            SDL_DestroyTexture(tex);
    }

    for (auto& btn : buttons)
    {
        delete btn;
        btn = 0;
    }

    buttons.clear();

    TTF_CloseFont(*font);
    *font = 0;

    SDL_SetWindowGrab(m_window, SDL_FALSE);
}


void gui::Explorer::update_current_directory()
{
    m_current_names.clear();
    m_current_textures.clear();
    m_current_names.emplace_back("(Move up a directory)");

    for (auto& entry : fs::directory_iterator(m_current_dir, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_directory())
        {
            if (m_mode == ExplorerMode::DIR)
            {
                m_current_names.emplace_back(entry.path().filename().string());
            }
        }
        else
        {
            if (m_mode == ExplorerMode::FILE)
            {
                m_current_names.emplace_back(entry.path().filename().string());
            }
        }
    }
}


void gui::Explorer::render_current_directory(common::Font& font, int entry_start)
{
    SDL_Rect current_text_rect = { entry_start, 0 };

    for (int i = 0; i < m_current_names.size(); ++i)
    {
        if (i >= m_current_textures.size())
        {
            m_current_textures.emplace_back(nullptr);
        }

        if (!m_current_textures[i])
        {
            m_current_textures[i] = common::render_text(m_rend, font.font(), m_current_names[i].c_str());
        }

        if (m_current_textures[i])
        {
            SDL_QueryTexture(m_current_textures[i], 0, 0, &current_text_rect.w, &current_text_rect.h);
            SDL_RenderCopy(m_rend, m_current_textures[i], 0, &current_text_rect);
            current_text_rect.y += font.char_dim().y;
        }
    }
}


std::string gui::Explorer::elem_at_mouse_pos(int my, int font_dim_y)
{
    int index = my / font_dim_y;

    if (index < m_current_names.size())
    {
        if (index == 0)
        {
            m_current_dir = fs::absolute(fs::path(m_current_dir)).parent_path().string();
            m_selected_item_highlight = { 0, 0, 0, 0 };
            return "";
        }

        return m_current_names[index];
    }
    else
        return "";
}


void gui::Explorer::highlight_elem_at_mouse(int my, int font_dim_y, int entry_start, int entry_width)
{
    if ((int)(my / font_dim_y) >= m_current_names.size())
        return;

    int y = (my / font_dim_y) * font_dim_y;
    SDL_Rect rect = { entry_start, y, entry_width, font_dim_y };

    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_rend, 255, 255, 255, 50);
    SDL_RenderFillRect(m_rend, &rect);
    SDL_SetRenderDrawBlendMode(m_rend, SDL_BLENDMODE_NONE);
}