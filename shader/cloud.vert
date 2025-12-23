#version 330 core

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 texCoord;

uniform mat4 model_from_view;
uniform mat4 view_from_clip;

out vec2 v_texCoord;
out vec3 view_ray;

void main() {
    // vertex 范围是 [-1, 1],这是NDC坐标
    // // 我们需要构造一个在远平面上的点vertex.y=-vertex.y;


    vec4 clipSpacePos = vec4(vertex.xy, 1.0, 1.0);  // z=1表示远平面
    
    // 转换到视图空间
    vec4 viewSpacePos = view_from_clip * clipSpacePos;
    viewSpacePos.xyz /= viewSpacePos.w;  // 透视除法
    
    // 转换到世界空间
    vec4 worldSpacePos = model_from_view * vec4(viewSpacePos.xyz, 1.0);
    
    // 相机在世界空间的位置就是model_from_view矩阵的平移部分
    vec3 cameraWorldPos = model_from_view[3].xyz;
    
    // view_ray是从相机到远平面点的方向
    view_ray = worldSpacePos.xyz - cameraWorldPos;
    

    view_ray.y=view_ray.y;
    // vec4 clip_pos = vec4(vertex.xy, 1.0, 1.0);  // 远平面上的点
    // vec4 view_pos = view_from_clip * clip_pos;    // 转换到视图空间
    // view_pos /= view_pos.w;
    // view_ray = (model_from_view * vec4(view_pos.xyz, 0.0)).xyz;

   // view_ray =(model_from_view * vec4((view_from_clip * vertex).xyz, 0.0)).xyz*0.001;
    // 删除不正确的缩放因子
     
    gl_Position =vertex;
    v_texCoord = texCoord;
}