#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "Material.glsl"
#include "UniformBufferObject.glsl"

layout(binding = 0) readonly uniform UniformBufferObjectStruct { UniformBufferObject Camera; };
layout(binding = 1) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 2) readonly buffer LightsArray { LightProperties[] Lights; }; 
layout(binding = 3) uniform sampler2D[] TextureSamplers;

layout(location = 0) in vec4 FragColor;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragTexCoord;
layout(location = 3) in flat int FragMaterialIndex;
layout(location = 4) in vec3 FragWorldPos;

layout(location = 0) out vec4 OutColor;

const vec4 dirLightColor = vec4(1.0);
const vec3 dirLightDir = normalize(vec3(5.0, 4.0, 3.0));

// Calculate the lighting of the directional light on the color of the object/model.
vec4 calcDirLight(LightProperties light, vec3 normal, vec3 viewDir) {

    // light direction
    vec3 lightDir = normalize(-light.LightPos);

    // ambient
    vec3 ambient = light.AmbientColor.xyz * light.AmbientColor.w;

    // diffuse 
    float diff = max(dot(normal, lightDir), 0.f);
    vec3 diffuse = diff * light.LightColor.xyz;

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float specPhong = 16.0f;
    float spec = pow(max(dot(reflectDir, viewDir), 0.1f), specPhong);
    float specStr = 0.5f;
    vec3 specColor = spec * specStr * light.LightColor.xyz;

    float multiplier = 1.0f;
    return vec4(multiplier * (specColor + diffuse + ambient), 1.f);
}

// Calculate the lighting of the point light on the color of the object/model.
vec4 calcPointLight(LightProperties light, vec3 normal, vec3 viewDir) {
	
    // light direction
    vec3 lightDir = normalize(light.LightPos - FragWorldPos);

    // ambient 
    vec3 ambient = light.AmbientColor.xyz * light.AmbientColor.w;

    // diffuse
    float diff = max(dot(normal, lightDir), 0.f);
    vec3 diffuse = diff * light.LightColor.xyz;

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float specPhong = 16.0f;
    float spec = pow(max(dot(reflectDir, viewDir), 0.1f), specPhong);
    float specStr = 0.5f;
    vec3 specColor = spec * specStr * light.LightColor.xyz;
    
    // Point Light Intensity
    float distance = length(light.LightPos - FragWorldPos);
    float intensity = 1.f / (distance * distance);
    
    // multiplier
    intensity *= 1.0f;

	// Return the resulting fragment texture with applied point light lighting.
	return vec4(intensity * (specColor + diffuse + ambient), 1.f);
}

vec4 calcSpotLight(LightProperties light, vec3 normal, vec3 viewDir) {
    
    // Set the intensity of the spot light
    float intensity = 0.8f;

    // Calculate the direction of the spot light.
    vec3 LightToPixel = normalize(light.LightPos - FragWorldPos);
    // placeholder dir
    vec3 dir = vec3(1.0f, 1.0f, 1.0f);
    float SpotFactor = dot(LightToPixel, dir);

    // If the pixel is within the cone
    if (SpotFactor > cos(0.5f)) {
        // Calculate the lighting using the point light
        vec4 Color = calcPointLight(light, normal, viewDir);
        // Return the calculated lighting
        return intensity * Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - cos(0.5f)));
    }
    else {
        // Outside the range of the spot light: return 0 = no light.
        return vec4(0,0,0,0);
    }
}

void main() 
{
    const int textureId = Materials[FragMaterialIndex].DiffuseTextureId;
    const float d = max(dot(dirLightDir, normalize(FragNormal)), 0.2);

    vec4 baseColor = vec4(FragColor.rgb * d, FragColor.a); // Apply lighting to base color
    if (textureId >= 0)
    {
        vec4 texColor = texture(TextureSamplers[textureId], FragTexCoord);
        baseColor.rgb *= texColor.rgb;
        baseColor.a *= texColor.a; 
    }

    if (baseColor.a < 0.01)
        discard;

    OutColor = dirLightColor * baseColor;

    // Get our view direction from the camera to the fragment
    vec3 viewDir = normalize(Camera.ModelViewInverse[3].xyz - FragWorldPos);

    // Lit Objects (with texture)
    if (Lights.length() > 0) {
        for (int i = 0; i < Lights.length(); i++) {
            if (Lights[i].LightType == PointLight) {
                OutColor += calcPointLight(Lights[i], FragNormal, viewDir) * baseColor;
            } else if (Lights[i].LightType == DirectionalLight) {
                OutColor += calcDirLight(Lights[i], FragNormal, viewDir) * baseColor;
            } else if (Lights[i].LightType == SpotLight) {
                OutColor += calcSpotLight(Lights[i], FragNormal, viewDir) * baseColor;
            }
        }
    } else {
    }
}
