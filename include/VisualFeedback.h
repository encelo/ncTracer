#ifndef CLASS_VISUALFEEDBACK
#define CLASS_VISUALFEEDBACK

#include <nctl/UniquePtr.h>
#include <ncine/TimeStamp.h>

namespace ncine {

class GLShaderProgram;
class GLShaderUniforms;
class GLShaderAttributes;
class GLTexture;
class GLBufferObject;

}

namespace nc = ncine;

/// The OpenGL based visual feedback class
class VisualFeedback
{
  public:
	struct TextureUploadModes
	{
		enum
		{
			TEXSUBIMAGE = 0,
			PBO,
			PBO_MAPPING,

			COUNT
		};
	};

	struct Configuration
	{
		bool progressiveCopy = true;
		int textureUploadMode = TextureUploadModes::TEXSUBIMAGE;
		float textureCopyDelay = 0.0f;
	};

	VisualFeedback();

	inline const Configuration &config() const { return config_; }
	inline Configuration &config() { return config_; }

	void initTexture(int width, int height);
	void resizeTexture(int width, int height);
	void randomizeTexture(unsigned char *pixelsPtr);
	void progressiveUpdate();
	void fixedUpdate();

	inline void startTimer() { lastUpdateTime_ = nc::TimeStamp::now(); }
	void update();

	inline int texWidth() const { return texWidth_; }
	inline int texHeight() const { return texHeight_; }
	inline unsigned int texSizeInBytes() const { return texSizeInBytes_; }
	inline unsigned char *texPixels() { return (mapPtr_ ? mapPtr_ : pixels_.get()); }

  private:
	nc::TimeStamp lastUpdateTime_;
	float lastTextureCopy_;

	Configuration config_;
	int texWidth_;
	int texHeight_;
	unsigned int texSizeInBytes_;

	static const int UniformsBufferSize = 256;
	unsigned char uniformsBuffer_[UniformsBufferSize];

	nctl::UniquePtr<unsigned char[]> pixels_;

	nctl::UniquePtr<nc::GLShaderProgram> texProgram_;
	nctl::UniquePtr<nc::GLShaderUniforms> texUniforms_;
	nctl::UniquePtr<nc::GLShaderAttributes> texAttributes_;
	nctl::UniquePtr<nc::GLTexture> texture_;

	nctl::UniquePtr<nc::GLBufferObject> pbo_;
	unsigned char *mapPtr_;
};

#endif
