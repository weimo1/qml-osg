#version 450 core


layout(std140, binding = 0) uniform CONSTANT_BUFFER
{
    mat4 gViewProjMat;
    vec4 gColor;
    vec3 gSunIlluminance;
    int gScatteringMaxPathDepth;
    uvec2 gResolution;
    float gFrameTimeSec;
    float gTimeSec;
    uvec2 gMouseLastDownPos;
    uint gFrameId;
    uint gTerrainResolution;
    float gScreenshotCaptureActive;
    vec2 RayMarchMinMaxSPP;
};

#define PI 3.1415926535897932384626433832795f


