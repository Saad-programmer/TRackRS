// SPDX-License-Identifier: GPL-2.0-or-later
#include "validation.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#ifdef __linux__
#include <sys/resource.h>
#endif
namespace validation {
Run run;
static unsigned number(const std::string& text) {
    std::size_t end = 0;
    const auto n = std::stoul(text, &end);
    if (end != text.size() || text.empty() || text[0] == '-' || n > 1000000)
        throw std::runtime_error("invalid unsigned option: " + text);
    return static_cast<unsigned>(n);
}
Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        const auto value = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error("missing value for " + key);
            return argv[i];
        };
        if (key == "--help") o.help = true;
        else if (key == "--headless") o.headless = o.enabled = true;
        else if (key == "--benchmark") o.benchmark = o.enabled = true;
        else if (key == "--stage") { o.stage = value(); o.enabled = true; }
        else if (key == "--renderer") { o.renderer=value(); if(o.renderer!="legacy"&&o.renderer!="pbr")throw std::runtime_error("renderer must be legacy or pbr"); }
        else if (key == "--quality") { o.quality=value(); if(o.quality!="low"&&o.quality!="medium"&&o.quality!="high"&&o.quality!="ultra")throw std::runtime_error("invalid render quality"); }
        else if (key == "--time-of-day") {o.timeOfDay=value();if(o.timeOfDay!="dawn"&&o.timeOfDay!="noon"&&o.timeOfDay!="golden-hour"&&o.timeOfDay!="dusk"&&o.timeOfDay!="night"&&o.timeOfDay!="midnight")throw std::runtime_error("invalid time of day");}
        else if (key == "--shadows") {o.shadows=value();if(o.shadows!="blob"&&o.shadows!="csm")throw std::runtime_error("shadows must be blob or csm");}
        else if (key == "--vegetation") {o.vegetation=value();if(o.vegetation!="3d"&&o.vegetation!="cards")throw std::runtime_error("vegetation must be 3d or cards");}
        else if (key == "--weather") {o.weather=value();if(o.weather!="level"&&o.weather!="clear"&&o.weather!="rain"&&o.weather!="snow"&&o.weather!="fog")throw std::runtime_error("weather must be level, clear, rain, snow or fog");}
        else if (key == "--vehicle") o.vehicle = value();
        else if (key == "--data-dir") o.data = value();
        else if (key == "--width") o.width=number(value());
        else if (key == "--height") o.height=number(value());
        else if (key == "--frames") o.frames = number(value());
        else if (key == "--seed") o.seed = number(value());
        else if (key == "--replay") o.replay = value();
        else if (key == "--dump-state") o.dump = value();
        else if (key == "--json") o.json = value();
        else if (key == "--screenshot") o.screenshot = value();
        else throw std::runtime_error("unknown option: " + key);
    }
    if(o.width<320||o.width>3840||o.height<200||o.height>2160)throw std::runtime_error("viewport outside320..3840 by200..2160");
    if (!o.frames) throw std::runtime_error("--frames must be positive");
    if (!o.enabled && (!o.replay.empty() || !o.dump.empty() || !o.json.empty() || !o.screenshot.empty()))
        throw std::runtime_error("validation output/replay needs --headless, --stage or --benchmark");
    if (o.stage.find("..") != std::string::npos || o.stage.find('\\') != std::string::npos)
        throw std::runtime_error("stage must be a virtual content path");
    if (o.stage.find(".level") == std::string::npos) o.stage = "/maps/" + o.stage + "/" + o.stage + ".level";
    return o;
}
std::vector<Input> readReplay(const std::string& path) {
    if (path.empty()) return {{0, 1.f, 0.f, 0.f, 0.f}};
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open replay: " + path);
    std::vector<Input> inputs;
    std::string line;
    unsigned lineno = 0;
    while (std::getline(file, line)) {
        ++lineno;
        if (line.empty() || line[0] == '#') continue;
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream row(line);
        Input v{};
        std::string extra;
        if (!(row >> v.tick >> v.throttle >> v.steering >> v.brake >> v.handbrake) ||
            (row >> extra) || (!inputs.empty() && v.tick <= inputs.back().tick) ||
            (inputs.empty() && v.tick != 0) || v.tick > 1000000 ||
            !std::isfinite(v.throttle) || !std::isfinite(v.steering) ||
            !std::isfinite(v.brake) || !std::isfinite(v.handbrake) ||
            std::abs(v.throttle) > 1 || std::abs(v.steering) > 1 ||
            v.brake < 0 || v.brake > 1 || v.handbrake < 0 || v.handbrake > 1)
            throw std::runtime_error("invalid replay row " + std::to_string(lineno));
        inputs.push_back(v);
    }
    if (inputs.empty()) throw std::runtime_error("empty replay: " + path);
    return inputs;
}
Input inputAt(const std::vector<Input>& inputs, unsigned tick) {
    if (inputs.empty()) throw std::runtime_error("replay not initialized");
    auto it = std::upper_bound(inputs.begin(), inputs.end(), tick,
        [](unsigned t, const Input& v) { return t < v.tick; });
    return it == inputs.begin() ? inputs.front() : *--it;
}
void hashFloat(std::uint64_t& hash, float value) {
    if (!std::isfinite(value)) throw std::runtime_error("non-finite simulation state");
    std::uint32_t bits;
    static_assert(sizeof(bits) == sizeof(value), "32-bit floats required");
    std::memcpy(&bits, &value, sizeof(bits));
    for (unsigned shift = 0; shift < 32; shift += 8) {
        hash ^= (bits >> shift) & 255;
        hash *= 1099511628211ULL;
    }
}
double percentile(std::vector<double> values, double fraction) {
    if (values.empty() || fraction < 0 || fraction > 1) throw std::runtime_error("invalid percentile");
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>(std::ceil(fraction * static_cast<double>(values.size())));
    return values[index ? index - 1 : 0];
}
std::string quote(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(c);
        else out << c;
    }
    return out.str() + '"';
}
void writeResults() {
    std::ostringstream hash;
    hash << std::hex << std::setw(16) << std::setfill('0') << run.hash;
    const std::string state = hash.str();
    std::cout << "STATE_HASH " << state << '\n';
    const auto save = [](const std::string& path, const std::string& text) {
        std::ofstream out(path);
        out << text;
        out.close();
        if (!out) throw std::runtime_error("cannot write result: " + path);
    };
    if (!run.options.dump.empty()) save(run.options.dump, state + "\n");
    if (run.options.json.empty()) return;
    if (run.frames.empty()) throw std::runtime_error("no benchmark samples");
    const auto mean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
    };
    std::ostringstream out;
    out << std::setprecision(10) << "{\n"
        << "  \"schema\": \"trackrs.legacy-benchmark/1\",\n"
        << "  \"stage\": " << quote(run.options.stage) << ",\n"
        << "  \"vehicle\": " << quote(run.options.vehicle) << ",\n"
        << "  \"renderer\": " << quote(run.renderer) << ",\n"
        << "  \"seed\": " << run.options.seed << ",\n"
        << "  \"ticks\": " << run.ticks << ",\n"
        << "  \"step_seconds\": 0.004,\n"
        << "  \"state_hash\": " << quote(state) << ",\n"
        << "  \"samples\": " << run.frames.size() << ",\n"
        << "  \"frame_ms_avg\": " << mean(run.frames) << ",\n"
        << "  \"frame_ms_p99\": " << percentile(run.frames, .99) << ",\n"
        << "  \"frame_ms_p999\": " << percentile(run.frames, .999) << ",\n"
        << "  \"cpu_update_ms_avg\": " << mean(run.cpu) << ",\n"
        << "  \"gpu_frame_ms_avg\": ";
    if (run.gpu.empty()) out << "null"; else out << mean(run.gpu);
    out << ",\n  \"load_ms\": " << run.loadMs << ",\n  \"passes\": {";
    bool first = true;
    for (const auto& entry : run.passes) {
        const auto& pass = entry.second;
        if (!first) out << ',';
        first = false;
        out << "\n    " << quote(entry.first) << ": { \"calls\": " << pass.cpu.size()
            << ", \"cpu_ms_per_frame\": " << std::accumulate(pass.cpu.begin(), pass.cpu.end(), 0.0) /
                static_cast<double>(run.frames.size()) << ", \"gpu_ms_per_frame\": ";
        if (pass.gpu.empty()) out << "null";
        else out << std::accumulate(pass.gpu.begin(), pass.gpu.end(), 0.0) / static_cast<double>(run.frames.size());
        out << " }";
    }
    out << "\n  },\n  \"draw_calls\": " << std::accumulate(run.draws.begin(), run.draws.end(), 0.0) /
        static_cast<double>(run.frames.size())
        << ",\n  \"triangles\": " << std::accumulate(run.triangles.begin(), run.triangles.end(), 0.0) /
        static_cast<double>(run.frames.size())
        << ",\n  \"vram_bytes\": null,\n  \"ram_peak_bytes\": ";
#ifdef __linux__
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) out << static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
    else out << "null";
#else
    out << "null";
#endif
    out << ",\n  \"measurement_notes\": \"Whole-frame and per-scope GPU queries perturb timings; "
        << "scope queries are collected after the core batch fence. Loading frame excluded; no warmup exclusion. "
        << "Draw calls and triangles are mean submitted primitives, including degenerates. "
        << "VRAM is unavailable without a portable driver memory query; no estimate is fabricated.\"\n}\n";
    save(run.options.json, out.str());
}
}
