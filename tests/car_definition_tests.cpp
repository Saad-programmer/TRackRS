// SPDX-License-Identifier: GPL-2.0-or-later
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "engine/content/car_definition.h"
#include "vendor/doctest/doctest.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <tinyxml2.h>
using namespace trackrs::content;
namespace fs = std::filesystem;
namespace {
const fs::path Vehicles = fs::path(TRACKRS_DATA_DIR) / "vehicles";
std::string read(const fs::path &p) {
  std::ifstream in(p, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}
std::vector<fs::path> files(const char *name) {
  std::vector<fs::path> out;
  for (const auto &d : fs::directory_iterator(Vehicles))
    for (const auto &f : fs::directory_iterator(d.path()))
      if (f.path().filename() == name || (std::string(name) == "*.vehicle" && f.path().extension() == ".vehicle"))
        out.push_back(f.path());
  std::sort(out.begin(), out.end());
  return out;
}
const char *Fictional[] = {"cordo_kitcar", "cordo_super500", "cordo_wrc", "evo_kitcar", "evo_super500",
                           "evo_wrc",      "fox_kitcar",     "fox_super500", "fox_wrc"};
} // namespace

TEST_CASE("the eleven real-car definitions validate, are honest and distinct") {
  const auto cars = files("car.json");
  REQUIRE(cars.size() == 11);
  std::set<std::string> curves, gearboxes, ids;
  for (const auto &path : cars) {
    const auto car = parseCar(read(path));
    CAPTURE(car.id);
    const auto v = validate(car);
    for (const auto &e : v.errors)
      FAIL_CHECK(e);
    CHECK(v.ok());
    CHECK(path.parent_path().filename() == car.id);
    CHECK(ids.insert(car.id).second);
    // Unsourced: nothing verified, provenance explains, placeholder body.
    CHECK(car.sources.empty());
    for (const auto &[key, ok] : car.verified)
      CHECK_FALSE(ok);
    CHECK(car.provenance.find("Unverified") != std::string::npos);
    CHECK(isPlaceholder(car));
    CHECK(menuName(car).find("(placeholder model)") != std::string::npos);
    CHECK(car.gearboxDerivation.rfind("synthetic", 0) == 0);
    CHECK_FALSE(car.license.trademarkNotice.empty());
    // Own data, never a shared curve or gearbox.
    std::ostringstream c, g;
    for (const auto &t : car.torque)
      c << t.rpm << ':' << t.nm << ';';
    for (float r : car.gearRatios)
      g << r << ';';
    CHECK(curves.insert(c.str()).second);
    CHECK(gearboxes.insert(g.str() + std::to_string(car.redlineRpm)).second);
    CHECK(fs::exists(Vehicles / car.body.fallback / (car.body.fallback + ".vehicle")));
    // Claimed top speed is reachable in top gear at the redline.
    CHECK(speedKmh(car, static_cast<unsigned>(car.gearRatios.size() - 1), car.redlineRpm) ==
          doctest::Approx(car.claimedTopSpeedKmh).epsilon(.02));
  }
}

TEST_CASE("legacy adapter: every original .vehicle converts and validates") {
  const auto vehicles = files("*.vehicle");
  REQUIRE(vehicles.size() >= 12);
  for (const auto &path : vehicles) {
    const auto id = path.stem().string();
    CAPTURE(id);
    const auto car = fromLegacyVehicle(read(path), id, path.string());
    const auto v = validate(car);
    for (const auto &e : v.errors)
      FAIL_CHECK(e);
    CHECK(car.legacy);
    CHECK_FALSE(isPlaceholder(car));
    CHECK(car.torque.size() >= 3);
  }
  for (const char *id : Fictional)
    CHECK(fs::exists(Vehicles / id / (std::string(id) + ".vehicle")));
}

TEST_CASE("legacy power mapping reproduces the calibration car") {
  // lancer_evo: powerscale 26000, peak normalised power 1.0 at 5500 rpm ->
  // the adapter's constant must map that back to ~223.7 kW (300 bhp).
  const auto path = Vehicles / "lancer_evo" / "lancer_evo.vehicle";
  const auto car = fromLegacyVehicle(read(path), "lancer_evo", path.string());
  const auto peak = peakPowerKw(car);
  CHECK(peak.value == doctest::Approx(223.7).epsilon(.01));
  CHECK(peak.rpm == doctest::Approx(5500).epsilon(.01));
  CHECK(car.driveLayout == "AWD");
  CHECK(car.frontTorqueShare == doctest::Approx(.45));
}

TEST_CASE("Car v2 -> legacy XML keeps the fallback body and maps SI data exactly") {
  const auto car = parseCar(read(Vehicles / "subaru_impreza_wrc97" / "car.json"));
  const auto fallback = read(Vehicles / car.body.fallback / (car.body.fallback + ".vehicle"));
  const auto xml = toLegacyVehicle(car, fallback);
  tinyxml2::XMLDocument doc;
  REQUIRE(doc.Parse(xml.c_str()) == tinyxml2::XML_SUCCESS);
  const auto *root = doc.FirstChildElement("vehicle");
  REQUIRE(root);
  CHECK(std::string(root->Attribute("name")) == "Subaru Impreza WRC97 (placeholder model)");
  CHECK(root->FirstChildElement("genparams")->FloatAttribute("mass") == doctest::Approx(car.massKg));
  // Gearbox: wheel revolutions per engine revolution = 1/(ratio * final).
  const auto *gear = root->FirstChildElement("drivesystem")->FirstChildElement("gearbox")->FirstChildElement("gear");
  for (float ratio : car.gearRatios) {
    REQUIRE(gear);
    CHECK(gear->FloatAttribute("absolute") == doctest::Approx(1.f / (ratio * car.finalDrive)).epsilon(1e-4));
    gear = gear->NextSiblingElement("gear");
  }
  CHECK(gear == nullptr);
  // Engine: round trip back through the legacy reader recovers the curve.
  const auto back = fromLegacyVehicle(xml, car.id, "generated");
  CHECK(peakPowerKw(back).value == doctest::Approx(peakPowerKw(car).value).epsilon(.01));
  CHECK(peakTorque(back).value == doctest::Approx(peakTorque(car).value).epsilon(.01));
  CHECK(back.frontTorqueShare == doctest::Approx(car.frontTorqueShare));
  CHECK(back.tyreRadius == doctest::Approx(car.tyreRadius));
  CHECK(back.brakeBiasFront == doctest::Approx(car.brakeBiasFront).epsilon(1e-3));
  // The fallback's body model and clips are kept untouched.
  CHECK(std::string(root->FirstChildElement("part")->Attribute("model")) ==
        std::string(tinyxml2::XMLDocument().Parse(fallback.c_str()) == tinyxml2::XML_SUCCESS ? "evo_rally.obj" : ""));
}

TEST_CASE("validator rejects dishonest or broken definitions") {
  auto car = parseCar(read(Vehicles / "peugeot_206_wrc" / "car.json"));
  REQUIRE(validate(car).ok());
  auto bad = car;
  bad.verified["engine"] = true; // claims verification without a source
  CHECK_FALSE(validate(bad).ok());
  bad.sources = {"http://example.org/insecure"};
  CHECK_FALSE(validate(bad).ok());
  bad.sources = {"https://example.org/homologation"};
  CHECK(validate(bad).ok());
  bad = car;
  bad.body.fallback.clear();
  CHECK_FALSE(validate(bad).ok());
  bad = car;
  bad.license.trademarkNotice.clear();
  CHECK_FALSE(validate(bad).ok());
  bad = car;
  std::swap(bad.gearRatios[0], bad.gearRatios[1]);
  CHECK_FALSE(validate(bad).ok());
  bad = car;
  bad.torque[1].rpm = bad.torque[0].rpm;
  CHECK_FALSE(validate(bad).ok());
  bad = car;
  bad.driveLayout = "RWD"; // share 0.5 contradicts RWD
  CHECK_FALSE(validate(bad).ok());
  bad = car;
  bad.massKg = 50;
  CHECK_FALSE(validate(bad).ok());
  CHECK_THROWS(parseCar("{not json"));
  CHECK_THROWS(fromLegacyVehicle("<x/>", "x", "x"));
  // Torque lookup is 0 outside the curve, interpolates inside.
  CHECK(torqueAt(car, 0) == 0);
  CHECK(torqueAt(car, car.redlineRpm + 1) == 0);
  CHECK(torqueAt(car, car.torque[1].rpm) == doctest::Approx(car.torque[1].nm));
  // Canonical writer round-trips.
  const auto again = parseCar(writeCar(car));
  CHECK(writeCar(again) == writeCar(car));
}

TEST_CASE("optional damage.visual_gain parses, validates and round-trips") {
  const auto base = read(files("car.json").front());
  auto car = parseCar(base);
  CHECK(car.damageVisualGain == 0.f);                                  // absent: use the setting
  CHECK(writeCar(car).find("\"damage\"") == std::string::npos);       // and nothing is written
  car.damageVisualGain = 3.5f;
  CHECK(validate(car).ok());
  const auto again = parseCar(writeCar(car));
  CHECK(again.damageVisualGain == doctest::Approx(3.5f));
  car.damageVisualGain = 40.f;
  CHECK_FALSE(validate(car).ok());
  car.damageVisualGain = .1f;
  CHECK_FALSE(validate(car).ok());
}

TEST_CASE("every content-pack car validates, is honest and shows its placeholder status") {
  const fs::path packs = fs::path(TRACKRS_DATA_DIR) / "packs";
  std::set<std::string> ids;
  std::size_t count = 0;
  for (const auto &pack : fs::directory_iterator(packs)) {
    const auto vehicles = pack.path() / "vehicles";
    if (!fs::is_directory(vehicles))
      continue;
    for (const auto &dir : fs::directory_iterator(vehicles)) {
      const auto file = dir.path() / "car.json";
      REQUIRE(fs::exists(file));
      const auto car = parseCar(read(file));
      INFO(file.string());
      CHECK(validate(car).ok());
      CHECK(car.id == dir.path().filename().string());
      CHECK(ids.insert(car.id).second); // ids are unique across all packs
      const bool anyVerified = std::any_of(car.verified.begin(), car.verified.end(), [](const auto &v) { return v.second; });
      CHECK(anyVerified == !car.sources.empty()); // verified only through cited data_overrides.json
      CHECK_FALSE(car.provenance.empty());
      if (!car.marque.empty())
        CHECK_FALSE(car.license.trademarkNotice.empty());
      if (car.body.mesh.empty())
        CHECK(menuName(car).find("(placeholder model)") != std::string::npos);
      CHECK(fs::exists(dir.path() / "livery.json"));
      ++count;
    }
  }
  CHECK(count >= 11); // morocco-sahara 2 + rally-raid 4 + heritage raid 5
}
