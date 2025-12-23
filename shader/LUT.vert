#version 330 core
layout(location = 0) in vec3 vertex;
out vec2 v_texCoord;

void main() {
        gl_Position = vec4(vertex, 1.0);
        
        // 直接从顶点坐标生成纹理坐标 [-1,1] -> [0,1]
        v_texCoord = (vertex.xy + vec2(1.0, 1.0)) * 0.5;
}