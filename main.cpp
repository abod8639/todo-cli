#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

// --- [ منطق البيانات - JSON ] ---
namespace json {
// (حافظ على دوال الـ JSON كما هي في كودك لأنها تعمل بشكل ممتاز بدون مكتبات
// خارجية) سأختصر المساحة هنا للتركيز على تحسينات الـ UI
struct DayRecord {
  int done = 0;
  int total = 0;
  std::vector<std::string> tasks;
  std::vector<bool> task_done;
};
using DayMap = std::map<std::string, DayRecord>;

// ... (دوال load و save و escape كما هي لديك)
std::string escape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"')
      out += "\\\"";
    else if (c == '\\')
      out += "\\\\";
    else if (c == '\n')
      out += "\\n";
    else
      out += c;
  }
  return out;
}
void save(const std::string &filename, const DayMap &days) {
  std::ofstream f(filename);
  f << "{\n  \"days\": {\n";
  bool first_day = true;
  for (auto &[date, rec] : days) {
    if (!first_day)
      f << ",\n";
    first_day = false;
    f << "    \"" << date << "\": {\n      \"done\": " << rec.done
      << ",\n      \"total\": " << rec.total << ",\n      \"tasks\": [";
    for (size_t i = 0; i < rec.tasks.size(); i++) {
      f << "\"" << escape(rec.tasks[i]) << "\"";
      if (i + 1 < rec.tasks.size())
        f << ", ";
    }
    f << "],\n      \"task_done\": [";
    for (size_t i = 0; i < rec.task_done.size(); i++) {
      f << (rec.task_done[i] ? "true" : "false");
      if (i + 1 < rec.task_done.size())
        f << ", ";
    }
    f << "]\n    }";
  }
  f << "\n  }\n}\n";
}
// (ملاحظة: تأكد من بقاء دالة load كاملة كما في كودك الأصلي)
} // namespace json

// --- [ تحسينات الواجهة Heatmap ] ---
Element renderEnhancedHeatmap(const json::DayMap &days_data,
                              const std::string &today) {
  const int WEEKS = 22; // عرض أكبر قليلاً
  auto cellColor = [](int lv) -> Color {
    if (lv == 0)
      return Color::RGB(30, 30, 35);
    if (lv == 1)
      return Color::RGB(0, 68, 0);
    if (lv == 2)
      return Color::RGB(0, 150, 0);
    if (lv == 3)
      return Color::RGB(0, 210, 0);
    return Color::RGB(50, 255, 50);
  };

  auto level = [&](const std::string &d) -> int {
    if (days_data.find(d) == days_data.end() || days_data.at(d).total == 0)
      return 0;
    float r = (float)days_data.at(d).done / days_data.at(d).total;
    if (r <= 0)
      return 0;
    if (r < 0.3)
      return 1;
    if (r < 0.6)
      return 2;
    if (r < 0.9)
      return 3;
    return 4;
  };

  auto now = std::chrono::system_clock::now();
  auto get_date = [&](int days_back) {
    auto d = now - std::chrono::hours(24 * days_back);
    auto tt = std::chrono::system_clock::to_time_t(d);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&tt));
    return std::string(buf);
  };

  int today_wd = []() {
    auto t =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return std::localtime(&t)->tm_wday;
  }();

  Elements grid_cols;
  for (int c = 0; c < WEEKS; c++) {
    Elements col_cells;
    for (int r = 0; r < 7; r++) {
      int days_ago = (WEEKS - 1 - c) * 7 + (today_wd - r);
      std::string d = get_date(days_ago);
      if (days_ago < 0)
        col_cells.push_back(text("  "));
      else
        col_cells.push_back(text(d == today ? "▣ " : "■ ") |
                            color(cellColor(level(d))));
    }
    grid_cols.push_back(vbox(std::move(col_cells)));
  }

  return hbox({vbox({text("S "), text("M "), text("T "), text("W "), text("T "),
                     text("F "), text("S ")}) |
                   dim,
               hbox(std::move(grid_cols))}) |
         borderStyled(ROUNDED) | color(Color::GrayLight);
}

// --- [ Helper function to get current date ] ---
std::string getTodayDate() {
  auto now = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(now);
  char buf[11];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&tt));
  return std::string(buf);
}

// --- [ المكون الرئيسي ] ---
struct TodoItem {
  std::string name;
  bool is_done = false;
  std::string created_at; // Timestamp when task was created
  
  TodoItem() : created_at("") {}
  TodoItem(const std::string& n) : name(n), created_at(getTodayDate()), is_done(false) {}
};

int main() {
  const std::string DATA_FILE = "todo_data.json";
  json::DayMap days_data;           // يُفضل إضافة دالة load هنا
  std::string today = "2026-02-26"; // مثال للوقت الحالي
  std::vector<TodoItem> tasks;

  auto screen = ScreenInteractive::TerminalOutput();
  std::string new_task_name;
  int selected = 0;
  bool show_heatmap = true;

  auto persist = [&] {
    json::DayRecord rec;
    for (auto &t : tasks) {
      rec.tasks.push_back(t.name);
      rec.task_done.push_back(t.is_done);
      if (t.is_done)
        rec.done++;
    }
    rec.total = tasks.size();
    days_data[today] = rec;
    json::save(DATA_FILE, days_data);
  };

  auto input_box = Input(&new_task_name, " ✎ Add task...");

  // تحسين معالجة الأحداث (CatchEvent)
  auto task_list_comp = Renderer([&] {
    Elements rows;
    for (int i = 0; i < (int)tasks.size(); ++i) {
      // 1. Task counter
      auto counter = text("  " + std::to_string(i + 1) + ". ") | color(Color::GrayLight);
      
      // 2. Checkbox icon
      auto icon_text = text(tasks[i].is_done ? "✔" : "○");
      if (tasks[i].is_done) {
        icon_text = icon_text | color(Color::Green) | dim;
      } else {
        icon_text = icon_text | color(Color::Yellow);
      }

      // 3. Task name
      auto task_element = text(tasks[i].name) | flex;
      if (tasks[i].is_done) {
        task_element = task_element | strikethrough | dim;
      }

      // 4. Apply selection styling
      if (i == selected) {
        counter = counter | inverted;
        icon_text = icon_text | inverted | bold;
        task_element = task_element | inverted | bold;
      }

      // 5. Build task row with metadata
      auto task_row = hbox({counter, text(" "), icon_text, text(" "), task_element});
      
      rows.push_back(task_row);
    }

    if (rows.empty())
      return vbox({
        text("") | size(HEIGHT, EQUAL, 1),
        text("     ✨ No tasks yet! ") | center | bold | color(Color::Cyan),
        text("     Add one to get started... ") | center | dim,
        text("") | size(HEIGHT, EQUAL, 1)
      });

    return vbox(std::move(rows));
  });

  auto main_container = Container::Vertical({input_box, task_list_comp});

  auto final_renderer = Renderer(main_container, [&] {
    int completed = std::count_if(tasks.begin(), tasks.end(), [](auto &t) {
      return t.is_done;
    });
    float progress = tasks.empty() ? 0 : (float)completed / tasks.size();

    // Status bar with stats
    auto stats_bar = hbox({
      text(" 📊 Tasks: ") | bold | color(Color::White),
      text(std::to_string(completed) + "/" + std::to_string(tasks.size())) | 
        color(Color::Cyan) | bold,
      filler(),
      text(" [H] Heatmap  [Q] Quit ") | dim | color(Color::GrayDark)
    }) | color(Color::Blue);

    // Progress section
    auto progress_section = hbox({
      text(" ⏳ ") | color(Color::Cyan),
      text("Progress: ") | bold,
      gauge(progress) | color(
        progress < 0.3 ? Color::Red : 
        progress < 0.7 ? Color::Yellow : 
        Color::Green
      ) | flex,
      text(" " + std::to_string((int)(progress * 100)) + "% ")
    }) | borderRounded | color(Color::White);

    // Input section with better styling
    auto input_section = hbox({
      text(" ➕ ") | color(Color::Yellow) | bold,
      input_box->Render() | flex
    }) | border | color(Color::Yellow);

    // Task list with better framing
    auto task_list_section = task_list_comp->Render() | frame | 
      size(HEIGHT, EQUAL, 9) | borderDouble | color(Color::Blue);

    // Keyboard help with better formatting
    auto help_section = vbox({
      hbox({
        text(" ⌨ Keyboard Shortcuts") | bold | color(Color::White)
      }) | size(WIDTH, EQUAL, 80) | color(Color::GrayLight),
      hbox({
        text("   [↑↓] Navigate  [Space] Toggle  [D] Delete  [H] Heatmap  [Q] Quit") | dim
      })
    });

    return vbox({
      text("") | size(HEIGHT, EQUAL, 1),
      hbox({
        text("\n 🚀 POWER TODO CLI ") | bold | color(Color::Cyan),
        filler()
      }) | size(WIDTH, GREATER_THAN, 80),
      text("") | size(HEIGHT, EQUAL, 1),
      stats_bar | size(WIDTH, GREATER_THAN, 80),
      text("") | size(HEIGHT, EQUAL, 1),
      show_heatmap 
        ? renderEnhancedHeatmap(days_data, today) | center
        : (text(" 📈 Heatmap Hidden (press H to show)") | center | dim),
      text("") | size(HEIGHT, EQUAL, 1),
      progress_section | size(WIDTH, GREATER_THAN, 80),
      text("") | size(HEIGHT, EQUAL, 1),
      input_section | size(WIDTH, GREATER_THAN, 80),
      text("") | size(HEIGHT, EQUAL, 1),
      task_list_section | size(WIDTH, GREATER_THAN, 80),
      text("") | size(HEIGHT, EQUAL, 1),
      help_section | size(WIDTH, GREATER_THAN, 80),
      text("") | size(HEIGHT, EQUAL, 1)
    }) | size(WIDTH, GREATER_THAN, 80) | center;
  });

  // معالجة مفاتيح الاختصار العالمية
  auto global_events = CatchEvent(final_renderer, [&](Event e) {
    if (e == Event::Character('q'))
      screen.ExitLoopClosure()();
    if (e == Event::Character('h')) {
      show_heatmap = !show_heatmap;
      return true;
    }
    if (e == Event::ArrowDown) {
      selected = std::min((int)tasks.size() - 1, selected + 1);
      return true;
    }
    if (e == Event::ArrowUp) {
      selected = std::max(0, selected - 1);
      return true;
    }
    if (e == Event::Character(' ')) {
      if (!tasks.empty()) {
        tasks[selected].is_done = !tasks[selected].is_done;
        persist();
      }
      return true;
    }
    if (e == Event::Character('d')) {
      if (!tasks.empty()) {
        tasks.erase(tasks.begin() + selected);
        selected = std::max(0, selected - 1);
        persist();
      }
      return true;
    }
    if (e == Event::Return && !new_task_name.empty()) {
      tasks.push_back(TodoItem(new_task_name));
      new_task_name = "";
      selected = tasks.size() - 1; // Move selection to new task
      persist();
      return true;
    }
    return false;
  });

  screen.Loop(global_events);
  return 0;
}