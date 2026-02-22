#include <iostream>
#include <list>
#include <string>
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

// قمنا بتغيير الاسم هنا إلى TodoItem لتجنب التعارض
struct TodoItem {
    std::string name;
    bool is_done = false;
};

int main() {
    auto screen = ScreenInteractive::TerminalOutput();

    // استخدمنا الاسم الجديد TodoItem
    std::list<TodoItem> tasks;
    std::string new_task_name;

    auto task_list = Container::Vertical({});

    InputOption input_option;
    input_option.on_enter = [&] {
        if (!new_task_name.empty()) {
            tasks.push_back({new_task_name, false});
            task_list->Add(Checkbox(&tasks.back().name, &tasks.back().is_done));
            new_task_name.clear();
        }
    };

    auto input_component = Input(&new_task_name, "Type a new task and press Enter...", input_option);
    auto quit_button = Button("Quit", screen.ExitLoopClosure());

    auto main_container = Container::Vertical({
        task_list,
        input_component,
        quit_button,
    });

    auto renderer = Renderer(main_container, [&] {
        return vbox({
            text(" My To-Do List ") | bold | center,
            separator(),
            task_list->Render() | frame | size(HEIGHT, GREATER_THAN, 5),
            separator(),
            hbox(text(" New Task: "), input_component->Render()),
            separator(),
            quit_button->Render() | center,
        }) | border;
    });

    screen.Loop(renderer);

    return 0;
}
