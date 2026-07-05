#include "peephole.h"
#include <sstream>
#include <vector>

namespace {
std::string trim(const std::string &s) {
  size_t a = s.find_first_not_of(" \t\r");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r");
  return s.substr(a, b - a + 1);
}
bool isLabel(const std::string &t) { return !t.empty() && t.back() == ':'; }
bool isDirective(const std::string &t) { return !t.empty() && t[0] == '.'; }
bool startsWith(const std::string &t, const std::string &p) { return t.rfind(p, 0) == 0; }
}

std::string peephole(const std::string &asmText, int *removed) {
  std::vector<std::string> lines;
  {
    std::stringstream ss(asmText);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
  }
  int rem = 0;

  // Reglas: código inalcanzable, mov redundante y salto a la siguiente etiqueta.
  std::vector<std::string> a;
  bool skipping = false;
  for (auto &ln : lines) {
    std::string t = trim(ln);
    if (skipping) {
      if (isLabel(t) || isDirective(t)) {
        skipping = false;
        a.push_back(ln);
      } else {
        rem++;
      }
      continue;
    }
    a.push_back(ln);
    if (startsWith(t, "jmp ") || t == "ret") skipping = true;
  }

  std::vector<std::string> b;
  for (auto &ln : a) {
    std::string t = trim(ln);
    bool redundant = false;
    if (startsWith(t, "movq ")) {
      std::string rest = trim(t.substr(5));
      size_t comma = rest.find(',');
      if (comma != std::string::npos) {
        std::string src = trim(rest.substr(0, comma));
        std::string dst = trim(rest.substr(comma + 1));
        if (src == dst && !src.empty() && src[0] == '%') redundant = true;
      }
    }
    if (redundant) rem++;
    else b.push_back(ln);
  }

  std::vector<std::string> c;
  for (size_t i = 0; i < b.size(); i++) {
    std::string t = trim(b[i]);
    if (startsWith(t, "jmp ")) {
      std::string target = trim(t.substr(4));
      size_t j = i + 1;
      while (j < b.size() && trim(b[j]).empty()) j++;
      if (j < b.size() && trim(b[j]) == target + ":") {
        rem++;
        continue;
      }
    }
    c.push_back(b[i]);
  }

  if (removed) *removed = rem;
  std::string out;
  for (auto &ln : c) {
    out += ln;
    out += "\n";
  }
  return out;
}
