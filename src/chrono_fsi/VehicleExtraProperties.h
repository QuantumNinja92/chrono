/*
 * VehicleProperties.h
 *
 *  Created on: Apr 1, 2015
 *      Author: Arman Pazouki
 */

#ifndef VEHICLEEXTRAPROPERTIES_H_
#define VEHICLEEXTRAPROPERTIES_H_

#include "chrono_parallel/physics/ChSystemParallel.h"

// Arman : maybe too many includes
#include "core/ChFileutils.h"
#include "core/ChStream.h"

#include "chrono_parallel/physics/ChSystemParallel.h"
#include "chrono_parallel/lcp/ChLcpSystemDescriptorParallel.h"
#include "chrono_parallel/collision/ChCNarrowphaseRUtils.h"

#include "chrono_utils/ChUtilsVehicle.h"
#include "chrono_utils/ChUtilsGeometry.h"
#include "chrono_utils/ChUtilsCreators.h"
#include "chrono_utils/ChUtilsGenerators.h"
#include "chrono_utils/ChUtilsInputOutput.h"

//#include "hmmwvParams.h"

using namespace chrono;
using namespace chrono::collision;

// Tire Coefficient of friction
float mu_t = 0.8;

// Callback class for providing driver inputs.
class MyDriverInputs : public utils::DriverInputsCallback {
 public:
  MyDriverInputs(double delay) : m_delay(delay) {}

  virtual void onCallback(double time, double& throttle, double& steering, double& braking) {
    throttle = 0;
    steering = 0;
    braking = 0;

    double eff_time = time - m_delay;

    // Do not generate any driver inputs for a duration equal to m_delay.
    if (eff_time < 0)
      return;

    if (eff_time > 0.5)
      throttle = 1.0;
    else if (eff_time > 0.25)
      throttle = 4 * (eff_time - 0.25);
  }

 private:
  double m_delay;
};

// Callback class for specifying rigid tire contact model.
// This version uses cylindrical contact shapes.
class MyCylindricalTire : public utils::TireContactCallback {
 public:
  virtual void onCallback(ChSharedPtr<ChBody> wheelBody, double radius, double width) {
    wheelBody->GetCollisionModel()->ClearModel();
    wheelBody->GetCollisionModel()->AddCylinder(0.46, 0.46, width / 2);
    wheelBody->GetCollisionModel()->BuildModel();

    wheelBody->GetMaterialSurface()->SetFriction(mu_t);
  }
};

// Callback class for specifying rigid tire contact model.
// This version uses a collection of convex contact shapes (meshes).
class MyLuggedTire : public utils::TireContactCallback {
 public:
  MyLuggedTire() {
    std::string lugged_file("hmmwv/lugged_wheel_section.obj");
    geometry::ChTriangleMeshConnected lugged_mesh;
    utils::LoadConvexMesh(vehicle::GetDataFile(lugged_file), lugged_mesh, lugged_convex);
    num_hulls = lugged_convex.GetHullCount();
  }

  virtual void onCallback(ChSharedPtr<ChBody> wheelBody, double radius, double width) {
    ChCollisionModelParallel* coll_model = (ChCollisionModelParallel*)wheelBody->GetCollisionModel();
    coll_model->ClearModel();

    // Assemble the tire contact from 15 segments, properly offset.
    // Each segment is further decomposed in convex hulls.
    for (int iseg = 0; iseg < 15; iseg++) {
      ChQuaternion<> rot = Q_from_AngAxis(iseg * 24 * CH_C_DEG_TO_RAD, VECT_Y);
      for (int ihull = 0; ihull < num_hulls; ihull++) {
        std::vector<ChVector<> > convexhull;
        lugged_convex.GetConvexHullResult(ihull, convexhull);
        coll_model->AddConvexHull(convexhull, VNULL, rot);
      }
    }

    // Add a cylinder to represent the wheel hub.
    coll_model->AddCylinder(0.223, 0.223, 0.126);

    coll_model->BuildModel();

    wheelBody->GetMaterialSurface()->SetFriction(mu_t);
  }

 private:
  ChConvexDecompositionHACDv2 lugged_convex;
  int num_hulls;
};

// Callback class for specifying rigid tire contact model.
// This version uses a collection of convex contact shapes (meshes).
// In addition, this version overrides the visualization assets of the provided
// wheel body with the collision meshes.
class MyLuggedTire_vis : public utils::TireContactCallback {
 public:
  MyLuggedTire_vis() {
    std::string lugged_file("hmmwv/lugged_wheel_section.obj");
    utils::LoadConvexMesh(vehicle::GetDataFile(lugged_file), lugged_mesh, lugged_convex);
  }

  virtual void onCallback(ChSharedPtr<ChBody> wheelBody, double radius, double width) {
    // Clear any existing assets (will be overriden)
    wheelBody->GetAssets().clear();

    wheelBody->GetCollisionModel()->ClearModel();
    for (int j = 0; j < 15; j++) {
      utils::AddConvexCollisionModel(
          wheelBody, lugged_mesh, lugged_convex, VNULL, Q_from_AngAxis(j * 24 * CH_C_DEG_TO_RAD, VECT_Y), false);
    }
    // This cylinder acts like the rims
    utils::AddCylinderGeometry(wheelBody.get_ptr(), 0.223, 0.126);
    wheelBody->GetCollisionModel()->BuildModel();

    wheelBody->GetMaterialSurface()->SetFriction(mu_t);
  }

 private:
  ChConvexDecompositionHACDv2 lugged_convex;
  geometry::ChTriangleMeshConnected lugged_mesh;
};

// Callback class for specifying chassis contact model.
// This version uses a box representing the chassis.
// In addition, this version overrides the visualization assets of the provided
// chassis body with the collision meshes.
class MyChassisBoxModel_vis : public utils::ChassisContactCallback {
 public:
  //	MyChassisBoxModel_vis() {
  //    std::string chassis_file("hmmwv/myVehicle.obj");
  //    utils::LoadConvexMesh(vehicle::GetDataFile(chassis_file), chassis_mesh, chassis_convex);
  //  }

  //  MyChassisBoxModel_vis(ChSharedPtr<ChBodyAuxRef> chassisBody, ChVector<> hdim) {
  //    // Clear any existing assets (will be overriden)
  //    chassisBody->GetAssets().clear();
  //
  //    chassisBody->GetCollisionModel()->ClearModel();
  //    utils::AddBoxGeometry(
  //        chassisBody.get_ptr(), hdim, ChVector<>(0, 0, 0), ChQuaternion<>(1, 0, 0, 0), true);
  //    chassisBody->GetCollisionModel()->BuildModel();
  //
  //    chassisBody->GetMaterialSurface()->SetFriction(mu_t);
  //  }

  virtual void onCallback(ChSharedPtr<ChBodyAuxRef> chassisBody) {
    // Clear any existing assets (will be overriden)
    chassisBody->GetAssets().clear();

    chassisBody->GetCollisionModel()->ClearModel();
    utils::AddBoxGeometry(
        chassisBody.get_ptr(), ChVector<>(1, .5, .05), ChVector<>(0, 0, 0), ChQuaternion<>(1, 0, 0, 0), true);
    chassisBody->GetCollisionModel()->BuildModel();

    chassisBody->GetMaterialSurface()->SetFriction(mu_t);
  }

 private:
  ChConvexDecompositionHACDv2 chassis_convex;
  geometry::ChTriangleMeshConnected chassis_mesh;
};

#endif /* VEHICLEEXTRAPROPERTIES_H_ */
