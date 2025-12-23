#version 460

in vec2 v_texCoord;
in vec3 view_ray;
out vec4 frag_color;

const float PI = 3.14159265358979323846;


struct AtmosphereParameter
{
    float SeaLevel;
    float PlanetRadius;
    float AtmosphereHeight;
    float SunLightIntensity;
    vec3 SunLightColor;
    float SunDiskAngle;
    float RayleighScatteringScale;
    float RayleighScatteringScalarHeight;
    float MieScatteringScale;
    float MieAnisotropy;
    float MieScatteringScalarHeight;
    float OzoneAbsorptionScale;
    float OzoneLevelCenterHeight;
    float OzoneLevelWidth;
};

// Uniform变量声明
uniform AtmosphereParameter atmosphereParams;
uniform vec3 camera_pos;
uniform vec3 sun_direction;
uniform vec2 iResolution;
uniform float exposure;
uniform vec3 earth_center;


uniform vec2 sun_size;
uniform float iTime;

// 云盒uniform参数

// ============ 云的外观参数 ============
// 云的外观参数
const vec3 kCloudColor = vec3(1.0, 1.0, 1.0);         // 纯白色
const vec3 kCloudShade = vec3(0.8, 0.8, 0.8);         // 浅灰色阴影
const float kCloudExtinction =0.8;                   // 增加消光系数
// ============ 改进的云盒参数 ============

// 云盒参数 (恢复使用固定值)
const vec3 kCloudBoxCenter = vec3(0.0, 0.0, 6364000.0);
const vec3 kCloudBoxSize = vec3(1000.0, 1000.0, 50.0);
const vec3 kCloudBoxMin = kCloudBoxCenter - kCloudBoxSize * 0.5;
const vec3 kCloudBoxMax = kCloudBoxCenter + kCloudBoxSize * 0.5;

// 移除uniform变量相关的代码

uniform sampler2D transmittanceLUT;
uniform sampler2D mutlutLUT;

const float time = 0.2;



uniform sampler3D _ShapeNoiceTex;      // 3D基础形状纹理（包含Perlin和Worley噪声）
uniform sampler3D _DetailNoiceTex;     // 3D细节纹理（高频Worley噪声）
uniform sampler2D _WeatherNoiceTex;    // 2D天气纹理（控制云的覆盖率等属性）
uniform sampler2D blueNoiseTexture;    // 蓝噪声纹理，用于消除云渲染分层

uniform sampler2D cloudTexture;

vec2 GetTransmittanceLutUv(float bottomRadius, float topRadius, float cosTheta, float r)
{
    // 确保在有效范围内
    r = clamp(r, bottomRadius, topRadius);
    cosTheta = clamp(cosTheta, -1.0, 1.0);
    
    // 计算归一化的半径
    float H = sqrt(topRadius * topRadius - bottomRadius * bottomRadius);
    float rho = sqrt(r * r - bottomRadius * bottomRadius);
    
    // 映射 r -> v
    float v = rho / H;
    
    // 映射 cos(theta) -> u
    // 注意：LUT 通常存储从 mu=cos(theta)=-1 到 1
    float u = (cosTheta + 1.0) * 0.5;  // 从 [-1, 1] 映射到 [0, 1]
    
    // 添加偏移避免边界问题
    float invWidth = 1.0 / float(256);
    float invHeight = 1.0 / float(64);
    u = clamp(u, invWidth, 1.0 - invWidth);
    v = clamp(v, invHeight, 1.0 - invHeight);
    
    return vec2(u, v);
}




vec3 TransmittanceToAtmosphere(in AtmosphereParameter param, vec3 p, vec3 dir, sampler2D lut)
{
    float bottomRadius = param.PlanetRadius;
    float topRadius = param.PlanetRadius + param.AtmosphereHeight;

    vec3 upVector = normalize(p);
    float cos_theta = dot(upVector, dir);
    float r = length(p);

    vec2 uv = GetTransmittanceLutUv(bottomRadius, topRadius, cos_theta, r);
    return texture(lut, uv).rgb;
}
vec3 RayleighCoefficient(in AtmosphereParameter param, float h)
{
    const vec3 sigma = vec3(5.802, 13.558, 33.1) * 1e-6;
    float H_R = param.RayleighScatteringScalarHeight;
    float rho_h = exp(-(h / H_R));
    return sigma * rho_h * param.RayleighScatteringScale;
}

float RayleiPhase(in AtmosphereParameter param, float cos_theta)
{
    return (3.0 / (16.0 * PI)) * (1.0 + cos_theta * cos_theta);
}

vec3 MieCoefficient(in AtmosphereParameter param, float h)
{
    const vec3 sigma = (3.996 * 1e-6) * vec3(1.0, 1.0, 1.0);
    float H_M = param.MieScatteringScalarHeight;
    float rho_h = exp(-(h / H_M));
    return sigma * rho_h * param.MieScatteringScale;
}

float MiePhase(in AtmosphereParameter param, float cos_theta)
{
    float g = param.MieAnisotropy;

    float a = 3.0 / (8.0 * PI);
    float b = (1.0 - g*g) / (2.0 + g*g);
    float c = 1.0 + cos_theta*cos_theta;
    float d = pow(1.0 + g*g - 2*g*cos_theta, 1.5);
    
    return a * b * (c / d);
}

vec3 Scattering(in AtmosphereParameter param, vec3 p, vec3 lightDir, vec3 viewDir)
{
    float cos_theta = dot(lightDir, viewDir);

    float h = length(p) - param.PlanetRadius;
    vec3 rayleigh = RayleighCoefficient(param, h) * RayleiPhase(param, cos_theta);
    vec3 mie = MieCoefficient(param, h) * MiePhase(param, cos_theta);

    return rayleigh + mie;
}

vec3 MieAbsorption(in AtmosphereParameter param, float h)
{
    const vec3 sigma = (4.4 * 1e-6) * vec3(1.0, 1.0, 1.0);
    float H_M = param.MieScatteringScalarHeight;
    float rho_h = exp(-(h / H_M));
    return sigma * rho_h * param.MieScatteringScale;
}

vec3 OzoneAbsorption(in AtmosphereParameter param, float h)
{
    #define sigma_lambda (vec3(0.650f, 1.881f, 0.085f)) * 1e-6
    float center = param.OzoneLevelCenterHeight;
    float width = param.OzoneLevelWidth;
    float rho = max(0, 1.0 - (abs(h - center) / width));
    return sigma_lambda * rho * param.OzoneAbsorptionScale;
}

float RayIntersectSphere(vec3 center, float radius, vec3 rayStart, vec3 rayDir)
{
  float OS = length(center - rayStart);
    float SH = dot(center - rayStart, rayDir);
    float OH = sqrt(OS*OS - SH*SH);
    float PH = sqrt(radius*radius - OH*OH);

    // ray miss sphere
    if(OH > radius) return -1;

    // use min distance
    float t1 = SH - PH;
    float t2 = SH + PH;
    float t = (t1 < 0) ? t2 : t1;

    return t;
}


vec3 GetMultiScattering(in AtmosphereParameter param, vec3 p, vec3 lightDir, sampler2D lut)
{
    float h = length(p) - param.PlanetRadius;
    vec3 sigma_s = RayleighCoefficient(param, h) + MieCoefficient(param, h); 
    
    float cosSunZenithAngle = dot(normalize(p), lightDir);
    vec2 uv = vec2(cosSunZenithAngle * 0.5 + 0.5, h / param.AtmosphereHeight);
    vec3 G_ALL = texture(lut, uv).rgb;
    
    return G_ALL * sigma_s;
}


vec3 Transmittance(in AtmosphereParameter param, vec3 p1, vec3 p2)
{
    const int N_SAMPLE = 16;  // 减少采样数量以提高性能

    vec3 dir = normalize(p2 - p1);
    float distance = length(p2 - p1);
    float ds = distance / float(N_SAMPLE);
    vec3 sum = vec3(0.0);
    vec3 p = p1 + (dir * ds) * 0.5;

    for(int i=0; i<N_SAMPLE; i++)
    {
        float h = length(p) - param.PlanetRadius;

        vec3 scattering = RayleighCoefficient(param, h) + MieCoefficient(param, h);
        vec3 absorption = OzoneAbsorption(param, h) + MieAbsorption(param, h);
        vec3 extinction = scattering + absorption;

        sum += extinction * ds;
        p += dir * ds;
    }

    return exp(-sum);
}



vec3 GetSkyView(
    in AtmosphereParameter param, vec3 camera, vec3 viewDir, vec3 lightDir
    )
{

    const int N_SAMPLE = 32;
    vec3 color = vec3(0.0);

    // 光线和大气层, 星球求交
    float atmosphereDist = RayIntersectSphere(
        vec3(0), param.PlanetRadius + param.AtmosphereHeight, 
        camera  , viewDir);

    float groundDist = RayIntersectSphere(vec3(0), param.PlanetRadius,
        camera  , viewDir);

    float maxDist = (groundDist > 0.0) ? min(atmosphereDist, groundDist) : atmosphereDist;

    float ds = maxDist / float(N_SAMPLE);
    vec3 sunLuminance = param.SunLightColor * param.SunLightIntensity;

    vec3 p = camera + (viewDir * ds) * 0.5;

    vec3 opticalDepth = vec3(0.0, 0.0, 0.0);


    for(int i=0; i<N_SAMPLE; i++)
    {
       
         float h = length(p) - param.PlanetRadius;
        vec3 extinction = RayleighCoefficient(param, h) + MieCoefficient(param, h) +  // scattering
                            OzoneAbsorption(param, h) + MieAbsorption(param, h);        // absorption
        opticalDepth += extinction * ds;



        float t = (float(i) + 0.5) * ds;
        
        vec3 p1 = camera + viewDir * t;

        float sunPathDist = RayIntersectSphere(
            vec3(0), param.PlanetRadius + param.AtmosphereHeight, 
            p1, lightDir);
        vec3 p2 = p1 + (-lightDir) * sunPathDist;

       vec3 t1 = TransmittanceToAtmosphere(param,p,lightDir,transmittanceLUT);
         //vec3 t1  = Transmittance (param, p1, p2);
           vec3 s  = Scattering(param, p, lightDir, viewDir);
        vec3 t2 = exp(-opticalDepth);
        
        // 单次散射
        vec3 inScattering = t1 * s * t2 * ds * sunLuminance;
        color += inScattering;

        vec3 multiScattering = GetMultiScattering(param, p, lightDir, mutlutLUT);
        
       color += multiScattering * t2 * ds * sunLuminance*0.2;


        p += viewDir * ds;
    }
    return color;
}


vec3 GetSunDisk(in AtmosphereParameter param, vec3 eyePos, vec3 viewDir, vec3 lightDir)
{
    // 计算入射光照
    float cosine_theta = dot(viewDir, lightDir);
    float theta = acos(cosine_theta) * (180.0 / PI);
    vec3 sunLuminance = param.SunLightColor * param.SunLightIntensity;

    // 判断光线是否被星球阻挡
    float disToPlanet = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius, eyePos, viewDir);
    if(disToPlanet >= 0) return vec3(0,0,0);

    // 和大气层求交
    float disToAtmosphere = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, eyePos, viewDir);
    if(disToAtmosphere < 0) return vec3(0,0,0);

    // 计算衰减
   vec3 hitPoint = eyePos + viewDir * disToAtmosphere;
   sunLuminance *= Transmittance(param, hitPoint, eyePos);

    if(theta < param.SunDiskAngle) return sunLuminance;
    return vec3(0,0,0);
}





// 辅助函数
float saturate(float x) { return clamp(x, 0.0, 1.0); }
float remap(float value, float lo, float ho, float ln, float hn) {
    return ln + (value - lo) * (hn - ln) / (ho - lo);
}
float GetDensityHeightGradient(vec3 pos, float min, float max)
{
    float heightGradient = (pos.y - min) / (max - min);
    return saturate(heightGradient);
}
// FBM变换矩阵
const mat3 m = mat3(
    0.00,  1.60,  1.20,
   -1.60,  0.72, -0.96,
   -1.20, -0.96,  1.28
);

// ============================================================================
// Hash 函数
// ============================================================================

float hash(float n) {
    return fract(cos(n) * 114514.1919);
}

// ============================================================================
// 3D 噪声函数
// ============================================================================

float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = smoothstep(0.0, 1.0, fract(x));
    
    float n = p.x + p.y * 10.0 + p.z * 100.0;
    
    return mix(
        mix(
            mix(hash(n + 0.0),   hash(n + 1.0),   f.x),
            mix(hash(n + 10.0),  hash(n + 11.0),  f.x), 
            f.y
        ),
        mix(
            mix(hash(n + 100.0), hash(n + 101.0), f.x),
            mix(hash(n + 110.0), hash(n + 111.0), f.x), 
            f.y
        ), 
        f.z
    );
}

// ============================================================================
// FBM (Fractional Brownian Motion) - 4个八度
// ============================================================================

float fbm(vec3 p) {
    float f = 0.0;
    
    f += 0.5000 * noise(p);
    p = m * p;
    
    f += 0.2500 * noise(p);
    p = m * p;
    
    f += 0.1666 * noise(p);
    p = m * p;
    
    f += 0.0834 * noise(p);
    
    return f;
}

// ============================================================================
// 核心密度函数 - 传入世界坐标pos
// ============================================================================


bool RayIntersectCloudBox(vec3 ray_origin, vec3 ray_dir, out float t_min, out float t_max) {
    // 使用固定的云盒参数
    vec3 box_min = kCloudBoxMin;
    vec3 box_max = kCloudBoxMax;
    
    // 2. 计算与六个平面的交点（Slab方法）
    vec3 inv_dir = 1.0 / ray_dir;  // 避免除法
    
    vec3 t0 = (box_min - ray_origin) * inv_dir;
    vec3 t1 = (box_max - ray_origin) * inv_dir;
    
    // 3. 确保t0是近点，t1是远点
    vec3 t_near = min(t0, t1);
    vec3 t_far = max(t0, t1);
    
    // 4. 找到最远的近点和最近的远点
    t_min = max(max(t_near.x, t_near.y), t_near.z);
    t_max = min(min(t_far.x, t_far.y), t_far.z);
    
    // 5. 确保交点在相机前方
    t_min = max(t_min, 0.0);
    
    // 6. 检查是否有效相交
    return t_max > t_min && t_max > 0.0;
}
float CalcuAbsorbance(float t, float d, float l)
{
    return t * d * l;
}
float HenyeyGreenstein(float cos, float anisotropy)
{
    float g = anisotropy;
    float gg = g * g;

    float a = 3 * (1 - gg);
    float b = 8 * PI * (2 + gg);
    float c = 1 + cos * cos;
    float d = pow((1 + gg - 2 * g * cos), 3 / 2);

    return a / b * c / d;
}
// ============================================================
// 基础相位函数
// ============================================================
float HenyeyGreensteinPhase(float g, float cosTheta) {
    float g2 = g * g;
    float numerator = 1.0 - g2;
    float denominator = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 / (4.0 * PI)) * (numerator / denominator);
}

// ============================================================
// 双瓣相位函数 (前向+后向散射)
// ============================================================
float DualLobPhase(float g0, float g1, float w, float cosTheta) {
    return mix(
        HenyeyGreensteinPhase(g0, cosTheta),  // 前向散射
        HenyeyGreensteinPhase(g1, cosTheta),  // 后向散射
        w
    );
}


float hgPhase(float g, float cosTheta)
{
    float numer = 1.0f - g * g;
    float denom = 1.0f + g * g + 2.0f * g * cosTheta;
    return numer / (4.0f * PI * denom * sqrt(denom));
}

float dualLobPhase(float g0, float g1, float w, float cosTheta)
{
    return mix(hgPhase(g0, cosTheta), hgPhase(g1, cosTheta), w);
}
// ============================================================
// Beer-Powder 效应 (云边缘增亮效果)
// ============================================================
float BeerPowder(float density, float cosTheta) {
    // Beer定律基础衰减
    float beer = exp(-density);
    
    // Powder效应: 薄云边缘散射增强
    float powder = 1.0 - exp(-density * 2.0);
    
    // 根据太阳角度调整powder强度
    float powder_strength = 1.0 * smoothstep(0.5, 1.0, cosTheta);
    
    return mix(beer, powder, powder_strength);
}

float GetCloudDensity(vec3 point_earth_space) {
    // 1. 边界检查 (保持不变)
    // ... (保持不变)
    vec3 local_pos = point_earth_space - kCloudBoxCenter;
    vec3 half_size = kCloudBoxSize * 0.5;
    if (abs(local_pos.x) > half_size.x || 
        abs(local_pos.y) > half_size.y || 
        abs(local_pos.z) > half_size.z) {
        return 0.0;
    }
    // 2. 归一化坐标 & 归一化高度 h [0, 1]
    vec3 normalized_pos = local_pos / half_size;  // [-1, 1]
    float h = normalized_pos.y * 0.5 + 0.5;       // [0, 1]

    // 3. 统一天气图采样
    // 采样频率与参考代码接近: normalized_pos.xz * 0.8 / 2.0 = 0.4
    vec2 weather_uv = normalized_pos.xz * 0.4 + 0.5; 
    vec4 weather_value = texture(_WeatherNoiceTex, weather_uv);

    // 从天气图计算覆盖率（借鉴您的逻辑）
    float coverage = pow(weather_value.r, 3); // r通道用于基础覆盖

// --- I. 垂直密度剖面 (高度曲线) ---

    // 1. 高度偏移 (云底/云顶扰动)
    // 采样频率保持不变：normalized_pos.xz * 0.3 + 0.5 (使用G通道)
    float base_height_offset = texture(_WeatherNoiceTex, normalized_pos.xz * 0.3 + 0.5).g;
    base_height_offset = (base_height_offset - 0.5) * 0.9;
    // 
    // 仅使用 base_height_offset 调整高度，简化高度细节扰动
    float bottom = 0.05 + base_height_offset;
    float top = 0.95 - base_height_offset * 0.5; 
    // 核心垂直调整：模拟云层压缩/拉伸 (等同于您代码中的 adjusted_height/modulated_height 的效果)
    // 注意：我们将 h 直接用于 smoothstep，不再使用 adjusted_height/modulated_height
    float bottom_fade = smoothstep(bottom, bottom + 0.25, h);
    float top_fade = 1.0 - smoothstep(top - 0.25, top, h);
    float height_density_curve = bottom_fade * top_fade;

    // 2. 中部鼓包 (积云特征) - **保留并简化**
    float mid_boost = smoothstep(0.2, 0.5, h) * smoothstep(0.8, 0.5, h);
    height_density_curve = mix(height_density_curve, 1.0, mid_boost * 0.6);

    // 3. 垂直厚度乘子 (天气图B通道)
    float cloud_thickness = 0.8 + weather_value.b * 0.6; // 频率与 weather_uv (0.4) 接近
    height_density_curve *= cloud_thickness; 
// --- II. 3D 形状噪声 (基础形状) ---

    // 沿用您的FBM/Worley噪声混合，但变量名和流程更清晰
    vec3 shape_uv = normalized_pos * 0.5 + 0.5;
    vec4 shape_low = texture(_ShapeNoiceTex, shape_uv * 0.5); 
    vec4 shape_mid = texture(_ShapeNoiceTex, shape_uv * 1.5); 

    // FBM 叠加 (保持您的权重调整)
    float base_shape = shape_low.r * 1.5 + // 低频
                        shape_mid.r * 1.2 ; // 中频
    // Worley噪声侵蚀 (保持您的强度调整)
    //base_shape *=2;
    float worley_fbm = dot(shape_low.gba, vec3(0.625, 0.25, 0.125));
    // remap(base_shape, 1.0 - worley_fbm * 0.6, 1.0, 0.0, 1.0)
    // 解释：将 base_shape 从 [1.0 - worley_fbm * 0.6, 1.0] 映射到 [0.0, 1.0]
    float density = remap(base_shape, 1.0 - worley_fbm * 0.4, 1.0, 0.0, 1.0); 

// --- III. 形状与密度曲线组合 (Shape + Height) ---

    // 垂直扰动 (取代 3D噪声调制 的厚度变化)
    // 3D 噪声调制: 借鉴参考代码中 detailNoiseMixByHeight 的思路，用高度混合
    vec3 shape_3d_uv = normalized_pos * 2.0 + 0.5;
    float shape_3d = texture(_ShapeNoiceTex, shape_3d_uv).g;
    // 使用 shape_3d 噪声来影响垂直密度曲线 (您原有逻辑)
    float shape_influence = smoothstep(0.3, 0.7, density);
    height_density_curve *= mix(0.5, 1.0, shape_3d * shape_influence + (1.0 - shape_influence));

    // 垂直扰动 (用于云顶细节，保持您的逻辑，但移除 modulated_height 依赖)
    vec3 curl_uv1 = normalized_pos * 0.8 + 0.5;
    vec3 curl_uv2 = normalized_pos * 2.0 + 0.5;
    float curl1 = texture(_ShapeNoiceTex, curl_uv1).r - 0.5;
    float curl2 = texture(_DetailNoiceTex, curl_uv2).g - 0.5;
    
    float vertical_distortion = (curl1 * 0.6 + curl2 * 0.3);
    // 扰动权重只在云层中部发挥作用，使用 h 代替 modulated_height
    float distortion_weight = smoothstep(0.1, 0.3, h) * smoothstep(0.9, 0.7, h); 
    height_density_curve = clamp(height_density_curve + vertical_distortion * distortion_weight, 0.0, 1.0);
    // 组合形状噪声和垂直密度
    density *= height_density_curve;
// --- IV. 后处理与细节侵蚀 ---

    // 1. 覆盖率混合 (借鉴参考代码中的 remap(basicCloudNoise, 1.0 - coverage, 1, 0, 1))
    // Remap(density, coverage_threshold * (1.0 - coverage), 1.0, 0.0, 1.0)
       // 解释：将 density 从 [0.1 * (1 - coverage), 1.0] 映射到 [0.0, 1.0]
    float coverage_threshold = 0.1; 
    density = remap(density, coverage_threshold * (1.0 - coverage), 1.0, 0.0, 1.0);
    density = clamp(density, 0.0, 1.0);


    vec3 edge_factor = 1.0 - abs(normalized_pos);
    float edge_fade = min(min(edge_factor.x, edge_factor.y), edge_factor.z);
    edge_fade = smoothstep(0.0, 0.15, edge_fade);
    density *= edge_fade;


    // 2. 细节侵蚀 (保持您的逻辑，但使用 h 代替 modulated_height)
    if (density > 0.05) { 
        vec3 detail_uv = normalized_pos * 3.5 + 0.5; 
        vec3 detail_noise = texture(_DetailNoiceTex, detail_uv).rgb;
        float detail_fbm = dot(detail_noise, vec3(0.625, 0.25, 0.125));
    
    // 侵蚀强度依赖于高度 h
    float erode_strength = mix(0.1, 0.2, h); 
    float erode = (1.0 - detail_fbm) * erode_strength;
    density -= erode * density; 
    }
    // 3. 最终裁剪和输出
    if (density < 0.16) { 
        return 0.0;
    }
    // 最终密度增强
    density = clamp(density, 0.0, 1.0);
    density = pow(density, 1.3);
  return density * 1.3; // 整体增强
}

float hash12(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}
float SampleShapedensity(vec3 worldPos);
// ============================================================
// 简单的云层渲染函数（用于调试形状）
// ============================================================
void RenderSimpleWhiteClouds(vec3 view_direction, inout vec3 radiance) {
    vec3 camera_earth_space = camera_pos - earth_center;
    float t_min, t_max;
    
    // 正确计算射线与云盒的交点
    if (!RayIntersectCloudBox(camera_earth_space, view_direction, t_min, t_max)) {
        return;
    }
    //t_min, t_max 在云盒内计算
    
    const int STEPS = 64;
    float step_size = (t_max - t_min) / float(STEPS);
    
    // 添加蓝噪声jitter
    vec2 screenUV = gl_FragCoord.xy / iResolution.xy;
    vec2 blueNoiseUV = screenUV * 8.0;
    float blueNoise = texture(blueNoiseTexture, blueNoiseUV).r;
    // 使用蓝噪声来抖动初始步进位置
    float initial_jitter = blueNoise * step_size;


    vec3 cloud_color = vec3(0.0); 
    float transmittance=1.0;

    vec3 scattering = vec3(0.0f);
    // 光线步进
    for (int i = 0; i < STEPS; i++) {

    float offset = hash12(gl_FragCoord.xy + float(i) * 13.7);
    float t = t_min + (float(i) + offset) * step_size;
        
        //float t = t_min + (initial_jitter + float(i)) * step_size; //起始的距离
        vec3 curr_pos = camera_earth_space + view_direction * t;
        // 采样云密度
        float density =SampleShapedensity(curr_pos);
        
        // 如果有密度
        if (density > 0.0) {
            // 计算该点对光线的吸收

            float absorption = density * step_size;
            float steptransmittance = exp(-absorption);
            // 累加颜色（使用纯白色）
            vec3 sunColor = atmosphereParams.SunLightColor * atmosphereParams.SunLightIntensity;

            float cosTheta = dot(view_direction,sun_direction);
            float sunPhase = DualLobPhase(0.8, -0.3, 0.3, cosTheta);

            // 结合大气散射模型计算太阳透射率
            // 使用TransmittanceToAtmosphere函数计算从当前点到太阳的透射率
            vec3 sunAtmosphereTransmittance = TransmittanceToAtmosphere(atmosphereParams, curr_pos, -sun_direction, transmittanceLUT);
            //vec3 sunAtmosphereTransmittance = vec3(0.0);
            vec3 stepScattering = sunColor * sunAtmosphereTransmittance * sunPhase ;

            vec3 sigmaS = vec3(density);
            const float sigmaA = 0.0;
            vec3 sigmaE = max(vec3(1e-8f), sigmaA + sigmaS);
            vec3 sactterLitStep = stepScattering * sigmaS;
            
            sactterLitStep = transmittance * (sactterLitStep - sactterLitStep * steptransmittance);
            sactterLitStep /= sigmaE;
            scattering += sactterLitStep; 

            scattering += stepScattering * transmittance * (sigmaS * step_size);
            // cloud_color += vec3(1.0)*transmittance*absorption;
            transmittance *=  steptransmittance; 

            if (transmittance < 0.01) break;
        }
    }
    // cloud_color +=transmittance;
    //cloud_color *= radiance;
    vec3 finalColor = transmittance * radiance + scattering;
    //radiance += finalColor;
    radiance = radiance * transmittance + scattering;
}

// ============================================================
// 可视化单个形状纹理的函数
// ============================================================
float SampleShapedensity(vec3 worldPos) {

    float density=0.0;
    
    // 计算在云盒中的UV坐标
    vec3 localPos = (worldPos - kCloudBoxMin) / kCloudBoxSize;
    
    // 确保在云盒内
    if (any(lessThan(localPos, vec3(0.0))) || any(greaterThan(localPos, vec3(1.0)))) {
        return 0.0;
    }
    

    vec4 lowshape = texture(_ShapeNoiceTex, localPos*0.8);
    float shapeFBM = lowshape.g * 0.525 + lowshape.b * 0.25 + lowshape.a * 0.125;

    density = remap(lowshape.r, saturate(1.0 - shapeFBM), 1.0, 0.0, 1.0);



    float _DensityOffset=0.01;

    float heightGradient = GetDensityHeightGradient(worldPos, kCloudBoxMin.y, kCloudBoxMax.y);
    float roundButton = saturate(remap(heightGradient, 0, 0.07, 0, 1));
    // 圆化云的顶部
    float roundTop = saturate(remap(heightGradient, 0.2, 1, 1, 0));
    float roundFac = roundButton * roundTop;
    // 密度变化因子
    float densityButton = heightGradient * saturate(remap(heightGradient, 0, _DensityOffset, 0, 1));
    float densityTop = saturate(remap(heightGradient, 1 - _DensityOffset, 1, 1, 0));
    float densityFac = densityButton * densityTop * 2;

    density *= roundFac * densityFac;
   


   if(density > 0) {
        // 为体积云添加天气属性
        vec2 weatherUV = (worldPos.xz - kCloudBoxMin.xz) / max(kCloudBoxSize.x, kCloudBoxSize.z);
        weatherUV = worldPos.xz * 0.0038;
        // 获取天气纹理, r 通道存取体积云的覆盖百分比，g 通道存取云层降雨的可能性， b 通道存取云的类型
        vec4 weather = texture(_WeatherNoiceTex, weatherUV);
        // 云层覆盖率

        float _CloudCoverage = 0.8;
        float cloudCoverage = weather.r;
        cloudCoverage = mix(cloudCoverage, 1, _CloudCoverage);
        cloudCoverage = mix(0, cloudCoverage, _CloudCoverage);
        //density *= cloudCoverage;

        bool enableDetail = true;

        if(density > 0 && enableDetail){

            // 获取高频的 Worley 噪声
            vec3 sampleDetailUV = worldPos.xyz *0.014;
            vec3 detailNoise = texture(_DetailNoiceTex, sampleDetailUV).rgb;
            // 计算 Worley 噪声的FBM
            float detailFBM = detailNoise.r * 0.5 + detailNoise.g * 0.25 + detailNoise.b * 0.125;

            float densityScale = 1.0;
            float _DetailScale = 1.0;
            
            float detailErode = (1 - detailFBM) * densityScale * 0.1 * _DetailScale;
            density -= detailErode;
        }


   }
    
    density *= 0.15;
    // 设置密度阈值，低于阈值的密度设为0
    const float densityThreshold = 0.004;
    if (density < densityThreshold) {
        density = 0.0;
    }
    float _DensityScale = 1.0;
    float _DensityMultiplier = 1.3;
    
    density = max(0, density - densityThreshold * _DensityScale) * _DensityMultiplier;

// density = max(0.0, density - 0.02);
// density = pow(density, 0.9);  // 轻微提对比度
// density *= 1.3;
    // 应用对比度调整
    // const float contrast = 2.2;
    // density = pow(density, contrast);
    
    // // 应用密度参数调整
    // const float densityParam = 0.8;
    // density *= densityParam;

    return density*0.8;
}

// ============================================================
// 简单的云层渲染函数（用于调试形状纹理）
// ============================================================


// ============================================================
// 云层渲染主函数 (改进版)
// ============================================================
void RenderCloudBox(vec3 view_direction, inout vec3 radiance) {
    // 为了调试目的，我们暂时使用形状纹理可视化渲染
    RenderSimpleWhiteClouds(view_direction, radiance);
}

void main()
{
    // 创建默认的大气参数
    AtmosphereParameter param = atmosphereParams;
    
    // 使用view_ray作为视图方向
    vec3 viewDir = normalize(view_ray);
    
    // 调试：确保相机位置合理
    vec3 eyePos = camera_pos - earth_center;
    
    // 计算天空颜色
    vec3 color = GetSkyView(param, eyePos, viewDir, 
    -sun_direction);
    
   
    vec3 suncolor= GetSunDisk(param, eyePos, viewDir, -sun_direction);

    //color=vec3(1.0);
   // color += suncolor;


    RenderCloudBox(viewDir, color);
    // 应用曝光
    color *= exposure;
    
    // 简单的色调映射
    color = 1.0 - exp(-color);
    
    frag_color = vec4(color, 1.0);
}