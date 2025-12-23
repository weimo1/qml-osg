#version 430

// Compute Shader 用于生成 Transmittance LUT
layout(local_size_x = 8, local_size_y = 8) in;

// 输出纹理
layout(rgba16f, binding = 0) uniform image2D transmittanceLUT;

// 大气参数 (直接在着色器中写死)
const float kSeaLevel = 0.0;
const float kPlanetRadius = 6371.0;     // km
const float kAtmosphereHeight = 100.0;  // km
const float kSunLightIntensity = 20.0;

const vec3 kSunLightColor = vec3(1.0, 1.0, 1.0);
const float kSunDiskAngle = 0.0093;

const float kRayleighScatteringScale = 1.0;
const float kRayleighScatteringScalarHeight = 8.0;
const float kMieScatteringScale = 1.0;
const float kMieAnisotropy = 0.8;

const float kMieScatteringScalarHeight = 1.2;
const float kOzoneAbsorptionScale = 1.0;
const float kOzoneLevelCenterHeight = 25.0;
const float kOzoneLevelWidth = 15.0;

const float kAtmosphereRadius = 6471.0;  // km

const vec3 kBetaR = vec3(5.8e-6, 1.35e-5, 2.95e-5);  // 瑞利散射
const vec3 kBetaM = vec3(2.0e-5);                     // 米氏散射
const float kScaleHeightR = 8.0;   // km
const float kScaleHeightM = 1.2;   // km

const int TRANSMITTANCE_WIDTH  = 256;
const int TRANSMITTANCE_HEIGHT = 64;

// 安全的平方根
float SafeSqrt(float x) {
    return sqrt(max(0.0, x));
}

// UV -> (r, mu) 的映射
void GetRMuFromTransmittanceUV(vec2 uv, out float r, out float mu)
{
    // uv 范围 [0, 1]
    
    // H: 从地表到大气层顶部的最大切线长度
    float H = sqrt(kAtmosphereRadius * kAtmosphereRadius - 
                   kPlanetRadius * kPlanetRadius);
    
    // 从 uv.y 反推 rho（到地平线的切线长度）
    float rho = H * uv.y;
    
    // 从 rho 反推 r（距地心距离）
    r = sqrt(rho * rho + kPlanetRadius * kPlanetRadius);
    
    // 从 uv.x 反推 mu
    // d_min: 从当前高度垂直向上到大气层顶部的距离
    float d_min = kAtmosphereRadius - r;
    // d_max: 从当前高度沿切线方向到大气层顶部的距离
    float d_max = rho + H;
    
    // 反推距离 d
    float d = d_min + uv.x * (d_max - d_min);
    
    // 从 d 反推 mu
    if (d == 0.0) {
        mu = 1.0;  // 垂直向上
    } else {
        // 利用余弦定理: d^2 = r^2 + H^2 - 2*r*H*cos(theta)
        // 简化后: mu = (H^2 - rho^2 - d^2) / (2*r*d)
        mu = (H * H - rho * rho - d * d) / (2.0 * r * d);
    }
    
    // 限制在 [-1, 1]
    mu = clamp(mu, -1.0, 1.0);
}

// 计算从点 p 沿方向 mu 到大气层顶部的距离
float DistanceToAtmosphereTop(float r, float mu)
{
    // 判别式
    float discriminant = r * r * (mu * mu - 1.0) + 
                        kAtmosphereRadius * kAtmosphereRadius;
    
    return max(0.0, -r * mu + SafeSqrt(discriminant));
}

// 计算 Transmittance
vec3 ComputeTransmittance(float r, float mu)
{
    // 采样数量
    const int SAMPLE_COUNT = 40;
    
    // 计算到大气层顶部的距离
    float distance = DistanceToAtmosphereTop(r, mu);
    
    if (distance <= 0.0) {
        return vec3(1.0);
    }
    
    float dx = distance / float(SAMPLE_COUNT);
    vec3 sum = vec3(0.0);
    
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        // 采样点位置
        float t = (float(i) + 0.5) * dx;
        
        // 当前采样点距地心的距离
        float r_i = sqrt(r * r + t * t + 2.0 * r * t * mu);
        
        // 高度
        float h = r_i - kPlanetRadius;
        h = max(h, 0.0);
        
        // 瑞利和米氏散射系数（湮灭系数）
        vec3 extinction = kBetaR * exp(-h / kScaleHeightR) + 
                         kBetaM * exp(-h / kScaleHeightM);
        
        // 累积光学深度
        sum += extinction * dx;
    }
    
    // 透射率 = exp(-光学深度)
    return exp(-sum);
}

void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texSize = imageSize(transmittanceLUT);
    
    // 检查边界
    if (texelCoord.x >= texSize.x || texelCoord.y >= texSize.y) {
        return;
    }
    
    // 计算 UV (范围 [0, 1])
    vec2 uv = (vec2(texelCoord) + 0.5) / vec2(texSize);
    
    // 从 UV 反推 (r, mu)
    float r, mu;
    GetRMuFromTransmittanceUV(uv, r, mu);
    
    // 计算 Transmittance
    vec3 transmittance = ComputeTransmittance(r, mu);
    
    // 写入纹理
    imageStore(transmittanceLUT, texelCoord, vec4(transmittance, 1.0));
}