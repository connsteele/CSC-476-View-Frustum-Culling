#version 330 core 
in vec3 fragNor;
in vec3 WPos;
//to send the color to a frame buffer
layout(location = 0) out vec4 color;

uniform vec3 LPos;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

/* Very simple Diffuse shader */
void main()
{
	vec3 Dcolor, Scolor;
   vec3 Dlight = normalize(LPos - WPos);
	vec3 normal = normalize(fragNor);
	Dcolor = MatDif*max(dot(normalize(Dlight), normal), 0)+MatAmb;
	vec3 H = normalize(-1*WPos) + normalize(Dlight);
	Scolor = MatSpec*pow(max(dot(normalize(H), normal), 0), MatShine);
	color = vec4(Dcolor+Scolor, 1.0);
}
