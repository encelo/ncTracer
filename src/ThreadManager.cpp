#include "ThreadManager.h"

#include "World.h"
#include "Camera.h"

namespace {

bool stopThreads = false;

}

void ThreadManager::start()
{
	const unsigned int numThreads = config_.numThreads;

	stopThreads = false;
	threads_.reserve(numThreads);
	tls_.resize(numThreads);

	for (unsigned int i = 0; i < numThreads; i++)
		threads_.emplace_back(threadFunc, i, std::ref(config_), std::ref(tls_[i]));
}

void ThreadManager::stop()
{
	stopThreads = true;
	for (unsigned int i = 0; i < threads_.size(); i++)
		threads_[i].join();

	threads_.clear();
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

void ThreadManager::threadFunc(int id, Configuration &conf, LocalStorage &tls)
{
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

		conf.camera->renderScene(*conf.world, conf.frame, startX, startY, tileSizeX, tileSizeY, true);
		iteration++;

		tls.progress = 1.0f / static_cast<float>(maxSamples) * (sample + iteration / static_cast<float>(numColumns * numRows));
	}
}
