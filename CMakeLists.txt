cmake_minimum_required(VERSION 3.11)
project(TodoCLI)

# جلب مكتبة FTXUI تلقائياً من GitHub
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG main
)
FetchContent_MakeAvailable(ftxui)

# إنشاء البرنامج التنفيذي
add_executable(todo_app main.cpp)

# ربط مشروعنا بمكتبات FTXUI الأساسية
target_link_libraries(todo_app PRIVATE ftxui::component ftxui::dom ftxui::screen)
