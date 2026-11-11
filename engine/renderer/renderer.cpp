// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "pengine.h"
#include "app.h"
#include "vehicle.h"
#include "render.h"
#include "engine/pbr/renderer.h"
#include "engine/car/car_renderer.h"
#include "engine/render/basic.h"
#include "engine/world/state.h"
#include "terrainmap.h"
#include "validation.h"
#include <physfs.h>
#include <filesystem>
#include <map>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
void trackrs::Renderer::vehiclePart(PApp& context, const PVehicleType &type, const PVehiclePart &part,
    const PVehicleTypePart &typepart, float alpha)
{
    std::unique_ptr<trackrs::pbr::HeadlightScope> headlights;
    if(typepart.model && typepart.model->imported){
        trackrs::draw::push();
        const auto position=part.ref_world.getPosition();
        trackrs::draw::translate(position.x,position.y,position.z);
        const auto orientation=part.ref_world.getInverseOrientationMatrix();
        trackrs::draw::multiply(orientation);
        glm::mat4 bodyToEye;trackrs::draw::read(trackrs::draw::ViewMatrix,glm::value_ptr(bodyToEye));
        trackrs::draw::pop();headlights=std::make_unique<trackrs::pbr::HeadlightScope>(bodyToEye);
    }
    if (typepart.model)
    {
        trackrs::draw::push();

        vec3f vpos = part.ref_world.getPosition();
        trackrs::draw::translate(vpos.x, vpos.y, vpos.z);

        mat44f vorim = part.ref_world.getInverseOrientationMatrix();
        trackrs::draw::multiply(vorim);

        float scale = typepart.scale;
        trackrs::draw::scale(scale,scale,scale);

        context.drawModel(*typepart.model, alpha);

        trackrs::draw::pop();
    }

    if (type.wheelmodel)
    {
        std::vector<glm::mat4> wheelMatrices;
        for (unsigned int i=0; i<typepart.wheel.size(); ++i)
        {
            trackrs::draw::push();

            vec3f wpos = part.wheel[i].ref_world.getPosition();
            trackrs::draw::translate(wpos.x,wpos.y,wpos.z);

            mat44f worim = part.wheel[i].ref_world.getInverseOrientationMatrix();
            trackrs::draw::multiply(worim);

            float scale = type.wheelscale * typepart.wheel[i].radius;
            trackrs::draw::scale(scale,scale,scale);

            if (type.wheelmodel->imported) {
                glm::mat4 matrix;
                trackrs::draw::read(trackrs::draw::ViewMatrix,glm::value_ptr(matrix));
                wheelMatrices.push_back(matrix);
            } else context.drawModel(*type.wheelmodel, alpha);

            trackrs::draw::pop();
        }
        if (!wheelMatrices.empty() && !type.wheelmodel->drawImportedInstances(wheelMatrices,alpha)) {
            for (const auto &matrix : wheelMatrices) {
                trackrs::draw::push();
                trackrs::draw::identity();
                trackrs::draw::multiply(glm::value_ptr(matrix));
                context.drawModel(*type.wheelmodel,alpha);
                trackrs::draw::pop();
            }
        }
    }
}

namespace {
trackrs::car::Surface surfaceOf(TerrainType t) {
    using S = trackrs::car::Surface;
    switch (t) {
    case TerrainType::RacingTarmac: case TerrainType::CountryRoad: case TerrainType::SlipperyRoad: return S::Tarmac;
    case TerrainType::Ice: case TerrainType::DeepSnow: return S::Snow;
    case TerrainType::FirmSand: case TerrainType::LooseSand: return S::Sand;
    case TerrainType::PressedEarth: case TerrainType::LooseEarth: return S::Dirt;
    case TerrainType::MoistEarth: case TerrainType::Mud: return S::Mud;
    case TerrainType::FineGravel: case TerrainType::RoughGravel: return S::Gravel;
    case TerrainType::Grass: case TerrainType::Shrubbery: return S::Grass;
    default: return S::Other;
    }
}
trackrs::mesh::Bytes readVirtual(const std::string& path) {
    const auto normal = std::filesystem::path(path).lexically_normal().generic_string();
    PHYSFS_File* file = PHYSFS_openRead(normal.c_str());
    if (!file) throw std::runtime_error("cannot open car file: " + normal);
    std::unique_ptr<PHYSFS_File, decltype(&PHYSFS_close)> handle(file, PHYSFS_close);
    const auto size = PHYSFS_fileLength(file);
    if (size < 0 || size > 128 * 1024 * 1024) throw std::runtime_error("invalid car file size: " + normal);
    trackrs::mesh::Bytes bytes(static_cast<std::size_t>(size));
    if (PHYSFS_readBytes(file, bytes.data(), static_cast<PHYSFS_uint64>(size)) != size) throw std::runtime_error("short car file read");
    return bytes;
}
glm::mat4 frame(const PReferenceFrame& ref, float scale) {
    trackrs::draw::push();
    trackrs::draw::identity();
    const vec3f p = ref.getPosition();
    trackrs::draw::translate(p.x, p.y, p.z);
    mat44f orientation = ref.getInverseOrientationMatrix();
    trackrs::draw::multiply(orientation);
    trackrs::draw::scale(scale, scale, scale);
    glm::mat4 m;
    trackrs::draw::read(trackrs::draw::ViewMatrix, glm::value_ptr(m));
    trackrs::draw::pop();
    return m;
}
} // namespace

bool trackrs::Renderer::car(const PVehicle& vehicle, TerrainType surface, float time)
{
    const auto& ws = trackrs::world::state();
    const PVehicleType& type = *vehicle.type;
    if (!ws.scene || !trackrs::car::settings().enabled || type.part.empty() || !type.part[0].model || !type.wheelmodel
        || vehicle.part.empty())
        return false;
    validation::PassScope timing(ws.depthPass ? "shadow_cars" : "cars");
    // The modelview stack holds view * (interpolation offset); model = view^-1 * stack.
    glm::mat4 stack, projection;
    trackrs::draw::read(trackrs::draw::ViewMatrix, glm::value_ptr(stack));
    trackrs::draw::read(trackrs::draw::ProjectionMatrix, glm::value_ptr(projection));
    const glm::mat4 view = ws.depthPass ? glm::mat4(1) : ws.view;
    const glm::mat4 toWorld = glm::inverse(view) * stack;
    const PVehiclePart& part = vehicle.part[0];
    const PVehicleTypePart& typepart = type.part[0];
    trackrs::car::Instance car;
    car.vehicle = &vehicle;
    car.type = &type;
    car.name = type.proper_name;
    car.bodyFile = typepart.model->getName();
    car.wheelFile = type.wheelmodel->getName();
    static std::map<const PVehicleType*, std::string> liveries;
    auto livery = liveries.find(&type);
    if (livery == liveries.end()) {
        std::string text;
        const auto file = std::filesystem::path(type.getName()).parent_path() / "livery.json";
        if (PHYSFS_exists(file.generic_string().c_str())) {
            const auto bytes = readVirtual(file.generic_string());
            text.assign(bytes.begin(), bytes.end());
        }
        livery = liveries.emplace(&type, text).first;
    }
    car.livery = livery->second;
    car.body = toWorld * frame(part.ref_world, typepart.scale);
    for (std::size_t i = 0; i < typepart.wheel.size() && i < part.wheel.size(); ++i)
        car.wheels.push_back(toWorld * frame(part.wheel[i].ref_world, type.wheelscale * typepart.wheel[i].radius));
    auto& v = const_cast<PVehicle&>(vehicle); // legacy getters are non-const
    car.rpm = v.getEngineRPM();
    car.redlineRpm = std::max(type.engine.getMaxRPM(), 1000.0f);
    car.gear = v.getCurrentGear();
    car.speed = std::abs(vehicle.forwardspeed);
    car.brake = vehicle.ctrl.brake1;
    car.reverse = car.gear < 0;
    car.surface = surfaceOf(surface);
    const glm::vec3 camera = ws.depthPass ? glm::vec3(car.body[3]) : glm::vec3(glm::inverse(ws.view)[3]);
    try {
        trackrs::car::draw(car, readVirtual, projection * view, camera, 2.f * std::atan(.6f), time, ws.linearCapture);
    } catch (const std::exception& error) {
        PUtil::outLog() << "Car renderer: " << error.what() << ", using the legacy path" << std::endl;
        trackrs::car::configure({false});
        return false;
    }
    return true;
}
