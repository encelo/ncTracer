#include "main.h"
#include "VisualFeedback.h"
#include "UserInterface.h"
#include "SceneContext.h"

#include <ncine/Application.h>

namespace {

const unsigned int imageWidth = 1280;
const unsigned int imageHeight = 720;

}

nctl::UniquePtr<nc::IAppEventHandler> createAppEventHandler()
{
	return nctl::makeUnique<MyEventHandler>();
}

void MyEventHandler::onPreInit(nc::AppConfiguration &config)
{
#ifdef __ANDROID__
	const char *extStorage = getenv("EXTERNAL_STORAGE");
	nctl::String dataPath;
	dataPath = extStorage ? extStorage : "/sdcard";
	dataPath += "/ncine/";
	config.dataPath() = dataPath;
#endif

	config.resolution.x = imageWidth;
	config.resolution.y = imageHeight;

	config.withScenegraph = false;
	config.withAudio = false;
	config.withDebugOverlay = false;
	config.withThreads = false;
	config.isResizable = true;
	config.vaoPoolSize = 1;

	config.windowTitle = "ncTracer";
	config.windowIconFilename = "icon48.png";

	config.consoleLogLevel = nc::ILogger::LogLevel::WARN;
}

void MyEventHandler::onInit()
{
	vf_ = nctl::makeUnique<VisualFeedback>();
	vf_->initTexture(imageWidth, imageHeight);

	sc_ = nctl::makeUnique<SceneContext>();
	sc_->init(imageWidth, imageHeight);

	ui_ = nctl::makeUnique<UserInterface>(*vf_, *sc_);
}

void MyEventHandler::onShutdown()
{
	sc_->stopTracing();
}

void MyEventHandler::onFrameStart()
{
	const VisualFeedback::Configuration &vfConf = vf_->config();

	if (vfConf.progressiveCopy)
		sc_->copyToTexture(vf_->texPixels());

	vf_->update();
	ui_->createGuiMainWindow();
	ui_->cameraInteraction();
}

void MyEventHandler::onKeyReleased(const nc::KeyboardEvent &event)
{
	if (event.sym == nc::KeySym::ESCAPE || event.sym == nc::KeySym::Q)
		nc::theApplication().quit();
}
