#version 330
in vec3 vWorldPosition;
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;
in vec3 vCameraPosition;

out vec4 color;

uniform float mieDirectionalG;
uniform vec3 up;
uniform vec3 cameraPosition;
uniform float iTime;  // 添加时间uniform用于动画

// 云层uniforms
uniform float cloudDensity;
uniform float cloudHeight;
uniform float cloudBaseHeight;
uniform sampler2D cloudMap;  // 主云图纹理采样器 (Worley噪声)
uniform sampler2D detailMap; // 细节纹理采样器 (Perlin噪声)
uniform sampler2D coverageMap; // 低频噪声纹理采样器 (覆盖遮罩)

// constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

const float n = 1.0003;
const float N = 2.545E25;

// optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
// 66 arc seconds -> degrees, and the cosine of that
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
// 1.0 / ( 4.0 * pi )
const float ONE_OVER_FOURPI = 0.07957747154594767;

float rayleighPhase(float cosTheta) 
{
    return THREE_OVER_SIXTEENPI * (1.0 + pow(cosTheta, 2.0));
}

float hgPhase(float cosTheta, float g) 
{
    float g2 = pow(g, 2.0);
    float inverse = 1.0 / pow(1.0 - 2.0 * g * cosTheta + g2, 1.5);
    return ONE_OVER_FOURPI * ((1.0 - g2) * inverse);
}

// 采样云图纹理
float sampleCloudMap(vec2 coord) {
    // 使用fract确保纹理坐标在[0,1]范围内
    vec2 clampedCoord = fract(coord);
    return texture2D(cloudMap, clampedCoord).r;
}

// 采样细节纹理
float sampleDetailMap(vec2 coord) {
    // 使用fract确保纹理坐标在[0,1]范围内
    vec2 clampedCoord = fract(coord);
    return texture2D(detailMap, clampedCoord).r;
}

// 采样覆盖遮罩
float sampleCoverageMap(vec2 coord) {
    // 使用fract确保纹理坐标在[0,1]范围内
    vec2 clampedCoord = fract(coord);
    return texture2D(coverageMap, clampedCoord).r;
}

// 生成云形状(包含覆盖遮罩)
float generateCloudShape(vec2 uv) {
    // === 第一步:生成覆盖遮罩(云的分布) ===
    // 使用非常低频的噪声,创建大块的"有云区域"和"无云区域"
    float coverage = 0.0;
    
    // 多层低频噪声叠加,创建不规则的云团分布
    coverage += sampleCoverageMap(uv * 0.1) * 0.5;   // 超大尺度分布
    coverage += sampleCoverageMap(uv * 0.2) * 0.3;   // 大尺度分布
    coverage += sampleCoverageMap(uv * 0.5) * 0.2;   // 中等尺度分布
    
    // 应用阈值,创建明确的"有云/无云"区域
    float coverageThreshold = 0.5;  // 调整这个值控制云的覆盖率(0.5 = 50%天空有云)
    if (coverage < coverageThreshold) {
        return 0.0; // 这个区域没有云
    }
    
    // 将coverage重新映射到[0,1]范围,用于后续混合
    float cloudPresence = (coverage - coverageThreshold) / (1.0 - coverageThreshold);
    
    // === 第二步:在有云的区域生成云的形状细节 ===
    float cloudShape = 0.0;
    
    // 不同频率和权重的噪声层
    cloudShape += sampleCloudMap(uv * 0.5) * 0.5;     // 大尺度云朵形状
    cloudShape += sampleCloudMap(uv * 1.0) * 0.3;     // 中等尺度云朵细节
    cloudShape += sampleCloudMap(uv * 2.0) * 0.2;     // 小尺度云朵细节
    
    // 添加细节噪声层
    cloudShape += sampleDetailMap(uv * 1.0) * 0.4;
    cloudShape += sampleDetailMap(uv * 2.0) * 0.3;
    cloudShape += sampleDetailMap(uv * 4.0) * 0.2;
    cloudShape += sampleDetailMap(uv * 8.0) * 0.1;
    
    // === 第三步:组合覆盖遮罩和云形状 ===
    // 用cloudPresence调制云的密度
    cloudShape *= cloudPresence;
    
    return cloudShape;
}

// 获取云层alpha值
float getCloudAlpha(vec3 direction) {
    // 旋转90度，使云层铺在天空上（绕X轴旋转90度）
    vec3 rotatedDirection = vec3(direction.x, direction.z, -direction.y);
    
    // 只在上半球渲染云（y轴正方向）
    if (rotatedDirection.y <= 0.0) {
        return 0.0;
    }
    
    // 定义云层高度
    float cloudAltitude = 1000.0;
    
    // 计算射线与云层平面的交点
    float t = cloudAltitude / rotatedDirection.y;
    vec3 cloudPos = rotatedDirection * t;
    
    // 使用UV坐标采样云纹理，增加缩放因子使云朵更分散
    vec2 cloudUV = cloudPos.xz * 0.001;  // 增加缩放因子，使云朵更大更分散
    
    // 添加时间偏移实现云移动
    float speedShape = iTime * 0.0001;
    vec2 windDir = vec2(speedShape, speedShape * 0.2); // 风向
    cloudUV += windDir;  // 风速
    
    // 限制只在纹理的中心区域渲染云
    // 根据y坐标判断是否为边缘区域
    float yCoord = abs(rotatedDirection.y); // 使用绝对值
    float edgeThreshold = 0.1; // 靠近y=0的位置为边缘
    if (yCoord < edgeThreshold) {
        return 0.0; // 在边缘区域不渲染云
    }
    
    // 生成云形状(已包含覆盖遮罩)
    float cloudShape = generateCloudShape(cloudUV);
    
    // 应用密度阈值，创建更清晰的云朵边缘
    float densityThreshold = 0.1;
    if (cloudShape < densityThreshold) {
        cloudShape = 0.0;
    } else {
        // 增强云层的对比度
        cloudShape = (cloudShape - densityThreshold) / (1.0 - densityThreshold);
        cloudShape = pow(cloudShape, 0.7);
    }
    
    // 根据高度角度调整云的透明度
    float cloudFade = smoothstep(0.0, 0.1, rotatedDirection.y);
    
    // 应用云密度参数
    float finalDensity = cloudShape * cloudFade * cloudDensity * 0.1;
    
    return finalDensity;
}

// 计算大气散射光照
vec3 calculateAtmosphericLight(vec3 direction, vec3 sunDir) {
    // optical length
    // cutoff angle at 90 to avoid singularity in next formula.
    float zenithAngle = acos(max(0.0, dot(up, direction)));
    float inverse = 1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - ((zenithAngle * 180.0) / pi), -1.253));
    float sR = rayleighZenithLength * inverse;
    float sM = mieZenithLength * inverse;

    // combined extinction factor
    vec3 Fex = exp(-(vBetaR * sR + vBetaM * sM));

    // in scattering
    float cosTheta = dot(direction, sunDir);

    float rPhase = rayleighPhase(cosTheta * 0.5 + 0.5);
    vec3 betaRTheta = vBetaR * rPhase;

    float mPhase = hgPhase(cosTheta, mieDirectionalG);
    vec3 betaMTheta = vBetaM * mPhase;

    vec3 Lin = pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * (1.0 - Fex), vec3(1.5));
    Lin *= mix(vec3(1.0), pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * Fex, vec3(1.0 / 2.0)), clamp(pow(1.0 - dot(up, sunDir), 5.0), 0.0, 1.0));

    // nightsky
    float theta = acos(direction.y);
    float phi = atan(direction.z, direction.x);
    vec2 uv = vec2(phi, theta) / vec2(2.0 * pi, pi) + vec2(0.5, 0.0);
    vec3 L0 = vec3(0.1) * Fex;

    // composition + solar disc
    float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (vSunE * 19000.0 * Fex) * sundisk;

    vec3 texColor = (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);

    vec3 retColor = pow(texColor, vec3(1.0 / (1.2 + (1.2 * vSunfade))));

    return retColor;
}

void main() 
{
    // 使用世界坐标计算方向
    vec3 direction = normalize(vWorldPosition - cameraPosition);
    vec3 sunDir = vSunDirection;

    // 计算大气散射颜色
    vec3 skyColor = calculateAtmosphericLight(direction, sunDir);
    
    // 获取云层不透明度
    float cloudAlpha = getCloudAlpha(direction);
    
    // 简化云层颜色混合
    vec3 cloudColor = vec3(1.0, 1.0, 1.0);  // 纯白色云
    vec3 finalColor = mix(skyColor, cloudColor, cloudAlpha);
    
    // 确保颜色不会全黑
    finalColor = max(finalColor, vec3(0.02));
    
    color = vec4(finalColor, 1.0);
}