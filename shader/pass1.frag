#version 330 core

out vec4 FragColor;
in vec2 vTexCoord;

void main()
{
    // 第一 Pass 输出红色渐变
    FragColor = vec4(vTexCoord.x, 0.0, 1.0, 0.0);
}
