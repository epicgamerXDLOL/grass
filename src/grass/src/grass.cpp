#include "grass.h"
#include "common.h"
#include "button.h"
#include "file_tree.h"
#include "text_entry.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <SDL_image.h>

#define BG_COLOR 30, 30, 30

namespace fs = std::filesystem;


Grass::Grass()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    m_window = SDL_CreateWindow("Grass", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 800, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    m_rend = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_RenderClear(m_rend);
    SDL_RenderPresent(m_rend);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    TTF_Init();
    IMG_Init(IMG_INIT_PNG);
}


Grass::~Grass()
{
    SDL_DestroyRenderer(m_rend);
    SDL_DestroyWindow(m_window);

    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}


void Grass::mainloop()
{
    constexpr SDL_Rect main_text_dimensions = {
        300,
        40,
        1000 - 300,
        800 - 40
    };

    /* Core ui elements that should not be touched */

    TTF_Font* font_textbox = TTF_OpenFont("res/CascadiaCode.ttf", 16);
    TTF_Font* font_tree = TTF_OpenFont("res/CascadiaCode.ttf", 14);

    SDL_Point font_textbox_dim;
    TTF_SizeText(font_textbox, " ", &font_textbox_dim.x, &font_textbox_dim.y);

    SDL_Point font_tree_dim;
    TTF_SizeText(font_tree, " ", &font_tree_dim.x, &font_tree_dim.y);

    std::vector<gui::TextEntry> text_entries;
    text_entries.emplace_back(gui::TextEntry(main_text_dimensions, { 50, 50, 50 }, gui::Cursor({ main_text_dimensions.x, main_text_dimensions.y }, { 255, 255, 255 }, font_textbox_dim), gui::Text(font_textbox, { main_text_dimensions.x, main_text_dimensions.y }, "", font_textbox_dim, { 255, 255, 255 })));

    std::vector<gui::Button> buttons;

    gui::Folder folder(".", gui::Text(font_tree, { 0, 60 }, "", font_tree_dim, { 255, 255, 255 }), m_rend, true);
    gui::Tree tree(
        { 0, main_text_dimensions.y, main_text_dimensions.x, 800 - main_text_dimensions.y },
        folder,
        // when changing font size make sure to also change the 20 below to the y value of the char dimensions specified above
        { 0, main_text_dimensions.y, 200, font_tree_dim.y },
        m_rend
    );

    for (auto& f : tree.folder().folders())
    {
        f.collapse(m_rend);
    }

    tree.update_display();

    bool running = true;
    SDL_Event evt;

    /* Extra misc variables that should be added to */

    std::string current_open_fp;
    SDL_Texture* editor_image = nullptr;

    int prev_wx, prev_wy;
    SDL_GetWindowSize(m_window, &prev_wx, &prev_wy);


    /* Kb event variables and other very simple variables here */

    bool mouse_down = false;
    bool ctrl_down = false;

    while (running)
    {
        int mx, my;

        {
            // sdl_capturemouse is not good so here is a workaround
            int global_mx, global_my;
            SDL_GetGlobalMouseState(&global_mx, &global_my);

            int wx, wy;
            SDL_GetWindowPosition(m_window, &wx, &wy);

            mx = global_mx - wx;
            my = global_my - wy;
        }

        int wx, wy;
        SDL_GetWindowSize(m_window, &wx, &wy);

        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
            {
                mouse_down = true;

                for (auto& btn : buttons)
                {
                    btn.check_clicked(mx, my);
                }

                bool has_selected_item{ false };

                for (auto& e : text_entries)
                {
                    if (e.check_clicked(mx, my))
                    {
                        m_selected_entry = &e;
                        has_selected_item = true;
                        m_selected_entry->mouse_down(mx, my);
                    }
                }

                if (!has_selected_item)
                    m_selected_entry = nullptr;

                gui::File* file = tree.check_file_click(tree.folder(), mx, my);

                if (file)
                {
                    if (tree.is_unsaved(current_open_fp))
                    {
                        std::ofstream ofs(current_open_fp + "~", std::ofstream::out | std::ofstream::trunc);
                        ofs << text_entries[0].text()->str();
                        ofs.close();
                    }

                    text_entries[0].stop_highlight();

                    current_open_fp = file->path();

                    if (current_open_fp.substr(current_open_fp.size() - 4, current_open_fp.size()) == ".png")
                    {
                        editor_image = IMG_LoadTexture(m_rend, current_open_fp.c_str());

                        if (!editor_image)
                            editor_image = gui::common::render_text(m_rend, font_textbox, SDL_GetError());

                        text_entries[0].text()->set_contents({ "" });
                        reset_entry_to_default(text_entries[0]);
                        text_entries[0].hide();
                    }
                    else
                    {
                        if (editor_image)
                            SDL_DestroyTexture(editor_image);

                        editor_image = nullptr;

                        text_entries[0].show();
                        
                        if (fs::exists(current_open_fp + "~"))
                            load_file(current_open_fp + "~", text_entries[0]);
                        else
                            load_file(current_open_fp, text_entries[0]);
                    }

                    tree.update_display();

                    SDL_SetWindowTitle(m_window,
                        (std::string("Grass | Editing ") + file->name().str().c_str() + (tree.is_unsaved(file->path()) ? " - UNSAVED" : "")).c_str()
                    );
                }

                gui::Folder* folder = tree.check_folder_click(tree.folder(), mx, my);

                if (folder)
                {
                    tree.collapse_folder(*folder, m_rend);
                    tree.update_display();
                }
            } break;

            case SDL_MOUSEBUTTONUP:
                mouse_down = false;

                if (m_selected_entry)
                {
                    m_selected_entry->mouse_up();
                }

                for (auto& btn : buttons)
                {
                    btn.set_down(false);
                }
                break;

            case SDL_TEXTINPUT:
                if (m_selected_entry)
                {
                    m_selected_entry->insert_char(evt.text.text[0]);
                    m_selected_entry->stop_highlight();
                    tree.append_unsaved_file(current_open_fp, m_window);
                }

                break;
            case SDL_KEYDOWN:
            {
                if (m_selected_entry)
                {
                    switch (evt.key.keysym.scancode)
                    {
                    case SDL_SCANCODE_RETURN:
                        m_selected_entry->insert_char('\n');
                        tree.append_unsaved_file(current_open_fp, m_window);
                        m_selected_entry->stop_highlight();
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                        if (!mouse_down)
                        {
                            m_selected_entry->remove_char();
                            tree.append_unsaved_file(current_open_fp, m_window);
                        }
                        break;
                    case SDL_SCANCODE_RCTRL:
                    case SDL_SCANCODE_LCTRL:
                        ctrl_down = true;
                        break;
                    }
                }

                if (m_selected_entry)
                {
                    SDL_Point movement{ 0, 0 };

                    if (evt.key.keysym.sym == SDLK_RIGHT)
                        movement.x = 1;

                    if (evt.key.keysym.sym == SDLK_LEFT)
                        movement.x = -1;

                    if (evt.key.keysym.sym == SDLK_UP)
                        movement.y = -1;

                    if (evt.key.keysym.sym == SDLK_DOWN)
                        movement.y = 1;

                    m_selected_entry->move_cursor_characters(movement.x, movement.y);

                    m_selected_entry->conditional_move_bounds_characters(
                        movement.x * m_selected_entry->move_bounds_by(), 
                        movement.y * m_selected_entry->move_bounds_by()
                    );

                    m_selected_entry->conditional_jump_to_eol();

                    switch (evt.key.keysym.sym)
                    {
                    case SDLK_s:
                        if (ctrl_down && m_selected_entry == &text_entries[0])
                        {
                            std::ofstream ofs(current_open_fp, std::ofstream::out | std::ofstream::trunc);

                            for (auto& line : text_entries[0].text()->contents())
                            {
                                ofs << line << "\n";
                            }

                            ofs.close();

                            tree.erase_unsaved_file(current_open_fp, m_window);
                        }
                        break;
                    case SDLK_d:
                        if (ctrl_down && m_selected_entry == &text_entries[0])
                        {
                            tree.erase_unsaved_file(current_open_fp, m_window);
                            load_file(current_open_fp, text_entries[0]);
                        }
                    }
                }
            } break;
            case SDL_KEYUP:
            {
                switch (evt.key.keysym.scancode)
                {
                case SDL_SCANCODE_RCTRL:
                case SDL_SCANCODE_LCTRL:
                    ctrl_down = false;
                    break;
                }
            } break;
            case SDL_MOUSEWHEEL:
                if (gui::common::within_rect(tree.rect(), mx, my))
                {
                    tree.scroll(-evt.wheel.y, wy);
                }
                else if (gui::common::within_rect(text_entries[0].rect(), mx, my))
                {
                    text_entries[0].scroll(-evt.wheel.y);
                }
                break;
            }
        }

        SDL_RenderClear(m_rend);

        if (mouse_down)
        {
            if (m_selected_entry)
            {
                m_selected_entry->move_cursor_to_click(mx, my);
            }
        }

        for (auto& btn : buttons)
        {
            btn.check_hover(mx, my);
            btn.render(m_rend);
        }

        tree.render(m_rend);

        for (auto& e : text_entries)
        {
            if (e.hidden())
                continue;

            bool render_mouse = false;
            if (m_selected_entry == &e)
                render_mouse = true;

            e.render(m_rend, render_mouse);
        }

        if (prev_wx != wx || prev_wy != wy)
        {
            text_entries[0].resize_to(wx, wy);
            prev_wx = wx;
            prev_wy = wy;
        }

        SDL_SetRenderDrawColor(m_rend, BG_COLOR, 255);

        if (editor_image)
        {
            int img_w, img_h;
            SDL_QueryTexture(editor_image, nullptr, nullptr, &img_w, &img_h);

            int img_x = (text_entries[0].rect().w / 2) - img_w/2 + text_entries[0].rect().x;
            int img_y = (text_entries[0].rect().h / 2) - img_h/2 + text_entries[0].rect().y;

            SDL_Rect dstrect = {
                img_x,
                img_y,
                img_w,
                img_h
            };

            SDL_RenderCopy(m_rend, editor_image, nullptr, &dstrect);
        }

        SDL_RenderPresent(m_rend);
    }

    TTF_CloseFont(font_textbox);
    
    for (auto& path : tree.unsaved())
    {
        if (fs::exists(path + "~"))
            fs::remove(path + "~");
    }
}


void Grass::load_file(const std::string& fp, gui::TextEntry& entry)
{
    std::ifstream ifs(fp);

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) lines.emplace_back(line);

    ifs.close();

    entry.text()->set_contents(lines);
    reset_entry_to_default(entry);
}


void Grass::reset_entry_to_default(gui::TextEntry& entry)
{
    entry.reset_bounds_x();
    entry.reset_bounds_y();
    entry.set_cursor_pos_characters(0, 0);
    entry.update_cache();
}