#version 330
in vec3 vWorldPosition;
in vec3 cameraPosition;
in vec3 vSunDirection;

out vec4 color;

uniform float cloudDensity;
uniform sampler2D blueNoise;

// 体积云uniforms
uniform sampler2D cloudMap;
uniform vec3 sunDirection;

// 新增的云层控制参数uniforms
uniform float densityThreshold;
uniform float contrast;
uniform float densityFactor;
uniform float stepSize;

// 光照参数
const vec3 lightColor = vec3(1.2, 1.2, 1.2);
const float specularStrength = 0.4;
const float shininess = 12.0;

// 大气散射参数 - 保持为uniform变量，因为它们在其他地方使用
uniform float mieDirectionalG;
uniform vec3 up;
uniform float sunZenithAngle;
uniform float sunAzimuthAngle;
uniform float rayleigh;
uniform float turbidity;
uniform float mieCoefficient;

// constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

const float n = 1.0003;
const float N = 2.545E25;

// 增强光学长度参数以增强散射效果
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
// 1.0 / ( 4.0 * pi )
const float ONE_OVER_FOURPI = 0.07957747154594767;

// 基础颜色和光照颜色定义
#define baseBright  vec3(1.26,1.25,1.29)
#define baseDark    vec3(0.31,0.31,0.32)

#define bottom 130
#define top 200
#define width 400

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

// 计算 pos 点的云密度 - 调整为更自然的云朵效果
float getDensity(vec3 pos) {
    // 高度衰减 - 云层中部密度最大，模拟真实云朵分布
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(max(weight, 0.0), 0.8);  // 调整指数使过渡更自然

    // 改进的纹理坐标计算，确保在整个四边形上正确映射噪声纹理
    // 使用世界坐标并进行适当的缩放，同时添加偏移避免重复模式
    vec2 coord1 = (pos.xz + vec2(500.0, 500.0)) * 0.0002;  // 调整缩放因子
    vec2 coord2 = (pos.xz + vec2(1500.0, 2500.0)) * 0.0004;  // 调整缩放因子
    
    // 确保纹理坐标在[0,1]范围内
    coord1 = fract(coord1);
    coord2 = fract(coord2);
    
    // 使用改进的采样函数，添加clamp确保噪声值在合理范围内
    // 调整各层噪声的权重，使云层更自然
    float noise = clamp(sampleCloudMap(coord1), 0.0, 1.0) * 0.6;       // 主要噪声层
    noise += clamp(sampleCloudMap(coord1 * 2.0), 0.0, 1.0) * 0.3;     // 细节层
    noise += clamp(sampleCloudMap(coord1 * 4.0), 0.0, 1.0) * 0.1;     // 更细的细节层
    noise += clamp(sampleCloudMap(coord2), 0.0, 1.0) * 0.05;          // 不同的坐标层
    noise += clamp(sampleCloudMap(coord2 * 2.0), 0.0, 1.0) * 0.02;   // 更细的细节层

    noise *= weight;

    // 使用uniform参数控制密度阈值，调整阈值使云层更自然
    if(noise < densityThreshold) {
        noise = 0.0;
    } else {
        // 使用uniform参数控制对比度，适度增强对比度使云层边缘更清晰
        noise = pow(noise, clamp(contrast, 1.0, 3.0));
    }

    // 使用uniform参数控制密度因子，适度增强密度因子使云层更明显
    noise *= densityFactor * 0.8;  // 适度增强密度因子
    
    // 确保最终噪声值在合理范围内，防止负值导致黑点
    noise = clamp(noise, 0.0, 1.0);

    return noise;
}

// Rayleigh scattering coefficient - 使用uniform参数
vec3 calculateBetaR() {
    vec3 lambda = vec3(680E-9, 550E-9, 450E-9); // wavelength of light (RGB)
    vec3 lambda2 = lambda * lambda;
    vec3 lambda4 = lambda2 * lambda2;
    vec3 result = (8.0 * pow(pi, 3.0) * pow(n * n - 1.0, 2.0) * (6.0 + 3.0 * 0.0) / (3.0 * N * lambda4 * (6.0 - 7.0 * 0.0)));
    return result * rayleigh; // 使用uniform参数
}

// Mie scattering coefficient - 使用uniform参数
vec3 calculateBetaM() {
    const vec3 c = vec3(6.544E-17, 1.882E-16, 4.07E-16);
    vec3 K = 0.686 * turbidity * c; // 使用uniform参数
    return 0.434 * K * mieCoefficient; // 使用uniform参数
}

// Rayleigh phase function

float rayleighPhase(float cosTheta) 
{
    return THREE_OVER_SIXTEENPI * (1.0 + pow(cosTheta, 2.0));
}

// Henyey-Greenstein phase function
// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float hgPhase(float g, float cosTheta)
{
    float numer = 1.0 - g * g;
    float denom = 1.0 + g * g + 2.0 * g * cosTheta;
    return numer / (4.0 * 3.14159265 * denom * sqrt(denom));
}

// 双叶相位函数
float dualLobPhase(float g0, float g1, float w, float cosTheta)
{
    return mix(hgPhase(g0, cosTheta), hgPhase(g1, cosTheta), w);
}

// 相位函数
float phaseFunction(float cosTheta) 
{
    // 使用双叶相位函数来更好地模拟云层的前向和后向散射
    float g0 = 0.5;  // 前向散射
    float g1 = -0.5; // 后向散射
    float w = 0.2;   // 混合权重
    return dualLobPhase(g0, g1, w, cosTheta);
}

// 计算大气散射光照 - 增强背景颜色
vec3 calculateAtmosphericLight(vec3 direction) {
    vec3 sunDir = vSunDirection;
    
    // optical length
    // cutoff angle at 90 to avoid singularity in next formula.
    float zenithAngle = acos(max(0.0, dot(up, direction)));
    float inverse = 1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - ((zenithAngle * 180.0) / pi), -1.253));
    float sR = rayleighZenithLength * inverse;
    float sM = mieZenithLength * inverse;

    // combined extinction factor - 增强散射效果
    vec3 betaR = calculateBetaR();
    vec3 betaM = calculateBetaM();
    vec3 Fex = exp(-(betaR * sR + betaM * sM));

    // in scattering
    float cosTheta = dot(direction, sunDir);

    float rPhase = rayleighPhase(cosTheta);
    vec3 betaRTheta = betaR * rPhase;

    float mPhase = hgPhase(mieDirectionalG, cosTheta);  // 使用uniform变量mieDirectionalG
    vec3 betaMTheta = betaM * mPhase;

    vec3 Lin = pow(1.0 * ((betaRTheta + betaMTheta) / (betaR + betaM)) * (1.0 - Fex), vec3(1.5));

    // nightsky
    float theta = acos(direction.y); // elevation --> y-axis, [-pi/2, pi/2]
    float phi = atan(direction.z, direction.x); // azimuth --> x-axis [-pi/2, pi/2]
    vec2 uv = vec2(phi, theta) / vec2(2.0 * pi, pi) + vec2(0.5, 0.0);
    vec3 L0 = vec3(0.1) * Fex;

    // composition + solar disc
    float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (0.9 * Fex) * sundisk;

    vec3 texColor = (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.0006);

    texColor *= 1.8;
    vec3 retColor = pow(texColor, vec3(1.0 / (1.2 + (1.2 * 0.4))));

    // 确保返回的颜色不会全黑，添加一个最小值
    retColor = max(retColor, vec3(0.0));

    // 确保返回的颜色不会全黑，添加一个最小值
    retColor = max(retColor, vec3(0.02));

    // 确保返回的颜色不会全黑，添加一个最小值
    retColor = max(retColor, vec3(0.02));
    
    return retColor;
}

// 计算太阳可见度 (从当前点向太阳方向的透射率)
float calculateSunVisibility(vec3 point, vec3 sunDir) {
    float sunVisibility = 1.0;
    vec3 sunVisStep = point;
    
    // 计算光线与云盒的相交点
    vec3 boundsMin = vec3(-width, bottom, -width);
    vec3 boundsMax = vec3(width, top, width);
    vec3 invRayDir = 1.0 / sunDir;
    vec2 rayDist = rayBoxDst(boundsMin, boundsMax, point, invRayDir);
    
    // 如果没有相交，返回完全可见
    if(rayDist.y <= 0.0) {
        return 1.0;
    }
    
    // 向太阳方向的步进次数和步长
    const int lightSteps = 8;
    float stepLength = rayDist.y / float(lightSteps);
    vec3 step = sunDir * stepLength;
    
    for(int j = 0; j < lightSteps; j++) {
        float lightDensity = getDensity(sunVisStep);
        // 使用Beer定律计算透射率
        sunVisibility *= exp(-lightDensity * stepLength * 1.5);
        sunVisStep += step;
    }
    
    return sunVisibility;
}

// 计算环境光
vec3 calculateAmbientLight(vec3 point) {
    // 简单的环境光计算，基于高度和天空颜色
    float heightFactor = clamp((point.y - bottom) / (top - bottom), 0.0, 1.0);
    vec3 skyColor = vec3(0.5, 0.7, 1.0) * (0.5 + 0.5 * heightFactor);
    return skyColor * 0.8;
}

// 计算背光增强
vec3 calculateBackLight(vec3 point, vec3 viewDir) {
    // 向上方向追踪
    vec3 backLightDir = normalize(vec3(0.0, 1.0, 0.0));
    float backScatter = max(0.0, dot(-viewDir, backLightDir));
    return vec3(0.4, 0.6, 0.9) * backScatter * 0.6;
}

// 获取体积云颜色（完整光照版本）
vec4 getCloudWithFullLighting(vec3 worldPos, vec3 cameraPos, vec3 lightPos) {
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
    
    // 计算步长
    float stepLength = stepSize;
    vec3 step = rayDir * stepLength;
    int maxStepCount = int(rayDist.y / stepLength);
    
    // 限制最大步数
    maxStepCount = min(maxStepCount, 200);
    
    // 初始化累积值
    float transmittance = 1.0;  // 透射率
    vec3 scattering = vec3(0.0);  // 散射光
    
    vec3 point = rayStart;
    
    // 计算屏幕UV坐标用于蓝噪声采样
    vec2 screenUV = gl_FragCoord.xy / vec2(1920.0, 1080.0);
    
    // 采样蓝噪声纹理
    float blueNoiseValue = texture2D(blueNoise, screenUV).r;
    float blueNoiseValue2 = texture2D(blueNoise, screenUV * 2.0).g;
    
    // 使用蓝噪声对步进起始点做偏移
    point += step * (blueNoiseValue * 0.5 + blueNoiseValue2 * 0.3);
    
    // 添加基于世界坐标的噪声偏移
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
        
        // 采样密度
        float density = getDensity(point);
        
        // 根据距离眼睛的距离进行线性插值，使远处的云层更透明
        float distanceToCamera = length(point - cameraPos);
        float maxDistance = 50000.0;
        float distanceFactor = max(0.0, 1.0 - distanceToCamera / maxDistance);
        density *= distanceFactor;

        // 控制透明度
        density *= cloudDensity;
        
        // 确保密度在合理范围内
        density = clamp(density, 0.0, 1.0);
        
        // 如果密度接近0，跳过计算
        if (density < 0.005) {
            point += step;
            continue;
        }
        
        // 计算太阳可见度
        float sunVisibility = calculateSunVisibility(point, sunDirection);
        
        // 计算相位函数
        float cosTheta = dot(rayDir, sunDirection);
        float phaseValue = phaseFunction(cosTheta);
        
        // 计算直接光照
        vec3 directLight = lightColor * sunVisibility * phaseValue;
        
        // 计算环境光
        vec3 ambientLight = calculateAmbientLight(point);
        
        // 计算背光增强
        vec3 backLight = calculateBackLight(point, rayDir);
        
        // 组合光照
        vec3 totalLight = directLight * 1.0 + ambientLight * 0.4 + backLight * 0.3;
        
        // 使用寒霜引擎的散射光积分公式
        vec3 sigmaS = vec3(density);
        const float sigmaA = 0.0;
        vec3 sigmaE = max(vec3(1e-8f), sigmaA + sigmaS);
        
        // 使用Beer定律计算步长透射率
        float stepTransmittance = exp(-density * stepLength * 1.2);
        
        // 计算步进点的散射光
        vec3 stepScattering = totalLight;
        vec3 sactterLitStep = stepScattering * sigmaS;
        sactterLitStep = transmittance * (sactterLitStep - sactterLitStep * stepTransmittance);
        sactterLitStep /= sigmaE;
        scattering += sactterLitStep;
        
        // 更新透射率
        transmittance *= stepTransmittance;
        
        // 如果透射率接近0，可以提前退出循环
        if(transmittance < 0.01) {
            transmittance = 0.0;
            break;
        }
        
        // 更新点位置
        point += step;
    }
    
    // 返回云层的颜色和alpha值
    // vec4的rgb分量是散射光，a分量是不透明度
    vec4 result = vec4(scattering, 1.0 - transmittance);
    
    // 确保返回的值在合理范围内
    result.rgb = max(result.rgb, vec3(0.0));
    result.a = clamp(result.a, 0.0, 1.0);
    
    // 如果云层完全透明，则返回透明黑色
    if (result.a < 0.001) {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
    
    return result;
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
    vec3 direction = normalize(worldPos - cameraPosition);

    // 计算大气散射背景
    vec3 skyColor = calculateAtmosphericLight(direction);
    
    // 调试：检查skyColor是否有效
    if (isnan(skyColor.r) || isnan(skyColor.g) || isnan(skyColor.b) || 
        isinf(skyColor.r) || isinf(skyColor.g) || isinf(skyColor.b)) {
        skyColor = vec3(0.2, 0.4, 0.8); // 默认天空蓝
    }
    
    // 调试：如果skyColor全为0，则使用默认颜色
    if (length(skyColor) < 0.001) {
        skyColor = vec3(0.4, 0.6, 1.0); // 默认天空蓝
    }
    
    // 获取体积云颜色（完整光照版本）
    vec4 cloudResult = getCloudWithFullLighting(worldPos, cameraPosition, sunDirection * 1000.0);
    
    // 调试：检查cloudResult是否有效
    if (isnan(cloudResult.r) || isnan(cloudResult.g) || isnan(cloudResult.b) || isnan(cloudResult.a) ||
        isinf(cloudResult.r) || isinf(cloudResult.g) || isinf(cloudResult.b) || isinf(cloudResult.a)) {
        cloudResult = vec4(0.0, 0.0, 0.0, 0.0);
    }
    
    // 调试：如果cloudResult全为0，则使用默认颜色
    if (length(cloudResult.rgb) < 0.001 && cloudResult.a > 0.999) {
        // 如果没有散射光且几乎完全透射，则返回默认云颜色
        cloudResult = vec4(0.8, 0.8, 0.8, 0.5);
    }
    
    // 使用基于物理的渲染公式: finalColor = transmittance * skyBackgroundColor + scattering
    // cloudResult.a 是透射率
    // 最终颜色 = 透射率 * 背景天空颜色 + 散射光
    vec3 finalColor = cloudResult.a * skyColor + cloudResult.rgb;
    
    // 确保最终颜色不会全黑
    finalColor = max(finalColor, vec3(0.02));
    
    // 最终颜色范围检查
    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));
    
    color = vec4(finalColor, 1.0);
}