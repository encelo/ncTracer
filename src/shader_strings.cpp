#include "shader_strings.h"

char const *const ShaderStrings::texture_vs = R"glsl(
uniform vec2 size;
uniform mat4 projection;
uniform mat4 modelView;
out vec2 vTexCoords;

void main()
{
	vec2 aPosition = vec2(0.5 - float(gl_VertexID >> 1), -0.5 + float(gl_VertexID % 2));
	vec2 aTexCoords = vec2(1.0 - float(gl_VertexID >> 1), float(gl_VertexID % 2));
	vec4 position = vec4(aPosition.x * size.x, aPosition.y * size.y, 0.0, 1.0);

	gl_Position = projection * modelView * position;
	vTexCoords = vec2(aTexCoords.x, aTexCoords.y);
}
)glsl";

char const *const ShaderStrings::texture_fs = R"glsl(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D uTexture;
in vec2 vTexCoords;
out vec4 fragColor;

void main()
{
	fragColor = texture(uTexture, vTexCoords);
}
)glsl";
