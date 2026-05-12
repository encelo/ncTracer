#include <ncine/config.h>
#if !NCINE_WITH_IMGUI
	#error nCine must have ImGui integration enabled for this application to work
#endif
#if !NCINE_WITH_LUA
	#error nCine must have Lua integration enabled for this application to work
#endif
#if !NCINE_WITH_THREADS
	#error nCine must have threads integration enabled for this application to work
#endif

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

	config.logging.consoleLevel = nc::ILogger::LogLevel::WARN;

	config.window.resolution.x = imageWidth;
	config.window.resolution.y = imageHeight;
	config.window.resizable = true;
	config.window.title = "ncTracer";
	config.window.iconFilename = "icon48.png";

	config.graphics.opengl.vaoPoolSize = 1;

	config.audio.enabled = false;
	config.jobSystem.enabled = false;
	config.features.scenegraph = false;
	config.features.debugOverlay = false;
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
	if (event.sym == nc::KeySym::ESCAPE)
		nc::theApplication().quit();
}
