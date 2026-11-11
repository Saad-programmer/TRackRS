// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Opt-in legacy-engine validation driver. Owns replay and measurement data for
 * one process; main-thread only. Normal interactive runs never consult replay
 * data. Hashing is allocation-free; frame samples are reserved at startup.
 */
#include <cstdint>
#include <string>
#include <vector>
#include <map>
namespace validation {
struct Pass { std::vector<double> cpu, gpu; };
struct PendingQuery { Pass* pass; unsigned begin, end; };
struct Input { unsigned tick; float throttle, steering, brake, handbrake; };
struct Options {
    bool enabled = false, headless = false, benchmark = false, help = false;
    unsigned frames = 1500, seed = 1000, width = 640, height = 480;
    std::string stage = "/maps/ahead/ahead.level", vehicle = "cordo_kitcar";
    std::string replay, dump, json, screenshot, data, renderer, quality, timeOfDay, shadows, vegetation, weather, post, crowd, cars, telemetry;
};
struct Run {
    Options options;
    std::vector<Input> inputs;
    std::vector<double> frames, cpu, gpu;
    std::vector<std::uint64_t> draws, triangles;
    std::map<std::string, Pass> passes;
    std::vector<PendingQuery> queries;
    bool capture = false;
    unsigned primitive = 0, vertices = 0;
    std::uint64_t frameDraws = 0, frameTriangles = 0;
    unsigned ticks = 0;
    bool started = false, complete = false;
    std::uint64_t hash = 14695981039346656037ULL;
    double loadMs = 0;
    std::string renderer;
    int result = 0;
};
extern Run run;
Options parse(int argc, char** argv);
std::vector<Input> readReplay(const std::string& path);
Input inputAt(const std::vector<Input>& inputs, unsigned tick);
void hashFloat(std::uint64_t& hash, float value);
double percentile(std::vector<double> values, double fraction);
std::string quote(const std::string& value);
void writeResults();
}
