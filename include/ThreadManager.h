#ifndef CLASS_THREADMANAGER
#define CLASS_THREADMANAGER

#define STD_THREADS (0)
#if STD_THREADS
	#include <vector>
	#include <thread>
#else
	#include <nctl/Array.h>
	#include <ncine/Thread.h>
	namespace nc = ncine;
#endif

namespace pm {
	class World;
	class Tracer;
	class Camera;
	class RGBColor;
}

/// Threads management class
class ThreadManager
{
  public:
	struct Configuration
	{
		unsigned int numThreads = 1;
		int tileSize = 16;
		pm::World *world = nullptr;
		pm::Tracer *tracer = nullptr;
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

#if STD_THREADS
	static void threadFunc(int id, Configuration &conf, LocalStorage &tls);
#else
	struct ThreadArg
	{
		int id_;
		const Configuration *conf_;
		LocalStorage *tls_;

		ThreadArg()
		    : id_(-1), conf_(nullptr), tls_(nullptr) {}
		ThreadArg(int id, const Configuration *conf, LocalStorage *tls)
		    : id_(id), conf_(conf), tls_(tls) {}
	};

	static void threadFunc(void *arg);
#endif

	Configuration config_;

#if STD_THREADS
	std::vector<std::thread> threads_;
	std::vector<LocalStorage> tls_;
#else
	nctl::Array<nc::Thread> threads_;
	nctl::Array<ThreadArg> args_;
	nctl::Array<LocalStorage> tls_;
#endif
};

#endif
