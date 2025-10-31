#version 330
in vec3 vWorldPosition;
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;

out vec4 color;

uniform float mieDirectionalG;
uniform vec3 up;
uniform vec3 cameraPosition;
uniform float sunZenithAngle;
uniform float sunAzimuthAngle;
uniform float cloudDensity;
uniform float cloudHeight;
uniform vec3 atmosphereColor;

const float pi = 3.141592653589793238462643383279502884197169;
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
const float ONE_OVER_FOURPI = 0.07957747154594767;

// 体积云参数
const float CLOUD_MIN_HEIGHT = 1500.0;  // 云层最低高度
const float CLOUD_MAX_HEIGHT = 4000.0;  // 云层最高高度
const int RAY_STEPS = 64;               // 光线步进步数
const int LIGHT_STEPS = 6;              // 光照采样步数
const float ABSORPTION = 0.35;          // 吸收系数

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

// Perlin噪声
float noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float n = i.x + i.y * 157.0 + 113.0 * i.z;
    return mix(
        mix(
            mix(fract(sin(n + 0.0) * 43758.5453), fract(sin(n + 1.0) * 43758.5453), f.x),
            mix(fract(sin(n + 157.0) * 43758.5453), fract(sin(n + 158.0) * 43758.5453), f.x),
            f.y
        ),
        mix(
            mix(fract(sin(n + 113.0) * 43758.5453), fract(sin(n + 114.0) * 43758.5453), f.x),
            mix(fract(sin(n + 270.0) * 43758.5453), fract(sin(n + 271.0) * 43758.5453), f.x),
            f.y
        ),
        f.z
    );
}

// 分形布朗运动 - 生成云的形状
float fbm(vec3 p)
{
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    
    for(int i = 0; i < 5; i++)
    {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value;
}

// 云密度函数 - 使用FBM生成体积云
float cloudDensityFunction(vec3 pos)
{
    float height = pos.z;
    
    // 高度范围检查
    if (height < CLOUD_MIN_HEIGHT || height > CLOUD_MAX_HEIGHT) {
        return 0.0;
    }
    
    // 高度衰减
    float heightGradient = smoothstep(CLOUD_MIN_HEIGHT, CLOUD_MIN_HEIGHT + 500.0, height) * 
                          (1.0 - smoothstep(CLOUD_MAX_HEIGHT - 500.0, CLOUD_MAX_HEIGHT, height));
    
    // 基础云形状
    float baseCloud = fbm(pos * 0.0008);
    
    // 细节噪声
    float detail = fbm(pos * 0.003) * 0.5;
    
    // 组合云密度
    float density = (baseCloud + detail - 0.5) * heightGradient;
    density = max(0.0, density) * cloudDensity;
    
    return density;
}

// 光线与云层盒交叉检测
bool rayBoxIntersection(vec3 rayOrigin, vec3 rayDir, out float tMin, out float tMax)
{
    vec3 boxMin = vec3(-50000.0, -50000.0, CLOUD_MIN_HEIGHT);
    vec3 boxMax = vec3(50000.0, 50000.0, CLOUD_MAX_HEIGHT);
    
    vec3 invDir = 1.0 / rayDir;
    vec3 t0 = (boxMin - rayOrigin) * invDir;
    vec3 t1 = (boxMax - rayOrigin) * invDir;
    
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    tMin = max(max(tmin.x, tmin.y), tmin.z);
    tMax = min(min(tmax.x, tmax.y), tmax.z);
    
    return tMax > max(tMin, 0.0);
}

// 计算大气散射光照
vec3 calculateAtmosphericLight(vec3 direction)
{
    vec3 sunDir = vSunDirection;
    
    float zenithAngle = acos(max(0.0, dot(up, direction)));
    float inverse = 1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - ((zenithAngle * 180.0) / pi), -1.253));
    float sR = rayleighZenithLength * inverse;
    float sM = mieZenithLength * inverse;
    
    vec3 Fex = exp(-(vBetaR * sR + vBetaM * sM));
    
    float cosTheta = dot(direction, sunDir);
    
    float rPhase = rayleighPhase(cosTheta * 0.5 + 0.5);
    vec3 betaRTheta = vBetaR * rPhase;
    
    float mPhase = hgPhase(cosTheta, mieDirectionalG);
    vec3 betaMTheta = vBetaM * mPhase;
    
    vec3 Lin = pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * (1.0 - Fex), vec3(1.5));
    Lin *= mix(vec3(1.0), pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * Fex, vec3(1.0 / 2.0)), 
               clamp(pow(1.0 - dot(up, sunDir), 5.0), 0.0, 1.0));
    
    vec3 L0 = vec3(0.1) * Fex;
    
    float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (vSunE * 19000.0 * Fex) * sundisk;
    
    return (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);
}

// 光照采样 - 计算向太阳方向的透射
float lightMarch(vec3 pos, vec3 sunDir)
{
    float totalDensity = 0.0;
    float stepSize = (CLOUD_MAX_HEIGHT - CLOUD_MIN_HEIGHT) / float(LIGHT_STEPS);
    
    for(int i = 0; i < LIGHT_STEPS; i++)
    {
        vec3 samplePos = pos + sunDir * stepSize * float(i);
        totalDensity += cloudDensityFunction(samplePos) * stepSize;
    }
    
    // Beer-Lambert定律 - 光线透射
    return exp(-totalDensity * ABSORPTION);
}

void main() 
{
    vec3 direction = normalize(vWorldPosition - cameraPosition);
    vec3 sunDir = vSunDirection;
    
    // 计算大气天空盒颜色
    vec3 skyColor = calculateAtmosphericLight(direction) * 2.0;
    
    // 光线步进体积云渲染
    float tMin, tMax;
    vec3 rayOrigin = cameraPosition;
    vec3 rayDir = direction;
    
    // 检测与云层交叉
    if (!rayBoxIntersection(rayOrigin, rayDir, tMin, tMax)) {
        // 没有交叉，直接返回天空颜色
        vec3 finalColor = pow(skyColor, vec3(1.0 / (1.2 + (1.2 * vSunfade))));
        finalColor *= 0.2;
        color = vec4(finalColor, 1.0);
        return;
    }
    
    // 光线步进参数
    float stepSize = (tMax - tMin) / float(RAY_STEPS);
    vec3 step = rayDir * stepSize;
    vec3 pos = rayOrigin + rayDir * tMin;
    
    // 累计透射率和散射光
    float transmittance = 1.0;
    vec3 scatteredLight = vec3(0.0);
    
    // 光线步进循环
    for(int i = 0; i < RAY_STEPS; i++)
    {
        float density = cloudDensityFunction(pos);
        
        if(density > 0.001)
        {
            // 计算向太阳方向的透射
            float lightTransmittance = lightMarch(pos, sunDir);
            
            // 从大气获取环境光照
            vec3 ambientLight = calculateAtmosphericLight(rayDir) * 0.3;
            
            // 太阳直接光照
            vec3 sunLight = vec3(1.0, 0.95, 0.9) * vSunE * 0.00008;
            
            // HG相位函数 - 前向散射
            float cosTheta = dot(rayDir, sunDir);
            float phase = hgPhase(cosTheta, 0.6);
            
            // 组合光照
            vec3 lighting = (sunLight * lightTransmittance * phase + ambientLight) * density;
            
            // 累计散射光
            scatteredLight += lighting * transmittance * stepSize;
            
            // 更新透射率
            transmittance *= exp(-density * ABSORPTION * stepSize);
            
            // 早期退出优化
            if(transmittance < 0.01) break;
        }
        
        pos += step;
    }
    
    // 混合云和天空
    vec3 finalColor = skyColor * transmittance + scatteredLight;
    
    // 色调映射和伽玛校正
    finalColor = pow(finalColor, vec3(1.0 / (1.2 + (1.2 * vSunfade))));
    finalColor *= 0.2;
    
    color = vec4(finalColor, 1.0);
}