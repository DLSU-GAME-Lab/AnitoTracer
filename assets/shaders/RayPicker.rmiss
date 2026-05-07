#version 460
#extension GL_EXT_ray_tracing : require

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

void main() {
    // No hit - set hasHit to 0
   pickerPayload.objectID = -1;
    pickerPayload.instanceID = -1;
    pickerPayload.primID = -1;
    pickerPayload.hitT = 0.0;
    pickerPayload.bary0 = 0.0;
    pickerPayload.bary1 = 0.0;
    pickerPayload.hit = 0;
    pickerPayload.padding = 0;
}