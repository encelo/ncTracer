#include "main.h"
#include "VisualFeedback.h"
#include "UserInterface.h"
#include <ncine/Application.h>
#include <ncine/IFile.h>

namespace {

const unsigned int imageWidth = 1920;
const unsigned int imageHeight = 1080;

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
	config.withInfoText = false;
	config.withProfilerGraphs = false;
	config.withThreads = false;
	config.isResizable = true;
	config.vaoPoolSize = 2;
}

void MyEventHandler::onInit()
{
	vf_ = nctl::makeUnique<VisualFeedback>();
	vf_->initTexture(imageWidth, imageHeight);
	ui_ = nctl::makeUnique<UserInterface>(*vf_);
}

void MyEventHandler::onFrameStart()
{
	ui_->createGuiMainWindow();
	vf_->update();
}

void MyEventHandler::onKeyReleased(const nc::KeyboardEvent &event)
{
	if (event.sym == nc::KeySym::ESCAPE || event.sym == nc::KeySym::Q)
		nc::theApplication().quit();
}
