#ifndef CLASS_SCENE_CONTEXT
#define CLASS_SCENE_CONTEXT

#include <nctl/UniquePtr.h>
#include <ncine/Timer.h>

#include "ThreadManager.h"

#include "World.h"
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
		int maxThreads = static_cast<int>(std::thread::hardware_concurrency());
		int numThreads = maxThreads - 1;
		int tileSize = 16;
		int maxDepth = 2;

		float cameraEye[3] = { 0.0f, 0.0f, -10.0f };
		float cameraLookAt[3] = { 0.0f, 0.0f, 0.0f };
		float cameraUp[3] = { 0.0f, 1.0f, 0.0f };
		float cameraExposure = 1.0f;
	};

	inline const Configuration &config() const { return config_; }
	inline Configuration &config() { return config_; }

	void init(int width, int height);
	void trace();
	void copyToTexture(unsigned char *pixelsPtr);

	void reset();
	inline void stopTracing() { threads_.stop(); }
	inline bool isTracing() const { return threads_.threadsRunning(); }
	inline float tracingProgress() const { return threads_.progress(); }
	float tracingTime() const;
	void savePbm(const char *filename);

	inline pm::World &world() { return world_; }
	inline pm::Camera &camera() { return camera_; }

  private:
	Configuration config_;
	Configuration currentConfig_;
	nc::Timer timer_;
	mutable float tracingTime_;
	ThreadManager threads_;

	pm::World world_;
	pm::PinHole camera_;
	nctl::UniquePtr<pm::RGBColor[]> frame_;

	void applyConfig();
	void retrieveConfig();

	void setupWorld(pm::World &world);
	void setupCornellBox(pm::World &world);
	void validateWorld(const pm::World &world);
};

#endif
