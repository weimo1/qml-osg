#version 330
layout(location = 0) in vec3 aPos;

out vec3 vWorldPosition;
out vec3 cameraPosition;

uniform mat4 viewInverse;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;

void main() 
{
    // 使用模型矩阵变换顶点位置
    mat4 modelMatrix = viewInverse * osg_ModelViewMatrix;
    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    vWorldPosition = worldPosition.xyz;

    gl_Position = osg_ProjectionMatrix * osg_ModelViewMatrix * vec4(aPos, 1.0);
    gl_Position.z = gl_Position.w; // 确保天空盒正确渲染

    // 设置cameraPosition为(0,0,0)，因为天空盒使用绝对参考框架
    cameraPosition = vec3(0.0, 0.0, 0.0);
}