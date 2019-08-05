#ifndef CLASS_VISUALFEEDBACK
#define CLASS_VISUALFEEDBACK

#include <ncine/Matrix4x4.h>
#include <ncine/Random.h>

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

	VisualFeedback();
	void initTexture(int width, int height);
	void randomizeTexture(unsigned char *pixelsPtr);
	void progressiveUpdate();
	void fixedUpdate();

	inline int texWidth() const { return texWidth_; }
	inline int texHeight() const { return texHeight_; }
	inline unsigned int texSizeInBytes() const { return texSizeInBytes_; }

	inline int textureUploadMode() const { return textureUploadMode_; }
	void setTextureUploadMode(int mode);

	inline unsigned char *texPixels() { return (mapPtr_ ? mapPtr_ : pixels_.get()); }

  private:
	int texWidth_;
	int texHeight_;
	unsigned int texSizeInBytes_;
	int textureUploadMode_ = 0;

	static const int UniformsBufferSize = 256;
	unsigned char uniformsBuffer_[UniformsBufferSize];

	nctl::UniquePtr<unsigned char[]> pixels_;
	nc::Random rng_; // TEMP

	nctl::UniquePtr<nc::GLShaderProgram> texProgram_;
	nctl::UniquePtr<nc::GLShaderUniforms> texUniforms_;
	nctl::UniquePtr<nc::GLShaderAttributes> texAttributes_;
	nctl::UniquePtr<nc::GLTexture> texture_;

	nctl::UniquePtr<nc::GLBufferObject> pbo_;
	unsigned char *mapPtr_;

	nc::Matrix4x4f projection_;
	nc::Matrix4x4f modelView_;
};

#endif
