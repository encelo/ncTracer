#include <fstream>

#include "SceneContext.h"
#include "ObjectsPool.h"

#include <ncine/TextureSaverPng.h>

void initWorld(pm::World &world, pm::PinHole &camera, pm::Tracer::Type &tracerType);

// 0 - single thread, 1 - tiled single thread, 2 - tiled multi-thread
#define THREADING_TYPE (2)

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

void SceneContext::init(int width, int height)
{
	LOGI("Poor Man's Tracer\n");
	world_.viewPlane().setDimensions(width, height);
	resizeFrame(width, height);

	LOGI("Setting up the scene...");
	config_.camera = objectsPool().retrieveCamera(pm::Camera::Type::PINHOLE);
	pm::PinHole *camera = static_cast<pm::PinHole *>(config_.camera);

	initWorld(world_, *camera, config_.tracerType);
}

void SceneContext::resizeFrame(int width, int height)
{
	FATAL_ASSERT(width > 0);
	FATAL_ASSERT(height > 0);

	if (static_cast<unsigned int>(width * height) != frameNumPixels_)
	{
		frameNumPixels_ = width * height;
		frame_ = nctl::makeUnique<pm::RGBColor[]>(frameNumPixels_);
	}
}

void SceneContext::startTracing()
{
	LOGI("Rendering started");
	tracingStartTime_ = nc::TimeStamp::now();

	pm::Tracer *tracer = objectsPool().retrieveTracer(config_.tracerType);
#if THREADING_TYPE == 0
	LOGI(" with one thread...");
	config_.camera->renderScene(world_, *tracer, frame_.get());
#elif THREADING_TYPE == 1
	LOGI(" with one thread (tiled)...");
	for (int i = 0; i < world_.viewPlane().height(); i += config_.tileSize)
		for (int j = 0; j < world_.viewPlane().width(); j += config_.tileSize)
			config_.camera->renderScene(world_, *tracer, frame_.get(), j, i, config_.tileSize);
#elif THREADING_TYPE == 2
	LOGI_X(" with %u threads...", config_.numThreads);

	ThreadManager::Configuration &threadsConfig = threads_.config();
	threadsConfig.numThreads = config_.numThreads;
	threadsConfig.tileSize = config_.tileSize;
	threadsConfig.world = &world_;
	threadsConfig.tracer = objectsPool().retrieveTracer(config_.tracerType);
	threadsConfig.camera = config_.camera;
	threadsConfig.frame = frame_.get();

	threads_.start();
#endif
}

void SceneContext::copyToTexture(unsigned char *pixelsPtr)
{
	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();

	const float invGamma = world_.viewPlane().invGamma();

	for (int r = 0; r < height; r++)
	{
		for (int c = 0; c < width; c++)
		{
			const unsigned int index = r * width + c;
			ASSERT(index < frameNumPixels_);
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
			ASSERT(index < frameNumPixels_);
			frame_[index].set(0.0f, 0.0f, 0.0f);
		}
	}
}

void SceneContext::showSampler(pm::Sampler *sampler)
{
	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();
	const int minDim = (height < width) ? height : width;

	const int halfDiff = (width > minDim) ? (width - minDim) / 2 : (height - minDim) / 2;
	if (width != height)
	{
		for (int i = 0; i < minDim; i++)
		{
			const unsigned int index = (width > minDim) ? halfDiff + i * width : i + halfDiff * width;
			frame_[index].set(1.0f, 0.0f, 1.0f);
			const unsigned int nextIndex = (width > minDim) ? index + minDim - 1 : index + (minDim - 1) * width;
			frame_[nextIndex].set(1.0f, 0.0f, 1.0f);
		}
	}

	static unsigned long jump = 0;
	static int count = 0;
	jump = 0;
	count = 0;
	for (unsigned int i = 0; i < sampler->numSamples(); i++)
	{
		pm::Vector2 vec = sampler->sampleUnitSquare(jump, count);
		const unsigned int index = (width > minDim)
		                           ? halfDiff + vec.x * minDim + static_cast<unsigned int>(vec.y * minDim) * width
		                           : vec.x * minDim + static_cast<unsigned int>(vec.y * minDim + halfDiff) * width;
		ASSERT(index < frameNumPixels_);
		frame_[index].set(1.0f, 1.0f, 1.0f);
	}
}

float SceneContext::tracingTime() const
{
	if (threads_.threadsRunning())
		tracingTime_ = tracingStartTime_.secondsSince();
	return tracingTime_;
}

void SceneContext::savePbm(const char *filename, bool binary)
{
	char magicNumber[3] = {'P', '3', '\0'};
	if (binary)
		magicNumber[1] = '6';

	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();
	const float invGamma = world_.viewPlane().invGamma();

	std::ofstream file;
	file.open(filename);
	file << magicNumber << "\n" << width << " " << height << "\n" << 255 << "\n";
	for (int i = height - 1; i >= 0; i--)
	{
		for (int j = 0; j < width; j++)
		{
			const unsigned int index = static_cast<unsigned int>(i * width + j);
			ASSERT(index < frameNumPixels_);
			const pm::RGBColor &pixel = frame_[index];

			// Tonemapping
			pm::RGBColor tonemapped = pixel * 16.0f;
			tonemapped = tonemapped / (pm::RGBColor(1.0f, 1.0f, 1.0f) + tonemapped);
			tonemapped.pow(invGamma);

			if (binary)
			{
				const char out[3] = { char(tonemapped.r * 255), char(tonemapped.g * 255), char(tonemapped.b * 255) };
				file.write(out, sizeof(out));
			}
			else
			{
				const unsigned int out[3] = { unsigned(tonemapped.r * 255), unsigned(tonemapped.g * 255), unsigned(tonemapped.b * 255) };
				file << out[0] << " " << out[1] << " " << out[2] << " ";
			}
		}
		if (binary == false)
			file << "\n";
	}
	file.close();
}

void SceneContext::savePng(const char *filename)
{
	const int width = world_.viewPlane().width();
	const int height = world_.viewPlane().height();
	const float invGamma = world_.viewPlane().invGamma();

	nctl::UniquePtr<uint8_t[]> intPixels = nctl::makeUnique<uint8_t[]>(width * height * 3);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			const unsigned int index = static_cast<unsigned int>(i * width + j);
			ASSERT(index < frameNumPixels_);
			const pm::RGBColor &pixel = frame_[index];

			// Tonemapping
			pm::RGBColor tonemapped = pixel * 16.0f;
			tonemapped = tonemapped / (pm::RGBColor(1.0f, 1.0f, 1.0f) + tonemapped);
			tonemapped.pow(invGamma);

			// Vertical flipping
			const unsigned int intIndex = static_cast<unsigned int>((height - i - 1) * width + j);
			intPixels[intIndex * 3 + 0] = uint8_t(tonemapped.r * 255);
			intPixels[intIndex * 3 + 1] = uint8_t(tonemapped.g * 255);
			intPixels[intIndex * 3 + 2] = uint8_t(tonemapped.b * 255);
		}
	}

	nc::TextureSaverPng saver;
	nc::TextureSaverPng::Properties props;
	props.width = width;
	props.height = height;
	props.format = nc::TextureSaverPng::Format::RGB8;
	props.pixels = intPixels.get();
	saver.saveToFile(props, filename);
}

void SceneContext::setCameraType(pm::Camera::Type type)
{
	if (type != config_.camera->type())
	{
		pm::Camera *newCamera = objectsPool().retrieveCamera(type);
		newCamera->editEye() = config_.camera->eye();
		newCamera->editLookAt() = config_.camera->lookAt();
		newCamera->editUp() = config_.camera->up();
		newCamera->editExposureTime() = config_.camera->exposureTime();
		config_.camera = newCamera;
	}
}
