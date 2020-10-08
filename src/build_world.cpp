#include "Ortographic.h"
#include "PinHole.h"
#include "Jittered.h"
#include "MultiJittered.h"
#include "Hammersley.h"
#include "Halton.h"
#include "NRooks.h"
#include "Sphere.h"
#include "Plane.h"
#include "Matte.h"
#include "Ambient.h"
#include "PointLight.h"
#include "Phong.h"
#include "Reflective.h"
#include "Directional.h"
#include "AmbientOccluder.h"
#include "AreaLight.h"
#include "Emissive.h"
#include "Rectangle.h"
#include "EnvironmentLight.h"
#include "Tracer.h"

#include <ncine/common_macros.h>

#define CORNELL_BOX (1)

#if !CORNELL_BOX
	#define AMBIENT (0)
	#define AMBIENT_OCCLUSION (1)

	#define POINT_LIGHTS (1)
	#define AREA_LIGHTS (0)
	#define PATH_TRACE (0)
#endif

namespace {

const unsigned int numSamples = 4;

std::unique_ptr<pm::Rectangle> rectangleFromVertices(const pm::Vector3 pA, const pm::Vector3 pB, const pm::Vector3 pC)
{
	auto rect = std::make_unique<pm::Rectangle>(pA, pB - pA, pC - pA, cross(pB - pA, pC - pA).normalize());
	return rect;
}

void setupSpheres(pm::World &world, pm::PinHole &camera, pm::Tracer::Type &tracerType)
{
	tracerType = pm::Tracer::Type::AREALIGHTING;

	camera.editEye().set(0.0f, 2.0f, -8.0f);
	camera.editUp().set(0.0f, 1.0f, 0.0f);
	camera.editLookAt().set(0.0f, 2.0f, 0.0f);
	camera.editViewDistance() = 4.0f;
	camera.computeUvw();

	auto vpSampler = world.createSampler<pm::NRooks>(numSamples);
	world.viewPlane().setSampler(vpSampler);
	world.viewPlane().editMaxDepth() = 5;

	auto hammersley = world.createSampler<pm::Hammersley>(16);

#if AMBIENT
	auto ambient = std::make_unique<pm::Ambient>();
	ambient->setRadianceScale(0.01f);
	world.setAmbientLight(std::move(ambient));
#elif AMBIENT_OCCLUSION
	auto multiJittered = world.createSampler<pm::MultiJittered>(64);

	auto ambient = std::make_unique<pm::AmbientOccluder>();
	ambient->setRadianceScale(0.01f);
	ambient->setColor(0.25f, 0.25f, 0.25f);
	ambient->setMinAmount(0.0f);
	ambient->setSampler(multiJittered);
	world.setAmbientLight(std::move(ambient));
#endif

	auto plane = std::make_unique<pm::Plane>(pm::Vector3(0.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 1.0f, 0.0f));
	auto white = std::make_unique<pm::Phong>();
	white->setCd(1.0f, 1.0f, 1.0f);
	white->setKa(0.25f);
	white->setKd(0.45f);
	//white->setKs(0.25f);
	//white->setSpecularExp(32.0f);
	white->diffuse().setSampler(hammersley);
	plane->setMaterial(white.get());
	world.addObject(std::move(plane));
	world.addMaterial(std::move(white));

	auto sphere1 = std::make_unique<pm::Sphere>(pm::Vector3(0.0f, 1.0f, 0.0f), 1.0f);
	auto red = std::make_unique<pm::Phong>();
	red->setCd(1.0f, 0.0f, 0.0f);
	red->setKa(0.25f);
	red->setKd(0.65f);
	red->setKs(0.15f);
	red->setSpecularExp(32.0f);
	red->diffuse().setSampler(hammersley);
	sphere1->setMaterial(red.get());
	world.addObject(std::move(sphere1));
	world.addMaterial(std::move(red));

	auto sphere2 = std::make_unique<pm::Sphere>(pm::Vector3(2.0f, 0.5f, 0.0f), 0.5f);
	auto green = std::make_unique<pm::Phong>();
	green->setCd(0.0f, 1.0f, 0.0f);
	green->setKa(0.1f);
	green->setKd(0.45f);
	green->setKs(0.5f);
	green->setSpecularExp(32.0f);
	green->diffuse().setSampler(hammersley);
	sphere2->setMaterial(green.get());
	world.addObject(std::move(sphere2));
	world.addMaterial(std::move(green));

	auto sphere3 = std::make_unique<pm::Sphere>(pm::Vector3(-2.0f, 2.0f, 0.0f), 0.75f);
	auto blue = std::make_unique<pm::Phong>();
	blue->setCd(0.0f, 0.0f, 1.0f);
	blue->setKa(0.1f);
	blue->setKd(0.75f);
	blue->setKs(0.25f);
	blue->setSpecularExp(24.0f);
	blue->diffuse().setSampler(hammersley);
	sphere3->setMaterial(blue.get());
	world.addObject(std::move(sphere3));
	world.addMaterial(std::move(blue));

#if POINT_LIGHTS
	auto light1 = std::make_unique<pm::PointLight>(0.0f, 2.0f, -2.0f);
	//light1->setColor(1.0f, 0.0f, 1.0f);
	light1->setRadianceScale(0.1f);
	world.addLight(std::move(light1));

	auto light2 = std::make_unique<pm::PointLight>(3.0f, 3.0f, -2.0f);
	light2->setRadianceScale(0.1f);
	//light2->setCastShadows(false);
	//world.addLight(std::move(light2));

	auto light3 = std::make_unique<pm::PointLight>(-3.0f, 3.0f, -2.0);
	light3->setRadianceScale(0.1f);
	//light3->setColor(0.5f, 1.0f, 0.33f);
	//light3->setCastShadows(false);
	//world.addLight(std::move(light3));

	auto light4 = std::make_unique<pm::Directional>(-1.0f, 1.0f, 0.0f);
	light4->setRadianceScale(0.0001f);
	light4->setColor(0.5f, 1.0f, 0.33f);
	//light4->setCastShadows(false);
	//world.addLight(std::move(light4));
#elif AREA_LIGHTS
	auto emissive = world.createMaterial<pm::Emissive>();
	emissive->setRadianceScale(0.25f);

	auto object = world.createObject<pm::Rectangle>(pm::Vector3(-1.0f, 3.0f, -1.0f), pm::Vector3(2.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 2.0f), pm::Vector3(0.0f, -1.0f, 0.0f));
	object->setSampler(hammersley);
	object->setMaterial(emissive);
	object->setCastShadows(false);
	auto light1 = world.createLight<pm::AreaLight>(object);

	auto object2 = world.createObject<pm::Rectangle>(pm::Vector3(-4.0f, 0.0f, 0.5f), pm::Vector3(0.0f, 1.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 1.0f), pm::Vector3(1.0f, 0.0f, 0.0f));
	object2->setSampler(hammersley);
	object2->setMaterial(emissive);
	object2->setCastShadows(false);
	auto light2 = world.createLight<pm::AreaLight>(object2);

	auto envSampler = world.createSampler<pm::NRooks>(256);
	auto envEmissive = world.createMaterial<pm::Emissive>();
	envEmissive->setRadianceScale(0.1f);
	envEmissive->setCe(1.0f, 1.0f, 0.6f);
	auto envlight = world.createLight<pm::EnvironmentLight>(envEmissive);
	envlight->setSampler(envSampler);
#elif PATH_TRACE
	auto object = std::make_unique<pm::Rectangle>(pm::Vector3(-1.0f, 3.0f, -1.0f), pm::Vector3(2.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 2.0f), pm::Vector3(0.0f, -1.0f, 0.0f));
	object->setSampler(hammersley);
	auto emissive = std::make_unique<pm::Emissive>();
	emissive->setRadianceScale(0.25f);
	//emissive->setCe(1.0f, 0.0f, 0.0f);
	object->setMaterial(emissive.get());
	world.addObject(std::move(object));
	world.addMaterial(std::move(emissive));

	auto object2 = std::make_unique<pm::Rectangle>(pm::Vector3(-4.0f, 0.0f, 0.5f), pm::Vector3(0.0f, 1.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 1.0f), pm::Vector3(1.0f, 0.0f, 0.0f));
	object2->setSampler(hammersley);
	auto emissive2 = std::make_unique<pm::Emissive>();
	emissive2->setRadianceScale(0.2f);
	//emissive2->setCe(1.0f, 0.0f, 0.0f);
	object2->setMaterial(emissive2.get());
	world.addObject(std::move(object2));
	world.addMaterial(std::move(emissive2));
#endif
}

void setupCornellBox(pm::World &world, pm::PinHole &camera, pm::Tracer::Type &tracerType)
{
	tracerType = pm::Tracer::Type::GLOBALTRACE;

	camera.editEye().set(278.0f, 273.0f, -800.0f);
	camera.editUp().set(0.0f, 1.0f, 0.0f);
	camera.editLookAt().set(278.0f, 273.0f, 0.0f);
	camera.editViewDistance() = 4.0f;
	camera.computeUvw();

	auto vpSampler = world.createSampler<pm::MultiJittered>(64);
	world.viewPlane().setSampler(vpSampler);
	world.viewPlane().editMaxDepth() = 5;

	auto hammersley = world.createSampler<pm::Hammersley>(64);

	// Materials
	auto white = world.createMaterial<pm::Matte>();
	white->setCd(0.7f, 0.7f, 0.7f);
	white->ambient().setSampler(hammersley);
	white->diffuse().setSampler(hammersley);

	auto red = world.createMaterial<pm::Matte>();
	red->setCd(0.7f, 0.0f, 0.0f);
	red->ambient().setSampler(hammersley);
	red->diffuse().setSampler(hammersley);

	auto green = world.createMaterial<pm::Matte>();
	green->setCd(0.0f, 0.7f, 0.0f);
	green->ambient().setSampler(hammersley);
	green->diffuse().setSampler(hammersley);

	auto emissive = world.createMaterial<pm::Emissive>();

	// Light
	auto lightRect = world.createObject<pm::Rectangle>(pm::Vector3(213.0f, 548.79f, 227.0f), pm::Vector3(343.0f - 213.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 332.0f - 227.0f), pm::Vector3(0.0f, -1.0f, 0.0f));
	lightRect->setSampler(hammersley);
	lightRect->setMaterial(emissive);
	//auto light = world.createLight<pm::AreaLight>(lightRect);

	// Walls
	auto floor = world.createObject<pm::Rectangle>(pm::Vector3(0.0f, 0.0f, 0.0f), pm::Vector3(552.8f, 0.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 559.2f), pm::Vector3(0.0f, 1.0f, 0.0f));
	floor->setMaterial(white);

	auto ceiling = world.createObject<pm::Rectangle>(pm::Vector3(0.0f, 548.8f, 0.0f), pm::Vector3(556.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 0.0f, 559.2f), pm::Vector3(0.0f, -1.0f, 0.0f));
	ceiling->setMaterial(white);

	auto leftWall = world.createObject<pm::Rectangle>(pm::Vector3(552.8f, 0.0f, 0.0f), pm::Vector3(0.0f, 548.8f, 0.0f), pm::Vector3(0.0f, 0.0f, 559.2f), pm::Vector3(-1.0f, 0.0f, 0.0f));
	leftWall->setMaterial(red);

	auto rightWall = world.createObject<pm::Rectangle>(pm::Vector3(0.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 548.8f, 0.0f), pm::Vector3(0.0f, 0.0f, 559.2f), pm::Vector3(1.0f, 0.0f, 0.0f));
	rightWall->setMaterial(green);

	auto backWall = world.createObject<pm::Rectangle>(pm::Vector3(0.0f, 0.0f, 559.2f), pm::Vector3(0.0f, 548.8f, 0.0f), pm::Vector3(556.0f, 0.0f, 0.0f), pm::Vector3(0.0f, 0.0f, -1.0f));
	backWall->setMaterial(white);

	// Short object
	auto short1 = rectangleFromVertices(pm::Vector3(130.0f, 165.0f, 65.0f), pm::Vector3(82.0f, 165.0f, 225.0f), pm::Vector3(290.0f, 165.0f, 114.0f));
	short1->setMaterial(white);
	world.addObject(std::move(short1));

	auto short2 = rectangleFromVertices(pm::Vector3(290.0f, 0.0f, 114.0f), pm::Vector3(290.0f, 165.0f, 114.0f), pm::Vector3(240.0f, 0.0f, 272.0f));
	short2->setMaterial(white);
	world.addObject(std::move(short2));

	auto short3 = rectangleFromVertices(pm::Vector3(130.0f, 0.0f, 65.0f), pm::Vector3(130.0f, 165.0f, 65.0f), pm::Vector3(290.0f, 0.0f, 114.0f));
	short3->setMaterial(white);
	world.addObject(std::move(short3));

	auto short4 = rectangleFromVertices(pm::Vector3(82.0f, 0.0f, 225.0f), pm::Vector3(82.0f, 165.0f, 225.0f), pm::Vector3(130.0f, 0.0f, 65.0f));
	short4->setMaterial(white);
	world.addObject(std::move(short4));

	auto short5 = rectangleFromVertices(pm::Vector3(240.0f, 0.0f, 272.0f), pm::Vector3(240.0f, 165.0f, 272.0f), pm::Vector3(82.0f, 0.0f, 225.0f));
	short5->setMaterial(white);
	world.addObject(std::move(short5));

	// Tall object
	auto tall1 = rectangleFromVertices(pm::Vector3(423.0f, 330.0f, 247.0f), pm::Vector3(265.0f, 330.0f, 296.0f), pm::Vector3(472.0f, 330.0f, 406.0f));
	tall1->setMaterial(white);
	world.addObject(std::move(tall1));

	auto tall2 = rectangleFromVertices(pm::Vector3(423.0f, 0.0f, 247.0f), pm::Vector3(423.0f, 330.0f, 247.0f), pm::Vector3(472.0f, 0.0f, 406.0f));
	tall2->setMaterial(white);
	world.addObject(std::move(tall2));

	auto tall3 = rectangleFromVertices(pm::Vector3(472.0f, 0.0f, 406.0f), pm::Vector3(472.0f, 330.0f, 406.0f), pm::Vector3(314.0f, 0.0f, 456.0f));
	tall3->setMaterial(white);
	world.addObject(std::move(tall3));

	auto tall4 = rectangleFromVertices(pm::Vector3(314.0f, 0.0f, 456.0f), pm::Vector3(314.0f, 330.0f, 456.0f), pm::Vector3(265.0f, 0.0f, 296.0f));
	tall4->setMaterial(white);
	world.addObject(std::move(tall4));

	auto tall5 = rectangleFromVertices(pm::Vector3(265.0f, 0.0f, 296.0f), pm::Vector3(265.0f, 330.0f, 296.0f), pm::Vector3(423.0f, 0.0f, 247.0f));
	tall5->setMaterial(white);
	world.addObject(std::move(tall5));
}

void validateWorld(const pm::World &world)
{
	if (world.viewPlane().samplerState().sampler() == nullptr)
	{
		LOGE("Missing viewplane sampler!");
		exit(EXIT_FAILURE);
	}

	for (const auto &object : world.objects())
	{
		if (object->material() == nullptr)
		{
			LOGE("Missing material!");
			exit(EXIT_FAILURE);
		}
	}

	for (const auto &material : world.materials())
	{
		if (material->type() == pm::Material::Type::MATTE)
		{
			const pm::Matte *matte = static_cast<pm::Matte *>(material.get());
			if (matte->ambient().sampler() == nullptr)
			{
				LOGE("Missing ambient sampler from matte material!");
				exit(EXIT_FAILURE);
			}
			else if (matte->diffuse().sampler() == nullptr)
			{
				LOGE("Missing diffuse sampler from matte material!");
				exit(EXIT_FAILURE);
			}
		}
		else if (material->type() == pm::Material::Type::PHONG)
		{
			const pm::Phong *phong = static_cast<pm::Phong *>(material.get());
			if (phong->ambient().sampler() == nullptr)
			{
				LOGE("Missing ambient sampler from phong material!");
				exit(EXIT_FAILURE);
			}
			else if (phong->diffuse().sampler() == nullptr)
			{
				LOGE("Missing diffuse sampler from phong material!");
				exit(EXIT_FAILURE);
			}
			else if (phong->specular().sampler() == nullptr)
			{
				LOGE("Missing specular sampler from phong material!");
				exit(EXIT_FAILURE);
			}
		}
	}
}

}

void initWorld(pm::World &world, pm::PinHole &camera, pm::Tracer::Type &tracerType)
{
	world.viewPlane().editPixelSize() = 0.004f;

#if CORNELL_BOX
	setupCornellBox(world, camera, tracerType);
#else
	setupSpheres(world, camera, tracerType);
#endif
	validateWorld(world);

	LOGI_X("Scene statistics: %u objects, %u materials, %u lights, %u samplers",
	       world.objects().size(), world.materials().size(), world.lights().size(), world.samplers().size());

#if 0
	//pm::Ortographic camera;
	world.viewPlane().setPixelSize(0.005f);

	pm::PinHole camera;
	camera.setEye(0.0f, 1.0f, -5.0f);
	camera.setUp(0.0f, 1.0f, 0.0f);
	camera.setLookAt(0.0f, 1.0f, 0.0f);
	camera.setViewDistance(4.0f);
	camera.computeUvw();
	camera.setExposureTime(1.0f);
#endif
}
