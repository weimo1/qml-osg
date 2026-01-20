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

uniform sampler2D transmittanceLUT;
uniform sampler2D mutlutLUT;

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
        
       color += multiScattering * t2 * ds * sunLuminance*0.5;


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



uniform sampler3D _ShapeNoiceTex;      // 3D基础形状纹理（包含Perlin和Worley噪声）
uniform sampler3D _DetailNoiceTex;     // 3D细节纹理（高频Worley噪声）
uniform sampler2D _WeatherNoiceTex;    // 2D天气纹理（控制云的覆盖率等属性）
uniform sampler2D blueNoiseTexture;    // 蓝噪声纹理，用于消除云渲染分层




// ============ 云的外观参数 ============
// 云的外观参数
const vec3 kCloudColor = vec3(1.0, 1.0, 1.0);         // 纯白色
const vec3 kCloudShade = vec3(0.8, 0.8, 0.8);         // 浅灰色阴影
const float kCloudExtinction = 0.8;                   // 增加消光系数

// ============ 改进的云盒参数 ============

// 云盒中心和尺寸（增加高度）
const vec3 kCloudBoxCenter = vec3(0.0, 0.0, 6364000.0);  // 提高1km
const vec3 kCloudBoxSize = vec3(1000.0, 1000.0, 50.0);   // 增加到8km厚度

// ============ 云密度函数 ============

float Remap(float value, float lo, float ho, float ln, float hn) {
    return ln + (value - lo) * (hn - ln) / (ho - lo);
}
bool RayIntersectCloudBox(vec3 ray_origin, vec3 ray_dir, out float t_min, out float t_max) {
    // 1. 计算盒子的最小和最大角点
    vec3 box_min = kCloudBoxCenter - kCloudBoxSize * 0.5;
    vec3 box_max = kCloudBoxCenter + kCloudBoxSize * 0.5;
    
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



// ============ 完全重写的密度函数（增加体积感）============
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
    float density = Remap(base_shape, 1.0 - worley_fbm * 0.4, 1.0, 0.0, 1.0); 

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
    density = Remap(density, coverage_threshold * (1.0 - coverage), 1.0, 0.0, 1.0);
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
// ============================================================
// 高级云光照系统 - 完整实现
// ============================================================

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



// ============================================================
// 高度梯度光照 (上层亮,下层暗)
// ============================================================
float ComputeHeightGradient(vec3 sample_pos) {
    vec3 local_pos = sample_pos - kCloudBoxCenter;
    vec3 half_size = kCloudBoxSize * 0.5;
    float height_fraction = (local_pos.z / half_size.z) * 0.5 + 0.5;
    
    // 下层0.5倍,上层1.2倍
    float gradient = mix(0.5, 1.2, height_fraction);
    
    // 云顶额外增亮(模拟大气散射)
    gradient += smoothstep(0.7, 1.0, height_fraction) * 0.3;
    
    return gradient;
}

// ============================================================
// 边缘光照 (Rim Light / 次表面散射)
// ============================================================
float ComputeRimLight(vec3 view_dir, vec3 sun_dir, float density) {
    // 计算视线和太阳方向的夹角
    float cosTheta = dot(view_dir, sun_dir);
    
    // 边缘光只在背光侧生效
    float rim = smoothstep(0.0, -0.5, cosTheta);
    
    // 边缘光强度随密度衰减(厚云穿透少)
    float density_atten = exp(-density * 2.0);
    
    // 边缘光锐化
    rim = pow(rim, 4.0);
    
    return rim * density_atten * 0.4;
}

// ============================================================
// 环境光遮蔽 (AO)
// ============================================================
float ComputeAmbientOcclusion(vec3 sample_pos, float density) {
    // 简化的AO: 基于局部密度
    vec3 local_pos = sample_pos - kCloudBoxCenter;
    vec3 half_size = kCloudBoxSize * 0.5;
    float height_fraction = (local_pos.z / half_size.z) * 0.5 + 0.5;
    
    // 底部遮蔽更强
    float height_ao = mix(0.6, 1.0, height_fraction);
    
    // 密度越大,遮蔽越强
    float density_ao = mix(1.0, 0.7, smoothstep(0.3, 1.0, density));
    
    return height_ao * density_ao;
}

// ============================================================
// 改进的云自阴影计算
// ============================================================
float ComputeCloudSelfShadowing(vec3 sample_pos, vec3 sun_dir, float initial_density) {
    const int SHADOW_STEPS = 24; // 🔥 增加步数，提高阴影精度
    
    vec3 shadow_pos = sample_pos;
    float shadow_transmittance = 1.0;
    float accumulated_density = 0.0;
    
    // 自适应步长: 高密度区域用小步长
    // 🔥 调整 adaptive_step 的影响范围，确保步长不会太小影响性能
    float adaptive_step = 0.5 * mix(1.0, 0.7, initial_density);
    float shadow_step_size = adaptive_step; 
    // 1. 获取抖动值 (使用像素 UV 和 时间/哈希 来确保每条射线不同)
    vec2 screenUV = gl_FragCoord.xy / iResolution.xy; 
    
    // 🔥 关键：使用哈希函数 (Hash) 或基于 UV 和 Step 的索引来采样蓝噪声，保证随机性
    // 假设您有一个基于屏幕位置和时间的哈希函数
    // float random_val = Hash(screenUV, Time); 
    
    // 更简单的做法：直接用屏幕 UV 采样，但 UV 缩放不同于主射线
    float random_offset_0_1 = texture(blueNoiseTexture, screenUV * 4.0).g; // 换个通道和缩放
    
    // 2. 抖动起始位置：将起始点沿着太阳方向推离或拉近，范围 [0, 1) * step_size
    float jitter_dist = random_offset_0_1 * shadow_step_size; 
    shadow_pos += sun_dir * jitter_dist; // 从 sample_pos 开始，加上一个随机偏移
    
    
    for (int i = 0; i < SHADOW_STEPS; i++) {
        shadow_pos += sun_dir * shadow_step_size;
        // 边界检查
        vec3 local_pos = shadow_pos - kCloudBoxCenter;
        vec3 half_size = kCloudBoxSize * 0.5;
        if (abs(local_pos.x) > half_size.x || 
            abs(local_pos.y) > half_size.y || 
            abs(local_pos.z) > half_size.z) {
            break;
        }
        
        float shadow_density = GetCloudDensity(shadow_pos);
        accumulated_density += shadow_density * shadow_step_size;
        
        shadow_transmittance *= exp(-shadow_density * shadow_step_size * kCloudExtinction);
        
        if (shadow_transmittance < 0.05) break; // 提前退出阈值略微放宽
    }
    
    // 🔥 关键改进: 柔和阴影的近似 (防止全黑，解决阴影过硬)
    // 使用 Soft Absorb 近似，确保最低透射率
    float min_transmittance = 0.2; // 阴影区最低亮度 (防止全黑)
    
    // 柔化阴影: (1 - 柔化系数) * 硬阴影 + 柔化系数 * 柔和阴影
    float soft_shadow = max(shadow_transmittance, min_transmittance);
    float hard_shadow = shadow_transmittance;
    
    // 使用混合因子来平衡硬阴影和软阴影
    float shadow_blend_factor = 0.7; // 更多软阴影
    return mix(hard_shadow, soft_shadow, shadow_blend_factor);
}




vec3 ComputeStepScattering(vec3 sample_pos, vec3 view_dir, vec3 sun_dir) {
    float density = GetCloudDensity(sample_pos);
    
    // ===== 1. 计算自阴影 (🔥 关键优化) =====
    // SelfShadowing决定了有多少太阳光能到达该点
    float sun_visibility = ComputeCloudSelfShadowing(sample_pos, sun_dir, density);
    
    // ===== 2. 相位函数 (用于单次散射) =====
    float cosTheta = dot(view_dir, sun_dir);
    // 使用双瓣 HG，前向 G=0.75, 后向 G=-0.45, 前向权重 W=0.8 (更聚焦)
    float phase_term = DualLobPhase(0.75, -0.45, 0.8, cosTheta);
    
    // ===== 3. 单次散射 (解决饱和问题) =====
    vec3 sun_color = vec3(1.0, 0.98, 0.92); // 暖色太阳光
    float sun_intensity = 3.0; // 🔥 降低强度，防止云核过曝 (原先可能太高)
    
    // 直射光项 = 太阳光 * (自阴影衰减 + BeerPowder增亮) * 相位函数
    // 🔥 将 BeerPowder 视为对 phase_term 的调整或增强，而不是一个独立的乘数
    float beer_powder_factor = mix(1.0, 1.5, BeerPowder(density * 5.0, max(0.0, cosTheta))); // 薄云边缘增亮 1.5倍
    
    // 单次散射光
    vec3 direct_light = sun_color * sun_intensity * sun_visibility * phase_term * beer_powder_factor;
    
    // ===== 4. 多重散射近似 (解决阴影区灰暗问题) =====
    vec3 ambient_color = vec3(0.5, 0.6, 0.7); // 蓝色天空环境光
    
    // 🔥 多重散射贡献：在自阴影区域增加白色填充光
    // (1.0 - sun_visibility) 代表阴影深度。
    float ms_blend = smoothstep(0.0, 1.0, density) * 0.7; // 密度越大，多重散射越强
    
    vec3 multi_scatter_color = mix(ambient_color, kCloudColor, ms_blend);
    
    // 阴影贡献：自阴影越深，环境光和多重散射越重要
    // 我们用 kCloudShade 作为阴影的基色，并用环境光提亮
    vec3 ms_contribution = multi_scatter_color * ambient_color * (1.0 - sun_visibility) * 1.5; // 🔥 增大乘数 1.5
    
    // 环境光与 AO
    float ambient_occlusion = ComputeAmbientOcclusion(sample_pos, density);
    vec3 ambient_light = ambient_color * 0.3 * ambient_occlusion;
    
    // 组合环境光和多重散射
    vec3 total_ambient = ambient_light + ms_contribution;
    
    // ===== 5. 边缘光 (Rim Light) =====
    float rim_light = ComputeRimLight(view_dir, sun_dir, density);
    vec3 rim_contribution = sun_color * rim_light * 2.0; // 🔥 增强 Rim Light
    
    // ===== 6. 高度梯度 (作为最终强度调整) =====
    float height_gradient = ComputeHeightGradient(sample_pos);
    
    // ===== 7. 组合最终光照 =====
    vec3 total_light = (direct_light + total_ambient + rim_contribution) * height_gradient;
    
    // ===== 8. 云基础颜色 (精简) =====
    // 基础颜色应该保持 kCloudColor，让光照去决定最终颜色
    vec3 cloud_base_color = kCloudColor;
    
    // 厚云颜色校正: 降低厚云的颜色饱和度，使其略微偏灰
    cloud_base_color = mix(cloud_base_color, vec3(0.85, 0.85, 0.85), smoothstep(0.5, 1.0, density));
    
    // 🔥 最终输出：将总光照乘以消光系数，使光照符合物理衰减
    return cloud_base_color * total_light ;
}

// ============================================================
// 云层渲染主函数 (改进版)
// ============================================================
void RenderCloudBox(vec3 view_direction, inout vec3 radiance) {
    vec3 camera_earth_space = camera_pos - earth_center;
    float t_min, t_max;
    
    if (!RayIntersectCloudBox(camera_earth_space, view_direction, t_min, t_max)) {
        return;
    }
    
    const int STEPS = 128;
    float step_size = (t_max - t_min) / float(STEPS);
    
    // 蓝噪声抖动
    vec2 screenUV = gl_FragCoord.xy / iResolution;
    vec2 blueNoiseUV = screenUV * 8.0;
    float blueNoise = texture(blueNoiseTexture, blueNoiseUV).r;
    
    vec3 cloud_color = vec3(0.0);
    float transmittance = 1.0;
    
    float initial_jitter = blueNoise * step_size;

    for (int i = 0; i < STEPS; i++) {
        if (transmittance < 0.02) break;
        
        float t = t_min + initial_jitter + float(i) * step_size;
        vec3 curr_pos = camera_earth_space + view_direction * t;
        
        float density = GetCloudDensity(curr_pos);
        
        if (density > 0.005) {
            // 🔥 使用改进的光照计算
            vec3 step_color = ComputeStepScattering(curr_pos, view_direction, sun_direction);
            
            float step_transmittance = exp(-density * step_size * kCloudExtinction);
            float extinction = density * kCloudExtinction;
            float step_T = exp(-extinction * step_size);
            // 体积积分
            cloud_color += step_color * transmittance*(1.0-step_T);
            transmittance *= step_T;
        }
    }
    
    
    radiance = radiance * transmittance + cloud_color;
}


// ============ 球壳云参数定义 ============
const float EARTH_RADIUS = 6364000.0;              // 地球半径（米）
const float CLOUD_LAYER_START = 5000.0;            // 云层底部海拔（米）
const float CLOUD_LAYER_THICKNESS = 12000.0;       // 云层厚度（米）

const float SPHERE_INNER_RADIUS = EARTH_RADIUS + CLOUD_LAYER_START;
const float SPHERE_OUTER_RADIUS = SPHERE_INNER_RADIUS + CLOUD_LAYER_THICKNESS;

// ============================================================
// 光线与球体求交（标准算法）
// ============================================================
bool RaySphereIntersection(vec3 ray_origin, vec3 ray_dir, vec3 sphere_center, float radius, out float t) {
    vec3 oc = ray_origin - sphere_center;
    float a = dot(ray_dir, ray_dir);
    float b = 2.0 * dot(ray_dir, oc);
    float c = dot(oc, oc) - radius * radius;
    
    float discriminant = b * b - 4.0 * a * c;
    
    if (discriminant < 0.0) {
        return false;
    }
    
    // 两个交点
    float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
    
    // 取最近的正值交点
    if (t1 > 0.0) {
        t = t1;
        return true;
    } else if (t2 > 0.0) {
        t = t2;
        return true;
    }
    
    return false;
}

// ============================================================
// 测试用密度函数：返回固定值（全白云层）
// ============================================================
float SampleCloudDensity_Test(vec3 world_pos, vec3 sphere_center) {
    // 计算高度比例
    float dist_from_center = length(world_pos - sphere_center);
    float height_fraction = (dist_from_center - SPHERE_INNER_RADIUS) / CLOUD_LAYER_THICKNESS;
    
    // 只在云层范围内返回密度
    if (height_fraction < 0.0 || height_fraction > 1.0) {
        return 0.0;
    }
    
    // 返回固定密度（全白）
    return 1.0;
}

// ============================================================
// 云层渲染主函数
// ============================================================
void RenderCloudSphere(vec3 view_direction, inout vec3 radiance) {
    // 相机位置（相对于地球中心）
    vec3 camera_earth_space = camera_pos - earth_center;
    
    // 球心位置（跟随相机的XY，Z方向固定在地球中心）
    vec3 sphere_center = vec3(camera_earth_space.x, camera_earth_space.y, -EARTH_RADIUS);
    
    // ============================================================
    // 第一步：计算与云层球壳的交点
    // ============================================================
    float t_inner_near, t_inner_far;
    float t_outer_near, t_outer_far;
    
    bool hit_inner = RaySphereIntersection(camera_earth_space, view_direction, sphere_center, SPHERE_INNER_RADIUS, t_inner_near);
    bool hit_outer = RaySphereIntersection(camera_earth_space, view_direction, sphere_center, SPHERE_OUTER_RADIUS, t_outer_near);
    
    // 如果没有击中外球，直接返回
    if (!hit_outer) {
        return;
    }
    
    // ============================================================
    // 第二步：确定光线步进的起点和终点
    // ============================================================
    vec3 start_pos, end_pos;
    float t_start, t_end;
    
    // 判断相机位置
    float camera_dist = length(camera_earth_space - sphere_center);
    
    if (camera_dist < SPHERE_INNER_RADIUS) {
        // 相机在内球内部（地球表面以下，通常不会发生）
        return;
    } 
    else if (camera_dist >= SPHERE_INNER_RADIUS && camera_dist <= SPHERE_OUTER_RADIUS) {
        // 相机在云层内部
        t_start = 0.0;
        start_pos = camera_earth_space;
        
        // 终点是外球
        t_end = t_outer_near;
        end_pos = camera_earth_space + view_direction * t_end;
    } 
    else {
        // 相机在云层外部
        // 起点是外球
        t_start = t_outer_near;
        start_pos = camera_earth_space + view_direction * t_start;
        
        // 如果击中内球，终点是内球；否则是外球的另一侧
        if (hit_inner) {
            // 需要获取内球的第二个交点
            vec3 oc = camera_earth_space - sphere_center;
            float a = dot(view_direction, view_direction);
            float b = 2.0 * dot(view_direction, oc);
            float c = dot(oc, oc) - SPHERE_INNER_RADIUS * SPHERE_INNER_RADIUS;
            float discriminant = b * b - 4.0 * a * c;
            
            float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
            float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
            
            // 终点是内球的远端交点
            t_end = max(t1, t2);
            end_pos = camera_earth_space + view_direction * t_end;
        } else {
            // 没有击中内球，需要获取外球的第二个交点
            vec3 oc = camera_earth_space - sphere_center;
            float a = dot(view_direction, view_direction);
            float b = 2.0 * dot(view_direction, oc);
            float c = dot(oc, oc) - SPHERE_OUTER_RADIUS * SPHERE_OUTER_RADIUS;
            float discriminant = b * b - 4.0 * a * c;
            
            float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
            float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
            
            t_end = max(t1, t2);
            end_pos = camera_earth_space + view_direction * t_end;
        }
    }
    
    // ============================================================
    // 第三步：光线步进
    // ============================================================
    const int MAX_STEPS = 64;
    float march_length = length(end_pos - start_pos);
    float step_size = march_length / float(MAX_STEPS);
    
    vec3 ray_step = view_direction * step_size;
    vec3 current_pos = start_pos-earth_center;
    
    // 累积变量
    vec3 cloud_color = vec3(0.0);
    float transmittance = 1.0;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        // 提前退出
        if (transmittance < 0.01) {
            break;
        }
        
        // 采样密度（测试版：返回1.0）
        float density = SampleCloudDensity_Test(current_pos, sphere_center);
        
        if (density > 0.01) {
            // 白色云
            vec3 sample_color = vec3(1.0);
            
            // Beer定律
            float extinction = density * step_size * 0.00001; // 注意：调整这个系数控制云的不透明度
            float step_trans = exp(-extinction);
            
            // 累积颜色
            cloud_color += sample_color * transmittance * (1.0 - step_trans);
            
            // 更新透射率
            transmittance *= step_trans;
        }
        
        // 前进
        current_pos += ray_step;
    }
    
    // ============================================================
    // 第四步：混合到背景
    // ============================================================
    radiance = radiance * transmittance + cloud_color;
}


void main()
{
    AtmosphereParameter param = atmosphereParams;
    
    // 使用view_ray作为视图方向
    vec3 viewDir = normalize(view_ray);
    
    // 调试：确保相机位置合理
    vec3 eyePos = camera_pos - earth_center;
    
    //计算天空颜色
    vec3 color = GetSkyView(param, eyePos, viewDir, 
    sun_direction);
    
   
    vec3 suncolor= GetSunDisk(param, eyePos, viewDir, sun_direction);

     color += suncolor;

    RenderCloudSphere(viewDir, color);
    // 应用曝光
    color *= exposure;
   
   // 简单的色调映射
   color = 1.0 - exp(-color);
   
   frag_color = vec4(color, 1.0);
}