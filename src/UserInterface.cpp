#include <ncine/imgui.h>

#include "UserInterface.h"
#include "VisualFeedback.h"
#include "SceneContext.h"
#include "LuaSerializer.h"

#include "Plane.h"
#include "Sphere.h"
#include "Rectangle.h"

#include "Matte.h"
#include "Phong.h"
#include "Emissive.h"

#include "Directional.h"
#include "PointLight.h"
#include "Ambient.h"
#include "AmbientOccluder.h"
#include "AreaLight.h"
#include "EnvironmentLight.h"

#include "Regular.h"
#include "PureRandom.h"
#include "Jittered.h"
#include "MultiJittered.h"
#include "NRooks.h"
#include "Hammersley.h"
#include "Halton.h"

namespace {

bool windowHovered = false;

class CameraState
{
  public:
	CameraState()
	    : exposureTime(1.0f), viewDistance(1.0f), zoom(1.0f) {}

	void loadTo(pm::Camera &camera) const
	{
		camera.editEye() = eye;
		camera.editLookAt() = lookAt;
		camera.editUp() = up;
		camera.editExposureTime() = exposureTime;

		if (camera.type() == pm::Camera::Type::PINHOLE)
		{
			pm::PinHole &pinhole = static_cast<pm::PinHole &>(camera);
			pinhole.editViewDistance() = viewDistance;
			pinhole.editZoom() = zoom;
		}
	}

	void saveFrom(const pm::Camera &camera)
	{
		eye = camera.eye();
		lookAt = camera.lookAt();
		up = camera.up();
		exposureTime = camera.exposureTime();

		if (camera.type() == pm::Camera::Type::PINHOLE)
		{
			const pm::PinHole &pinhole = static_cast<const pm::PinHole &>(camera);
			viewDistance = pinhole.viewDistance();
			zoom = pinhole.zoom();
		}
	}

  private:
	pm::Vector3 eye;
	pm::Vector3 lookAt;
	pm::Vector3 up;

	float exposureTime;
	float viewDistance;
	float zoom;
} cameraState;

int inputTextCallback(ImGuiInputTextCallbackData *data)
{
	nctl::String *string = reinterpret_cast<nctl::String *>(data->UserData);
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		// Resize string callback
		ASSERT(data->Buf == string->data());
		string->setLength(data->BufTextLen);
		data->Buf = string->data();
	}
	return 0;
}

const char *geometryTypeToString(pm::Geometry::Type type)
{
	switch (type)
	{
		case pm::Geometry::Type::PLANE: return "Plane";
		case pm::Geometry::Type::SPHERE: return "Sphere";
		case pm::Geometry::Type::RECTANGLE: return "Rectangle";
	}
	return "Unknown";
}

const char *materialTypeToString(pm::Material::Type type)
{
	switch (type)
	{
		case pm::Material::Type::MATTE: return "Matte";
		case pm::Material::Type::PHONG: return "Phong";
		case pm::Material::Type::EMISSIVE: return "Emissive";
	}
	return "Unknown";
}

const char *lightTypeToString(pm::Light::Type type)
{
	switch (type)
	{
		case pm::Light::Type::DIRECTIONAL: return "Directional";
		case pm::Light::Type::POINT: return "Point";
		case pm::Light::Type::AMBIENT: return "Ambient";
		case pm::Light::Type::AMBIENT_OCCLUDER: return "Ambient Occluder";
		case pm::Light::Type::AREA: return "Area";
		case pm::Light::Type::ENVIRONMENT: return "Environment";
	}
	return "Unknown";
}

const char *samplerTypeToString(pm::Sampler::Type type)
{
	switch (type)
	{
		case pm::Sampler::Type::REGULAR: return "Regular";
		case pm::Sampler::Type::PURE_RANDOM: return "Pure Random";
		case pm::Sampler::Type::JITTERED: return "Jittered";
		case pm::Sampler::Type::MULTI_JITTERED: return "Multi Jittered";
		case pm::Sampler::Type::NROOKS: return "NRooks";
		case pm::Sampler::Type::HAMMERSLEY: return "Hammersley";
		case pm::Sampler::Type::HALTON: return "Halton";
	}
	return "Unknown";
}

}

///////////////////////////////////////////////////////////
// CONSTRUCTORS and DESTRUCTOR
///////////////////////////////////////////////////////////

UserInterface::UserInterface(VisualFeedback &vf, SceneContext &sc)
    : auxString_(MaxStringLength), filename_(MaxStringLength), vf_(vf), sc_(sc)
{
	filename_ = "world.lua";

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#ifdef __ANDROID__
	io.FontGlobalScale = 2.0f;
#endif

	cameraState.saveFrom(*sc_.config().camera);
}

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

void UserInterface::createGuiMainWindow()
{
	SceneContext::Configuration &scConf = sc_.config();
	VisualFeedback::Configuration &vfConf = vf_.config();

	ImGui::Begin("ncTracer");
	windowHovered = ImGui::IsItemHovered();

	if (ImGui::CollapsingHeader("Texture"))
	{
		ImGui::Text("Size: %d x %d", vf_.texWidth(), vf_.texHeight());
		ImGui::Combo("Upload Mode", &vfConf.textureUploadMode, "glTexSubImage2D\0Pixel Buffer Object\0PBO with Mapping\0\0");
		ImGui::SliderFloat("Delay", &vfConf.textureCopyDelay, 0.0f, 60.0f, "%.1f");
		ImGui::SameLine();
		ImGui::Checkbox("Enabled", &vfConf.progressiveCopy);
	}

	if (ImGui::CollapsingHeader("Performances"))
	{
		ImGui::SliderInt("Tile Size", &scConf.tileSize, 4, 256);
		ImGui::SliderInt("Num Threads", &scConf.numThreads, 1, scConf.maxThreads);

		const char *tracerItems[] = { "RayCast", "Whitted", "AreaLighting", "PathTrace", "GlobalTrace" };
		static int currentTracer = static_cast<int>(scConf.tracerType);
		ImGui::Combo("Tracer Type", &currentTracer, tracerItems, IM_ARRAYSIZE(tracerItems));
		scConf.tracerType = static_cast<pm::Tracer::Type>(currentTracer);
	}

	if (ImGui::CollapsingHeader("View Plane"))
	{
		pm::ViewPlane &viewPlane = sc_.world().viewPlane();

		static int width = viewPlane.width();
		ImGui::InputInt("Width", &width);
		static int height = viewPlane.height();
		ImGui::InputInt("Height", &height);
		if (ImGui::Button("Apply"))
		{
			viewPlane.setDimensions(width, height);
			sc_.resizeFrame(width, height);
			vf_.resizeTexture(width, height);
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			width = viewPlane.width();
			height = viewPlane.height();
		}
		ImGui::SliderFloat("Pixel Size", &viewPlane.editPixelSize(), 0.0001f, 0.1f);
		static float gamma = viewPlane.gamma();
		ImGui::SliderFloat("Gamma", &gamma, 1.9f, 2.5f);
		viewPlane.setGamma(gamma);
		const pm::Tracer::Type tracerType = sc_.config().tracerType;
		if (tracerType != pm::Tracer::Type::RAYCAST && tracerType != pm::Tracer::Type::AREALIGHTING)
			ImGui::SliderInt("Max Depth", &viewPlane.editMaxDepth(), 1, 5);

		const pm::Sampler::Type samplerType = viewPlane.sampler()->type();
		auxString_.format("%s Sampler", samplerTypeToString(samplerType));
		createSamplerGuiTree(viewPlane.sampler());
	}

	if (ImGui::CollapsingHeader("Camera"))
	{
		const char *cameraItems[] = { "Ortographic", "Pin Hole" };
		static int currentCamera = static_cast<int>(scConf.camera->type());
		ImGui::Combo("Type", &currentCamera, cameraItems, IM_ARRAYSIZE(cameraItems));
		sc_.setCameraType(static_cast<pm::Camera::Type>(currentCamera));

		ImGui::InputFloat3("Eye", scConf.camera->editEye().data());
		ImGui::InputFloat3("Look At", scConf.camera->editLookAt().data());
		ImGui::InputFloat3("Up", scConf.camera->editUp().data());
		ImGui::InputFloat("Exposure", &scConf.camera->editExposureTime());
		if (scConf.camera->type() == pm::Camera::Type::PINHOLE)
		{
			pm::PinHole *pinhole = static_cast<pm::PinHole *>(scConf.camera);
			ImGui::InputFloat("View Distance", &pinhole->editViewDistance());
			ImGui::InputFloat("Zoom", &pinhole->editZoom());
		}

		ImGui::NewLine();
		if (ImGui::Button("Quick Load##Camera"))
		{
			cameraState.loadTo(*scConf.camera);
			onCameraChanged();
		}
		ImGui::SameLine();
		if (ImGui::Button("Quick Save##Camera"))
			cameraState.saveFrom(*scConf.camera);

		ImGui::NewLine();
		ImGui::Checkbox("Input Interactions", &scConf.camInteraction.enabled);
		ImGui::SliderFloat("Keyboard Speed", &scConf.camInteraction.keyboardSpeed, 0.5f, 20.0f);
		ImGui::SliderFloat("Mouse Speed", &scConf.camInteraction.mouseSpeed, 0.125f, 5.0f);

		scConf.camera->computeUvw();
	}

	if (ImGui::CollapsingHeader("World"))
	{
		pm::World &world = sc_.world();

		ImGui::InputText("Filename", filename_.data(), MaxStringLength,
		                 ImGuiInputTextFlags_CallbackResize, inputTextCallback, &filename_);

		if (ImGui::Button("Load"))
		{
			LuaSerializer::load(filename_.data(), world);
			const int width = world.viewPlane().width();
			const int height = world.viewPlane().height();
			sc_.resizeFrame(width, height);
			vf_.resizeTexture(width, height);
		}
		ImGui::SameLine();
		if (ImGui::Button("Save"))
			LuaSerializer::save(filename_.data(), world);
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			if (sc_.isTracing())
				sc_.stopTracing();
			world.clear();
		}

		ImGui::ColorEdit3("Background", world.editBackground().data());

		std::vector<std::unique_ptr<pm::Sampler>> &samplers = world.samplers();
		auxString_.format("Samplers (#%d)", samplers.size());
		if (ImGui::TreeNode(auxString_.data()))
		{
			int index = 0;
			for (auto it = samplers.begin(); it != samplers.end(); ++it)
			{
				const pm::Sampler::Type samplerType = (*it)->type();
				auxString_.format("Sampler #%d - %s", index, samplerTypeToString(samplerType));
				createSamplerGuiTree((*it).get());
				index++;
			}
			ImGui::TreePop();
		}

		std::vector<std::unique_ptr<pm::Material>> &materials = world.materials();
		auxString_.format("Materials (#%d)", materials.size());
		if (ImGui::TreeNode(auxString_.data()))
		{
			int index = 0;
			for (auto it = materials.begin(); it != materials.end(); ++it)
			{
				const pm::Material::Type materialType = (*it)->type();
				auxString_.format("Material #%d - %s", index, materialTypeToString(materialType));
				ImGui::PushID(auxString_.data());
				createMaterialGuiTree((*it).get());
				ImGui::PopID();
				index++;
			}
			ImGui::TreePop();
		}

		std::vector<std::unique_ptr<pm::Light>> &lights = world.lights();
		auxString_.format("Lights (#%d)", lights.size());
		if (ImGui::TreeNode(auxString_.data()))
		{
			int index = 0;
			for (auto it = lights.begin(); it != lights.end(); ++it)
			{
				const pm::Light::Type lightType = (*it)->type();
				auxString_.format("Light #%d - %s", index, lightTypeToString(lightType));

				ImGui::PushID(auxString_.data());
				const bool lightTreeOpen = createLightGuiTree((*it).get());
				ImGui::PopID();

				if (lightTreeOpen == false)
				{
					ImGui::SameLine();
					auxString_.format("Delete###Light#%d", index);
					if (ImGui::Button(auxString_.data()))
					{
						it = lights.erase(it);
						break;
					}
				}
				index++;
			}
			ImGui::TreePop();
		}

#if 0
		const char* lightItems[] = {"Directional", "Point", "Ambient", "Ambient Occluder", "Area", "Environment"};
		static int currentLight = 0;
		ImGui::Combo("Type###Light", &currentLight, lightItems, IM_ARRAYSIZE(lightItems));
		ImGui::SameLine();
		if (ImGui::Button("Add Light"))
		{
			std::unique_ptr<pm::Light> light;
			const pm::Light::Type lightType = static_cast<pm::Light::Type>(currentLight);
			switch (lightType)
			{
				case pm::Light::Type::DIRECTIONAL:
					light = std::make_unique<pm::Directional>();
					break;
				case pm::Light::Type::POINT:
					light = std::make_unique<pm::PointLight>();
					break;
				case pm::Light::Type::AMBIENT:
					light = std::make_unique<pm::Ambient>();
					break;
				case pm::Light::Type::AMBIENT_OCCLUDER:
					light = std::make_unique<pm::AmbientOccluder>();
					break;
				case pm::Light::Type::AREA:
					//light = std::make_unique<pm::AreaLight>();
					break;
				case pm::Light::Type::ENVIRONMENT:
					//light = std::make_unique<pm::EnvironmentLight>(0.0f, 0.0f, 0.0f);
					break;
			}
			if (light)
				world.addLight(std::move(light));
		}
#endif

		std::vector<std::unique_ptr<pm::Geometry>> &objects = world.objects();
		auxString_.format("Objects (#%d)", objects.size());
		if (ImGui::TreeNode(auxString_.data()))
		{
			int index = 0;
			for (auto it = objects.begin(); it != objects.end(); ++it)
			{
				const pm::Geometry::Type objectType = (*it)->type();
				pm::Material *material = (*it)->material();
				const pm::Material::Type materialType = material->type();
				auxString_.format("Object #%d - (%s%s)", index, (materialType == pm::Material::Type::EMISSIVE) ? "Emissive " : "", geometryTypeToString(objectType));

				ImGui::PushID(auxString_.data());
				const bool objectTreeOpen = createObjectGuiTree((*it).get());
				ImGui::PopID();

				if (objectTreeOpen == false)
				{
					ImGui::SameLine();
					auxString_.format("Delete###Object#%d", index);
					if (ImGui::Button(auxString_.data()))
					{
						it = objects.erase(it);
						break;
					}
				}

				index++;
			}
			ImGui::TreePop();
		}

#if 0
		const char* objectItems[] = {"Plane", "Sphere", "Rectangle"};
		static int currentObject = 0;
		ImGui::Combo("Type###Object", &currentObject, objectItems, IM_ARRAYSIZE(objectItems));
		ImGui::SameLine();
		if (ImGui::Button("Add Object"))
		{
			std::unique_ptr<pm::Geometry> object;
			const pm::Geometry::Type objectType = static_cast<pm::Geometry::Type>(currentObject);
			switch (objectType)
			{
				case pm::Geometry::Type::PLANE:
					object = std::make_unique<pm::Plane>();
					break;
				case pm::Geometry::Type::SPHERE:
					object = std::make_unique<pm::Sphere>();
					break;
				case pm::Geometry::Type::RECTANGLE:
					object = std::make_unique<pm::Rectangle>();
					break;
			}
			if (object)
				world.addObject(std::move(object));
		}
#endif
	}

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
	if (ImGui::Button("Trace"))
	{
		vf_.startTimer();
		sc_.reset();
		sc_.stopTracing();
		sc_.startTracing();
	}

	if (!sc_.isTracing())
	{
		if (ImGui::Button("Save PBM"))
			sc_.savePbm("image.pbm", true);
		ImGui::SameLine();
		if (ImGui::Button("Save PNG"))
			sc_.savePng("image.png");
	}

	ImGui::End();
}

void UserInterface::cameraInteraction()
{
	const SceneContext::Configuration::CameraInteraction interaction = sc_.config().camInteraction;
	if (windowHovered || interaction.enabled == false)
		return;

	const float deltaTime = ImGui::GetIO().DeltaTime;
	pm::Camera *camera = sc_.config().camera;

	const pm::Vector3 forward = (camera->lookAt() - camera->eye()).normalized();
	const pm::Vector3 right = cross(camera->up(), forward);
	bool cameraChanged = false;

	if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
	{
		camera->editEye() += forward * interaction.keyboardSpeed * deltaTime;
		camera->editLookAt() += forward * interaction.keyboardSpeed * deltaTime;
		cameraChanged = true;
	}
	if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
	{
		camera->editEye() -= forward * interaction.keyboardSpeed * deltaTime;
		camera->editLookAt() -= forward * interaction.keyboardSpeed * deltaTime;
		cameraChanged = true;
	}
	if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
	{
		camera->editEye() += right * interaction.keyboardSpeed * deltaTime;
		camera->editLookAt() += right * interaction.keyboardSpeed * deltaTime;
		cameraChanged = true;
	}
	if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
	{
		camera->editEye() -= right * interaction.keyboardSpeed * deltaTime;
		camera->editLookAt() -= right * interaction.keyboardSpeed * deltaTime;
		cameraChanged = true;
	}

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		const ImVec2 mouseDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		ImGui::ResetMouseDragDelta();
		camera->editLookAt().x += mouseDelta.x * interaction.mouseSpeed * deltaTime;
		camera->editLookAt().y += mouseDelta.y * interaction.mouseSpeed * deltaTime;
		cameraChanged = true;
	}

	if (cameraChanged)
		onCameraChanged();
}

///////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
///////////////////////////////////////////////////////////

void UserInterface::createSamplerGuiTree(pm::Sampler *sampler)
{
	ASSERT(sampler);

	ImGui::PushID(reinterpret_cast<const void *>(sampler));

	if (ImGui::TreeNode(auxString_.data()))
	{
		int numSamples = sampler->numSamples();
		ImGui::PushItemWidth(100.0f);
		if (ImGui::InputInt("Samples", &numSamples))
		{
			if (numSamples < 1)
				numSamples != sampler->numSamples();

			if (numSamples > 0 && numSamples != sampler->numSamples())
				sampler->resize(numSamples);
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("Show"))
		{
			sc_.stopTracing();
			sc_.reset();
			sc_.showSampler(sampler);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

bool UserInterface::createLightGuiTree(pm::Light *light)
{
	ASSERT(light);

	if (ImGui::TreeNode(auxString_.data()))
	{
		const pm::Light::Type lightType = light->type();
		ImGui::Checkbox("Cast Shadow", &light->editCastShadows());
		switch (lightType)
		{
			case pm::Light::Type::DIRECTIONAL:
			{
				pm::Directional *directional = static_cast<pm::Directional *>(light);
				ImGui::InputFloat("Radiance Scale", &directional->editRadianceScale());
				ImGui::ColorEdit3("Color", directional->editColor().data());
				ImGui::InputFloat3("Direction", directional->editDirection().data());
				break;
			}
			case pm::Light::Type::POINT:
			{
				pm::PointLight *pointLight = static_cast<pm::PointLight *>(light);
				ImGui::InputFloat("Radiance Scale", &pointLight->editRadianceScale());
				ImGui::ColorEdit3("Color", pointLight->editColor().data());
				ImGui::InputFloat3("Location", pointLight->editLocation().data());
				break;
			}
			case pm::Light::Type::AMBIENT:
			{
				pm::Ambient *ambient = static_cast<pm::Ambient *>(light);
				ImGui::InputFloat("Radiance Scale", &ambient->editRadianceScale());
				ImGui::ColorEdit3("Color", ambient->editColor().data());
				break;
			}
			case pm::Light::Type::AMBIENT_OCCLUDER:
			{
				pm::AmbientOccluder *ambientOccluder = static_cast<pm::AmbientOccluder *>(light);
				ImGui::InputFloat("Radiance Scale", &ambientOccluder->editRadianceScale());
				ImGui::ColorEdit3("Color", ambientOccluder->editColor().data());
				ImGui::ColorEdit3("Minimum Amount", ambientOccluder->editMinAmount().data());
				break;
			}
			case pm::Light::Type::AREA:
			{
				pm::AreaLight *areaLight = static_cast<pm::AreaLight *>(light);
				pm::Geometry *object = &areaLight->object();
				const pm::Geometry::Type objectType = object->type();
				auxString_.format("Emissive (%s)", geometryTypeToString(objectType));
				createObjectGuiTree(object);
				break;
			}
			case pm::Light::Type::ENVIRONMENT:
			{
				pm::EnvironmentLight *environmentLight = static_cast<pm::EnvironmentLight *>(light);
				pm::Material *material = &environmentLight->material();
				const pm::Material::Type materialType = material->type();
				auxString_.format("%s Material", materialTypeToString(materialType));
				createMaterialGuiTree(material);
				break;
			}
		}
		ImGui::TreePop();
		return true;
	}

	return false;
}

bool UserInterface::createObjectGuiTree(pm::Geometry *object)
{
	ASSERT(object);

	if (ImGui::TreeNode(auxString_.data()))
	{
		const pm::Geometry::Type objectType = object->type();
		pm::Material *material = object->material();
		ImGui::Checkbox("Cast Shadow", &object->editCastShadows());

		switch (objectType)
		{
			case pm::Geometry::Type::PLANE:
			{
				pm::Plane *plane = static_cast<pm::Plane *>(object);
				ImGui::InputFloat3("Point", plane->editPoint().data());
				ImGui::InputFloat3("Normal", plane->editNormal().data());
				break;
			}
			case pm::Geometry::Type::SPHERE:
			{
				pm::Sphere *sphere = static_cast<pm::Sphere *>(object);
				ImGui::InputFloat3("Center", sphere->editCenter().data());
				ImGui::InputFloat("Radius", &sphere->editRadius());
				break;
			}
			case pm::Geometry::Type::RECTANGLE:
			{
				pm::Rectangle *rectangle = static_cast<pm::Rectangle *>(object);
				ImGui::InputFloat3("Point", rectangle->editPoint().data());
				ImGui::InputFloat3("Side A", rectangle->editSideA().data());
				ImGui::InputFloat3("Side B", rectangle->editSideB().data());
				ImGui::InputFloat3("Normal", rectangle->editNormal().data());
				rectangle->updateDimensions();
				break;
			}
		}

		auxString_.format("%s Material", materialTypeToString(material->type()));
		createMaterialGuiTree(material);
		ImGui::TreePop();
		return true;
	}

	return false;
}

bool UserInterface::createMaterialGuiTree(pm::Material *material)
{
	ASSERT(material);

	if (ImGui::TreeNode(auxString_.data()))
	{
		const pm::Material::Type materialType = material->type();
		switch (materialType)
		{
			case pm::Material::Type::MATTE:
			{
				ImGui::PushID("Ambient");
				pm::Matte *matte = static_cast<pm::Matte *>(material);
				ImGui::InputFloat("Ambient Kd", &matte->ambient().editKd());
				ImGui::ColorEdit3("Ambient Cd", matte->ambient().editCd().data());
				if (matte->ambient().sampler())
				{
					const pm::Sampler::Type ambientSamplerType = matte->ambient().sampler()->type();
					auxString_.format("Ambient: %s Sampler", samplerTypeToString(ambientSamplerType));
					createSamplerGuiTree(matte->ambient().sampler());
				}
				ImGui::PopID();

				ImGui::PushID("Diffuse");
				ImGui::InputFloat("Diffuse Kd", &matte->diffuse().editKd());
				ImGui::ColorEdit3("Diffuse Cd", matte->diffuse().editCd().data());
				if (matte->diffuse().sampler())
				{
					const pm::Sampler::Type diffuseSamplerType = matte->diffuse().sampler()->type();
					auxString_.format("Diffuse: %s Sampler", samplerTypeToString(diffuseSamplerType));
					createSamplerGuiTree(matte->diffuse().sampler());
				}
				ImGui::PopID();
				break;
			}
			case pm::Material::Type::PHONG:
			{
				ImGui::PushID("Ambient");
				pm::Phong *phong = static_cast<pm::Phong *>(material);
				ImGui::InputFloat("Ambient Kd", &phong->ambient().editKd());
				ImGui::ColorEdit3("Ambient Cd", phong->ambient().editCd().data());
				if (phong->ambient().sampler())
				{
					const pm::Sampler::Type ambientSamplerType = phong->ambient().sampler()->type();
					auxString_.format("Ambient: %s Sampler", samplerTypeToString(ambientSamplerType));
					createSamplerGuiTree(phong->ambient().sampler());
				}
				ImGui::PopID();

				ImGui::PushID("Diffuse");
				ImGui::InputFloat("Diffuse Kd", &phong->diffuse().editKd());
				ImGui::ColorEdit3("Diffuse Cd", phong->diffuse().editCd().data());
				if (phong->diffuse().sampler())
				{
					const pm::Sampler::Type diffuseSamplerType = phong->diffuse().sampler()->type();
					auxString_.format("Diffuse: %s Sampler", samplerTypeToString(diffuseSamplerType));
					createSamplerGuiTree(phong->diffuse().sampler());
				}
				ImGui::PopID();

				ImGui::PushID("Specular");
				ImGui::InputFloat("Specular Ks", &phong->specular().editKs());
				ImGui::ColorEdit3("Specular Cs", phong->specular().editCs().data());
				ImGui::InputFloat("Specular Exp", &phong->specular().editExp());
				if (phong->specular().sampler())
				{
					const pm::Sampler::Type specularSamplerType = phong->specular().sampler()->type();
					auxString_.format("Specular: %s Sampler", samplerTypeToString(specularSamplerType));
					createSamplerGuiTree(phong->specular().sampler());
				}
				ImGui::PopID();
				break;
			}
			case pm::Material::Type::EMISSIVE:
			{
				pm::Emissive *emissive = static_cast<pm::Emissive *>(material);
				ImGui::InputFloat("Radiance Scale", &emissive->editRadianceScale());
				ImGui::ColorEdit3("Emissive Ce", emissive->editCe().data());
				break;
			}
		}
		ImGui::TreePop();
		return true;
	}

	return false;
}

void UserInterface::onCameraChanged()
{
	sc_.config().camera->computeUvw();
	sc_.stopTracing();
	sc_.reset();
	sc_.startTracing();
}
