#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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
}

int main() {
  json::DayMap test_data;
  
  json::DayRecord rec1;
  rec1.done = 2;
  rec1.total = 3;
  rec1.tasks = {"Build todo cli", "Improve UI", "Test save"};
  rec1.task_done = {true, true, false};
  rec1.task_tags = {"", "URGENT", ""};
  test_data["2026-02-23"] = rec1;
  
  json::save("test_output.json", test_data);
  
  std::ifstream f("test_output.json");
  std::string line;
  std::cout << "Saved content:\n";
  while (std::getline(f, line)) {
    std::cout << line << "\n";
  }
  
  return 0;
}
