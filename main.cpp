// Todo CLI v3.0 — Professional rewrite
// Pages: TASKS page / HABITS page (separate views)
// Keyboard conflicts in input mode fully resolved

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

// ─────────────────────────────────────────────
//  JSON (minimal hand-rolled, no deps)
// ─────────────────────────────────────────────
namespace json {

std::string escape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"')
      out += "\\\"";
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
  std::vector<std::string> task_tags;
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
      if (e == '"')
        val += '"';
      else if (e == '\\') val += '\\';
      else if (e == 'n')  val += '\n';
      else
        val += e;
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
          if (src.substr(pos, 4) == "true") {
            rec.task_done.push_back(true);
            pos += 4;
          } else if (src.substr(pos, 5) == "false") {
            rec.task_done.push_back(false);
            pos += 5;
          } else
            pos++;
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

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
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

// ─────────────────────────────────────────────
//  Heatmap
// ─────────────────────────────────────────────
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
    default:
      return Color::RGB(80, 255, 120);
    }
  };

  auto level = [&](const std::string &d) -> int {
    auto it = data.find(d);
    if (it == data.end() || it->second.total == 0) return 0;
    float r = (float)it->second.done / it->second.total;
    if (r <= 0.0f)  return 0;
    if (r < 0.25f)
      return 1;
    if (r < 0.50f)
      return 2;
    if (r < 0.75f)
      return 3;
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
      if (d > today)
        cell = text("∙ ") | color(Color::RGB(40, 44, 52));
      else if (d == today)
        cell = text("■ ") | color(Color::RGB(255, 215, 0)) | bold;
      else
        cell = text("■ ") | color(cellColor(level(d)));
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

  return vbox(all_rows) | borderStyled(ROUNDED, Color::RGB(56, 62, 71)) |
         size(HEIGHT, GREATER_THAN, 12);
}

// ─────────────────────────────────────────────
//  App state
// ─────────────────────────────────────────────

// AppPage — which page is active (not which widget is focused)
enum class AppPage { TASKS, HABITS };

// InputMode — whether the input field is active (typing)
enum class InputMode { ACTIVE, INACTIVE };

struct TodoItem {
  std::string name;
  bool is_done = false;
  std::string tag = "";
};

// ─────────────────────────────────────────────
//  Shared rendering helpers
// ─────────────────────────────────────────────
static Element makeTagBadge(const std::string &tag, bool isHabit) {
  if (tag.empty())
    return separatorEmpty();
  Color c = isHabit ? (tag == "DAILY"    ? Color::RGB(60, 120, 200)
                       : tag == "WEEKLY" ? Color::RGB(130, 80, 200)
                                         : Color::RGB(80, 100, 180))
                    : (tag == "URGENT"   ? Color::RGB(220, 60, 60)
                       : tag == "RUSH"   ? Color::RGB(220, 120, 60)
                       : tag == "BUDGET" ? Color::RGB(80, 160, 80)
                                         : Color::RGB(80, 100, 180));
  return text(" " + tag + " ") | bgcolor(c) | color(Color::White) | bold;
}

static Element makeItemRow(const TodoItem &item, int idx, bool selected) {
  auto num =
      text(" " + std::to_string(idx + 1) + ". ") | color(Color::GrayLight);
  auto icon = item.is_done ? text(" ✓ ") | color(Color::RGB(57, 211, 83)) | bold
                           : text(" ○ ") | color(Color::RGB(200, 200, 80));
  auto lbl = item.is_done ? text(item.name) | strikethrough |
                                color(Color::RGB(90, 90, 90)) | flex
                          : text(item.name) | flex;

  auto row = hbox({num, icon, lbl, filler()});
  if (selected)
    row = row | bgcolor(Color::RGB(35, 50, 80)) | bold;
  return row | size(HEIGHT, EQUAL, 1);
}

// ─────────────────────────────────────────────
//  Progress bar helper
// ─────────────────────────────────────────────
static std::pair<int, int> countProgress(const std::vector<TodoItem> &items) {
  int done = 0;
  for (auto &it : items)
    if (it.is_done)
      done++;
  return {done, (int)items.size()};
}

static Color progressColor(float r) {
  if (r < 0.3f)
    return Color::RGB(200, 60, 60);
  if (r < 0.7f)
    return Color::Yellow;
  return Color::RGB(57, 211, 83);
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main() {
  const std::string TASKS_FILE = "todo_tasks.json";
  const std::string HABITS_FILE = "todo_habits.json";
  json::DayMap tasks_data = json::load(TASKS_FILE);
  json::DayMap      habits_data = json::load(HABITS_FILE);
  std::string today = todayStr();

  // Load today's tasks
  std::vector<TodoItem> tasks;
  if (tasks_data.count(today)) {
    auto &rec = tasks_data[today];
    for (size_t i = 0; i < rec.tasks.size(); i++) {
      std::string tag = (i < rec.task_tags.size()) ? rec.task_tags[i] : "";
      tasks.push_back({rec.tasks[i], i < rec.task_done.size() && rec.task_done[i], tag});
    }
  }

  // Load today's habits
  std::vector<TodoItem> habits;
  if (habits_data.count(today)) {
    auto &rec = habits_data[today];
    for (size_t i = 0; i < rec.tasks.size(); i++) {
      std::string tag = (i < rec.task_tags.size()) ? rec.task_tags[i] : "";
      habits.push_back({rec.tasks[i], i < rec.task_done.size() && rec.task_done[i], tag});
    }
  }

  auto screen = ScreenInteractive::TerminalOutput();

  // ── State ──────────────────────────────────
  AppPage page = AppPage::TASKS;           // which page is visible
  InputMode inputMode = InputMode::ACTIVE; // is input box focused
  int task_sel = 0;                        // selected index in tasks list
  int habit_sel = 0;     // selected index in habits list (independent!)
  std::string new_input; // text in input box

  // ── Persist ────────────────────────────────
  auto persist = [&] {
    json::DayRecord task_rec;
    task_rec.total = (int)tasks.size();
    for (auto &t : tasks) {
      task_rec.tasks.push_back(t.name);
      task_rec.task_done.push_back(t.is_done);
      task_rec.task_tags.push_back(t.tag);
      if (t.is_done)
        task_rec.done++;
    }
    tasks_data[today] = task_rec;
    json::save(TASKS_FILE, tasks_data);

    json::DayRecord hab_rec;
    hab_rec.total = (int)habits.size();
    for (auto &h : habits) {
      hab_rec.tasks.push_back(h.name);
      hab_rec.task_done.push_back(h.is_done);
      hab_rec.task_tags.push_back(h.tag);
      if (h.is_done)
        hab_rec.done++;
    }
    habits_data[today] = hab_rec;
    json::save(HABITS_FILE, habits_data);
  };

  // ── Input component ────────────────────────
  // Placeholder text changes per page
  auto getPlaceholder = [&]() -> std::string {
    if (page == AppPage::TASKS)
      return "  Add task... (#TAG optional, e.g. Buy milk #URGENT) — Enter to "
             "add";
    else
      return "  Add habit... (#DAILY or #WEEKLY optional) — Enter to add";
  };

  auto raw_input = Input(&new_input, getPlaceholder());

  // ─────────────────────────────────────────
  //  RENDERER
  // ─────────────────────────────────────────
  auto root = CatchEvent(
      Renderer(
          raw_input,
          [&] {
            bool inInput = (inputMode == InputMode::ACTIVE);

            // ── TASKS PAGE ──────────────────────────────────────────────
            if (page == AppPage::TASKS) {
              auto [tasks_done, tasks_total] = countProgress(tasks);
              float tp =
                  tasks_total > 0 ? (float)tasks_done / tasks_total : 0.f;
              std::string tpct = std::to_string(tasks_done) + "/" +
                                 std::to_string(tasks_total) + "  " +
                                 std::to_string((int)(tp * 100)) + "%";

              // Task list
              Elements task_rows;
              for (int i = 0; i < tasks_total; i++) {
                bool sel = !inInput && (i == task_sel);
                auto row = makeItemRow(tasks[i], i, sel);
                // append tag
                auto tag_el = makeTagBadge(tasks[i].tag, false);
                task_rows.push_back(hbox({row | flex, tag_el}) |
                                    size(HEIGHT, EQUAL, 1));
              }

              Element task_area = task_rows.empty()
                                      ? vbox({separatorEmpty(),
                                              text("  No tasks yet — type "
                                                   "below and press Enter") |
                                                  dim | center,
                                              separatorEmpty()})
                                      : vbox(task_rows);

              // Border highlights: blue when list focused, else dimmed
              Color list_border =
                  inInput ? Color::RGB(48, 60, 100) : Color::RGB(80, 140, 255);

              auto task_box = task_area | frame |
                              size(HEIGHT, GREATER_THAN, 4) |
                              borderStyled(ROUNDED, list_border);

              // Input box
              Color inp_border =
                  inInput ? Color::RGB(80, 140, 255) : Color::RGB(48, 60, 100);
              auto input_row = hbox({
                                   text(inInput ? " ›› " : "    ") |
                                       color(Color::RGB(255, 200, 50)) | bold,
                                   raw_input->Render() | flex,
                                   text(inInput ? " Enter " : " i ") | dim,
                               }) |
                               borderStyled(ROUNDED, inp_border);

              // Hint bar
              Element hint;
              if (inInput) {
                hint = hbox({
                    text(" INSERT ") | bgcolor(Color::RGB(30, 80, 180)) | bold |
                        color(Color::White),
                    text("  Enter:add   Esc:list mode   Ctrl+H:habits page   "
                         "Q:quit   #TAG for tags") |
                        dim,
                });
              } else {
                hint = hbox({
                    text(" TASKS  ") | bgcolor(Color::RGB(20, 100, 50)) | bold |
                        color(Color::White),
                    text(
                        "  ↑↓/jk:move   Space:toggle   D:delete   "
                        "M:move→habits   I/ins:type   Ctrl+H:habits   Q:quit") |
                        dim,
                });
              }

              // Page tab bar
              auto tab_bar = hbox({
                  text(" ◆ TASKS ") | bgcolor(Color::RGB(20, 100, 50)) |
                      color(Color::White) | bold,
                  text("   HABITS ") | color(Color::RGB(90, 90, 90)) | dim,
                  filler(),
                  text(" Ctrl+H → habits ") | dim | color(Color::GrayDark),
              });

              // Header
              auto header =
                  hbox({
                      text(" ✦ TODO CLI ") | bold |
                          color(Color::RGB(120, 180, 255)),
                      text(" v3.0 ") | dim | color(Color::GrayLight),
                      filler(),
                      text(" " + today + " ") | color(Color::GrayLight),
                  }) |
                  size(HEIGHT, EQUAL, 1);

              // Progress bar
              auto prog_bar = hbox({
                  text(" TASKS > ") | color(Color::RGB(57, 211, 83)) | bold,
                  gauge(tp) | color(progressColor(tp)) | flex,
                  text(" " + tpct + " ") | color(Color::GrayLight),
              });

              return vbox({
                         header,
                         separator() | color(Color::RGB(48, 60, 120)),
                         tab_bar,
                         separator() | color(Color::RGB(48, 60, 120)),
                         renderHeatmap(tasks_data, today),
                         separatorEmpty(),
                         prog_bar,
                         separatorEmpty(),
                         task_box,
                         separatorEmpty(),
                         input_row,
                         separatorEmpty(),
                         hint,
                     }) |
                     borderStyled(DOUBLE, Color::RGB(40, 55, 110)) |
                     size(WIDTH, GREATER_THAN, 72) | center;
            }

            // ── HABITS PAGE ──────────────────────────────────────────────
            else {
              auto [hab_done, hab_total] = countProgress(habits);
              float hp = hab_total > 0 ? (float)hab_done / hab_total : 0.f;
              std::string hpct = std::to_string(hab_done) + "/" +
                                 std::to_string(hab_total) + "  " +
                                 std::to_string((int)(hp * 100)) + "%";

              // Habit list
              Elements hab_rows;
              for (int i = 0; i < hab_total; i++) {
                bool sel = !inInput && (i == habit_sel);
                auto row = makeItemRow(habits[i], i, sel);
                auto tag_el = makeTagBadge(habits[i].tag, true);
                hab_rows.push_back(hbox({row | flex, tag_el}) |
                                   size(HEIGHT, EQUAL, 1));
              }

              Element hab_area = hab_rows.empty()
                                     ? vbox({separatorEmpty(),
                                             text("  No habits yet — type "
                                                  "below and press Enter") |
                                                 dim | center,
                                             separatorEmpty()})
                                     : vbox(hab_rows);

              Color list_border =
                  inInput ? Color::RGB(60, 40, 100) : Color::RGB(180, 100, 255);

              auto hab_box = hab_area | frame | size(HEIGHT, GREATER_THAN, 4) |
                             borderStyled(ROUNDED, list_border);

              // Input box
              Color inp_border =
                  inInput ? Color::RGB(180, 100, 255) : Color::RGB(60, 40, 100);
              auto input_row = hbox({
                                   text(inInput ? " ›› " : "    ") |
                                       color(Color::RGB(200, 120, 255)) | bold,
                                   raw_input->Render() | flex,
                                   text(inInput ? " Enter " : " i ") | dim,
                               }) |
                               borderStyled(ROUNDED, inp_border);

              // Hint bar
              Element hint;
              if (inInput) {
                hint = hbox({
                    text(" INSERT ") | bgcolor(Color::RGB(100, 30, 160)) |
                        bold | color(Color::White),
                    text("  Enter:add   Esc:list mode   Ctrl+T:tasks page   "
                         "Q:quit   #DAILY #WEEKLY") |
                        dim,
                });
              } else {
                hint = hbox({
                    text(" HABITS ") | bgcolor(Color::RGB(100, 30, 160)) |
                        bold | color(Color::White),
                    text("  ↑↓/jk:move   Space:toggle   D:delete   "
                         "M:move→tasks   I/ins:type   Ctrl+T:tasks   Q:quit") |
                        dim,
                });
              }

              // Page tab bar
              auto tab_bar = hbox({
                  text("   TASKS  ") | color(Color::RGB(90, 90, 90)) | dim,
                  text(" ◆ HABITS ") | bgcolor(Color::RGB(80, 30, 130)) |
                      color(Color::White) | bold,
                  filler(),
                  text(" Ctrl+T → tasks ") | dim | color(Color::GrayDark),
              });

              // Header
              auto header =
                  hbox({
                      text(" ✦ TODO CLI ") | bold |
                          color(Color::RGB(200, 140, 255)),
                      text(" v3.0 ") | dim | color(Color::GrayLight),
                      filler(),
                      text(" " + today + " ") | color(Color::GrayLight),
                  }) |
                  size(HEIGHT, EQUAL, 1);

              // Progress bar
              auto prog_bar = hbox({
                  text(" HABITS > ") | color(Color::RGB(180, 100, 255)) | bold,
                  gauge(hp) | color(progressColor(hp)) | flex,
                  text(" " + hpct + " ") | color(Color::GrayLight),
              });

              // Habit heatmap (habits_data)
              return vbox({
                         header,
                         separator() | color(Color::RGB(80, 40, 120)),
                         tab_bar,
                         separator() | color(Color::RGB(80, 40, 120)),
                         renderHeatmap(habits_data, today),
                         separatorEmpty(),
                         prog_bar,
                         separatorEmpty(),
                         hab_box,
                         separatorEmpty(),
                         input_row,
                         separatorEmpty(),
                         hint,
                     }) |
                     borderStyled(DOUBLE, Color::RGB(70, 30, 110)) |
                     size(WIDTH, GREATER_THAN, 72) | center;
            }
          }),

      // ─────────────────────────────────────────
      //  EVENT HANDLER
      //  Key design:
      //  • In INPUT mode (INSERT), only Enter/Esc/Ctrl+H/Ctrl+T are
      //  intercepted.
      //    All other keys are forwarded to the input widget → no conflicts.
      //  • In LIST mode, vim keys (j/k/d/m/i/space/q) work freely.
      // ─────────────────────────────────────────
      [&](Event e) -> bool {
        // ────────────── INSERT MODE ──────────────
        if (inputMode == InputMode::ACTIVE) {

          // Enter → add item to current page
          if (e == Event::Return) {
            if (!new_input.empty()) {
              // Extract tag from #TAG at end
              std::string tag;
              std::string name = new_input;
              size_t hp = name.rfind('#');
              if (hp != std::string::npos && hp > 0) {
                tag = name.substr(hp + 1);
                while (!tag.empty() && std::isspace((unsigned char)tag.back()))
                  tag.pop_back();
                name = name.substr(0, hp);
                while (!name.empty() &&
                       std::isspace((unsigned char)name.back()))
                  name.pop_back();
              }
              if (!name.empty()) {
                if (page == AppPage::TASKS) {
                  tasks.push_back({name, false, tag});
                  task_sel = (int)tasks.size() - 1;
                } else {
                  habits.push_back({name, false, tag});
                  habit_sel = (int)habits.size() - 1;
                }
                new_input.clear();
                persist();
              }
            }
            return true;
          }

          // Esc → switch to LIST mode (stay on same page)
          if (e == Event::Escape) {
            inputMode = InputMode::INACTIVE;
            new_input.clear();
            return true;
          }

          // Ctrl+G → switch to HABITS page (stay in insert mode)
          if (e == Event::CtrlG) {
            page = AppPage::HABITS;
            new_input.clear();
            return true;
          }

          // Ctrl+T → switch to TASKS page (stay in insert mode)
          if (e == Event::CtrlT) {
            page = AppPage::TASKS;
            new_input.clear();
            return true;
          }

          // All other events → let input widget handle (typing freely)
          return raw_input->OnEvent(e);
        }

        // ────────────── LIST MODE (INACTIVE input) ──────────────

        // Switch page: Ctrl+G or Ctrl+T
        if (e == Event::CtrlG) {
          page = AppPage::HABITS;
          return true;
        }
        if (e == Event::CtrlT) {
          page = AppPage::TASKS;
          return true;
        }

        // Enter INSERT mode: 'i' or Insert key
        if (e == Event::Character('i') || e == Event::Insert) {
          inputMode = InputMode::ACTIVE;
          return true;
        }

        // Quit
        if (e == Event::Character('q') || e == Event::Character('Q')) {
          persist();
          screen.ExitLoopClosure()();
          return true;
        }

        // ── TASKS page LIST mode ──────────────────
        if (page == AppPage::TASKS) {
          if (tasks.empty())
            return true;

          if (e == Event::ArrowDown || e == Event::Character('j')) {
            task_sel = std::min((int)tasks.size() - 1, task_sel + 1);
            return true;
          }
          if (e == Event::ArrowUp || e == Event::Character('k')) {
            task_sel = std::max(0, task_sel - 1);
            return true;
          }
          if (e == Event::Character(' ')) {
            tasks[task_sel].is_done = !tasks[task_sel].is_done;
            persist();
            return true;
          }
          // D → delete
          if (e == Event::Character('d') || e == Event::Character('D')) {
            tasks.erase(tasks.begin() + task_sel);
            if (!tasks.empty())
              task_sel = std::min(task_sel, (int)tasks.size() - 1);
            else
              task_sel = 0;
            persist();
            return true;
          }
          // M → move selected task to habits
          if (e == Event::Character('m') || e == Event::Character('M')) {
            TodoItem item = tasks[task_sel];
            tasks.erase(tasks.begin() + task_sel);
            if (!tasks.empty())
              task_sel = std::min(task_sel, (int)tasks.size() - 1);
            else
              task_sel = 0;
            item.is_done = false;
            habits.push_back(item);
            habit_sel = (int)habits.size() - 1;
            page = AppPage::HABITS;
            persist();
            return true;
          }
          return true;
        }

        // ── HABITS page LIST mode ──────────────────
        if (page == AppPage::HABITS) {
          if (habits.empty())
            return true;

          if (e == Event::ArrowDown || e == Event::Character('j')) {
            habit_sel = std::min((int)habits.size() - 1, habit_sel + 1);
            return true;
          }
          if (e == Event::ArrowUp || e == Event::Character('k')) {
            habit_sel = std::max(0, habit_sel - 1);
            return true;
          }
          if (e == Event::Character(' ')) {
            habits[habit_sel].is_done = !habits[habit_sel].is_done;
            persist();
            return true;
          }
          // D → delete
          if (e == Event::Character('d') || e == Event::Character('D')) {
            habits.erase(habits.begin() + habit_sel);
            if (!habits.empty())
              habit_sel = std::min(habit_sel, (int)habits.size() - 1);
            else
              habit_sel = 0;
            persist();
            return true;
          }
          // M → move selected habit to tasks
          if (e == Event::Character('m') || e == Event::Character('M')) {
            TodoItem item = habits[habit_sel];
            habits.erase(habits.begin() + habit_sel);
            if (!habits.empty())
              habit_sel = std::min(habit_sel, (int)habits.size() - 1);
            else
              habit_sel = 0;
            item.is_done = false;
            tasks.push_back(item);
            task_sel = (int)tasks.size() - 1;
            page = AppPage::TASKS;
            persist();
            return true;
          }
          return true;
        }

        return false;
      });

  screen.Loop(root);
  return 0;
}