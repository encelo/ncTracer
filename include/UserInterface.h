#ifndef CLASS_USERINTERFACE
#define CLASS_USERINTERFACE

#include <nctl/String.h>

class VisualFeedback;
class SceneContext;

namespace pm {

class World;
class Sampler;
class Light;
class Geometry;
class Material;

}

/// The ImGui user interface class
class UserInterface
{
  public:
	UserInterface(VisualFeedback &vf, SceneContext &sc);

	void createGuiMainWindow();

  private:
	static const unsigned int MaxStringLength = 256;

	nctl::String auxString_;
	nctl::String filename_;
	VisualFeedback &vf_;
	SceneContext &sc_;

	void createSamplerGuiTree(pm::Sampler *sampler);
	bool createLightGuiTree(pm::Light *light);
	bool createObjectGuiTree(pm::Geometry *object);
	bool createMaterialGuiTree(pm::Material *material);
};

#endif
