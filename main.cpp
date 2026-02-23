#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ftxui/dom/canvas.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

// ══════════════════════════════════════════════════════
//  JSON
// ══════════════════════════════════════════════════════
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
      if (e == '"') val += '"'; else if (e == '\\') val += '\\';
      else if (e == 'n') val += '\n'; else val += e;
    } else if (c == '"') break;
    else val += c;
  }
  return val;
}

DayMap load(const std::string &filename) {
  DayMap result;
  std::ifstream f(filename);
  if (!f.is_open()) return result;
  std::string src((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
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
      pos++; skipWS(src, pos);
      if (key == "done" || key == "total") {
        std::string num;
        while (pos < src.size() && std::isdigit((unsigned char)src[pos])) num += src[pos++];
        int v = num.empty() ? 0 : std::stoi(num);
        if (key == "done") rec.done = v; else rec.total = v;
      } else if (key == "tasks" || key == "task_tags") {
        if (src[pos] != '[') continue; pos++;
        while (pos < src.size()) {
          skipWS(src, pos);
          if (src[pos] == ']') { pos++; break; }
          if (src[pos] == '"') { pos++; auto s = parseStr(src, pos); if (key=="tasks") rec.tasks.push_back(s); else rec.task_tags.push_back(s); }
          else pos++;
          skipWS(src, pos);
          if (pos < src.size() && src[pos] == ',') pos++;
        }
      } else if (key == "task_done") {
        if (src[pos] != '[') continue; pos++;
        while (pos < src.size()) {
          skipWS(src, pos);
          if (src[pos] == ']') { pos++; break; }
          if (src.substr(pos,4) == "true")  { rec.task_done.push_back(true);  pos+=4; }
          else if (src.substr(pos,5) == "false") { rec.task_done.push_back(false); pos+=5; }
          else pos++;
          skipWS(src, pos);
          if (pos < src.size() && src[pos] == ',') pos++;
        }
      } else { while (pos < src.size() && src[pos] != ',' && src[pos] != '}') pos++; }
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
    if (!first) f << ",\n"; first = false;
    f << "    \"" << date << "\": {\n";
    f << "      \"done\": " << rec.done << ",\n      \"total\": " << rec.total << ",\n";
    f << "      \"tasks\": [";
    for (size_t i = 0; i < rec.tasks.size(); i++) { f << "\"" << escape(rec.tasks[i]) << "\""; if (i+1<rec.tasks.size()) f << ", "; }
    f << "],\n      \"task_done\": [";
    for (size_t i = 0; i < rec.task_done.size(); i++) { f << (rec.task_done[i]?"true":"false"); if (i+1<rec.task_done.size()) f << ", "; }
    f << "],\n      \"task_tags\": [";
    for (size_t i = 0; i < rec.task_tags.size(); i++) { f << "\"" << escape(rec.task_tags[i]) << "\""; if (i+1<rec.task_tags.size()) f << ", "; }
    f << "]\n    }";
  }
  f << "\n  }\n}\n";
}
} // namespace json

// ══════════════════════════════════════════════════════
//  Date helpers
// ══════════════════════════════════════════════════════
static std::string todayStr() {
  auto now = std::chrono::system_clock::now();
  auto tt  = std::chrono::system_clock::to_time_t(now);
  char buf[11];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&tt));
  return buf;
}

static int weekdayOf(const std::string &date) {
  struct tm tm = {}; std::istringstream ss(date);
  ss >> std::get_time(&tm, "%Y-%m-%d"); std::mktime(&tm);
  return tm.tm_wday;
}

static std::string monthShort(const std::string &date) {
  struct tm tm = {}; std::istringstream ss(date);
  ss >> std::get_time(&tm, "%Y-%m-%d");
  const char *m[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  return m[tm.tm_mon];
}

// ══════════════════════════════════════════════════════
//  3D Cube — improved rendering
// ══════════════════════════════════════════════════════
struct V3 { float x, y, z; };

static V3 rxyz(V3 p, float ax, float ay, float az) {
  // rotate X
  float cy = std::cos(ax), sy = std::sin(ax);
  p = {p.x, p.y*cy - p.z*sy, p.y*sy + p.z*cy};
  // rotate Y
  float cx = std::cos(ay), sx = std::sin(ay);
  p = {p.x*cx + p.z*sx, p.y, -p.x*sx + p.z*cx};
  // rotate Z
  float cz = std::cos(az), sz = std::sin(az);
  p = {p.x*cz - p.y*sz, p.x*sz + p.y*cz, p.z};
  return p;
}

// Canvas size (in "cells" — each cell = 2px wide, 4px tall in braille)
static const int CW = 80;   // canvas pixel width
static const int CH = 64;   // canvas pixel height

// Project 3D → canvas pixel
static std::pair<int,int> proj(V3 p) {
  float fov = 5.5f;
  float scale = fov / (fov + p.z);
  int px = (int)(CW/2 + p.x * scale * 18.f);
  int py = (int)(CH/2 + p.y * scale * 18.f);
  return {px, py};
}

// static void drawLine(Canvas &c, V3 a, V3 b, Color col) {
//   auto [ax, ay] = proj(a);
//   auto [bx, by] = proj(b);
//   c.DrawPointLine(ax, ay, bx, by, col);
// }

// Draw a filled circle on canvas (for vertex dots)
static void drawLine(Canvas &c, V3 p1, V3 p2, Color col) {
  auto [x1, y1] = proj(p1);
  auto [x2, y2] = proj(p2);
  c.DrawPointLine(static_cast<int>(x1), static_cast<int>(y1), 
                  static_cast<int>(x2), static_cast<int>(y2), col);
}

Element renderCube(float t) {
  // Cube vertices  (-1..1)³
  std::vector<V3> verts = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},  // back face  0-3
    {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1}   // front face 4-7
  };

  // Slow, smooth rotation
  float ax = t * 0.37f;
  float ay = t * 0.53f;
  float az = t * 0.19f;

  std::vector<V3> rv;
  for (auto &v : verts) rv.push_back(rxyz(v, ax, ay, az));

  // Edge list  {i, j, face}  face 0=back,1=front,2=side
  // Colour per group:
  //   back edges  → dim teal
  //   front edges → bright cyan
  //   side edges  → blue-violet
  struct Edge { int a, b; int grp; };
  std::vector<Edge> edges = {
    // back face
    {0,1,0},{1,2,0},{2,3,0},{3,0,0},
    // front face
    {4,5,1},{5,6,1},{6,7,1},{7,4,1},
    // side pillars
    {0,4,2},{1,5,2},{2,6,2},{3,7,2},
  };

  // Face diagonals (for inner "X" on each face — gives depth feel)
  std::vector<Edge> diags = {
    {0,2,0},{1,3,0},   // back
    {4,6,1},{5,7,1},   // front
  };

  // z-sort edges by average z so front edges are drawn last (painter)
  auto avgZ = [&](const Edge &e) {
    return (rv[e.a].z + rv[e.b].z) / 2.f;
  };
  std::sort(edges.begin(), edges.end(), [&](auto &a, auto &b){
    return avgZ(a) < avgZ(b);
  });

  Canvas c(CW, CH);

  // Colour scheme
  auto grpColor = [](int grp, float brightness) -> Color {
    // grp0=back, grp1=front, grp2=side
    if (grp == 0) return Color::RGB(80, 255, 120);
    // if (grp == 0) return Color::RGB((int)(0*brightness),  (int)(180*brightness), (int)(200*brightness));
    if (grp == 1) return Color::RGB(80, 255, 120);
    // if (grp == 1) return Color::RGB((int)(80*brightness), (int)(220*brightness), (int)(255*brightness));
    return         Color::RGB(80, 255, 120);
    // return         Color::RGB((int)(120*brightness),(int)(100*brightness),(int)(255*brightness));
  };

  // Draw diagonal crosses on faces (dim)
  for (auto &e : diags)
    drawLine(c, rv[e.a], rv[e.b], Color::RGB(80, 255, 120));

  // Draw main edges
  for (auto &e : edges) {
    // Brightness based on z (pseudo-lighting)
    float z_mid = (rv[e.a].z + rv[e.b].z) / 2.f;
    float bright = 0.5f + 0.5f * ((z_mid + 1.5f) / 3.0f);
    bright = std::max(0.3f, std::min(1.0f, bright));
    drawLine(c, rv[e.a], rv[e.b], grpColor(e.grp, bright));
  }

  // Draw vertex dots (front vertices brighter)
  for (int i = 0; i < 8; i++) {
    float z = rv[i].z;
    Color vcol = z > 0
      ? Color::RGB(80, 255, 120)   // front
      : Color::RGB(80, 255, 120);  // back
    drawLine(c, rv[i], rv[i], vcol);
  }

  // Canvas widget
  int termW = CW / 2 + 2;   // braille cols ≈ CW/2
  int termH = CH / 4 + 2;   // braille rows ≈ CH/4
  return canvas(std::move(c))
    | size(WIDTH,  EQUAL, termW)
    | size(HEIGHT, EQUAL, termH)
    | center;
}

// ══════════════════════════════════════════════════════
//  Heatmap
// ══════════════════════════════════════════════════════

Element renderHeatmap(const json::DayMap &data, const std::string &today) {
  const int WEEKS = 20;

auto cellColor = [](int lv) -> Color {
    switch (lv) {
      case 0: return Color::RGB(35, 35, 45);    
      case 1: return Color::RGB(0, 75, 30);     
      case 2: return Color::RGB(0, 155, 60);    
      case 3: return Color::RGB(0, 215, 80);    
      case 4: return Color::RGB(80, 255, 120);  
      default: return Color::RGB(80, 255, 120);
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
// ══════════════════════════════════════════════════════
//  App state
// ══════════════════════════════════════════════════════
enum class AppFocus { INPUT, TASKS, HABITS };
struct TodoItem { std::string name; bool is_done=false; std::string tag=""; };

int main() {
  const std::string TASKS_FILE  = "todo_tasks.json";
  const std::string HABITS_FILE = "todo_habits.json";
  json::DayMap tasks_data   = json::load(TASKS_FILE);
  json::DayMap habits_data  = json::load(HABITS_FILE);
  std::string  today        = todayStr();

  std::vector<TodoItem> tasks, habits;
  if (tasks_data.count(today)) {
    auto &rec=tasks_data[today];
    for (size_t i=0;i<rec.tasks.size();i++) {
      std::string tag = i<rec.task_tags.size()?rec.task_tags[i]:"";
      tasks.push_back({rec.tasks[i], i<rec.task_done.size()&&rec.task_done[i], tag});
    }
  }
  if (habits_data.count(today)) {
    auto &rec=habits_data[today];
    for (size_t i=0;i<rec.tasks.size();i++) {
      std::string tag = i<rec.task_tags.size()?rec.task_tags[i]:"";
      habits.push_back({rec.tasks[i], i<rec.task_done.size()&&rec.task_done[i], tag});
    }
  }

  auto screen = ScreenInteractive::TerminalOutput();
  std::string new_task;
  int         selected    = 0;
  AppFocus    focus       = AppFocus::INPUT;
  long long   start_ms    = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now().time_since_epoch()).count();

  auto persist = [&] {
    auto makeRec = [](const std::vector<TodoItem> &v) {
      json::DayRecord r; r.total=(int)v.size();
      for (auto &t:v) { r.tasks.push_back(t.name); r.task_done.push_back(t.is_done);
                        r.task_tags.push_back(t.tag); if(t.is_done) r.done++; }
      return r;
    };
    tasks_data[today]  = makeRec(tasks);  json::save(TASKS_FILE,  tasks_data);
    habits_data[today] = makeRec(habits); json::save(HABITS_FILE, habits_data);
  };

  auto raw_input = Input(&new_task, " Add task (or '+H name' for habit, '#TAG' suffix for tag)...");

  auto root = CatchEvent(
    Renderer(raw_input, [&] {
      // ── time
      long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
      float t = (now_ms - start_ms) * 0.001f;

      // ── progress
      auto prog = [](const std::vector<TodoItem>&v) -> std::pair<float,std::string> {
        int tot=(int)v.size(), dn=0;
        for (auto &x:v) if(x.is_done) dn++;
        float p = tot>0?(float)dn/tot:0.f;
        return {p, std::to_string(dn)+"/"+std::to_string(tot)+"  "+std::to_string((int)(p*100))+"%"};
      };
      auto [tp,ts] = prog(tasks);
      auto [hp,hs] = prog(habits);

      // ── input row
      bool if_ = (focus==AppFocus::INPUT);
      auto input_row = hbox({
        text(" + ")|color(Color::Yellow),
        raw_input->Render()|flex,
        text(if_?" [Enter] ":" [Tab] ")|dim,
      })|borderStyled(ROUNDED, if_?Color::RGB(120,180,255):Color::RGB(60,70,100));

      // ── task list builder
      auto makeList = [&](const std::vector<TodoItem>&v, bool focused) -> Element {
        if (v.empty())
          return vbox({separatorEmpty(),
                       text("  (empty — add with Enter)")|dim|center,
                       separatorEmpty()});
        Elements rows;
        for (int i=0;i<(int)v.size();i++) {
          auto &t=v[i]; bool sel=focused&&(i==selected);
          auto num  = text(" "+std::to_string(i+1)+". ")|color(Color::GrayLight);
          auto icon = t.is_done ? text(" v ")|color(Color::RGB(57,211,83))
                                 : text(" o ")|color(Color::Yellow);
          auto lbl  = t.is_done ? text(t.name)|strikethrough|color(Color::GrayDark)|flex
                                 : text(t.name)|flex;
          Element tag_el = text("");
          if (!t.tag.empty()) {
            Color tc = Color::RGB(100,100,200);
            if (t.tag=="URGENT") tc=Color::RGB(220,60,60);
            else if (t.tag=="RUSH") tc=Color::RGB(220,120,60);
            else if (t.tag=="DAILY") tc=Color::RGB(60,140,200);
            tag_el = text(" "+t.tag+" ")|bgcolor(tc)|color(Color::White);
          }
          auto row = hbox({num,icon,lbl,filler(),tag_el});
          if (sel) row = row|bgcolor(Color::RGB(38,48,75))|bold;
          rows.push_back(row|size(HEIGHT,EQUAL,1));
        }
        return vbox(rows);
      };

      bool tf = (focus==AppFocus::TASKS), hf = (focus==AppFocus::HABITS);
      auto task_box = makeList(tasks,tf)|frame|size(HEIGHT,GREATER_THAN,3)|
                      borderStyled(ROUNDED,tf?Color::RGB(120,180,255):Color::RGB(48,60,100));
      auto habit_box= makeList(habits,hf)|frame|size(HEIGHT,GREATER_THAN,3)|
                      borderStyled(ROUNDED,hf?Color::RGB(200,120,255):Color::RGB(48,60,100));

      // ── hint bar
      Element hint;
      if (focus==AppFocus::INPUT)
        hint=hbox({text(" INPUT ")|bgcolor(Color::RGB(50,80,180))|bold,
                   text("  Enter:add  Tab:tasks  Ctrl+L:habits  Q:quit  (+H prefix for habits, #TAG suffix)")|dim});
      else if (focus==AppFocus::TASKS)
        hint=hbox({text(" TASKS ")|bgcolor(Color::RGB(30,100,50))|bold,
                   text("  jk/arrows:move  Space:toggle  D:delete  Esc:input  Ctrl+L:habits  Q:quit")|dim});
      else
        hint=hbox({text(" HABITS ")|bgcolor(Color::RGB(100,40,140))|bold,
                   text("  jk/arrows:move  Space:toggle  D:delete  Esc/Tab:tasks  Q:quit")|dim});

      // ── progress bar helper
      auto progBar = [](float p, const std::string &label, Color c_) {
        return hbox({
          gauge(p)|color(c_)|flex,
          text(" "+label)|color(Color::GrayLight),
        });
      };
      Color tc_ = tp<0.3f?Color::RGB(200,60,60):tp<0.7f?Color::Yellow:Color::RGB(57,211,83);
      Color hc_ = hp<0.3f?Color::RGB(200,60,60):hp<0.7f?Color::Yellow:Color::RGB(57,211,83);

      return vbox({
        // header
        hbox({
          text(" TODO CLI ")|bold|color(Color::RGB(120,180,255)),
          text(" v2.1 ")|dim,
          filler(),
          text(" "+today+" ")|color(Color::GrayLight),
        }),
        separatorEmpty(),

        // heatmap + cube side by side
        hbox({
          renderHeatmap(tasks_data, today)|flex,
          separator(),
          vbox({
            text(" 3D Cube ")|bold|center|color(Color::RGB(80,180,255)),
            renderCube(t),
          })|size(WIDTH,EQUAL,24),
        }),

        separatorEmpty(),

        // tasks
        hbox({text(" TASKS  ")|color(Color::RGB(57,211,83)), progBar(tp,ts,tc_)}),
        task_box,
        separatorEmpty(),

        // habits
        hbox({text(" HABITS ")|color(Color::RGB(180,100,255)), progBar(hp,hs,hc_)}),
        habit_box,
        separatorEmpty(),

        // input
        input_row,
        separatorEmpty(),
        hint,
      })|borderStyled(DOUBLE,Color::RGB(48,60,120))|size(WIDTH,GREATER_THAN,70)|center;
    }),

    [&](Event e) -> bool {
      // ── INPUT ──────────────────────────────────────────
      if (focus==AppFocus::INPUT) {
        if (e==Event::Return) {
          if (!new_task.empty()) {
            bool is_habit = new_task.size()>3 && new_task[0]=='+'&&new_task[1]=='H'&&new_task[2]==' ';
            if (is_habit) new_task=new_task.substr(3);
            std::string tag;
            size_t hp=new_task.rfind('#');
            if (hp!=std::string::npos&&hp>0) {
              tag=new_task.substr(hp+1);
              while(!tag.empty()&&std::isspace((unsigned char)tag.back())) tag.pop_back();
              new_task=new_task.substr(0,hp);
              while(!new_task.empty()&&std::isspace((unsigned char)new_task.back())) new_task.pop_back();
            }
            if (is_habit) { habits.push_back({new_task,false,tag}); selected=(int)habits.size()-1; focus=AppFocus::HABITS; }
            else           { tasks.push_back({new_task,false,tag});  selected=(int)tasks.size()-1;  focus=AppFocus::TASKS; }
            new_task.clear(); persist();
          }
          return true;
        }
        if (e==Event::Tab&&!tasks.empty()) { focus=AppFocus::TASKS; selected=0; return true; }
        if (e==Event::CtrlL&&!habits.empty()) { focus=AppFocus::HABITS; selected=0; return true; }
        if (e==Event::Character('q')) { persist(); screen.ExitLoopClosure()(); return true; }
        return raw_input->OnEvent(e);
      }

      // ── TASKS ──────────────────────────────────────────
      if (focus==AppFocus::TASKS) {
        if (e==Event::Escape||e==Event::Tab) { focus=AppFocus::INPUT; return true; }
        if (tasks.empty()) return true;
        if (e==Event::ArrowDown||e==Event::Character('j')) { selected=std::min((int)tasks.size()-1,selected+1); return true; }
        if (e==Event::ArrowUp  ||e==Event::Character('k')) { selected=std::max(0,selected-1); return true; }
        if (e==Event::Character(' ')) { tasks[selected].is_done=!tasks[selected].is_done; persist(); return true; }
        if (e==Event::Character('d')) { tasks.erase(tasks.begin()+selected); selected=std::min(selected,(int)tasks.size()-1); if(selected<0)selected=0; persist(); return true; }
        if (e==Event::CtrlL&&!habits.empty()) { focus=AppFocus::HABITS; selected=0; return true; }
        if (e==Event::Character('q')) { persist(); screen.ExitLoopClosure()(); return true; }
        return true;
      }

      // ── HABITS ─────────────────────────────────────────
      if (focus==AppFocus::HABITS) {
        if (e==Event::Escape||e==Event::Tab) { focus=AppFocus::TASKS; selected=0; return true; }
        if (habits.empty()) return true;
        if (e==Event::ArrowDown||e==Event::Character('j')) { selected=std::min((int)habits.size()-1,selected+1); return true; }
        if (e==Event::ArrowUp  ||e==Event::Character('k')) { selected=std::max(0,selected-1); return true; }
        if (e==Event::Character(' ')) { habits[selected].is_done=!habits[selected].is_done; persist(); return true; }
        if (e==Event::Character('d')) { habits.erase(habits.begin()+selected); selected=std::min(selected,(int)habits.size()-1); if(selected<0)selected=0; persist(); return true; }
        if (e==Event::Character('q')) { persist(); screen.ExitLoopClosure()(); return true; }
        return true;
      }

      return false;
    }
  );

  screen.Loop(root);
  return 0;
}