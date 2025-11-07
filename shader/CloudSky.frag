#version 330
in vec3 vWorldPosition;
in vec3 cameraPosition;

out vec4 color;

uniform float cloudDensity;
uniform sampler2D blueNoise;   // 蓝噪声纹理

// 体积云uniforms
uniform sampler2D cloudMap;    // 云噪声纹理
uniform vec3 sunDirection;     // 太阳方向

// 新增的云层控制参数uniforms
uniform float densityThreshold;
uniform float contrast;
uniform float densityFactor;
uniform float stepSize;

// 基础颜色和光照颜色定义
#define baseBright  vec3(1.26,1.25,1.29)    // 基础颜色 -- 亮部
#define baseDark    vec3(0.31,0.31,0.32)    // 基础颜色 -- 暗部

#define bottom 130  // 云层底部
#define top 200     // 云层顶部
#define width 400    // 云层 xz 坐标范围 [-width, width]

// 光线与包围盒相交函数
vec2 rayBoxDst(vec3 boundsMin, vec3 boundsMax, vec3 rayOrigin, vec3 invRaydir) {
    vec3 t0 = (boundsMin - rayOrigin) * invRaydir;
    vec3 t1 = (boundsMax - rayOrigin) * invRaydir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float dstA = max(max(tmin.x, tmin.y), tmin.z); // 进入点
    float dstB = min(tmax.x, min(tmax.y, tmax.z)); // 出去点

    float dstToBox = max(0.0, dstA);
    float dstInsideBox = max(0.0, dstB - dstToBox);
    
    return vec2(dstToBox, dstInsideBox);
}

// 改进的噪声采样函数，使用插值减少噪点
float sampleCloudMap(vec2 coord) {
    // 使用fract确保纹理坐标在[0,1]范围内
    vec2 clampedCoord = fract(coord);
    
    // 使用线性插值采样以减少锯齿
    return texture2D(cloudMap, clampedCoord).x;
}

// 计算 pos 点的云密度
float getDensity(vec3 pos) {
    // 高度衰减 - 云层中部密度最大
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(max(weight, 0.0), 0.5);  // 开根号使过渡更平滑

    // 改进的纹理坐标计算，确保在整个四边形上正确映射噪声纹理
    // 使用世界坐标并进行适当的缩放，同时添加偏移避免重复模式
    vec2 coord1 = (pos.xz + vec2(500.0, 500.0)) * 0.00025;  // 调整偏移量
    
    // 使用不同的偏移量来避免重复模式
    vec2 coord2 = (pos.xz + vec2(1500.0, 2500.0)) * 0.0005;  // 不同的缩放和偏移
    
    // 确保纹理坐标在[0,1]范围内
    coord1 = fract(coord1);
    coord2 = fract(coord2);
    
    // 使用改进的采样函数，添加clamp确保噪声值在合理范围内
    // 降低各层噪声的权重，使云层更稀疏
    float noise = clamp(sampleCloudMap(coord1), 0.0, 1.0) * 0.3;       // 降低权重 0.5->0.3
    noise += clamp(sampleCloudMap(coord1 * 2.0), 0.0, 1.0) * 0.15;     // 降低权重 0.25->0.15
    noise += clamp(sampleCloudMap(coord1 * 4.0), 0.0, 1.0) * 0.075;    // 降低权重 0.125->0.075
    noise += clamp(sampleCloudMap(coord2), 0.0, 1.0) * 0.0375;         // 使用不同的坐标
    noise += clamp(sampleCloudMap(coord2 * 2.0), 0.0, 1.0) * 0.01875;  // 使用不同的坐标
    noise += clamp(sampleCloudMap(coord2 * 4.0), 0.0, 1.0) * 0.009375; // 使用不同的坐标

    noise *= weight;

    // 使用uniform参数控制密度阈值，提高阈值使云层更稀疏
    if(noise < densityThreshold * 1.5) {  // 提高阈值倍数 1.0->1.5
        noise = 0.0;
    } else {
        // 使用uniform参数控制对比度，但限制范围防止过度对比
        noise = pow(noise, clamp(contrast, 0.5, 3.0));
    }

    // 使用uniform参数控制密度因子，降低密度因子使云层更稀疏
    noise *= densityFactor * 0.7;  // 降低密度因子 1.0->0.7
    
    // 确保最终噪声值在合理范围内，防止负值导致黑点
    noise = clamp(noise, 0.0, 1.0);

    return noise;
}

// 获取体积云颜色（无光照版本）
vec4 getCloud(vec3 worldPos, vec3 cameraPos, vec3 lightPos) {
    // 定义云盒的边界
    vec3 boundsMin = vec3(-width, bottom, -width);
    vec3 boundsMax = vec3(width, top, width);
    
    // 光线方向
    vec3 rayDir = normalize(worldPos - cameraPos);
    vec3 invRayDir = 1.0 / rayDir;
    
    // 计算光线与云盒的相交点
    vec2 rayDist = rayBoxDst(boundsMin, boundsMax, cameraPos, invRayDir);
    
    // 如果没有相交，返回透明
    if(rayDist.y <= 0.0) {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
    
    // 起始点和结束点
    vec3 rayStart = cameraPos + rayDir * rayDist.x;
    vec3 rayEnd = cameraPos + rayDir * (rayDist.x + rayDist.y);
    
    // 计算步长，增加步长使云层更稀疏
    float stepLength = stepSize * 1.5;  // 增加步长 1.0->1.5
    vec3 step = rayDir * stepLength;
    int maxStepCount = int(rayDist.y / stepLength);
    
    // 限制最大步数为200，减少步数使云层更稀疏
    maxStepCount = min(maxStepCount, 200);  // 降低最大步数 300->200
    
    // 初始化颜色累积器
    vec4 colorSum = vec4(0.0, 0.0, 0.0, 0.0);
    vec3 point = rayStart;
    
    // 计算屏幕UV坐标用于蓝噪声采样
    // 使用更准确的屏幕坐标计算
    vec2 screenUV = gl_FragCoord.xy / vec2(1920.0, 1080.0);  // 假设屏幕分辨率为1920x1080
    
    // 采样蓝噪声纹理，使用不同的噪声通道减少相关性
    float blueNoiseValue = texture2D(blueNoise, screenUV).r;
    float blueNoiseValue2 = texture2D(blueNoise, screenUV * 2.0).g; // 使用不同的UV和通道
    
    // 使用蓝噪声对步进起始点做偏移，解决分层问题
    // 使用两个噪声值来减少相关性
    point += step * (blueNoiseValue * 0.5 + blueNoiseValue2 * 0.3);
    
    // 添加基于世界坐标的噪声偏移，进一步减少层纹
    // 使用点坐标计算更稳定的噪声偏移
    vec3 worldNoiseOffset = vec3(
        fract(sin(dot(point * 0.01, vec3(12.9898, 78.233, 45.164))) * 43758.5453),
        fract(sin(dot(point * 0.01, vec3(39.346, 25.132, 89.754))) * 43758.5453),
        fract(sin(dot(point * 0.01, vec3(67.842, 12.453, 34.901))) * 43758.5453)
    );
    point += step * worldNoiseOffset * 0.1;

    // ray marching
    for(int i = 0; i < maxStepCount; i++) {
        // 检查是否超出云盒范围
        if(any(lessThan(point, boundsMin)) || any(greaterThan(point, boundsMax))) {
            break;
        }
        
        // 采样
        float density = getDensity(point);                // 当前点云密度
        
        // 根据距离眼睛的距离进行线性插值，使密度缓慢减为0
        float distanceToCamera = length(point - cameraPos);
        float maxDistance = 100000.0;  // 最大影响距离
        float distanceFactor = max(0.0, 1.0 - distanceToCamera / maxDistance);
        density *= distanceFactor;

        // 控制透明度，降低密度使云层更透明
        density *= cloudDensity * 0.7;  // 降低密度 1.0->0.7
        
        // 确保密度在合理范围内
        density = clamp(density, 0.0, 1.0);

        // 无光照版本 - 直接使用基础颜色
        vec3 baseColor = mix(baseBright, baseDark, density);   // 基础颜色
        
        // 确保基础颜色值在合理范围内
        baseColor = max(baseColor, vec3(0.0));
        
        // 混合 - 使用标准的alpha混合公式
        float stepAlpha = density * stepLength;
        // 限制步长alpha在合理范围内
        stepAlpha = clamp(stepAlpha, 0.0, 1.0);
        
        vec4 currentColor = vec4(baseColor, stepAlpha);
        // 使用更稳定的alpha混合公式
        colorSum.rgb = colorSum.rgb + currentColor.rgb * currentColor.a * (1.0 - colorSum.a);
        colorSum.a = colorSum.a + currentColor.a * (1.0 - colorSum.a);
        
        // 确保累积颜色和alpha在合理范围内
        colorSum.rgb = max(colorSum.rgb, vec3(0.0));
        colorSum.a = clamp(colorSum.a, 0.0, 1.0);
        
        // 如果累积alpha接近1，可以提前退出循环
        if(colorSum.a >= 0.95) {  // 降低alpha阈值 0.99->0.95
            colorSum.a = 1.0;
            break;
        }
        
        // 更新点位置
        point += step;
    }
    
    // 确保最终返回的颜色值在合理范围内
    colorSum.rgb = max(colorSum.rgb, vec3(0.0));
    colorSum.a = clamp(colorSum.a, 0.0, 1.0);
    
    return colorSum;
}

// 简单的降噪函数
vec3 simpleDenoise(vec3 color, float alpha) {
    // 使用更高级的降噪技术，减少视觉噪点
    // 1. 向中灰色混合，减少极端颜色值
    vec3 denoised = mix(color, vec3(0.5), 0.03);  // 降低降噪强度 0.05->0.03
    
    // 2. 确保颜色不会过暗或过亮
    denoised = clamp(denoised, vec3(0.02), vec3(0.98));
    
    // 3. 基于alpha值调整降噪强度
    // 低alpha值时降噪更强，高alpha值时保持更多细节
    float denoiseStrength = 0.05 * (1.0 - alpha);  // 降低降噪强度 0.1->0.05
    denoised = mix(color, denoised, denoiseStrength);
    
    return denoised;
}

void main() 
{
    vec3 worldPos = vWorldPosition;

    // 获取体积云颜色（无光照版本）
    vec4 cloud = getCloud(worldPos, cameraPosition, sunDirection * 1000.0);
    
    // 确保alpha值在合理范围内
    cloud.a = clamp(cloud.a, 0.0, 1.0);
    
    // 应用降噪处理，减少噪点
    cloud.rgb = simpleDenoise(cloud.rgb, cloud.a);
    
    // 使用深色背景
    vec3 backgroundColor = vec3(0.05, 0.05, 0.1);
    vec3 finalColor = mix(backgroundColor, cloud.rgb, cloud.a);
    
    // 最终颜色范围检查
    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));
    
    color = vec4(finalColor, 1.0);
}