#version 330
in vec3 vWorldPosition;
in vec3 cameraPosition;

uniform sampler2D cloudMap;  // 噪声纹理
uniform sampler2D blueNoise; // 蓝噪声纹理
uniform float iTime;         // 时间变量

out vec4 color;

#define bottom 13  // 云层底部
#define top 20      // 云层顶部
#define width 40     // 云层 xz 坐标范围 [-width, width]

// 计算 pos 点的云密度
float getDensity(vec3 pos) {
    // 高度衰减 - 云层中部密度最大
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(max(weight, 0.0), 0.5);  // 开根号使过渡更平滑

    // 添加时间驱动的云层移动效果
    vec2 offset = vec2(iTime * 0.005, iTime * 0.002);
    
    vec2  coord1 = pos.xz * 0.0025 + offset;
    float noise = texture2D(cloudMap, coord1).x;
    noise += texture2D(cloudMap, coord1 * 3.5).x/3.5;
    noise += texture2D(cloudMap, coord1 * 7.0).x/7.0;
    noise += texture2D(cloudMap, coord1 * 11.0).x/11.0;

    noise/=1.4472;
    
    noise *= weight;

    // 截断 - 只有密度大于阈值的部分才显示为云
    if(noise < 0.4) {
        noise = 0.0;
    }

    return noise;
}

// 获取体积云颜色
vec4 getCloud(vec3 worldPos, vec3 cameraPos) {
    vec3 direction = normalize(worldPos - cameraPos);   // 视线射线方向
    vec3 step = direction * 0.25;   // 步长
    vec4 colorSum = vec4(0);        // 积累的颜色
    vec3 point = cameraPos;         // 从相机出发开始测试

    // 计算屏幕UV坐标用于蓝噪声采样
    vec2 screenUV = gl_FragCoord.xy / vec2(1920.0, 1080.0);  // 假设屏幕分辨率为1920x1080
    
    // 采样蓝噪声纹理
    float blueNoiseValue = texture2D(blueNoise, screenUV).r;

    // 如果相机在云层下，将测试起始点移动到云层底部
    if(point.y < bottom) {
        point += direction * (abs(bottom - cameraPos.y) / abs(direction.y));
    }
    // 如果相机在云层上，将测试起始点移动到云层顶部
    if(top < point.y) {
        point += direction * (abs(cameraPos.y - top) / abs(direction.y));
    }

    // 如果目标像素遮挡了云层则放弃测试
    float len1 = length(point - cameraPos);     // 云层到眼距离
    float len2 = length(worldPos - cameraPos);  // 目标像素到眼距离
    if(len2 < len1) {
        return vec4(0);
    }

    // 使用蓝噪声对步进起始点做偏移，解决分层问题
    point += step * blueNoiseValue * 0.5;

    // ray marching
    for(int i=0; i<300; i++) {
        point += step;
        if(bottom>point.y || point.y>top || -width>point.x || point.x>width || -width>point.z || point.z>width) {
            break;
        }
        
        float density = getDensity(point) * 0.3;
        vec4 color = vec4(1.0, 1.0, 1.0, 1.0) * density;    // 白色云
        colorSum = colorSum + color * (1.0 - colorSum.a);   // 与累积的颜色混合
    }

    return colorSum;
}

void main() {
    // 获取体积云颜色
    vec4 cloud = getCloud(vWorldPosition, cameraPosition);
    
    // 背景颜色（蓝色天空）
    vec4 bgColor = vec4(0.5, 0.7, 1.0, 1.0);
    
    // 混合云和背景
    color = bgColor * (1.0 - cloud.a) + cloud;
}