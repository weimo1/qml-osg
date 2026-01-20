#version 460 core
layout (location = 0) in vec3 aPos;

uniform vec2 iResolution1;
uniform vec2 iMouse;
uniform float iTime;
out vec3 iResolution;

void main() {
    uint vertexId = uint(gl_VertexID);
    uint instanceId = uint(gl_InstanceID);
    
    // 生成全屏三角形的UV坐标
    // 这个三角形会覆盖整个NDC空间 [-1,1]
    vec2 uv = vec2(-1.0f);
    uv = vertexId == 1 ? vec2(-1.0f, 3.0f) : uv;
    uv = vertexId == 2 ? vec2( 3.0f,-1.0f) : uv;

    gl_Position = vec4(uv, 0.0, 1.0);

    gl_Position = vec4(aPos, 1.0);
    iResolution = vec3(iResolution1, 0.0);

}
