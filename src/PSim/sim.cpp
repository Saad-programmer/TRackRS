// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "psim.h"
#include "render.h"
#include "vehicle.h"

///
/// @brief constructor
///
PSim::PSim(float step) : terrain(nullptr), stepper(step), gravity(vec3f::zero())
{
}

///
/// @brief destructor
///
PSim::~PSim()
{
  clear();
}

///
/// @brief Load a vehicle type from a file
/// @param filename = name of the file to load from
/// @param ssModel = the PSSModel of the vehicle type
/// @retval the newly loaded vehicle type, or nullptr if failed to load
///
PVehicleType *PSim::loadVehicleType(const std::string &filename, PSSModel &ssModel)
{
  PVehicleType *vtype = vtypelist.find(filename);
  if (!vtype) {
    auto owner = std::make_unique<PVehicleType>();
    vtype = owner.get();
    if (!vtype->load(filename, ssModel)) {
      if (PUtil::isDebugLevel(DEBUGLEVEL_ENDUSER))
        PUtil::outLog() << "Failed to load " << filename << "\n";
      return nullptr;
    }
    vtypelist.add(owner.release());
  }
  return vtype;
}

///
/// @brief Put a new rigid body in the body vector
/// @retval the pointer to the new rigid body
///
PRigidBody *PSim::createRigidBody()
{
  auto owner = std::make_unique<PRigidBody>(gravity);
  PRigidBody *newbody = owner.get();
  body.push_back(std::move(owner));

  return newbody;
}

///
/// @brief Create a new vehicle and put it in the vehicle vector
/// @retval the pointer to the newly created vehicle, nullptr if problems occurred
///
PVehicle *PSim::createVehicle(XMLElement *element, const std::string &filepath, PSSModel &ssModel)
{
  const char *val;

  const char *type = element->Attribute("type");
  if (!type) {
    PUtil::outLog() << "Vehicle has no type\n";
    return nullptr;
  }

  vec3f pos = vec3f::zero();

  val = element->Attribute("pos");
  if (val) sscanf(val, "%f , %f , %f", &pos.x, &pos.y, &pos.z);

  quatf ori = quatf::identity();

  val = element->Attribute("ori");
  if (val) sscanf(val, "%f , %f , %f , %f", &ori.w, &ori.x, &ori.y, &ori.z);

  return createVehicle(type, pos, ori, filepath, ssModel);
}

///
/// @brief Create a new vehicle and put it in the vehicle vector
/// @retval the pointer to the newly created vehicle, nullptr if problems occurred
///
PVehicle *PSim::createVehicle(const std::string &type, const vec3f &pos, const quatf &ori, const std::string &filepath, PSSModel &ssModel)
{
  PVehicleType *vtype = loadVehicleType(PUtil::assemblePath(type, filepath), ssModel);

  return createVehicle(vtype, pos, ori /*, ssModel */);
}

///
/// @brief Create a new vehicle and put it in the vehicle vector
/// @retval the pointer to the newly created vehicle, nullptr if problems occurred
///
PVehicle *PSim::createVehicle(PVehicleType *type, const vec3f &pos, const quatf &ori /*, PSSModel &ssModel */)
{
  if (!type) return nullptr;

  auto owner = std::make_unique<PVehicle>(*this, type);
  PVehicle *newvehicle = owner.get();

  vec3f vpos = pos;
  if (terrain) vpos.z += terrain->getHeight(vpos.x, vpos.y);
  newvehicle->getBody().setPosition(vpos);

  newvehicle->getBody().setOrientation(ori);
  newvehicle->getBody().updateMatrices();

  newvehicle->getBody().capturePrevious();
  newvehicle->seedRandom(stageSeed ^ vehicle.size());
  newvehicle->updateParts();

  vehicle.push_back(std::move(owner));
  return newvehicle;
}

///
/// @brief Clear from the simulation any rigid body, vehicle and vehicle types
///
void PSim::clear()
{
  // Vehicle observers refer to bodies/types, so destroy them first.
  vehicle.clear();
  body.clear();
  vtypelist.clear();
  stepper.reset();
}

///
/// @brief Calls the vehicles and bodies ticks and update the parts of the vehicles
/// @param delta = how much time to compute
///
void PSim::tick(float delta)
{
  stepper.advance(delta, [this](float timeslice) {
    for (auto& b : body) b->capturePrevious();
    for (auto& v : vehicle) v->tick(timeslice);
    for (auto& b : body) b->tick(timeslice);
    for (auto& v : vehicle) v->updateParts();
  });
}
