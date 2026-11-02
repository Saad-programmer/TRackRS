// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "pengine.h"
#include "app.h"
#include "vehicle.h"
#include "render.h"
#include "engine/pbr/renderer.h"
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
        for (unsigned int i=0; i<typepart.wheel.size(); ++i)
        {
            trackrs::draw::push();

            vec3f wpos = part.wheel[i].ref_world.getPosition();
            trackrs::draw::translate(wpos.x,wpos.y,wpos.z);

            mat44f worim = part.wheel[i].ref_world.getInverseOrientationMatrix();
            trackrs::draw::multiply(worim);

            float scale = type.wheelscale * typepart.wheel[i].radius;
            trackrs::draw::scale(scale,scale,scale);

            context.drawModel(*type.wheelmodel, alpha);

            trackrs::draw::pop();
        }
    }
}
