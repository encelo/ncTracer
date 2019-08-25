#include "ThreadManager.h"

#if !STD_THREADS
	#include <nctl/String.h>
#endif

#include "World.h"
#include "Camera.h"

namespace {
	bool stopThreads = false;
}

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

void ThreadManager::start()
{
	const unsigned int numThreads = config_.numThreads;

	stopThreads = false;
#if STD_THREADS
	threads_.reserve(numThreads);
	tls_.resize(numThreads);

	for (unsigned int i = 0; i < numThreads; i++)
		threads_.emplace_back(threadFunc, i, std::ref(config_), std::ref(tls_[i]));
#else
	if (threads_.capacity() < numThreads)
		threads_.setCapacity(numThreads);
	if (tls_.capacity() < numThreads)
		tls_.setCapacity(numThreads);
	if (args_.capacity() < numThreads)
		args_.setCapacity(numThreads);

	for (unsigned int i = 0; i < numThreads; i++)
	{
		args_.pushBack(ThreadArg(i, &config_, &tls_[i]));
		threads_[i].run(threadFunc, &args_.back());
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
		threads_[i].setAffinityMask(nc::ThreadAffinityMask(i));
#endif
	}
#endif
}

void ThreadManager::stop()
{
	stopThreads = true;
#if STD_THREADS
	for (unsigned int i = 0; i < threads_.size(); i++)
		threads_[i].join();

	threads_.clear();
#else
	for (unsigned int i = 0; i < threads_.size(); i++)
		threads_[i].join();

	threads_.clear();
	tls_.clear();
	args_.clear();
#endif
}

bool ThreadManager::threadsRunning() const
{
	return !stopThreads && progress() < 1.0f;
}

float ThreadManager::progress(unsigned int threadId) const
{
	if (threadId < tls_.size())
		return tls_[threadId].progress;
	return 0.0f;
}

float ThreadManager::progress() const
{
	const unsigned long numThreads = tls_.size();

	float totalProgress = 0.0f;
	for (unsigned int i = 0; i < numThreads; i++)
		totalProgress += tls_[i].progress;
	totalProgress /= static_cast<float>(numThreads);

	return totalProgress;
}

///////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
///////////////////////////////////////////////////////////

#if STD_THREADS
void ThreadManager::threadFunc(int id, Configuration &conf, LocalStorage &tls)
{
#else
void ThreadManager::threadFunc(void *arg)
{
	ThreadArg *threadArg = reinterpret_cast<ThreadArg *>(arg);
	const int id = threadArg->id_;
	const Configuration &conf = *threadArg->conf_;
	LocalStorage &tls = *threadArg->tls_;

	#if !defined(__EMSCRIPTEN__)
	nctl::String threadName;
	threadName.format("Thread#%2.d", id);
	nc::Thread::setSelfName(threadName.data());
	#endif
#endif
	tls.hasFinished = false;
	tls.progress = 0.0f;

	const int width = conf.world->viewPlane().width();
	const int height = conf.world->viewPlane().height();
	const int maxSamples = conf.world->viewPlane().samplerState().numSamples();

	const int numColumns = (width / conf.tileSize) + 1;
	const int numRows = (height / conf.tileSize) + 1;

	int iteration = 0;
	int sample = 0;

	while (tls.hasFinished == false && stopThreads == false)
	{
		const int index = (iteration * conf.numThreads) + id;
		int column = index % numColumns;
		int row = index / numColumns;

		if (row > numRows - 1)
		{
			iteration = 0;
			sample++;
			if (sample > maxSamples)
			{
				tls.hasFinished = true;
				tls.progress = 1.0f;
				break;
			}
			continue;
		}

		const int startX = column * conf.tileSize;
		const int startY = row * conf.tileSize;
		const int tileSizeX = (startX + conf.tileSize > width) ? width - startX : conf.tileSize;
		const int tileSizeY = (startY + conf.tileSize > height) ? height - startY : conf.tileSize;

		conf.camera->renderScene(*conf.world, *conf.tracer, conf.frame, startX, startY, tileSizeX, tileSizeY, true);
		iteration++;

		tls.progress = 1.0f / static_cast<float>(maxSamples) * (sample + iteration / static_cast<float>(numColumns * numRows));
	}
}
