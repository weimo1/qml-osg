#version 330
in vec3 vWorldPosition;
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;
in vec3 vCameraPosition;

// 云朵相关输入
in vec3 vDirection;
in float vCloudDensity;
in float vTime;
in vec3 vWorldPos;
in float vCloudSpeed;

out vec4 color;

uniform float mieDirectionalG;
uniform vec3 up;
uniform float time;

// 云层uniforms
uniform float cloudDensity;
uniform float cloudHeight;
uniform float coverageThreshold;
uniform float densityThreshold;
uniform float edgeThreshold;

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
    // 防止除零错误
    float denominator = 1.0 - 2.0 * g * cosTheta + g2;
    if (denominator < 1e-6) {
        denominator = 1e-6;
    }
    float inverse = 1.0 / pow(denominator, 1.5);
    return ONE_OVER_FOURPI * ((1.0 - g2) * inverse);
}

// 采样云图纹理
float sampleCloudMap(vec2 coord) {
    vec2 clampedCoord = fract(coord);
    return texture2D(cloudMap, clampedCoord).r;
}

// 采样细节纹理
float sampleDetailMap(vec2 coord) {
    vec2 clampedCoord = fract(coord);
    return texture2D(detailMap, clampedCoord).r;
}

// 采样覆盖遮罩
float sampleCoverageMap(vec2 coord) {
    vec2 clampedCoord = fract(coord);
    return texture2D(coverageMap, clampedCoord).r;
}

// 简化的云形状生成函数
float generateCloudShape(vec2 uv) {
    // 生成覆盖遮罩(云的分布)
    float coverage = 0.0;
    coverage += sampleCoverageMap(uv * 0.1) * 0.5;   // 大尺度分布
    coverage += sampleCoverageMap(uv * 0.2) * 0.3;   // 中等尺度分布
    coverage += sampleCoverageMap(uv * 0.5) * 0.2;   // 小尺度分布
    
    // 应用阈值,创建明确的"有云/无云"区域
    if (coverage < coverageThreshold) {
        return 0.0;
    }
    
    // 在有云的区域生成云的形状细节
    float cloudShape = 0.0;
    
    // 主云图噪声
    cloudShape += sampleCloudMap(uv * 0.5) * 0.5;     // 大尺度云朵形状
    cloudShape += sampleCloudMap(uv * 1.0) * 0.3;     // 中等尺度云朵细节
    cloudShape += sampleCloudMap(uv * 2.0) * 0.2;     // 小尺度云朵细节
    
    // 添加细节噪声层
    cloudShape += sampleDetailMap(uv * 1.0) * 0.4;
    cloudShape += sampleDetailMap(uv * 2.0) * 0.3;
    cloudShape += sampleDetailMap(uv * 4.0) * 0.2;
    cloudShape += sampleDetailMap(uv * 8.0) * 0.1;
    
    // 用coverage调制云的密度
    float cloudPresence = (coverage - coverageThreshold) / (1.0 - coverageThreshold);
    cloudShape *= cloudPresence;
    
    return cloudShape;
}

// 获取2D云层alpha值
float getCloudAlpha(vec3 direction) {
    // 旋转90度,使云层铺在天空上(绕X轴旋转90度)
    vec3 rotatedDirection = vec3(direction.x, direction.z, -direction.y);
    
    // 只在上半球渲染云(y轴正方向)
    if (rotatedDirection.y <= 0.0) {
        return 0.0;
    }
    
    // 定义云层高度
    float cloudAltitude = cloudHeight;
    
    // 防止除零错误
    if (abs(rotatedDirection.y) < 1e-6) {
        return 0.0;
    }
    
    // 计算射线与云层平面的交点
    float t = cloudAltitude / rotatedDirection.y;
    vec3 cloudPos = rotatedDirection * t;
    
    // 使用UV坐标采样云纹理
    vec2 cloudUV = cloudPos.xz * 0.0005;
    
    // 添加时间偏移实现云移动
    float speedShape = vTime * 0.001 * vCloudSpeed;
    vec2 windDir = vec2(speedShape, speedShape * 0.2);
    cloudUV += windDir;


     // 限制只在纹理的中心区域渲染云
    // 根据y坐标判断是否为边缘区域
    float yCoord = abs(rotatedDirection.y); // 使用绝对值
    if (yCoord < edgeThreshold) {
        return 0.0; // 在边缘区域不渲染云
    }
    
    // 生成云形状
    float cloudShape = generateCloudShape(cloudUV);
    
    // 应用密度阈值
    if (cloudShape < densityThreshold) {
        cloudShape = 0.0;
    } else {
        cloudShape = (cloudShape - densityThreshold) / (1.0 - densityThreshold);
        cloudShape = pow(cloudShape, 0.7);
    }
    
    // 根据高度角度调整云的透明度
    float cloudFade = smoothstep(0.0, 0.1, rotatedDirection.y);
    
    // 应用云密度参数
    float finalDensity = cloudShape * cloudFade * vCloudDensity * 0.025;
    
    return finalDensity;
}

// 计算大气散射光照
vec3 calculateAtmosphericLight(vec3 direction, vec3 sunDir) {
    // optical length
    float zenithAngle = acos(max(0.0, dot(up, direction)));
    float inverse = 1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - ((zenithAngle * 180.0) / pi), -1.253));
    float sR = rayleighZenithLength * inverse;
    float sM = mieZenithLength * inverse;

    // combined extinction factor
    vec3 Fex = exp(-(vBetaR * sR + vBetaM * sM));
    
    // 限制Fex的最大值，防止过亮
    Fex = min(Fex, vec3(10.0));

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
    vec3 worldPos = vWorldPosition;
    vec3 direction = vDirection;
    vec3 sunDir = vSunDirection;

    // 计算大气散射颜色
    vec3 skyColor = calculateAtmosphericLight(direction, sunDir);
    
    // 获取2D云层alpha值
    float cloudAlpha = getCloudAlpha(direction);
    
    // 组合不同层次的云朵
    vec3 cloudColor = vec3(0.9, 0.9, 0.9); // 基础云朵颜色，稍微暗一些
    vec3 cloudShade = vec3(0.6, 0.6, 0.7); // 阴影颜色，更暗一些
    
    // 根据太阳方向计算云朵光照
    float sunLight = max(0.0, dot(normalize(worldPos), sunDir));
    vec3 litCloudColor = mix(cloudShade, cloudColor, sunLight * 0.4 + 0.3);
    
    // 应用云朵层次
    vec3 finalCloudColor = mix(cloudShade, litCloudColor, cloudAlpha);
    
    // 云朵对光线的散射影响
    vec3 cloudScattering = finalCloudColor * (cloudAlpha * 2.0);
    
    // 将云朵效果添加到最终颜色，调整混合比例
    vec3 texColor = skyColor * 0.5 + cloudScattering * 0.6;

    // 应用色调映射和颜色空间转换
    vec3 retColor = pow(texColor, vec3(1.0 / (1.2 + (1.0 * vSunfade))));
    
    color = vec4(retColor, 1.0);
}