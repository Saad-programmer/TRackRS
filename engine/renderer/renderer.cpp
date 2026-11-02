// SPDX-License-Identifier: GPL-2.0-or-later
#include "renderer.h"
#include "pengine.h"
#include "app.h"
#include "vehicle.h"
#include "render.h"
void trackrs::Renderer::vehiclePart(PApp& context, const PVehicleType &type, const PVehiclePart &part,
    const PVehicleTypePart &typepart, float alpha)
{
    if (typepart.model)
    {
        glPushMatrix();

        vec3f vpos = part.ref_world.getPosition();
        glTranslatef(vpos.x, vpos.y, vpos.z);

        mat44f vorim = part.ref_world.getInverseOrientationMatrix();
        glMultMatrixf(vorim);

        float scale = typepart.scale;
        glScalef(scale,scale,scale);

        context.drawModel(*typepart.model, alpha);

        glPopMatrix();
    }

    if (type.wheelmodel)
    {
        for (unsigned int i=0; i<typepart.wheel.size(); ++i)
        {
            glPushMatrix();

            vec3f wpos = part.wheel[i].ref_world.getPosition();
            glTranslatef(wpos.x,wpos.y,wpos.z);

            mat44f worim = part.wheel[i].ref_world.getInverseOrientationMatrix();
            glMultMatrixf(worim);

            float scale = type.wheelscale * typepart.wheel[i].radius;
            glScalef(scale,scale,scale);

            context.drawModel(*type.wheelmodel, alpha);

            glPopMatrix();
        }
    }
}
