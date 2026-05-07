#version 460
#extension GL_EXT_ray_tracing : require

layout(binding = 1, set = 0) buffer ResultBuffer 
{
    int objectID;
    int instanceID;
    int primID;
    float hitT;
    float bary0;
    float bary1;
    int hit;
    int padding;
} result;

layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) buffer Offsets { 
    uint vertexOffset;
    uint indexOffset;
} offsets[];

layout(push_constant) uniform PushConstants {
    vec3 origin;
    vec3 dir;
} pushConstants;

struct PickerPayload {
   int objectID;
    int instanceID;
    int primID;
    float hitT;
    float bary0;
    float bary1;
    int hit;
    int padding;
};

layout(location = 0) rayPayloadInEXT PickerPayload pickerPayload;

hitAttributeEXT vec2 attribs;

void main()
{
   // Record hit information
    pickerPayload.objectID = gl_InstanceCustomIndexEXT;  // or map this however you track objects
    pickerPayload.instanceID = gl_InstanceID;
    pickerPayload.primID = gl_PrimitiveID;
    pickerPayload.hitT = gl_HitTEXT;
    
    // Barycentric coordinates from hit attributes
    pickerPayload.bary0 = attribs.x;
    pickerPayload.bary1 = attribs.y;
    
    pickerPayload.hit = 1;
    pickerPayload.padding = 0;
}