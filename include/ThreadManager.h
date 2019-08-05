#ifndef CLASS_THREADS
#define CLASS_THREADS

#include <ncine/Timer.h>

#include <vector>
#include <thread>

#include "World.h"
#include "PinHole.h"
#include "RGBColor.h"

namespace nc = ncine;

/// Threads management class
class ThreadManager
{
  public:
	struct Configuration
	{
		unsigned int numThreads = 1;
		int tileSize = 16;
		pm::World *world = nullptr;
		pm::Camera *camera = nullptr;
		pm::RGBColor *frame = nullptr;
	};

	inline const Configuration &config() const { return config_; }
	inline Configuration &config() { return config_; }

	void start();
	void stop();
	bool threadsRunning() const;

	float progress(unsigned int threadId) const;
	float progress() const;

  private:
	struct LocalStorage
	{
		int hasFinished = false;
		float progress = 0.0f;
	};

	static void threadFunc(int id, Configuration &conf, LocalStorage &tls);

	Configuration config_;
	nc::Timer timer_;
	std::vector<std::thread> threads_;
	std::vector<LocalStorage> tls_;
};

#endif
