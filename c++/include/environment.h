#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <unordered_map>
#include <vector>

class Environment {
private:
  // Cada rib representa un scope; se busca desde el scope mas interno hacia afuera.
  std::vector<std::unordered_map<std::string, std::string>> ribs;

  int search_rib(const std::string &name) const {
    for (int idx = (int)ribs.size() - 1; idx >= 0; --idx)
      if (ribs[idx].count(name)) return idx;
    return -1;
  }

public:
  void add_level() { ribs.emplace_back(); }
  bool remove_level() {
    if (ribs.empty()) return false;
    ribs.pop_back();
    return true;
  }
  void clear() { ribs.clear(); }

  void add_var(const std::string &name, const std::string &value) {
    if (!ribs.empty()) ribs.back()[name] = value;
  }

  bool exists_in_current(const std::string &name) const {
    return !ribs.empty() && ribs.back().count(name) > 0;
  }

  bool check(const std::string &x) const { return search_rib(x) >= 0; }

  bool lookup(const std::string &x, std::string &out) const {
    int idx = search_rib(x);
    if (idx < 0) return false;
    out = ribs[idx].at(x);
    return true;
  }

  bool update(const std::string &x, const std::string &v) {
    int idx = search_rib(x);
    if (idx < 0) return false;
    ribs[idx][x] = v;
    return true;
  }
};

#endif
