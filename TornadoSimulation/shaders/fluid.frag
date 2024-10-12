#version 330 core

uniform sampler2D previousStateTexture;
uniform float deltaTime;

in vec2 TexCoords;

out vec4 FragColor;

void main()
{
	vec4 prevState = texture(previousStateTexture, TexCoords);

	FragColor = vec4(prevState.xyz, 1.0f);
}
