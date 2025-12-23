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

// 云层参数 (使用球面模型，云层分布在1500-3000米高度)
const float PLANET_RADIUS = 6372000.0;  // 地球半径 (米)
const float CLOUD_MIN_HEIGHT = 200.0;  // 云层最低高度 (米)
const float CLOUD_MAX_HEIGHT = 350.0;  // 云层最高高度 (米)
const float CLOUD_INNER_RADIUS = PLANET_RADIUS + CLOUD_MIN_HEIGHT;
const float CLOUD_OUTER_RADIUS = PLANET_RADIUS + CLOUD_MAX_HEIGHT;

// 云层边界参数
const vec2 inCloudMinMax = vec2(CLOUD_MIN_HEIGHT, CLOUD_MAX_HEIGHT);
const float SphereSize = PLANET_RADIUS;

// 移除原来的云盒参数
// const vec3 kCloudBoxCenter = vec3(0.0, 0.0, 6364000.0);
// const vec3 kCloudBoxSize = vec3(1000.0, 1000.0, 50.0);
// const vec3 kCloudBoxMin = kCloudBoxCenter - kCloudBoxSize * 0.5;
// const vec3 kCloudBoxMax = kCloudBoxCenter + kCloudBoxSize * 0.5;

// 移除uniform变量相关的代码

uniform sampler2D transmittanceLUT;
uniform sampler2D mutlutLUT;

const float time = 0.2;



uniform sampler3D _ShapeNoiceTex;      // 3D基础形状纹理（包含Perlin和Worley噪声）
uniform sampler3D _DetailNoiceTex;     // 3D细节纹理（高频Worley噪声）
uniform sampler2D _WeatherNoiceTex;    // 2D天气纹理（控制云的覆盖率等属性）
uniform sampler2D blueNoiseTexture;    // 蓝噪声纹理，用于消除云渲染分层

uniform sampler2D cloudTexture;

// 步进参数
const int Steps = 64;
const float FarPlane = 1000000.0;

// 云类型偏移
const float u_CloudTypeOffset = 0.0;

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

// 重映射函数
float Remap(float original_value, float original_min, float original_max, 
            float new_min, float new_max)
{
    return new_min + ((( original_value - original_min) / (original_max - original_min))
            * (new_max - new_min));
}

// 计算在云层中的相对高度
float GetHeightFractionForPoint(vec3 inPosition, vec2 inCloudMinMax)
{
    float height_fraction = (inPosition.z - inCloudMinMax.x) / (inCloudMinMax.y - inCloudMinMax.x);
    return saturate(height_fraction);
}

// 云的密度-高度梯度模型
// Stratus : 0.0
// Stratocumulus: 0.5
// Cumulus : 1.0
float GetDesityHeightGradientForPoint(in float RelativeHeight, in float CloudType)
{
    CloudType = Remap(CloudType, u_CloudTypeOffset, 1.0, 0.0, 1.0);
    RelativeHeight = clamp(RelativeHeight, 0.0, 1.0);

    // 根据2017年的分享，两个Remap相乘重建云属
    float Cumulus = max(0.0, Remap(RelativeHeight, 0.01, 0.3, 0.0, 1.0) * Remap(RelativeHeight, 0.6, 0.95, 1.0, 0.0));
    float Stratocumulus = max(0.0, Remap(RelativeHeight, 0.0, 0.25, 0.0, 1.0) * Remap(RelativeHeight, 0.3, 0.65, 1.0, 0.0));
    float Stratus = max(0.0, Remap(RelativeHeight, 0, 0.1, 0.0, 1.0) * Remap(RelativeHeight, 0.2, 0.3, 1.0, 0.0));

    // 云属过渡
    float a = mix(Stratus, Stratocumulus, clamp(CloudType * 2.0, 0.0, 1.0));
    float b = mix(Stratocumulus, Cumulus, clamp((CloudType - 0.5) * 2.0, 0.0, 1.0));
    return mix(a, b, CloudType);
}
// ============================================================================
// 核心密度函数 - 传入世界坐标pos
// ============================================================================


bool RayIntersectCloudBox(vec3 ray_origin, vec3 ray_dir, out float t_min, out float t_max) {
    // 使用固定的云盒参数（为了保持兼容性，使用新的球面模型参数）
    vec3 box_min = vec3(-500.0, -500.0, CLOUD_INNER_RADIUS);
    vec3 box_max = vec3(500.0, 500.0, CLOUD_OUTER_RADIUS);
    
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

// ============================================================
// 计算射线起始点
// ============================================================
vec2 FindRayStartPos(vec3 ro, vec3 rd, vec3 PlanetCenter, float SphereRadius)
{
    float t = RayIntersectSphere(PlanetCenter, SphereRadius, ro, rd);
    return vec2(t, t);
}

bool RayIntersectCloudSphere(vec3 ray_origin, vec3 ray_dir, out float t_min, out float t_max) {
    // 计算射线与两个球面的交点
    float inner_t = RayIntersectSphere(vec3(0.0), CLOUD_INNER_RADIUS, ray_origin, ray_dir);
    float outer_t = RayIntersectSphere(vec3(0.0), CLOUD_OUTER_RADIUS, ray_origin, ray_dir);
    
    // 如果射线不与任何球面相交，则无交点
    if (inner_t < 0.0 && outer_t < 0.0) {
        return false;
    }
    
    // 确保交点在相机前方
    inner_t = max(inner_t, 0.0);
    outer_t = max(outer_t, 0.0);
    
    // 确定云层的进入和离开点
    if (inner_t >= 0.0 && outer_t >= 0.0) {
        // 射线与两个球面都相交
        t_min = max(inner_t, 0.0);
        t_max = outer_t;
    } else if (inner_t >= 0.0) {
        // 只与内球相交
        t_min = max(inner_t, 0.0);
        t_max = 1e10; // 无穷远
    } else if (outer_t >= 0.0) {
        // 只与外球相交
        t_min = max(outer_t, 0.0);
        t_max = 1e10; // 无穷远
    } else {
        // 没有有效的交点
        return false;
    }
    
    // 确保t_min <= t_max
    if (t_min > t_max) {
        float temp = t_min;
        t_min = t_max;
        t_max = temp;
    }
    
    return t_max > t_min;
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
    float StepSize;
    
    // 使用球面相交测试来确定Raymarching的起点与终点
    if (!RayIntersectCloudSphere(camera_earth_space, view_direction, t_min, t_max)) {
        return;
    }
    
    // 调试信息：输出t_min和t_max
    // 如果你想看到调试信息，可以临时将颜色设置为与t_min和t_max相关的值
    // radiance = vec3(t_min/10000.0, t_max/10000.0, 0.0); // 取消注释以查看相交信息
    
    // 调试信息：输出相机位置
    // radiance = vec3(camera_earth_space.z/1000000.0, camera_earth_space.z/1000000.0, camera_earth_space.z/1000000.0); // 取消注释以查看相机高度
    
    // 判断视点位置，预估步进次数与初始步幅
    // 摄像机-地心距离
    float DistanceCameraPlanet = distance(camera_earth_space, vec3(0.0));
    vec3 PlanetCenter = vec3(0.0);
    vec3 rs, re;
    vec3 ro = camera_earth_space;
    vec3 rd = view_direction;
    
    // 根据不同的视点位置确定初始化不同的步幅
    if(DistanceCameraPlanet < SphereSize + inCloudMinMax.x) //在云层下
    {
        vec2 temp_rs = FindRayStartPos(ro, rd, PlanetCenter, SphereSize + inCloudMinMax.x);
        rs = ro + rd * temp_rs.x;
        // 进一步判断是否在地平线以下
        if (temp_rs.y < 0.0) // ZeroPoint.y 应该是 0.0
        {
            return;
        }
        
        vec2 temp_re = FindRayStartPos(ro, rd, PlanetCenter, SphereSize + inCloudMinMax.y);
        re = ro + rd * temp_re.x;
        StepSize = distance(re, rs) / float(Steps);
    }
    else if (DistanceCameraPlanet > SphereSize + inCloudMinMax.y) // 在云层上
    {
        vec2 temp_rs = FindRayStartPos(ro, rd, PlanetCenter, SphereSize + inCloudMinMax.y);
        rs = ro + rd * temp_rs.x;
        re = rs + rd * FarPlane;
        StepSize = distance(re, rs) / float(Steps);
    }
    else //在云层中
    {
        rs = ro;
        re = rs + rd * FarPlane;
        StepSize = distance(re, rs) / float(Steps);
    }
    
    // 使用计算出的步长
    float step_size = StepSize;
    t_min = distance(rs, ro);
    t_max = distance(re, ro);
    
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
    for (int i = 0; i < Steps; i++) {

        float offset = hash12(gl_FragCoord.xy + float(i) * 13.7);
        float t = t_min + (float(i) + offset) * step_size;
        
        //float t = t_min + (initial_jitter + float(i)) * step_size; //起始的距离
        vec3 curr_pos = camera_earth_space + view_direction * t;
        // 采样云密度
        float density =SampleShapedensity(curr_pos);
        
        // 如果有密度
        if (density > 0.001) { // 降低密度阈值以更容易看到云
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
    
    // 调试：如果云层渲染正常工作，应该能看到一些白色的云
    // 如果你想确认云层渲染是否被调用，可以取消下面一行的注释
    // radiance = vec3(1.0, 0.0, 0.0); // 红色表示云层渲染被调用
}

// ============================================================
// 可视化单个形状纹理的函数
// ============================================================
float SampleShapedensity(vec3 worldPos) {
    float density=0.0;
    
    // 计算点到地心的距离
    float distance_to_center = length(worldPos);
    
    // 检查点是否在云层范围内
    if (distance_to_center < CLOUD_INNER_RADIUS || distance_to_center > CLOUD_OUTER_RADIUS) {
        return 0.0;
    }
    
    // 计算归一化高度 [0, 1]
    float h = (distance_to_center - CLOUD_INNER_RADIUS) / (CLOUD_OUTER_RADIUS - CLOUD_INNER_RADIUS);
    
    // 使用球面坐标计算UV
    vec3 normalized_pos = worldPos / distance_to_center;
    vec2 sphere_uv = vec2(
        atan(normalized_pos.y, normalized_pos.x) / (2.0 * PI) + 0.5,
        asin(normalized_pos.z) / PI + 0.5
    );

    // 计算高度影响因子
    float height_fraction = GetHeightFractionForPoint(worldPos, inCloudMinMax);
    
    // 采样天气纹理
    vec2 weatherUV = sphere_uv * 0.0038;
    vec4 weather = texture(_WeatherNoiceTex, weatherUV);
    
    // 计算云属分布
    float cloudType = weather.b; // 从天气纹理的b通道获取云类型
    float densityHeightGradient = GetDesityHeightGradientForPoint(height_fraction, cloudType);
    
    vec4 lowshape = texture(_ShapeNoiceTex, vec3(sphere_uv, h) * 0.8);
    float shapeFBM = lowshape.g * 0.525 + lowshape.b * 0.25 + lowshape.a * 0.125;

    density = remap(lowshape.r, saturate(1.0 - shapeFBM), 1.0, 0.0, 1.0);
    
    // 应用高度梯度
    density *= densityHeightGradient;

    float _DensityOffset=0.01;

    // 对于球面模型，我们需要重新计算高度梯度
    float heightGradient = h; // 直接使用归一化高度
    float roundButton = saturate(remap(heightGradient, 0, 0.1, 0, 1)); // 增加底部厚度
    // 圆化云的顶部
    float roundTop = saturate(remap(heightGradient, 0.1, 1, 1, 0)); // 调整顶部圆化参数
    float roundFac = roundButton * roundTop;
    // 密度变化因子
    float densityButton = heightGradient * saturate(remap(heightGradient, 0, _DensityOffset * 2.0, 0, 1)); // 增加密度变化
    float densityTop = saturate(remap(heightGradient, 1 - _DensityOffset * 2.0, 1, 1, 0)); // 调整顶部密度
    float densityFac = densityButton * densityTop * 3; // 增加密度因子

    density *= roundFac * densityFac;
   
   if(density > 0) {
        // 为体积云添加天气属性
        // 云层覆盖率
        float _CloudCoverage = 0.8;
        float cloudCoverage = weather.r;
        cloudCoverage = mix(cloudCoverage, 1, _CloudCoverage);
        cloudCoverage = mix(0, cloudCoverage, _CloudCoverage);
        //density *= cloudCoverage;

        bool enableDetail = true;

        if(density > 0 && enableDetail){

            // 获取高频的 Worley 噪声
            vec3 sampleDetailUV = vec3(sphere_uv, h) * 0.014;
            vec3 detailNoise = texture(_DetailNoiceTex, sampleDetailUV).rgb;
            // 计算 Worley 噪声的FBM
            float detailFBM = detailNoise.r * 0.5 + detailNoise.g * 0.25 + detailNoise.b * 0.125;

            float densityScale = 1.0;
            float _DetailScale = 1.0;
            
            float detailErode = (1 - detailFBM) * densityScale * 0.1 * _DetailScale;
            density -= detailErode;
        }

   }
    
    density *= 0.5; // 增加密度以更容易看到云
    // 设置密度阈值，低于阈值的密度设为0
    const float densityThreshold = 0.001; // 降低密度阈值
    if (density < densityThreshold) {
        density = 0.0;
    }
    float _DensityScale = 1.0;
    float _DensityMultiplier = 2.0; // 增加密度倍数
    
    density = max(0, density - densityThreshold * _DensityScale) * _DensityMultiplier;

    return density; // 移除额外的0.8倍数
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

    // 调试模式：如果你想看到明显的云层，可以取消下面几行的注释
    // vec3 debugColor = vec3(0.0, 0.0, 0.0);
    // RenderCloudBox(viewDir, debugColor);
    // color = mix(color, debugColor, 0.5); // 混合原始颜色和调试颜色
    
    RenderCloudBox(viewDir, color);
    // 应用曝光
    color *= exposure;
    
    // 简单的色调映射
    color = 1.0 - exp(-color);
    
    frag_color = vec4(color, 1.0);
}