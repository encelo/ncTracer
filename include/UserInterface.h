#ifndef CLASS_USERINTERFACE
#define CLASS_USERINTERFACE

#include <nctl/String.h>

class VisualFeedback;
class SceneContext;

/// The ImGui user interface class
class UserInterface
{
  public:
	UserInterface(VisualFeedback &vf, SceneContext &sc);

	void createGuiMainWindow();

  private:
	nctl::String auxString_;
	VisualFeedback &vf_;
	SceneContext &sc_;

	int textureUploadMode_;
};

#endif
