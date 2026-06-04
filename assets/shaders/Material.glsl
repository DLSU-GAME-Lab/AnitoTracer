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

	// ===== NEW: Alpha/Transparency Support =====  
	// Note: These fields must match the C++ Material struct alignment
	// C++ Layout (48 bytes total, 3 vec4s for std140):
	//   Bytes 0-15:   glm::vec4 Diffuse
	//   Bytes 16-19:  int32_t DiffuseTextureId
	//   Bytes 20-23:  float Fuzziness
	//   Bytes 24-27:  float RefractionIndex
	//   Bytes 28-31:  uint32_t MaterialModel
	//   Bytes 32-35:  int32_t AlphaMapTextureId
	//   Bytes 36-39:  float AlphaCutoffThreshold
	//   Bytes 40-43:  uint32_t AlphaBlendMode
	//   Bytes 44-47:  float _pad (padding to 48-byte boundary)
	int AlphaMapTextureId;
	float AlphaCutoffThreshold;
	uint AlphaBlendMode;
	float _pad; // Padding to maintain 48-byte size
};


const uint PointLight = 0;
const uint DirectionalLight = 1;
const uint SpotLight = 2;

struct LightProperties 
{
	vec3 LightPos;
	vec3 LightDir;
	vec4 AmbientColor;
	vec4 LightColor;
	uint LightType;
};

LightProperties InitializeTestPLProperties() 
{
	LightProperties pl;
	pl.LightPos = vec3(0, 0.0, 0);
	pl.LightDir = vec3(0, -1, 0);
	pl.AmbientColor = vec4(1.0, 1.0, 1.0, 0.02);
	pl.LightColor = vec4(1.0, 0.4, 0.5, 500000.0f);
	pl.LightType = PointLight;

	return pl;
}

LightProperties InitializeTestDLProperties() 
{
	LightProperties dl;
	dl.LightPos = vec3(1, 0, 0);
	dl.LightDir = vec3(0, -1, 0);
	dl.AmbientColor = vec4(1.0, 1.0, 1.0, 1.0f);
	dl.LightColor = vec4(1.0, 0.4, 0.5, 1.0f);
	dl.LightType = DirectionalLight;

	return dl;
}

LightProperties InitializeTestSLProperties() 
{
	LightProperties sl;
	sl.LightPos = vec3(1000, 400, 0);
	sl.LightDir = vec3(0, -1, 0);
	sl.AmbientColor = vec4(1.0, 1.0, 1.0, 0.02);
	sl.LightColor = vec4(0.6, 1.0, 0.4, 500000.0f);
	sl.LightType = SpotLight;

	return sl;
}