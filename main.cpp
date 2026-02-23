// Fixed: AppFocus rename + removed BackTab

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
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

namespace json {

std::string escape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"')       out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else                out += c;
  }
  return out;
}

struct DayRecord {
  int done  = 0;
  int total = 0;
  std::vector<std::string> tasks;
  std::vector<bool>        task_done;
  std::vector<std::string> task_tags;  // التاجات لكل مهمة
};

using DayMap = std::map<std::string, DayRecord>;

static void skipWS(const std::string &s, size_t &pos) {
  while (pos < s.size() && std::isspace((unsigned char)s[pos])) pos++;
}

static std::string parseStr(const std::string &src, size_t &pos) {
  std::string val;
  while (pos < src.size()) {
    char c = src[pos++];
    if (c == '\\' && pos < src.size()) {
      char e = src[pos++];
      if (e == '"')       val += '"';
      else if (e == '\\') val += '\\';
      else if (e == 'n')  val += '\n';
      else val += e;
    } else if (c == '"') break;
    else val += c;
  }
  return val;
}

DayMap load(const std::string &filename) {
  DayMap result;
  std::ifstream f(filename);
  if (!f.is_open()) return result;
  std::string src((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());

  size_t pos = src.find("\"days\"");
  if (pos == std::string::npos) return result;
  pos = src.find('{', pos + 6);
  if (pos == std::string::npos) return result;
  pos++;

  while (pos < src.size()) {
    skipWS(src, pos);
    if (pos >= src.size() || src[pos] == '}') break;
    if (src[pos] != '"') { pos++; continue; }
    pos++;
    std::string date = parseStr(src, pos);
    if (date.size() != 10) continue;

    pos = src.find('{', pos);
    if (pos == std::string::npos) break;
    pos++;

    DayRecord rec;
    while (pos < src.size()) {
      skipWS(src, pos);
      if (pos >= src.size() || src[pos] == '}') { pos++; break; }
      if (src[pos] != '"') { pos++; continue; }
      pos++;
      std::string key = parseStr(src, pos);
      pos = src.find(':', pos);
      if (pos == std::string::npos) break;
      pos++;
      skipWS(src, pos);

      if (key == "done" || key == "total") {
        std::string num;
        while (pos < src.size() && std::isdigit((unsigned char)src[pos]))
          num += src[pos++];
        int v = num.empty() ? 0 : std::stoi(num);
        if (key == "done") rec.done = v; else rec.total = v;
      } else if (key == "tasks") {
        if (src[pos] != '[') continue;
        pos++;
        while (pos < src.size()) {
          skipWS(src, pos);
          if (src[pos] == ']') { pos++; break; }
          if (src[pos] == '"') { pos++; rec.tasks.push_back(parseStr(src, pos)); }
          else pos++;
          skipWS(src, pos);
          if (pos < src.size() && src[pos] == ',') pos++;
        }
      } else if (key == "task_done") {
        if (src[pos] != '[') continue;
        pos++;
        while (pos < src.size()) {
          skipWS(src, pos);
          if (src[pos] == ']') { pos++; break; }
          if (src.substr(pos, 4) == "true")  { rec.task_done.push_back(true);  pos += 4; }
          else if (src.substr(pos, 5) == "false") { rec.task_done.push_back(false); pos += 5; }
          else pos++;
          skipWS(src, pos);
          if (pos < src.size() && src[pos] == ',') pos++;
        }
      } else if (key == "task_tags") {
        if (src[pos] != '[') continue;
        pos++;
        while (pos < src.size()) {
          skipWS(src, pos);
          if (src[pos] == ']') { pos++; break; }
          if (src[pos] == '"') { pos++; rec.task_tags.push_back(parseStr(src, pos)); }
          else rec.task_tags.push_back("");
          pos++;
          skipWS(src, pos);
          if (pos < src.size() && src[pos] == ',') pos++;
        }
      } else {
        while (pos < src.size() && src[pos] != ',' && src[pos] != '}') pos++;
      }
      skipWS(src, pos);
      if (pos < src.size() && src[pos] == ',') pos++;
    }
    result[date] = rec;
    skipWS(src, pos);
    if (pos < src.size() && src[pos] == ',') pos++;
  }
  return result;
}

void save(const std::string &filename, const DayMap &days) {
  std::ofstream f(filename);
  f << "{\n  \"days\": {\n";
  bool first = true;
  for (auto &[date, rec] : days) {
    if (!first) f << ",\n";
    first = false;
    f << "    \"" << date << "\": {\n";
    f << "      \"done\": " << rec.done << ",\n";
    f << "      \"total\": " << rec.total << ",\n";
    f << "      \"tasks\": [";
    for (size_t i = 0; i < rec.tasks.size(); i++) {
      f << "\"" << escape(rec.tasks[i]) << "\"";
      if (i + 1 < rec.tasks.size()) f << ", ";
    }
    f << "],\n      \"task_done\": [";
    for (size_t i = 0; i < rec.task_done.size(); i++) {
      f << (rec.task_done[i] ? "true" : "false");
      if (i + 1 < rec.task_done.size()) f << ", ";
    }
    f << "],\n      \"task_tags\": [";
    for (size_t i = 0; i < rec.task_tags.size(); i++) {
      f << "\"" << escape(rec.task_tags[i]) << "\"";
      if (i + 1 < rec.task_tags.size()) f << ", ";
    }
    f << "]\n    }";
  }
  f << "\n  }\n}\n";
}

} // namespace json

static std::string todayStr() {
  auto now = std::chrono::system_clock::now();
  auto tt  = std::chrono::system_clock::to_time_t(now);
  char buf[11];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&tt));
  return buf;
}

static int weekdayOf(const std::string &date) {
  struct tm tm = {};
  std::istringstream ss(date);
  ss >> std::get_time(&tm, "%Y-%m-%d");
  std::mktime(&tm);
  return tm.tm_wday;
}

static std::string monthShort(const std::string &date) {
  struct tm tm = {};
  std::istringstream ss(date);
  ss >> std::get_time(&tm, "%Y-%m-%d");
  const char *m[] = {"Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};
  return m[tm.tm_mon];
}

Element renderHeatmap(const json::DayMap &data, const std::string &today) {
  const int WEEKS = 20;

  auto cellColor = [](int lv) -> Color {
    switch (lv) {
    case 0:
      return Color::RGB(35, 35, 45);
    case 1:
      return Color::RGB(0, 75, 30);
    case 2:
      return Color::RGB(0, 155, 60);
    case 3:
      return Color::RGB(0, 215, 80);
    case 4:
      return Color::RGB(80, 255, 120);
    default:
      return Color::RGB(80, 255, 120);
    }
  };

  auto level = [&](const std::string &d) -> int {
    auto it = data.find(d);
    if (it == data.end() || it->second.total == 0) return 0;
    float r = (float)it->second.done / it->second.total;
    if (r <= 0.0f)  return 0;
    if (r < 0.25f) return 1;
    if (r < 0.50f) return 2;
    if (r < 0.75f) return 3;
    return 4;
  };

  auto now = std::chrono::system_clock::now();
  int today_wd = weekdayOf(today);

  std::vector<std::vector<std::string>> grid(WEEKS, std::vector<std::string>(7, ""));
  for (int slot = 0; slot < WEEKS * 7; slot++) {
    auto d = now - std::chrono::hours(24 * slot);
    auto tt = std::chrono::system_clock::to_time_t(d);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&tt));
    int col = WEEKS - 1 - (slot + (6 - today_wd)) / 7;
    int row = ((today_wd - slot) % 7 + 7) % 7;
    if (col >= 0 && col < WEEKS && row >= 0 && row < 7)
      grid[col][row] = buf;
  }

  Elements month_row;
  month_row.push_back(text("    ")); 
  std::string last_m;
  for (int c = 0; c < WEEKS; c++) {
    std::string rep;
    for (int r = 0; r < 7 && rep.empty(); r++) rep = grid[c][r];
    std::string m = rep.size() >= 7 ? monthShort(rep) : "   ";
    if (m != last_m && !rep.empty()) {
      month_row.push_back(text(m) | color(Color::GrayLight));
      last_m = m;
    } else {
      month_row.push_back(text("  ")); 
    }
  }

  const std::string day_show[] = {"   ", "Mon", "   ", "Wed", "   ", "Fri", "   "};
  Elements all_rows;
  all_rows.push_back(hbox(month_row));

  for (int r = 0; r < 7; r++) {
    Elements row_els;
    row_els.push_back(text(day_show[r] + " ") | color(Color::GrayLight));
    for (int c = 0; c < WEEKS; c++) {
      const std::string &d = grid[c][r];
      if (d.empty()) {
        row_els.push_back(text("  ")); 
        continue;
      }

      Element cell;
      if (d > today) {
        cell = text("∙ ") | color(Color::RGB(40, 44, 52));
      } else if (d == today) {
        cell = text("■ ") | color(Color::RGB(255, 215, 0)) | bold; 
      } else {
        cell = text("■ ") | color(cellColor(level(d)));
      }
      row_els.push_back(cell);
    }
    all_rows.push_back(hbox(row_els));
  }

  all_rows.push_back(separatorEmpty());
  all_rows.push_back(hbox({
    text("  Less ") | dim,
    text("■ ") | color(cellColor(0)),
    text("■ ") | color(cellColor(1)),
    text("■ ") | color(cellColor(2)),
    text("■ ") | color(cellColor(3)),
    text("■ ") | color(cellColor(4)),
    text(" More ") | dim,
    filler(),
    text("■ Today ") | color(Color::RGB(255, 215, 0)) | bold,
  }) | color(Color::GrayLight));

  return vbox(all_rows) | borderStyled(ROUNDED, Color::RGB(56, 62, 71)) | size(HEIGHT, GREATER_THAN, 12);
}

enum class AppFocus { INPUT, TASKS, HABITS };

struct TodoItem {
  std::string name;
  bool is_done = false;
  std::string tag = "";
};

int main() {
  const std::string TASKS_FILE = "todo_tasks.json";
  const std::string HABITS_FILE = "todo_habits.json";
  json::DayMap      tasks_data = json::load(TASKS_FILE);
  json::DayMap      habits_data = json::load(HABITS_FILE);
  std::string       today     = todayStr();

  std::vector<TodoItem> tasks;
  if (tasks_data.count(today)) {
    auto &rec = tasks_data[today];
    for (size_t i = 0; i < rec.tasks.size(); i++) {
      std::string tag = (i < rec.task_tags.size()) ? rec.task_tags[i] : "";
      tasks.push_back({rec.tasks[i], i < rec.task_done.size() && rec.task_done[i], tag});
    }
  }

  std::vector<TodoItem> habits;
  if (habits_data.count(today)) {
    auto &rec = habits_data[today];
    for (size_t i = 0; i < rec.tasks.size(); i++) {
      std::string tag = (i < rec.task_tags.size()) ? rec.task_tags[i] : "";
      habits.push_back({rec.tasks[i], i < rec.task_done.size() && rec.task_done[i], tag});
    }
  }

  auto screen = ScreenInteractive::TerminalOutput();

  std::string new_task;
  int         selected     = 0;
  AppFocus    focus        = AppFocus::INPUT;

  auto persist = [&] {
    // Save tasks
    json::DayRecord task_rec;
    task_rec.total = (int)tasks.size();
    for (auto &t : tasks) {
      task_rec.tasks.push_back(t.name);
      task_rec.task_done.push_back(t.is_done);
      task_rec.task_tags.push_back(t.tag);
      if (t.is_done) task_rec.done++;
    }
    tasks_data[today] = task_rec;
    json::save(TASKS_FILE, tasks_data);

    // Save habits
    json::DayRecord habits_rec;
    habits_rec.total = (int)habits.size();
    for (auto &h : habits) {
      habits_rec.tasks.push_back(h.name);
      habits_rec.task_done.push_back(h.is_done);
      habits_rec.task_tags.push_back(h.tag);
      if (h.is_done) habits_rec.done++;
    }
    habits_data[today] = habits_rec;
    json::save(HABITS_FILE, habits_data);
  };

  auto raw_input = Input(&new_task, " Add task/habit (use +H prefix for habits) and press Enter...");

  auto root = CatchEvent(
    Renderer(raw_input, [&] {
      int tasks_total = (int)tasks.size();
      int tasks_done  = 0;
      for (auto &t : tasks) if (t.is_done) tasks_done++;
      float tasks_progress = tasks_total > 0 ? (float)tasks_done / tasks_total : 0.f;
      std::string tasks_pct = std::to_string(tasks_done) + "/" + std::to_string(tasks_total) +
                              "  " + std::to_string((int)(tasks_progress * 100)) + "%";

      int habits_total = (int)habits.size();
      int habits_done  = 0;
      for (auto &h : habits) if (h.is_done) habits_done++;
      float habits_progress = habits_total > 0 ? (float)habits_done / habits_total : 0.f;
      std::string habits_pct = std::to_string(habits_done) + "/" + std::to_string(habits_total) +
                               "  " + std::to_string((int)(habits_progress * 100)) + "%";

      bool input_focused = (focus == AppFocus::INPUT);
      auto input_row =
        hbox({
          text(" + ") | color(Color::Yellow),
          text(" COMMAND: ") | color(Color::GrayDark),
          raw_input->Render() | flex,
          text(input_focused ? " [Enter] " : " [Tab] ") | dim,
        }) |
        borderStyled(ROUNDED,
          input_focused ? Color::RGB(120, 180, 255) : Color::RGB(60, 70, 100));

      bool tasks_focused = (focus == AppFocus::TASKS);
      Elements task_rows;
      for (int i = 0; i < tasks_total; i++) {
        auto &t   = tasks[i];
        bool  sel = tasks_focused && (i == selected);
        auto num  = text(" " + std::to_string(i + 1) + ". ") | color(Color::GrayLight);
        auto icon = t.is_done
                      ? text(" v ") | color(Color::RGB(57, 211, 83))
                      : text(" o ") | color(Color::Yellow);
        auto lbl  = t.is_done
                      ? text(t.name) | strikethrough | color(Color::GrayDark) | flex
                      : text(t.name) | flex;
        
        Element tag_el = separatorEmpty();
        if (!t.tag.empty()) {
          Color tag_color = Color::RGB(100, 100, 200);
          if (t.tag == "URGENT") tag_color = Color::RGB(220, 60, 60);
          else if (t.tag == "RUSH") tag_color = Color::RGB(220, 120, 60);
          else if (t.tag == "BUDGET") tag_color = Color::RGB(100, 180, 100);
          
          tag_el = text(" " + t.tag + " ") | bgcolor(tag_color) | color(Color::RGB(200, 200, 200));
        }
        
        auto row  = hbox({num, icon, lbl, filler(), tag_el});
        if (sel) row = row | bgcolor(Color::RGB(38, 48, 75)) | bold;
        task_rows.push_back(row | size(HEIGHT, EQUAL, 1));
      }

      Element task_area = task_rows.empty()
        ? vbox({separatorEmpty(),
                text("  No tasks yet -- type above and press Enter") | dim | center,
                separatorEmpty()})
        : vbox(task_rows);

      auto task_box = task_area | frame | size(HEIGHT, GREATER_THAN, 3) |
                      borderStyled(ROUNDED,
                        tasks_focused ? Color::RGB(120, 180, 255)
                                      : Color::RGB(48, 60, 100));

      bool habits_focused = (focus == AppFocus::HABITS);
      Elements habit_rows;
      for (int i = 0; i < habits_total; i++) {
        auto &h   = habits[i];
        bool  sel = habits_focused && (i == selected);
        auto num  = text(" " + std::to_string(i + 1) + ". ") | color(Color::GrayLight);
        auto icon = h.is_done
                      ? text(" v ") | color(Color::RGB(57, 211, 83))
                      : text(" o ") | color(Color::Yellow);
        auto lbl  = h.is_done
                      ? text(h.name) | strikethrough | color(Color::GrayDark) | flex
                      : text(h.name) | flex;
        
        Element tag_el = separatorEmpty();
        if (!h.tag.empty()) {
          Color tag_color = Color::RGB(100, 100, 200);
          if (h.tag == "DAILY") tag_color = Color::RGB(100, 150, 200);
          else if (h.tag == "WEEKLY") tag_color = Color::RGB(150, 100, 200);
          
          tag_el = text(" " + h.tag + " ") | bgcolor(tag_color) | color(Color::RGB(200, 200, 200));
        }
        
        auto row  = hbox({num, icon, lbl, filler(), tag_el});
        if (sel) row = row | bgcolor(Color::RGB(38, 48, 75)) | bold;
        habit_rows.push_back(row | size(HEIGHT, EQUAL, 1));
      }

      Element habit_area = habit_rows.empty()
        ? vbox({text("  No habits yet") | dim | center})
        : vbox(habit_rows);

      auto habit_box = habit_area | frame | size(HEIGHT, GREATER_THAN, 3) |
                       borderStyled(ROUNDED,
                         habits_focused ? Color::RGB(120, 180, 255)
                                        : Color::RGB(48, 60, 100));

      Element hint;
      if (focus == AppFocus::INPUT) {
        hint = hbox({
          text(" INPUT ") | bgcolor(Color::RGB(50, 80, 180)) | bold,
          text("  Enter:add   Tab:tasks   Ctrl+L:habits   H:heatmap   Q:quit   (+H for habits)") | dim,
        });
      } else if (focus == AppFocus::TASKS) {
        hint = hbox({
            text(" TASKS ") | bgcolor(Color::RGB(30, 100, 50)) | bold,
            text("  up/down jk:move   Space:toggle   Ctrl+D:delete   "
                 "Esc/Tab:back   Ctrl+L:habits   Q:quit") |
                dim,
        });
      } else {
        hint = hbox({
            text(" HABITS ") | bgcolor(Color::RGB(100, 50, 100)) | bold,
            text("  up/down jk:move   Space:toggle   Ctrl+D:delete   "
                 "Esc/Tab:back   Ctrl+L:tasks   Q:quit") |
                dim,
        });
      }

      return vbox({
                 hbox({
                     text(" TODO CLI ") | bold |
                         color(Color::RGB(120, 180, 255)),
                     text(" v2.1.0 ") | dim | color(Color::GrayLight),
                     filler(),
                     text(" CONNECTED_SESSION ") |
                         bgcolor(Color::RGB(30, 100, 50)) | bold |
                         color(Color::GrayLight),
                     text(" " + today + " ") | color(Color::GrayLight),
                 }) | size(HEIGHT, EQUAL, 1),
                 separatorEmpty(),
                 renderHeatmap(tasks_data, today),
                 separatorEmpty(),
                 hbox({
                     text(" TASKS > ") | color(Color::RGB(57, 211, 83)),
                     gauge(tasks_progress) |
                         color(tasks_progress < 0.3f ? Color::RGB(200, 60, 60)
                               : tasks_progress < 0.7f
                                   ? Color::Yellow
                                   : Color::RGB(57, 211, 83)) |
                         flex,
                     text(" " + tasks_pct + " ") | color(Color::GrayLight),
                 }),
                 separatorEmpty(),
                 task_box,
                 separatorEmpty(),
                 hbox({
                     text(" HABITS > ") | color(Color::RGB(150, 100, 200)),
                     gauge(habits_progress) |
                         color(habits_progress < 0.3f ? Color::RGB(200, 60, 60)
                               : habits_progress < 0.7f
                                   ? Color::Yellow
                                   : Color::RGB(57, 211, 83)) |
                         flex,
                     text(" " + habits_pct + " ") | color(Color::GrayLight),
                 }),
                 separatorEmpty(),
                 habit_box,
                 separatorEmpty(),
                 input_row,
                 separatorEmpty(),
                 hint,
             }) |
             borderStyled(DOUBLE, Color::RGB(48, 60, 120)) |
             size(WIDTH, GREATER_THAN, 70) | center;
    }),

    [&](Event e) -> bool {

      // INPUT MODE
      if (focus == AppFocus::INPUT) {
        if (e == Event::Return) {
          if (!new_task.empty()) {
            bool is_habit = false;
            if (new_task.size() > 2 && new_task[0] == '+' && new_task[1] == 'H' && new_task[2] == ' ') {
              is_habit = true;
              new_task = new_task.substr(3);
            }

            std::string tag;
            size_t hash_pos = new_task.rfind('#');
            if (hash_pos != std::string::npos && hash_pos > 0) {
              tag = new_task.substr(hash_pos + 1);
              while (!tag.empty() && std::isspace((unsigned char)tag.back()))
                tag.pop_back();
              new_task = new_task.substr(0, hash_pos);
              while (!new_task.empty() && std::isspace((unsigned char)new_task.back()))
                new_task.pop_back();
            }

            if (is_habit) {
              habits.push_back({new_task, false, tag});
              selected = (int)habits.size() - 1;
              focus = AppFocus::HABITS;
            } else {
              tasks.push_back({new_task, false, tag});
              selected = (int)tasks.size() - 1;
              focus = AppFocus::TASKS;
            }
            new_task.clear();
            persist();
          }
          return true;
        }
        if (e == Event::Tab && !tasks.empty()) {
          focus = AppFocus::TASKS;
          selected = 0;
          return true;
        }
        if (e == Event::CtrlL && !habits.empty()) {
          focus = AppFocus::HABITS;
          selected = 0;
          return true;
        }
        if (e == Event::Character('q')) {
          persist();
          screen.ExitLoopClosure()();
          return true;
        }
        return raw_input->OnEvent(e);
      }

      // TASK MODE
      if (focus == AppFocus::TASKS) {
        if (e == Event::Escape || e == Event::Tab) {
          focus = AppFocus::INPUT;
          return true;
        }
        if (tasks.empty()) return true;

        if (e == Event::ArrowDown || e == Event::Character('j')) {
          selected = std::min((int)tasks.size() - 1, selected + 1);
          return true;
        }
        if (e == Event::ArrowUp || e == Event::Character('k')) {
          selected = std::max(0, selected - 1);
          return true;
        }
        if (e == Event::Character(' ')) {
          tasks[selected].is_done = !tasks[selected].is_done;
          persist();
          return true;
        }
        // Toggle habit or task  with 'h'
        if (e == Event::Character('h')) {
          TodoItem item = tasks[selected];
          tasks.erase(tasks.begin() + selected);
          if (!tasks.empty())
            selected = std::min(selected, (int)tasks.size() - 1);
          else
            selected = 0;

          item.is_done = false; // Reset done status when moving to habits
          habits.push_back(item);
          selected = (int)habits.size() - 1;
          focus = AppFocus::HABITS;
          persist();
          return true;
        }
        if (e == Event::Character('\x04')) {
          tasks.erase(tasks.begin() + selected);
          if (!tasks.empty())
            selected = std::min(selected, (int)tasks.size() - 1);
          else
            selected = 0;
          persist();
          return true;
        }
        if (e == Event::CtrlL && !habits.empty()) {
          focus = AppFocus::HABITS;
          selected = 0;
          return true;
        }
        if (e == Event::Character('q')) {
          persist();
          screen.ExitLoopClosure()();
          return true;
        }
        return true;
      }

      // HABITS MODE
      if (focus == AppFocus::HABITS) {
        if (e == Event::Escape || e == Event::Tab) {
          focus = AppFocus::INPUT;
          return true;
        }
        if (habits.empty()) return true;

        if (e == Event::ArrowDown || e == Event::Character('j')) {
          selected = std::min((int)habits.size() - 1, selected + 1);
          return true;
        }
        if (e == Event::ArrowUp || e == Event::Character('k')) {
          selected = std::max(0, selected - 1);
          return true;
        }
        if (e == Event::Character(' ')) {
          habits[selected].is_done = !habits[selected].is_done;
          persist();
          return true;
        }
        if (e == Event::Character('d')) {
          habits.erase(habits.begin() + selected);
          if (!habits.empty())
            selected = std::min(selected, (int)habits.size() - 1);
          else
            selected = 0;
          persist();
          return true;
        }
        if (e == Event::CtrlL && !tasks.empty()) {
          focus = AppFocus::TASKS;
          selected = 0;
          return true;
        }
        if (e == Event::Character('q')) {
          persist();
          screen.ExitLoopClosure()();
          return true;
        }
        return true;
      }

      return false;
    }
  );

  screen.Loop(root);
  return 0;
}