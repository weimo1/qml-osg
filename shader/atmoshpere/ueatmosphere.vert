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
    vec2 pad;
};

// 输出
out gl_PerVertex
{
    vec4 gl_Position;
};
flat out uint sliceId;

void main()
{
    // 使用内置变量 gl_VertexID 和 gl_InstanceID
    uint vertexId = uint(gl_VertexID);
    uint instanceId = uint(gl_InstanceID);
    
    // 生成全屏三角形的UV坐标
    // 这个三角形会覆盖整个NDC空间 [-1,1]
    vec2 uv = vec2(-1.0f);
    uv = vertexId == 1 ? vec2(-1.0f, 3.0f) : uv;
    uv = vertexId == 2 ? vec2( 3.0f,-1.0f) : uv;

    gl_Position = vec4(uv, 0.0, 1.0);
    sliceId = instanceId;
}