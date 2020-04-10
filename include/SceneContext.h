#ifndef CLASS_SCENE_CONTEXT
#define CLASS_SCENE_CONTEXT

#include <nctl/UniquePtr.h>
#include <ncine/TimeStamp.h>

#include "ThreadManager.h"

#include "World.h"
#include "Tracer.h"
#include "PinHole.h"
#include "RGBColor.h"

namespace nc = ncine;

/// Scene context class
class SceneContext
{
  public:
	struct Configuration
	{
		// Immediately applied configuration
		bool copyTexture = true;

		// Tracing configuration
#if STD_THREADS
		int maxThreads = static_cast<int>(std::thread::hardware_concurrency());
#else
		int maxThreads = static_cast<int>(nc::Thread::numProcessors());
#endif
		int numThreads = maxThreads - 1;
		int tileSize = 16;

		pm::Tracer::Type tracerType = pm::Tracer::Type::PATHTRACE;
		pm::Camera *camera = nullptr;
	};

	SceneContext()
	    : tracingTime_(0.0f), frameNumPixels_(0) {}

	inline const Configuration &config() const { return config_; }
	inline Configuration &config() { return config_; }

	void init(int width, int height);
	void resizeFrame(int width, int height);
	void startTracing();
	void copyToTexture(unsigned char *pixelsPtr);

	void showSampler(pm::Sampler *sampler);
	void reset();
	inline void stopTracing() { threads_.stop(); }
	inline bool isTracing() const { return threads_.threadsRunning(); }
	inline float tracingProgress() const { return threads_.progress(); }
	float tracingTime() const;
	void savePbm(const char *filename);
	void savePng(const char *filename);

	inline const pm::World &world() const { return world_; }
	inline pm::World &world() { return world_; }

	void setCameraType(pm::Camera::Type type);

  private:
	Configuration config_;
	nc::TimeStamp tracingStartTime_;
	mutable float tracingTime_;
	ThreadManager threads_;

	pm::World world_;
	unsigned int frameNumPixels_;
	nctl::UniquePtr<pm::RGBColor[]> frame_;

	void setupWorld(pm::World &world);
	void setupCornellBox(pm::World &world);
	void validateWorld(const pm::World &world);
};

#endif
