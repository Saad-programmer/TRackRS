// SPDX-License-Identifier: GPL-2.0-or-later
// trackrs-content: pack-lite content tool (M0).
//   trackrs-content validate <data-dir> [--edition demo|full|dev (default dev)] [--json out.json]
//       validates every pack under <data-dir>/packs; exit 1 if any pack is invalid.
//   trackrs-content credits <data-dir> [--output CREDITS.md] [--check]
//       renders the credits for every valid pack (--check: fail if out of date).
// Reads plain directories; the game reads the same layout through PhysFS.
#include "engine/content/pack.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace fs = std::filesystem;
using namespace trackrs::content;
namespace {
Source directorySource(const fs::path &root) {
  Source s;
  s.list = [root](const std::string &dir) {
    std::vector<std::string> out;
    std::error_code ec;
    for (const auto &e : fs::directory_iterator(root / dir.substr(1), ec))
      out.push_back(e.path().filename().string() + (e.is_directory() ? "/" : ""));
    return out;
  };
  s.read = [root](const std::string &path) -> std::optional<std::string> {
    std::ifstream f(root / path.substr(1), std::ios::binary);
    if (!f)
      return std::nullopt;
    std::ostringstream text;
    text << f.rdbuf();
    return text.str();
  };
  return s;
}
int usage() {
  std::cerr << "usage: trackrs-content validate <data-dir> [--edition demo|full|dev (default dev)] [--json out.json]\n"
               "       trackrs-content credits <data-dir> [--output CREDITS.md] [--check]\n";
  return 2;
}
} // namespace
int main(int argc, char **argv) {
  if (argc < 3)
    return usage();
  const std::string command = argv[1];
  const fs::path data = argv[2];
  DiscoverOptions options;
  options.edition = Edition::Dev; // validation lists everything, including test stages
  std::string json, output;
  bool check = false;
  for (int i = 3; i < argc; ++i) {
    const std::string a = argv[i];
    const bool more = i + 1 < argc;
    if (a == "--edition" && more) {
      const std::string e = argv[++i];
      options.edition = e == "demo" ? Edition::Demo : e == "dev" ? Edition::Dev : Edition::Full;
    } else if (a == "--json" && more) {
      json = argv[++i];
    } else if (a == "--output" && more) {
      output = argv[++i];
    } else if (a == "--check") {
      check = true;
    } else {
      return usage();
    }
  }
  if (!fs::is_directory(data)) {
    std::cerr << "not a directory: " << data << "\n";
    return 2;
  }
  const auto catalog = discover(directorySource(data), options);
  if (command == "validate") {
    bool ok = true;
    for (const auto &p : catalog.packs) {
      if (p.id == "classic-legacy-stages")
        continue;
      std::cout << (p.valid() ? "PASS " : "FAIL ") << p.id << " " << p.manifest.version.str() << ": "
                << p.stages.size() << " stage(s), " << p.championships.size() << " championship(s), "
                << p.files.size() << " file(s)\n";
      for (const auto &issue : p.issues)
        std::cout << "  " << (issue.severity == Issue::Severity::Error ? "error: " : "warning: ") << issue.message << "\n";
      ok = ok && p.valid();
    }
    if (!json.empty())
      std::ofstream(json) << summaryJson(catalog) << "\n";
    return ok ? 0 : 1;
  }
  if (command == "credits") {
    const auto text = credits(catalog);
    if (check) {
      std::ifstream f(output);
      std::ostringstream current;
      current << f.rdbuf();
      if (current.str() != text) {
        std::cerr << output << " is out of date: run trackrs-content credits <data> --output " << output << "\n";
        return 1;
      }
      std::cout << "PASS: " << output << " is up to date\n";
      return 0;
    }
    if (output.empty())
      std::cout << text;
    else
      std::ofstream(output) << text;
    return 0;
  }
  return usage();
}
