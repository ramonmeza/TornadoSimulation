#version 330 core

out vec4 FragColor;

uniform vec3 backgroundColor;

void main()
{
	FragColor = vec4(backgroundColor, 1.0f);
}
