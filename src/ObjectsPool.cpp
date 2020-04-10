#include "ObjectsPool.h"

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

ObjectsPool &objectsPool()
{
	static ObjectsPool instance;
	return instance;
}

pm::Tracer *ObjectsPool::retrieveTracer(pm::Tracer::Type type)
{
	switch (type)
	{
		case pm::Tracer::Type::RAYCAST: return &raycastTracer_;
		case pm::Tracer::Type::WHITTED: return &whittedTracer_;
		case pm::Tracer::Type::AREALIGHTING: return &areaLightingTracer_;
		case pm::Tracer::Type::PATHTRACE: return &pathTraceTracer_;
		case pm::Tracer::Type::GLOBALTRACE: return &globalTraceTracer_;
	}

	return &globalTraceTracer_;
}

pm::Camera *ObjectsPool::retrieveCamera(pm::Camera::Type type)
{
	switch (type)
	{
		case pm::Camera::Type::ORTOGRAPHIC: return &ortographicCamera_;
		case pm::Camera::Type::PINHOLE: return &pinHoleCamera_;
	}

	return &pinHoleCamera_;
}
