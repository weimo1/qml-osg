#version 330
layout(location = 0) in vec3 aPos;

out vec3 vWorldPosition;
out vec3 cameraPosition;
out vec3 vSunDirection;  // 声明vSunDirection varying变量

uniform mat4 viewInverse;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform vec3 sunDirection;  // 太阳方向uniform

void main() 
{
    mat4 modelMatrix = viewInverse * osg_ModelViewMatrix;
    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    vWorldPosition = worldPosition.xyz;

    gl_Position = osg_ProjectionMatrix * osg_ModelViewMatrix * vec4(aPos, 1.0);
    gl_Position.z = gl_Position.w;

    // 设置cameraPosition为视图矩阵的逆矩阵的平移部分
    cameraPosition = vec3(viewInverse[3][0], viewInverse[3][1], viewInverse[3][2]);
    
    // 传递太阳方向到片段着色器
    vSunDirection = sunDirection;
}