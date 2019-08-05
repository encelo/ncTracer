#include "VisualFeedback.h"
#include <ncine/GLShaderProgram.h>
#include <ncine/GLShaderUniforms.h>
#include <ncine/GLShaderAttributes.h>
#include <ncine/GLTexture.h>
#include <ncine/GLBufferObject.h>

#include "shader_strings.h"

VisualFeedback::VisualFeedback()
    : texWidth_(0), texHeight_(0), texSizeInBytes_(0),
      textureUploadMode_(TextureUploadModes::TEXSUBIMAGE), mapPtr_(nullptr)
{
	texProgram_ = nctl::makeUnique<nc::GLShaderProgram>();
	texProgram_->attachShaderFromString(GL_VERTEX_SHADER, ShaderStrings::texture_vs);
	texProgram_->attachShaderFromString(GL_FRAGMENT_SHADER, ShaderStrings::texture_fs);
	texProgram_->link(nc::GLShaderProgram::Introspection::ENABLED);
	texProgram_->use();

	texUniforms_ = nctl::makeUnique<nc::GLShaderUniforms>(texProgram_.get());
	texUniforms_->setUniformsDataPointer(uniformsBuffer_);
	texUniforms_->uniform("uTexture")->setIntValue(0);
	texAttributes_ = nctl::makeUnique<nc::GLShaderAttributes>(texProgram_.get());
	texUniforms_->uniform("texRect")->setFloatValue(1.0f, 0.0f, -1.0f, 0.0f);

	FATAL_ASSERT(UniformsBufferSize >= texProgram_->uniformsSize());

	pbo_ = nctl::makeUnique<nc::GLBufferObject>(GL_PIXEL_UNPACK_BUFFER);
}

void VisualFeedback::initTexture(int width, int height)
{
	texWidth_ = width;
	texHeight_ = height;
	texSizeInBytes_ = static_cast<unsigned int>(texWidth_ * texHeight_ * 3);

	texUniforms_->uniform("spriteSize")->setFloatValue(width, height);

	pixels_ = nctl::makeUnique<unsigned char[]>(texSizeInBytes_);
	pbo_->bufferData(texSizeInBytes_, nullptr, GL_STREAM_DRAW);

	texture_ = nctl::makeUnique<nc::GLTexture>(GL_TEXTURE_2D);
	texture_->texStorage2D(1, GL_RGB8, width, height);
	texture_->texParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	texture_->texParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	projection_ = nc::Matrix4x4f::ortho(0.0f, width, 0.0f, height, 0.0f, 1.0f);
	texUniforms_->uniform("projection")->setFloatVector(projection_.data());
	modelView_ = nc::Matrix4x4f::Identity;
	modelView_ *= nc::Matrix4x4f::translation(width * 0.5f, height * 0.5f, 0.0f);
	texUniforms_->uniform("modelView")->setFloatVector(modelView_.data());
	texUniforms_->commitUniforms();
}

void VisualFeedback::randomizeTexture(unsigned char *pixelsPtr)
{
	for (int i = 0; i < texWidth_ * texHeight_; i++)
	{
		pixelsPtr[i * 3 + 0] = rng_.fastInteger(0, 255);
		pixelsPtr[i * 3 + 1] = rng_.fastInteger(0, 255) * (textureUploadMode_ - 1);
		pixelsPtr[i * 3 + 2] = rng_.fastInteger(0, 255) * textureUploadMode_;
	}
}

void VisualFeedback::progressiveUpdate()
{
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (textureUploadMode_ == TextureUploadModes::PBO_MAPPING)
	{
		FATAL_ASSERT(mapPtr_ == nullptr);
		mapPtr_ = static_cast<GLubyte *>(pbo_->mapBufferRange(0, texSizeInBytes_, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
	}

	//unsigned char *pixelsPtr = mapPtr_ ? mapPtr_ : pixels_.get();
	//randomizeTexture(pixelsPtr);

	texProgram_->use();
	texture_->bind();
	if (textureUploadMode_ == TextureUploadModes::TEXSUBIMAGE)
	{
		pbo_->unbind();
		texture_->texSubImage2D(0, 0, 0, texWidth_, texHeight_, GL_RGB, GL_UNSIGNED_BYTE, pixels_.get());
	}
	else if (textureUploadMode_ == TextureUploadModes::PBO)
	{
		pbo_->bufferData(texSizeInBytes_, nullptr, GL_STREAM_DRAW); // Orphaning
		pbo_->bufferSubData(0, texSizeInBytes_, pixels_.get());
		texture_->texSubImage2D(0, 0, 0, texWidth_, texHeight_, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	}
	else if (textureUploadMode_ == TextureUploadModes::PBO_MAPPING)
	{
		FATAL_ASSERT(mapPtr_);
		pbo_->flushMappedBufferRange(0, texSizeInBytes_);
		pbo_->unmap();
		mapPtr_ = nullptr;

		texture_->texSubImage2D(0, 0, 0, texWidth_, texHeight_, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void VisualFeedback::fixedUpdate()
{
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	texProgram_->use();
	texture_->bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void VisualFeedback::setTextureUploadMode(int mode)
{
	FATAL_ASSERT(mode >= 0);
	FATAL_ASSERT(mode < TextureUploadModes::COUNT);
	textureUploadMode_ = mode;
}
