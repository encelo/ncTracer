#include <ncine/imgui.h>

#include "UserInterface.h"
#include "VisualFeedback.h"
#include "SceneContext.h"

UserInterface::UserInterface(VisualFeedback &vf, SceneContext &sc)
    : auxString_(256), vf_(vf), sc_(sc),
      textureUploadMode_(vf.textureUploadMode())
{
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

void UserInterface::createGuiMainWindow()
{
	SceneContext::Configuration &conf = sc_.config();

	ImGui::Begin("pmTracer");

	ImGui::Text("Texture size: %d x %d", vf_.texWidth(), vf_.texHeight());
	ImGui::Combo("Texture Upload", &textureUploadMode_, "glTexSubImage2D\0Pixel Buffer Object\0PBO with Mapping\0\0");
	ImGui::SameLine();
	ImGui::Checkbox("", &conf.copyTexture);
	vf_.setTextureUploadMode(textureUploadMode_);

	ImGui::Separator();
	ImGui::SliderInt("Tile Size", &conf.tileSize, 4, 256);
	ImGui::SliderInt("Num Threads", &conf.numThreads, 1, conf.maxThreads);
	ImGui::SliderInt("Max Depth", &conf.maxDepth, 1, 5);

	ImGui::Separator();
	ImGui::InputFloat3("Eye", conf.cameraEye);
	ImGui::InputFloat3("Look At", conf.cameraLookAt);
	ImGui::InputFloat3("Up", conf.cameraUp);
	ImGui::InputFloat("Exposure", &conf.cameraExposure);

	if (sc_.isTracing())
	{
		auxString_.format("%.2f%%", sc_.tracingProgress() * 100.0f);
		ImGui::Separator();
		ImGui::ProgressBar(sc_.tracingProgress(), ImVec2(-1.00f, 0.0f), auxString_.data());
		if (ImGui::Button("Stop"))
			sc_.stopTracing();
	}

	ImGui::Separator();
	ImGui::Text("Time: %.2fs", sc_.tracingTime());
	if (ImGui::Button("Retrace"))
	{
		sc_.reset();
		sc_.stopTracing();
		sc_.trace();
	}

	if (!sc_.isTracing() && ImGui::Button("Save PBM"))
		sc_.savePbm("image.pbm");

	ImGui::End();
}
