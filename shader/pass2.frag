#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture0;

void main()
{
    // 第二 Pass 直接采样第一 Pass 的纹理并显示
    vec4 texColor = texture(uTexture0, vTexCoord);
    FragColor = texColor;  // 可以改成 texColor + vec4(0,1,0,0) 来验证叠加效果
}
