#ifndef CLASS_OBJECTS_POOL
#define CLASS_OBJECTS_POOL

#include <nctl/UniquePtr.h>

#include "World.h"

#include "RayCast.h"
#include "Whitted.h"
#include "AreaLighting.h"
#include "PathTrace.h"
#include "GlobalTrace.h"

#include "Ortographic.h"
#include "PinHole.h"

/// Pool for various type of objects
class ObjectsPool
{
  public:
	pm::Tracer *retrieveTracer(pm::Tracer::Type type);
	pm::Camera *retrieveCamera(pm::Camera::Type type);

  private:
	pm::RayCast raycastTracer_;
	pm::Whitted whittedTracer_;
	pm::AreaLighting areaLightingTracer_;
	pm::PathTrace pathTraceTracer_;
	pm::GlobalTrace globalTraceTracer_;

	pm::Ortographic ortographicCamera_;
	pm::PinHole pinHoleCamera_;
};

ObjectsPool &objectsPool();

#endif
