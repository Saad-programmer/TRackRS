// SPDX-License-Identifier: GPL-2.0-or-later
#include "car_definition.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <tinyxml2.h>
namespace trackrs::content {
namespace {
constexpr float Pi = 3.14159265358979f;
float omega(float rpm) { return rpm * Pi / 30.f; }
using nlohmann::json;
float number(const json &j, const char *key, float fallback = 0) {
  return j.contains(key) && j[key].is_number() ? j[key].get<float>() : fallback;
}
std::string text(const json &j, const char *key) {
  return j.contains(key) && j[key].is_string() ? j[key].get<std::string>() : std::string();
}
float attribute(const tinyxml2::XMLElement *e, const char *name, float fallback = 0) {
  const char *v = e ? e->Attribute(name) : nullptr;
  return v ? std::strtof(v, nullptr) : fallback;
}
// "x, y, z" -> y (legacy wheel positions: +y is forward)
float positionY(const tinyxml2::XMLElement *wheel) {
  const char *pos = wheel->Attribute("pos");
  if (!pos)
    return 0;
  float x = 0, y = 0, z = 0;
  return std::sscanf(pos, "%f , %f , %f", &x, &y, &z) >= 2 ? y : 0;
}
std::string format(float v) {
  char buffer[32];
  std::snprintf(buffer, sizeof buffer, "%.6g", static_cast<double>(v));
  return buffer;
}
} // namespace

CarDefinition parseCar(std::string_view source) {
  const json j = json::parse(source);
  CarDefinition c;
  c.schema = text(j, "schema");
  c.id = text(j, "id");
  c.displayName = text(j, "display_name");
  c.marque = text(j, "marque");
  c.carClass = text(j, "class");
  c.era = text(j, "era");
  c.year = j.value("year", 0);
  if (j.contains("license")) {
    const auto &l = j["license"];
    c.license.data = text(l, "data");
    c.license.model = text(l, "model");
    c.license.author = text(l, "author");
    c.license.sourceUrl = text(l, "source_url");
    c.license.trademarkNotice = text(l, "trademark_notice");
  }
  if (j.contains("verified"))
    for (const auto &[k, v] : j["verified"].items())
      c.verified[k] = v.is_boolean() && v.get<bool>();
  if (j.contains("sources"))
    for (const auto &s : j["sources"])
      if (s.is_string())
        c.sources.push_back(s.get<std::string>());
  c.provenance = text(j, "provenance");
  if (j.contains("body")) {
    const auto &b = j["body"];
    c.body.mesh = text(b, "mesh");
    c.body.fallback = text(b, "fallback");
    c.body.importScale = number(b, "import_scale", 1);
  }
  if (j.contains("mass")) {
    c.massKg = number(j["mass"], "kerb_kg");
    c.frontWeight = number(j["mass"], "front_weight_dist", .5f);
  }
  if (j.contains("engine")) {
    const auto &e = j["engine"];
    c.engineLayout = text(e, "layout");
    c.idleRpm = number(e, "idle_rpm");
    c.redlineRpm = number(e, "redline_rpm");
    if (e.contains("torque_curve_nm"))
      for (const auto &p : e["torque_curve_nm"])
        if (p.is_array() && p.size() == 2)
          c.torque.push_back({p[0].get<float>(), p[1].get<float>()});
  }
  if (j.contains("drivetrain")) {
    const auto &d = j["drivetrain"];
    c.driveLayout = text(d, "layout");
    c.frontTorqueShare = number(d, "front_torque_share", c.driveLayout == "FWD" ? 1.f : 0.f);
    if (d.contains("gearbox")) {
      const auto &g = d["gearbox"];
      if (g.contains("ratios"))
        for (const auto &r : g["ratios"])
          c.gearRatios.push_back(r.get<float>());
      c.finalDrive = number(g, "final_drive", 1);
      c.reverseRatio = number(g, "reverse");
      c.shiftTime = number(g, "shift_time_s", .2f);
      c.gearboxDerivation = text(g, "derivation");
    }
  }
  if (j.contains("tyres")) {
    c.tyreRadius = number(j["tyres"], "radius_m");
    c.tyreWidth = number(j["tyres"], "width_m");
  }
  if (j.contains("aero")) {
    const auto &a = j["aero"];
    c.cd = number(a, "cd");
    c.frontalArea = number(a, "frontal_area_m2");
    c.clFront = number(a, "cl_front");
    c.clRear = number(a, "cl_rear");
  }
  if (j.contains("brakes"))
    c.brakeBiasFront = number(j["brakes"], "bias_front", .6f);
  if (j.contains("claims")) {
    c.claimedTopSpeedKmh = number(j["claims"], "top_speed_kmh");
    c.claimedZeroTo100 = number(j["claims"], "zero_to_100_s");
  }
  return c;
}

std::string writeCar(const CarDefinition &c) {
  json j;
  j["schema"] = c.schema;
  j["id"] = c.id;
  j["display_name"] = c.displayName;
  j["marque"] = c.marque;
  j["year"] = c.year;
  j["class"] = c.carClass;
  j["era"] = c.era;
  j["license"] = {{"data", c.license.data},
                  {"model", c.license.model},
                  {"author", c.license.author},
                  {"source_url", c.license.sourceUrl},
                  {"trademark_notice", c.license.trademarkNotice}};
  j["verified"] = c.verified;
  j["sources"] = c.sources;
  j["provenance"] = c.provenance;
  j["body"] = {{"mesh", c.body.mesh}, {"fallback", c.body.fallback}, {"import_scale", c.body.importScale}, {"unit", "m"}};
  j["mass"] = {{"kerb_kg", c.massKg}, {"front_weight_dist", c.frontWeight}};
  json curve = json::array();
  for (const auto &p : c.torque)
    curve.push_back({p.rpm, p.nm});
  j["engine"] = {{"layout", c.engineLayout}, {"idle_rpm", c.idleRpm}, {"redline_rpm", c.redlineRpm}, {"torque_curve_nm", curve}};
  j["drivetrain"] = {{"layout", c.driveLayout},
                     {"front_torque_share", c.frontTorqueShare},
                     {"gearbox",
                      {{"ratios", c.gearRatios},
                       {"final_drive", c.finalDrive},
                       {"reverse", c.reverseRatio},
                       {"shift_time_s", c.shiftTime},
                       {"derivation", c.gearboxDerivation}}}};
  j["tyres"] = {{"radius_m", c.tyreRadius}, {"width_m", c.tyreWidth}};
  j["aero"] = {{"cd", c.cd}, {"frontal_area_m2", c.frontalArea}, {"cl_front", c.clFront}, {"cl_rear", c.clRear}};
  j["brakes"] = {{"bias_front", c.brakeBiasFront}};
  j["claims"] = {{"top_speed_kmh", c.claimedTopSpeedKmh}, {"zero_to_100_s", c.claimedZeroTo100}};
  return j.dump(2) + "\n";
}

bool isPlaceholder(const CarDefinition &c) { return c.body.mesh.empty(); }
std::string menuName(const CarDefinition &c) {
  return isPlaceholder(c) ? c.displayName + " (placeholder model)" : c.displayName;
}

Validation validate(const CarDefinition &c) {
  Validation v;
  auto error = [&](const std::string &m) { v.errors.push_back(c.id + ": " + m); };
  auto range = [&](const char *name, float value, float lo, float hi) {
    if (!(value >= lo && value <= hi))
      error(std::string(name) + " out of range [" + format(lo) + ", " + format(hi) + "]: " + format(value));
  };
  if (c.schema != "trackrs.car/2")
    error("schema must be trackrs.car/2");
  if (c.id.empty() || !std::all_of(c.id.begin(), c.id.end(), [](char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
      }))
    error("id must be lower-case [a-z0-9_]");
  if (c.displayName.empty())
    error("display_name missing");
  if (c.license.data.empty())
    error("license.data (SPDX) missing");
  if (!c.legacy && c.license.trademarkNotice.empty() && !c.marque.empty())
    error("real marque without license.trademark_notice");
  if (c.body.mesh.empty() && c.body.fallback.empty())
    error("no own mesh and no fictional fallback body");
  if (!c.body.mesh.empty() && c.license.model.empty())
    error("mesh without license.model");
  // Honesty: nothing may claim to be verified without a source.
  const bool anyVerified = std::any_of(c.verified.begin(), c.verified.end(), [](const auto &kv) { return kv.second; });
  if (anyVerified && c.sources.empty())
    error("values marked verified but no sources listed");
  for (const auto &s : c.sources)
    if (s.rfind("https://", 0) != 0)
      error("source must be an https:// URL: " + s);
  if (!anyVerified && c.provenance.empty())
    error("unverified values need a provenance note");
  for (const char *key : {"mass", "engine", "gearbox", "tyres", "aero", "brakes"})
    if (!c.verified.count(key))
      error(std::string("verified.") + key + " missing");
  range("mass.kerb_kg", c.massKg, 400, 2500);
  range("mass.front_weight_dist", c.frontWeight, .2f, .8f);
  range("engine.idle_rpm", c.idleRpm, 300, 3000);
  range("engine.redline_rpm", c.redlineRpm, 3000, 16000);
  if (c.idleRpm >= c.redlineRpm)
    error("idle_rpm must be below redline_rpm");
  if (c.torque.size() < 3)
    error("torque_curve_nm needs at least 3 points");
  for (std::size_t i = 0; i < c.torque.size(); ++i) {
    if (!(c.torque[i].nm > 0 && c.torque[i].nm <= 1500))
      error("torque point " + std::to_string(i) + " N·m out of (0, 1500]");
    if (i && c.torque[i].rpm <= c.torque[i - 1].rpm)
      error("torque curve rpm must increase strictly");
  }
  if (!c.torque.empty() && (c.torque.front().rpm < .5f * c.idleRpm || c.torque.back().rpm > 1.1f * c.redlineRpm))
    error("torque curve outside [idle/2, 1.1 x redline]");
  if (c.driveLayout != "FWD" && c.driveLayout != "RWD" && c.driveLayout != "AWD")
    error("drivetrain.layout must be FWD, RWD or AWD");
  range("drivetrain.front_torque_share", c.frontTorqueShare, 0, 1);
  if (c.driveLayout == "FWD" && c.frontTorqueShare != 1)
    error("FWD needs front_torque_share 1");
  if (c.driveLayout == "RWD" && c.frontTorqueShare != 0)
    error("RWD needs front_torque_share 0");
  if (c.driveLayout == "AWD" && !(c.frontTorqueShare > 0 && c.frontTorqueShare < 1))
    error("AWD needs 0 < front_torque_share < 1");
  if (c.gearRatios.size() < 3 || c.gearRatios.size() > 8)
    error("gearbox needs 3..8 forward ratios");
  for (std::size_t i = 0; i < c.gearRatios.size(); ++i) {
    if (!(c.gearRatios[i] > 0))
      error("gear ratio must be positive");
    if (i && c.gearRatios[i] >= c.gearRatios[i - 1])
      error("gear ratios must decrease strictly (1st first)");
  }
  range("gearbox.final_drive", c.finalDrive, c.legacy ? .05f : 2.f, c.legacy ? 100.f : 7.f);
  if (c.gearboxDerivation.empty())
    error("gearbox.derivation missing (sourced, legacy or synthetic)");
  range("tyres.radius_m", c.tyreRadius, .2f, .45f);
  range("tyres.width_m", c.tyreWidth, .12f, .4f);
  range("aero.cd", c.cd, .2f, .8f);
  range("aero.frontal_area_m2", c.frontalArea, 1.2f, 3.f);
  range("brakes.bias_front", c.brakeBiasFront, c.legacy ? .1f : .3f, .85f); // legacy Fox cars are rear-biased
  if (v.errors.empty()) {
    const auto power = peakPowerKw(c).value;
    if (power < 30 || power > 700)
      v.warnings.push_back(c.id + ": implausible peak power " + format(power) + " kW");
    if (c.claimedTopSpeedKmh > 0) {
      const float topGear = speedKmh(c, static_cast<unsigned>(c.gearRatios.size() - 1), c.redlineRpm);
      if (topGear < .8f * c.claimedTopSpeedKmh)
        v.warnings.push_back(c.id + ": top gear at redline (" + format(topGear) + " km/h) below claimed top speed");
    }
  }
  return v;
}

float torqueAt(const CarDefinition &c, float rpm) {
  if (c.torque.empty() || rpm < c.torque.front().rpm || rpm > c.redlineRpm)
    return 0;
  for (std::size_t i = 1; i < c.torque.size(); ++i)
    if (rpm <= c.torque[i].rpm) {
      const auto &a = c.torque[i - 1], &b = c.torque[i];
      return a.nm + (b.nm - a.nm) * (rpm - a.rpm) / (b.rpm - a.rpm);
    }
  return c.torque.back().nm; // held from the last point to the redline
}
Peak peakTorque(const CarDefinition &c) {
  Peak p;
  for (const auto &t : c.torque)
    if (t.nm > p.value)
      p = {t.nm, t.rpm};
  return p;
}
Peak peakPowerKw(const CarDefinition &c) {
  Peak p;
  if (c.torque.empty())
    return p;
  const int steps = static_cast<int>((c.redlineRpm - c.torque.front().rpm) / 10.f);
  for (int i = 0; i <= steps; ++i) {
    const float rpm = c.torque.front().rpm + 10.f * static_cast<float>(i);
    const float kw = torqueAt(c, rpm) * omega(rpm) / 1000.f;
    if (kw > p.value)
      p = {kw, rpm};
  }
  return p;
}
float speedKmh(const CarDefinition &c, unsigned gear, float rpm) {
  if (gear >= c.gearRatios.size())
    return 0;
  return omega(rpm) / (c.gearRatios[gear] * c.finalDrive) * c.tyreRadius * 3.6f;
}

// xml text, car id and file name for diagnostics; documented in the header.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
CarDefinition fromLegacyVehicle(std::string_view xml, std::string_view id, std::string_view file) {
  tinyxml2::XMLDocument doc;
  if (doc.Parse(xml.data(), xml.size()) != tinyxml2::XML_SUCCESS)
    throw std::runtime_error("legacy vehicle XML parse error: " + std::string(file));
  const auto *root = doc.FirstChildElement("vehicle");
  if (!root)
    throw std::runtime_error("legacy vehicle has no <vehicle> root: " + std::string(file));
  CarDefinition c;
  c.legacy = true;
  c.legacyFile = std::string(file);
  c.id = std::string(id);
  c.displayName = root->Attribute("name") ? root->Attribute("name") : c.id;
  c.carClass = root->Attribute("class") ? root->Attribute("class") : "";
  c.license.data = "GPL-2.0-or-later";
  c.license.model = "LicenseRef-DATA_AUTHORS"; // per-file credits in doc/DATA_AUTHORS.txt
  c.license.author = "TrackRS / Trigger Rally contributors";
  c.provenance = "legacy adapter: converted from the original fictional .vehicle (power via LegacyPowerPerWatt)";
  for (const char *key : {"mass", "engine", "gearbox", "tyres", "aero", "brakes"})
    c.verified[key] = false;
  const auto *gen = root->FirstChildElement("genparams");
  c.massKg = attribute(gen, "mass", 1000);
  const auto *drive = root->FirstChildElement("drivesystem");
  const auto *engine = drive ? drive->FirstChildElement("engine") : nullptr;
  const float scale = attribute(engine, "powerscale", 1);
  for (auto *p = engine ? engine->FirstChildElement("powerpoint") : nullptr; p; p = p->NextSiblingElement("powerpoint")) {
    const float rpm = attribute(p, "rpm"), power = attribute(p, "power") * scale / LegacyPowerPerWatt;
    if (rpm <= 0)
      continue;
    if (power <= 0) // the zero-power point beyond the last one is the legacy limiter
      continue;
    c.torque.push_back({rpm, power / omega(rpm)});
  }
  if (!c.torque.empty()) {
    c.idleRpm = std::min(1000.f, c.torque.front().rpm);
    c.redlineRpm = c.torque.back().rpm; // power falls to zero beyond: the limiter
  }
  c.engineLayout = "legacy fictional engine";
  float absolute = 0;
  for (auto *g = drive ? drive->FirstChildElement("gearbox") : nullptr; g; g = nullptr)
    for (auto *gear = g->FirstChildElement("gear"); gear; gear = gear->NextSiblingElement("gear")) {
      if (gear->Attribute("absolute"))
        absolute = attribute(gear, "absolute");
      else
        absolute *= attribute(gear, "relative", 1);
      if (absolute > 0)
        c.gearRatios.push_back(1.f / absolute); // overall ratio; final drive folded in
    }
  c.finalDrive = 1;
  c.gearboxDerivation = "legacy";
  float front = 0, rear = 0, brakeFront = 0, brakeRear = 0;
  const auto *body = root->FirstChildElement("part");
  for (auto *w = body ? body->FirstChildElement("wheel") : nullptr; w; w = w->NextSiblingElement("wheel")) {
    const bool isFront = positionY(w) > 0;
    (isFront ? front : rear) += attribute(w, "drive");
    (isFront ? brakeFront : brakeRear) += attribute(w, "brake1");
    if (c.tyreRadius == 0)
      c.tyreRadius = attribute(w, "radius", .3f);
  }
  c.frontTorqueShare = front + rear > 0 ? front / (front + rear) : 0;
  c.driveLayout = rear == 0 ? "FWD" : front == 0 ? "RWD" : "AWD";
  c.brakeBiasFront = brakeFront + brakeRear > 0 ? brakeFront / (brakeFront + brakeRear) : .6f;
  c.tyreWidth = .2f; // not represented in the legacy format
  c.cd = .4f;        // legacy drag is a per-axis coefficient, not Cd·A
  c.frontalArea = 1.9f;
  c.body.mesh = body && body->Attribute("model") ? body->Attribute("model") : "";
  return c;
}

std::string toLegacyVehicle(const CarDefinition &c, std::string_view fallbackXml) {
  tinyxml2::XMLDocument doc;
  if (doc.Parse(fallbackXml.data(), fallbackXml.size()) != tinyxml2::XML_SUCCESS)
    throw std::runtime_error("fallback vehicle XML parse error for " + c.id);
  auto *root = doc.FirstChildElement("vehicle");
  if (!root)
    throw std::runtime_error("fallback vehicle has no <vehicle> root for " + c.id);
  root->SetAttribute("name", menuName(c).c_str());
  if (!c.carClass.empty())
    root->SetAttribute("class", c.carClass.c_str());
  auto *gen = root->FirstChildElement("genparams");
  if (!gen)
    gen = root->InsertNewChildElement("genparams");
  gen->SetAttribute("mass", format(c.massKg).c_str());
  auto *body = root->FirstChildElement("part");
  float oldRadius = 0;
  for (auto *w = body ? body->FirstChildElement("wheel") : nullptr; w && oldRadius == 0; w = w->NextSiblingElement("wheel"))
    oldRadius = attribute(w, "radius");
  if (gen->Attribute("wheelscale") && oldRadius > 0)
    gen->SetAttribute("wheelscale", format(attribute(gen, "wheelscale") * c.tyreRadius / oldRadius).c_str());
  auto *drive = root->FirstChildElement("drivesystem");
  if (!drive)
    drive = root->InsertNewChildElement("drivesystem");
  if (auto *old = drive->FirstChildElement("engine"))
    drive->DeleteChild(old);
  if (auto *old = drive->FirstChildElement("gearbox"))
    drive->DeleteChild(old);
  auto *engine = drive->InsertNewChildElement("engine");
  engine->SetAttribute("powerscale", "1");
  auto point = [&](float rpm, float watts) {
    auto *p = engine->InsertNewChildElement("powerpoint");
    p->SetAttribute("rpm", format(rpm).c_str());
    p->SetAttribute("power", format(watts * LegacyPowerPerWatt).c_str());
  };
  // Legacy physics interpolates *power* linearly between points, so sample
  // the torque curve densely (250 rpm) to reproduce it faithfully.
  const float first = c.torque.front().rpm;
  const int samples = static_cast<int>(std::ceil((c.redlineRpm - first) / 250.f));
  for (int i = 0; i < samples; ++i) {
    const float rpm = first + 250.f * static_cast<float>(i);
    point(rpm, torqueAt(c, rpm) * omega(rpm));
  }
  point(c.redlineRpm, torqueAt(c, c.redlineRpm) * omega(c.redlineRpm));
  point(c.redlineRpm * 1.04f, 0); // limiter: power falls to zero just above redline
  auto *gearbox = drive->InsertNewChildElement("gearbox");
  for (const float ratio : c.gearRatios)
    gearbox->InsertNewChildElement("gear")->SetAttribute("absolute", format(1.f / (ratio * c.finalDrive)).c_str());
  float brakeTotal = 0;
  for (auto *w = body ? body->FirstChildElement("wheel") : nullptr; w; w = w->NextSiblingElement("wheel"))
    brakeTotal += attribute(w, "brake1");
  for (auto *w = body ? body->FirstChildElement("wheel") : nullptr; w; w = w->NextSiblingElement("wheel")) {
    const bool isFront = positionY(w) > 0;
    const float share = isFront ? c.frontTorqueShare : 1.f - c.frontTorqueShare; // legacy: sum of shares = 2
    w->SetAttribute("drive", format(share).c_str());
    w->SetAttribute("radius", format(c.tyreRadius).c_str());
    if (brakeTotal > 0)
      w->SetAttribute("brake1", format(brakeTotal * (isFront ? c.brakeBiasFront : 1.f - c.brakeBiasFront) / 2.f).c_str());
  }
  tinyxml2::XMLPrinter printer;
  doc.Print(&printer);
  return printer.CStr();
}
} // namespace trackrs::content
