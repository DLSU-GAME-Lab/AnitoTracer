
const uint MaterialLambertian = 0;
const uint MaterialMetallic = 1;
const uint MaterialDielectric = 2;
const uint MaterialIsotropic = 3;
const uint MaterialDiffuseLight = 4;

struct Material
{
	vec4 Diffuse;
	int DiffuseTextureId;
	float Fuzziness;
	float RefractionIndex;
	uint MaterialModel;
};


const uint PointLight = 0;
const uint DirectionalLight = 1;
const uint SpotLight = 2;

struct LightProperties 
{
	vec3 LightPos;
	float padding1;
	vec4 AmbientColor;
	vec4 LightColor;
	uint LightType;
	uint padding2[3];
};

LightProperties InitializeTestPLProperties() 
{
	LightProperties pl;
	pl.LightPos = vec3(2600, 20.0, 0);
	pl.AmbientColor = vec4(1.0, 1.0, 1.0, 0.02);
	pl.LightColor = vec4(1.0, 0.4, 0.5, 1000000.0f);
	pl.LightType = PointLight;

	return pl;
}

LightProperties InitializeTestDLProperties() 
{
	LightProperties dl;
	dl.LightPos = vec3(1, 0, 0);
	dl.AmbientColor = vec4(1.0, 1.0, 1.0, 1.0f);
	dl.LightColor = vec4(1.0, 0.4, 0.5, 1.0f);
	dl.LightType = DirectionalLight;

	return dl;
}

LightProperties InitializeTestSLProperties() 
{
	LightProperties sl;
	sl.LightPos = vec3(1000, 400, 0);
	sl.AmbientColor = vec4(1.0, 1.0, 1.0, 0.02);
	sl.LightColor = vec4(0.6, 1.0, 0.4, 500000.0f);
	sl.LightType = SpotLight;

	return sl;
}