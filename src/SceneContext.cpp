#include <iostream>
#include <fstream>

#include "SceneContext.h"

#include "Ortographic.h"
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
#include "AreaLighting.h"
#include "AreaLight.h"
#include "Emissive.h"
#include "Rectangle.h"
#include "PathTrace.h"
#include "EnvironmentLight.h"
#include "GlobalTrace.h"

#define SINGLE_THREAD (0)
#define SINGLE_THREAD_TILED (0)
#define MULTI_THREAD_TILED (1)

#define AMBIENT (1)
#define AMBIENT_OCCLUSION (0)

#define POINT_LIGHTS (1)
#define AREA_LIGHTS (0)
#define PATH_TRACE (0)

#include <ncine/common_macros.h>

namespace {

const unsigned int numSamples = 4;

}

void SceneContext::init(int width, int height)
{
	LOGI("Poor Man's Tracer\n");

	LOGI("Setting up the scene...");
	world_.viewPlane().setDimensions(width, height);
	//setupWorld(world_);
	setupCornellBox(world_);
	validateWorld(world_);

	//std::cout << "Scene setting time: ";
	//std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms\n" << std::flush;

	LOGI_X("Scene statistics: %u objects, %u materials, %u lights, %u samplers",
	       world_.objects().size(), world_.materials().size(), world_.lights().size(), world_.samplers().size());

	world_.viewPlane().setPixelSize(0.004f);

	camera_.setEye(278.0f, 273.0f, -800.0f);
	camera_.setUp(0.0f, 1.0f, 0.0f);
	camera_.setLookAt(278.0f, 273.0f, 0.0f);
	camera_.setViewDistance(4.0f);
	camera_.computeUvw();

	/*
		//pm::Ortographic camera;
		world.viewPlane().setPixelSize(0.005f);

		pm::PinHole camera;
		camera.setEye(0.0f, 1.0f, -5.0f);
		camera.setUp(0.0f, 1.0f, 0.0f);
		camera.setLookAt(0.0f, 1.0f, 0.0f);
		camera.setViewDistance(4.0f);
		camera.computeUvw();
		camera.setExposureTime(1.0f);
	*/

	frame_ = nctl::makeUnique<pm::RGBColor[]>(width * height);

	retrieveConfig();
}

void SceneContext::trace()
{
	applyConfig();

	LOGI("Rendering started");
	timer_.start();

#if SINGLE_THREAD
	LOGI(" with one thread...");
	camera_.renderScene(world_, frame_.get());
#elif SINGLE_THREAD_TILED
	LOGI(" with one thread (tiled)...");
	for (int i = 0; i < height; i += tileSize)
		for (int j = 0; j < width; j += tileSize)
			camera_.renderScene(world_, frame_.get(), j, i, tileSize);
#elif MULTI_THREAD_TILED
	LOGI_X(" with %u threads...", config_.numThreads);

	ThreadManager::Configuration &threadsConfig = threads_.config();
	threadsConfig.numThreads = config_.numThreads;
	threadsConfig.tileSize = config_.tileSize;
	threadsConfig.world = &world_;
	threadsConfig.camera = &camera_;
	threadsConfig.frame = frame_.get();

	threads_.start();
#endif

	//LOGI_X("Rendering time: %f.2s", timer_.interval());

	// Save output to PBM ascii format
	//startTime = std::chrono::high_resolution_clock::now();
	//savePbm("image.pbm", frame_.get());
	//endTime = std::chrono::high_resolution_clock::now();
	//std::cout << "File save time: ";
	//std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms\n" << std::flush;
}

void SceneContext::copyToTexture(unsigned char *pixelsPtr)
{
	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();

	const float invGamma = 1.0f / 2.2f;

	for (int r = 0; r < height; r++)
	{
		for (int c = 0; c < width; c++)
		{
			const unsigned int index = r * width + c;
			const pm::RGBColor &pixel = frame_[index];

			// Tonemapping
			pm::RGBColor tonemapped = pixel * 16.0f;
			tonemapped = tonemapped / (pm::RGBColor(1.0f, 1.0f, 1.0f) + tonemapped);
			tonemapped.pow(invGamma);

			pixelsPtr[index * 3 + 0] = static_cast<unsigned char>(tonemapped.r * 255.0f);
			pixelsPtr[index * 3 + 1] = static_cast<unsigned char>(tonemapped.g * 255.0f);
			pixelsPtr[index * 3 + 2] = static_cast<unsigned char>(tonemapped.b * 255.0f);
		}
	}
}

void SceneContext::reset()
{
	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();

	for (int r = 0; r < height; r++)
	{
		for (int c = 0; c < width; c++)
		{
			const unsigned int index = r * width + c;
			frame_[index].set(0.0f, 0.0f, 0.0f);
		}
	}
}

float SceneContext::tracingTime() const
{
	if (threads_.threadsRunning())
		tracingTime_ = timer_.interval();
	return tracingTime_;
}

void SceneContext::savePbm(const char *filename)
{
	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();
	const float invGamma = 1.0f / 2.2f;

	std::ofstream file;
	file.open(filename);
	file << "P3\n" << width << " " << height << "\n" << 255 << "\n";
	for (int i = height - 1; i >= 0; i--)
	{
		for (int j = 0; j < width; j++)
		{
			const pm::RGBColor &pixel = frame_[i * width + j];

			// Tonemapping
			pm::RGBColor tonemapped = pixel * 16.0f;
			tonemapped = tonemapped / (pm::RGBColor(1.0f, 1.0f, 1.0f) + tonemapped);
			tonemapped.pow(invGamma);

			file << unsigned(tonemapped.r * 255) << " ";
			file << unsigned(tonemapped.g * 255) << " ";
			file << unsigned(tonemapped.b * 255) << " ";
		}
		file << "\n";
	}
	file.close();
}

void SceneContext::applyConfig()
{
	world_.viewPlane().setMaxDepth(config_.maxDepth);
	camera_.eye().set(config_.cameraEye);
	camera_.lookAt().set(config_.cameraLookAt);
	camera_.up().set(config_.cameraUp);
	camera_.exposureTime() = config_.cameraExposure;
	camera_.computeUvw();

	currentConfig_ = config_;
}

void SceneContext::retrieveConfig()
{
	config_.maxDepth = world_.viewPlane().maxDepth();

	config_.cameraEye[0] = camera_.eye().x;
	config_.cameraEye[1] = camera_.eye().y;
	config_.cameraEye[2] = camera_.eye().z;

	config_.cameraLookAt[0] = camera_.lookAt().x;
	config_.cameraLookAt[1] = camera_.lookAt().y;
	config_.cameraLookAt[2] = camera_.lookAt().z;

	config_.cameraUp[0] = camera_.up().x;
	config_.cameraUp[1] = camera_.up().y;
	config_.cameraUp[2] = camera_.up().z;

	config_.cameraExposure = camera_.exposureTime();

	currentConfig_ = config_;
}

std::unique_ptr<pm::Rectangle> rectangleFromVertices(const pm::Vector3 pA, const pm::Vector3 pB, const pm::Vector3 pC)
{
	auto rect = std::make_unique<pm::Rectangle>(pA, pB - pA, pC - pA, cross(pB - pA, pC - pA).normalize());
	return rect;
}

void SceneContext::setupWorld(pm::World &world)
{
	world.setTracer(std::make_unique<pm::AreaLighting>(world));
	//world.setTracer(std::make_unique<pm::PathTrace>(world));

	auto vpSampler = world.createSampler<pm::NRooks>(numSamples);
	world.viewPlane().setSampler(vpSampler);
	world.viewPlane().setMaxDepth(5);

	auto hammersley = world.createSampler<pm::NRooks>(numSamples);

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

void SceneContext::setupCornellBox(pm::World &world)
{
	world.setTracer(std::make_unique<pm::PathTrace>(world));

	auto vpSampler = world.createSampler<pm::MultiJittered>(64);
	world.viewPlane().setSampler(vpSampler);
	world.viewPlane().setMaxDepth(5);

	auto hammersley = world.createSampler<pm::Hammersley>(256);

	// Materials
	auto white = world.createMaterial<pm::Matte>();
	white->setCd(0.7f, 0.7f, 0.7f);
	white->diffuse().setSampler(hammersley);

	auto red = world.createMaterial<pm::Matte>();
	red->setCd(0.7f, 0.0f, 0.0f);
	red->diffuse().setSampler(hammersley);

	auto green = world.createMaterial<pm::Matte>();
	green->setCd(0.0f, 0.7f, 0.0f);
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

void SceneContext::validateWorld(const pm::World &world)
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
}
