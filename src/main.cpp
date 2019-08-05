#include "main.h"
#include "VisualFeedback.h"
#include "UserInterface.h"
#include "SceneContext.h"

#include <ncine/Application.h>
#include <ncine/IFile.h>

namespace {

const unsigned int imageWidth = 1280;
const unsigned int imageHeight = 720;

}

nc::IAppEventHandler *createAppEventHandler()
{
	return new MyEventHandler;
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

	config.xResolution = imageWidth;
	config.yResolution = imageHeight;

	config.withScenegraph = false;
	config.withAudio = false;
	config.withDebugOverlay = false;
	config.withThreads = false;
	config.isResizable = true;
	config.vaoPoolSize = 2;

	config.windowTitle = "ncTracer";

	config.consoleLogLevel = nc::ILogger::LogLevel::WARN;
}

void MyEventHandler::onInit()
{
	vf_ = nctl::makeUnique<VisualFeedback>();
	vf_->initTexture(imageWidth, imageHeight);

	sc_ = nctl::makeUnique<SceneContext>();
	sc_->init(imageWidth, imageHeight);
	sc_->trace();

	ui_ = nctl::makeUnique<UserInterface>(*vf_, *sc_);
}

void MyEventHandler::onShutdown()
{
	sc_->stopTracing();
}

void MyEventHandler::onFrameStart()
{
	const SceneContext::Configuration &conf = sc_->config();

	if (conf.copyTexture)
	{
		sc_->copyToTexture(vf_->texPixels());
		vf_->progressiveUpdate();
	}
	else
		vf_->fixedUpdate();

	ui_->createGuiMainWindow();
}

void MyEventHandler::onKeyReleased(const nc::KeyboardEvent &event)
{
	if (event.sym == nc::KeySym::ESCAPE || event.sym == nc::KeySym::Q)
		nc::theApplication().quit();
}
