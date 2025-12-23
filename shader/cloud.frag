#version 460 core

in vec2 v_texCoord;
in vec3 view_ray;
out vec4 frag_color;

const float PI = 3.14159265358979323846;
#define CLOUD_PLANE_BOT 3000.0
#define CLOUD_PLANE_TOP 3300.0
#define CLOUD_COVERAGE 0.5
#define CLOUD_DENSITY 1.0
#define PLANET_CENTER vec3(0.0, 0.0, -6371000.0)
#define PLANET_RADIUS 6371000.0

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
uniform sampler2D _curlNoiseTex;    // 3D curl噪声纹理，用于云的卷云效果
uniform sampler2D blueNoiseTexture;    // 蓝噪声纹理，用于消除云渲染分层



const float CLOUD_START_HEIGHT = 3000.0;      // 云层底部高度 5km
const float CLOUD_THICKNESS = 300.0;         // 云层厚度 3km
const int CLOUD_STEPS = 64;  



uniform vec3 u_WindDir = vec3(1.0, 0.0, 0.0);  // 风向
uniform float u_WindSpeed = 10.0;              // 风速
uniform float weatherScale = 0.0005;            // 天气图缩放
uniform vec2 weatherWind = vec2(0.01, 0.0);    // 风对天气图的影响
uniform float curlStrength = 100.0;            // curl噪声强度
uniform float curlScale = 0.01;                // curl噪声缩放
uniform float erosionStrength = 0.2;           // 侵蚀强度

uniform float lodBias = 0.0;  // LOD偏置

uniform float coverage_multiplier = 0.4;

const float earthRadius = 6371050.0;


#define EARTH_RADIUS earthRadius
#define SPHERE_INNER_RADIUS (EARTH_RADIUS + CLOUD_START_HEIGHT)
#define SPHERE_OUTER_RADIUS (SPHERE_INNER_RADIUS + CLOUD_THICKNESS)

vec3 sphereCenter = vec3(0.0, 0.0, -EARTH_RADIUS);


struct CloudLOD
{
    float distanceFade;   // 远处淡出
    float horizonFade;    // 地平线淡出
    float stepLOD;        // 步进质量
};

CloudLOD ComputeCloudLOD(
    float viewDistance,
    vec3 eyePos,
    vec3 viewDir
)
{
    CloudLOD lod;
    // ============================
    // 距离裁剪（20km → 40km）
    // ============================
    const float NEAR_DIST = 20000.0;
    const float FAR_DIST  = 40000.0;

    lod.distanceFade = 1.0 - smoothstep(
        NEAR_DIST,
        FAR_DIST,
        viewDistance
    );
    // ============================
    // 地平线裁剪
    // ============================
    float cosViewUp = dot(viewDir, normalize(eyePos));

    lod.horizonFade = smoothstep(0.05, 0.2, cosViewUp);

    // ============================
    // 步进 LOD（近 256 → 远 32）
    // ============================
    float dist01 = clamp(viewDistance / FAR_DIST, 0.0, 1.0);
    lod.stepLOD = dist01;

    return lod;
}




float saturate(float x) { return clamp(x, 0.0, 1.0); }

vec2 getUVProjection(vec3 p){
	return p.xy/SPHERE_INNER_RADIUS*500+0.5;
}

float hash12(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float remap(float value, float lo, float ho, float ln, float hn) {
    return ln + (value - lo) * (hn - ln) / (ho - lo);
}


float getHeightFraction(vec3 inPos){
	return (length(inPos) - SPHERE_INNER_RADIUS)/(SPHERE_OUTER_RADIUS - SPHERE_INNER_RADIUS);
}


float GetHeightFractionForPoint(vec3 inPosition, vec2 inCloudMinMax)
{
    float height_fraction = (inPosition.z - inCloudMinMax.x) / (inCloudMinMax.y - inCloudMinMax.x);

    return saturate(height_fraction);
}


float GetCloudInnerRadius() {
    return atmosphereParams.PlanetRadius + CLOUD_START_HEIGHT;
}

float GetCloudOuterRadius() {
    return GetCloudInnerRadius() + CLOUD_THICKNESS;
}

// 使用示例
float GetCloudHeightFraction(vec3 worldPos) {
    // 计算云层的内外半径
    vec3 center = earth_center;

    float innerRadius = atmosphereParams.PlanetRadius + CLOUD_START_HEIGHT;
    float outerRadius = innerRadius + CLOUD_THICKNESS;

    float r = length(worldPos - center);

    return saturate(
        (r - innerRadius) / (outerRadius - innerRadius)
    );
}

float Remap(float value, float lo, float ho, float ln, float hn) {
    return ln + (value - lo) * (hn - ln) / (ho - lo);
}

// Cloud types height density gradients
#define STRATUS_GRADIENT vec4(0.0, 0.1, 0.2, 0.3)
#define STRATOCUMULUS_GRADIENT vec4(0.02, 0.2, 0.48, 0.625)
#define CUMULUS_GRADIENT vec4(0.00, 0.1625, 0.88, 0.98)

float getDensityForCloud(float heightFraction, float cloudType)
{
	float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
	float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
	float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

	vec4 baseGradient = stratusFactor * STRATUS_GRADIENT + stratoCumulusFactor * STRATOCUMULUS_GRADIENT + cumulusFactor * CUMULUS_GRADIENT;
	// gradicent computation (see Siggraph 2017 Nubis-Decima talk)
	//return remap(heightFraction, baseGradient.x, baseGradient.y, 0.0, 1.0) * remap(heightFraction, baseGradient.z, baseGradient.w, 1.0, 0.0);
	return smoothstep(baseGradient.x, baseGradient.y, heightFraction) - smoothstep(baseGradient.z, baseGradient.w, heightFraction);

}




float threshold(const float v, const float t)
{
	return v > t ? v : 0.0;
}




float HenyeyGreensteinPhase(float g, float cosTheta) {
    float g2 = g * g;
    float numerator = 1.0 - g2;
    float denominator = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (1.0 / (4.0 * PI)) * (numerator / denominator);
}

float DualLobPhase(float g0, float g1, float w, float cosTheta) {
    return mix(
        HenyeyGreensteinPhase(g0, cosTheta),  // 前向散射
        HenyeyGreensteinPhase(g1, cosTheta),  // 后向散射
        w
    );
}

float GetCloudTypeDensity(float heightFraction, float cloud_min, float cloud_max, float feather)
{
    //云的底部羽化需要弱一些，所以乘0.5
    return saturate(Remap(heightFraction, cloud_min, cloud_min + feather * 0.5, 0, 1)) * saturate(Remap(heightFraction, cloud_max - feather, cloud_max, 1, 0));
}

float BeerPowder(float density, float cosTheta) {
    // Beer定律基础衰减
    float beer = exp(-density);
    
    // Powder效应: 薄云边缘散射增强
    float powder = 1.0 - exp(-density * 2.0);
    
    // 根据太阳角度调整powder强度
    float powder_strength = 1.0 * smoothstep(0.5, 1.0, cosTheta);
    
    return mix(beer, powder, powder_strength);
}



float SampleLowFrequencyNoisesWithLOD(vec3 p, float lod) {
    vec3 sampleUV = p * 0.0003;
    
    vec4 low_frequency_noises = textureLod(_ShapeNoiceTex, sampleUV, lod);
    
    float low_freq_fBm = dot(low_frequency_noises.gba, vec3(0.625, 0.25, 0.125));
    float base_cloud = Remap(low_frequency_noises.r, -(1.0 - low_freq_fBm), 1.0, 0.0, 1.0);
    
    return base_cloud;
}


float SampleHighFrequencyNoises(vec3 p, float height_fraction) {
    // 1. 应用Curl Noise扰动
    // 注意：这里假设您有一个2D的Curl噪声纹理
    // 如果没有，可以跳过这一步，或者使用其他扰动方法
    
    // 采样Curl噪声（2D纹理）
    // 假设_CurlNoiseTex是一个2D噪声纹理
    vec2 curl_uv = p.xy * 0.01;  // Curl噪声的缩放因子  curlScale
    vec2 curl_noise = texture(_curlNoiseTex, curl_uv).xy;
    // 将噪声从[0,1]映射到[-1,1]
    curl_noise = curl_noise * 2.0 - 1.0;
    
    // 应用Curl扰动，强度随高度减小
    float curl_strength = curlStrength * (1.0 - height_fraction);  //curlStrengt  
    p.xy += curl_noise.xy * curl_strength;

    // 2. 采样高频噪声
    // 高频噪声使用更高的频率（更小的缩放因子）
    vec3 sampleUV = p * 0.001;  // 注意：这个0.1可能需要根据您的实际需求调整
    
    // 假设_DetailNoiceTex的rgb通道存储不同尺度的Worley噪声
    vec3 high_frequency_noises = texture(_DetailNoiceTex, sampleUV).rgb;
    
    // 3. 构建高频Worley噪声FBM
    float high_freq_fBm = (high_frequency_noises.r * 0.625) + 
                         (high_frequency_noises.g * 0.25) + 
                         (high_frequency_noises.b * 0.125);
    
    return high_freq_fBm;
}

vec3 SampleWeather(vec3 pos) {
    // 计算UV坐标
    // Coverage.xy: 天气图的偏移
    // u_Coverage.zw: 天气图的缩放
    // 假设Coverage和u_Coverage是uniform vec4
    vec2 uv = pos.xy * weatherScale;
    
    // 添加风动画（可选）
    // uv += weatherWind * iTime;
    
    // 采样2D天气纹理，mip级别为0
    vec3 weatherData = texture(_WeatherNoiceTex, uv).rgb;
    
    return weatherData;
}

float SampleCloudType(vec3 pos)
{
    vec2 uv = pos.xy * weatherScale;
    vec3 weather = texture(_WeatherNoiceTex, uv).rgb;

    // 提升对比度，避免类型过于平均
    float cloudType = saturate(pow(weather.g, 1.3));

    return cloudType; // 0 = 层云，1 = 积云
}





float sampleCloudDensity(vec3 pos)
{
    // 1. 计算高度比例
    float height_fraction = getHeightFraction(pos);
    
    //2. 高度裁剪
    if (height_fraction <= 0.0 || height_fraction >= 1.0) {
        return 0.0;
    }

    pos += (u_WindDir + vec3(0.0, 0.1, 0.0)) * iTime * u_WindSpeed;

    // 用角度作为扰动源

    // vec2 curl_uv = pos.xy * 0.1;  // Curl噪声的缩放因子  curlScale
    
    // vec2 curl_noise = texture(_curlNoiseTex, curl_uv).xy;
    // // 将噪声从[0,1]映射到[-1,1]
    // curl_noise = curl_noise * 2.0 - 1.0;
    
    // // 应用Curl扰动，强度随高度减小

    // float curl_strength = curlStrength * (1.0 - height_fraction);  //curlStrengt  

    // pos.xz += curl_noise.xy *curl_strength ;

    // 2. 基础密度
    vec3 sampleUV = pos * 0.0003;

    vec4 low_frequency_noises = textureLod(_ShapeNoiceTex, sampleUV, 0.0);
    
    float low_freq_fBm = dot(low_frequency_noises.gba, vec3(0.625, 0.25, 0.125));
    float base_cloud = Remap(low_frequency_noises.r, -(1.0 - low_freq_fBm), 1.0, 0.0, 1.0);
    float base_density = base_cloud;

    
    //天气图样式
    float density = getDensityForCloud(height_fraction, 1.0);
    base_density *= (density/height_fraction);

     // 3. 天气图（保持原样，但频率别太高）
     vec2 weatherUV = pos.xy * 0.00005 + iTime * vec2(0.01, -0.008);
     vec2 weather_data = texture(_WeatherNoiceTex, weatherUV).rg;
     float coverage = weather_data.r;
        coverage = pow(coverage,1.8);

    float base_coverage = saturate(
    (base_density - (1.0 - coverage * 0.8)) / (coverage * 0.8));

    float roundButton = saturate(Remap(height_fraction, 0, 0.07, 0, 1));
    // 圆化云的顶部
    float roundTop = saturate(Remap(height_fraction, 0.2, 1, 1, 0));
    float roundFac = roundButton * roundTop;
    // 密度变化因子

    float _DensityOffset = 0.02;
    float densityButton = height_fraction * saturate(Remap(height_fraction, 0, _DensityOffset, 0, 1));
    float densityTop = saturate(Remap(height_fraction, 1 - _DensityOffset, 1, 1, 0));
    float densityFac = densityButton * densityTop * 2;
    
    base_coverage *= roundFac * densityFac;
   

    bool enableDetail=true;
    float final_density = base_coverage;
    if(final_density > 0.02 && enableDetail) {
        // ⚠️ 细节噪声应该是基础噪声的3-5倍频率
        vec3 detailUV = pos * 0.003; // 从0.01改成0.0015 (0.0003 * 5)
        
        // 添加随高度变化的扰动
        detailUV += vec3(
            height_fraction * 0.1,
            height_fraction * 0.05,
            0.0
        );
        
        vec3 high_frequency_noises = texture(_DetailNoiceTex, detailUV).rgb;
        float high_freq_fBm = dot(high_frequency_noises, vec3(0.625, 0.25, 0.125));
        
        // 侵蚀强度随高度变化：顶部更强，底部更弱
        float erosionMask = smoothstep(0.02, 0.3, final_density); // 控制侵蚀区域
        float heightErosion = mix(0.3, 0.7, height_fraction); // 高处侵蚀更强
        
        // 使用1-worley作为侵蚀（worley越小=侵蚀越强）
        float erosion = (1.0 - high_freq_fBm) * heightErosion * erosionMask;
        
        // 非线性侵蚀，保留云的核心
        final_density = final_density - erosion * final_density * 0.8;
    }
    final_density = max(0.0, final_density - 0.05);
    return final_density;
}

float sampleCloudDensity1(vec3 pos)
{
    // 1. 计算高度比例
    float height_fraction = getHeightFraction(pos);
    // 2. 高度裁剪
    if (height_fraction <= 0.4 || height_fraction >= 1.0) {
        return 0.0;
    }
    //风动画
    pos += (u_WindDir + vec3(0.0, 0.1, 0.0)) * iTime * u_WindSpeed;
    vec3 curlPos = pos;
    vec2 curlUV = curlPos.xy * curlScale;  // curlScale 建议0.008 ~ 0.015
    vec2 curlNoise = texture(_curlNoiseTex, curlUV).xy * 2.0 - 1.0;
    // 底部扰动更强，顶部弱化（模拟风吹散）
    float curlStrengthAdjusted = curlStrength * (1.0 - height_fraction) * 0.8;
    curlPos.xz += curlNoise * curlStrengthAdjusted;
    pos = curlPos;  // 替换原pos

    // 3. 风动画（加在Curl之后，让整体移动）
    pos += (u_WindDir + vec3(0.0, 0.1, 0.0)) * iTime * u_WindSpeed;

    // 4. 采样低频噪声（基础云形状）
    float base_cloud = SampleLowFrequencyNoisesWithLOD(pos, lodBias);
    
    // 5. 采样天气图
    vec2 weatherUV = pos.xy * 0.00006 + iTime * vec2(0.01, -0.008);
    vec2 weather_data = texture(_WeatherNoiceTex, weatherUV).rg;
    float coverage = weather_data.r;
    float cloudType = saturate(weather_data.g);

   // float heightProfile = getDensityForCloud(height_fraction, cloudType);

    // 增强覆盖率对比度
    coverage = pow(coverage, 1.8);

    // 6. 应用覆盖率重映射
    float base_coverage = remap(base_cloud, 1.0 - coverage * 0.9, 1.0, 0.0, 1.0);
    

    float heightProfile = smoothstep(0.0, 0.18, height_fraction);          // 底部缓慢上升
    heightProfile *= smoothstep(1.0, 0.65, height_fraction);               // 顶部快速衰减
    // 顶部加一点“anvil”效果（积雨云特征）
    float anvil = 1.0 - pow(height_fraction, 3.5);
    heightProfile *= mix(1.0, 1.6, anvil * 0.5);                           // 顶部稍厚一点
    // 根据云类型增强（积云更厚）
    heightProfile *= mix(1.0, 1.8, smoothstep(0.5, 1.0, cloudType));

    base_coverage *= heightProfile;



    float roundButton = saturate(Remap(height_fraction, 0, 0.07, 0, 1));
    // 圆化云的顶部
    float roundTop = saturate(Remap(height_fraction, 0.2, 1, 1, 0));
    float roundFac = roundButton * roundTop;
    // 密度变化因子

    float _DensityOffset = 0.02;
    float densityButton = height_fraction * saturate(Remap(height_fraction, 0, _DensityOffset, 0, 1));
    float densityTop = saturate(Remap(height_fraction, 1 - _DensityOffset, 1, 1, 0));
    float densityFac = densityButton * densityTop * 2;

    base_coverage *= roundFac * densityFac;

    if (base_coverage > 0.05) {
        vec3 detailUV = pos * 0.005;  // 比原来0.001高一个数量级
        
        // 加高度扰动，让顶部更蓬松
        detailUV += vec3(height_fraction * 0.3, height_fraction * 0.15, 0.0) * 0.5;
        
        vec3 highFreq = texture(_DetailNoiceTex, detailUV).rgb;
        float worley = highFreq.r * 0.625 + highFreq.g * 0.25 + highFreq.b * 0.125;
        
        // 侵蚀强度：顶部更强，底部保留核心
        float erosionMask = smoothstep(0.1, 0.9, height_fraction);
        float erosionStrength = mix(0.4, 1.2, erosionMask);
        
        // 非线性侵蚀：只吃边缘
        float erosion = (1.0 - worley) * erosionStrength * base_coverage * 1.3;
        base_coverage -= erosion;
    //     float high_freq = SampleHighFrequencyNoises(pos, height_fraction);
        
    //     // 侵蚀强度随高度变化
    //     float erosion_strength = 0.5 * (1.0 - height_fraction);
    //     float erosion = (1.0 - high_freq) * erosion_strength;
        
    //    base_coverage -= erosion;
    }
    
     // 10. 阈值裁剪，去除过于稀薄的云
   base_coverage= max(0.0, base_coverage - 0.1);

//     float density = mix(
//     base_coverage,
//     base_coverage * heightProfile,
//     0.6           // 不要给 1.0
// );
    return base_coverage;
}
// uv 投影
// float sampleCloudDensity2(vec3 pos) {
//     // 1. 计算高度比例
//     float height_fraction = getHeightFraction(pos);
    
//     // 2. 高度裁剪
//     if (height_fraction <= 0.0 || height_fraction >= 1.0) {
//         return 0.0;
//     }
    
//     // 3. 添加风动画
//     vec3 wind_offset = vec3(1.0, 0.0, 0.0) * iTime * 50.0;  // 简单的风偏移
//     pos += wind_offset;
    
//     // 4. 获取UV投影
//     vec2 uv = getUVProjection(pos);
//     vec2 moving_uv = getUVProjection(pos + wind_offset);
    
//     // 5. 采样低频噪声
//     vec4 low_frequency_noise = textureLod(_ShapeNoiceTex, vec3(uv * 0.001, height_fraction), 0.0);
//     float lowFreqFBM = dot(low_frequency_noise.gba, vec3(0.625, 0.25, 0.125));
//     float base_cloud = remap(low_frequency_noise.r, -(1.0 - lowFreqFBM), 1.0, 0.0, 1.0);
    
//     // 6. 采样天气图
//     vec3 weather_data = texture(_WeatherNoiceTex, moving_uv).rgb;
//     float cloud_coverage = weather_data.r * coverage_multiplier;
    
//     // 7. 应用覆盖率
//     float base_cloud_with_coverage = remap(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);
//     base_cloud_with_coverage *= cloud_coverage;
    
//   //  8. 昂贵的细节侵蚀（注释掉的代码）
//     bool expensive = true;
//     if(expensive) {
//        vec3 detailUV = pos * 0.01; // 注意：比 0.001 大一个数量级

//         vec3 highFreqNoise = texture(_DetailNoiceTex, detailUV).rgb;
//         float highFreqFBM = dot(highFreqNoise, vec3(0.625, 0.25, 0.125));

// // 高度相关侵蚀（顶部侵蚀强）
// float erosionByHeight = smoothstep(0.4, 0.9, height_fraction);

// // 侵蚀强度（非常克制）
// float erosion = highFreqFBM * erosionByHeight * 0.4;

// // 只侵蚀已有云
// base_cloud_with_coverage *= (1.0 - erosion);
//     }
    
//     return base_cloud_with_coverage;
// }


bool ShouldRenderCloud(vec3 eyePos, vec3 viewDir)
{
    // eyePos 是世界坐标（从地心出发）
    vec3 up = normalize(eyePos);

    // 只在看向天空时渲染
    float cosViewUp = dot(viewDir, up);

    return cosViewUp > 0.05; // 地平线以下不画
}


// 云层光线步进范围结构
struct CloudRayRange {
    bool valid;
    float tStart;
    float tEnd;
};


// 计算云层球壳相交
CloudRayRange calculateCloudIntersection(in AtmosphereParameter param, vec3 ro, vec3 rd) {
    CloudRayRange range;
    range.valid = false;
    
    // float groundHit = RayIntersectSphere(vec3(0), param.PlanetRadius, ro, rd);
    // if (groundHit > 0.0) {
    //     // 射线击中地面，地面以下不渲染云
    //     return range;
    // }
    
    float inner = param.PlanetRadius + CLOUD_START_HEIGHT;
    float outer = inner + CLOUD_THICKNESS;

    float t1 = RayIntersectSphere(vec3(0), inner, ro, rd);
    float t2 = RayIntersectSphere(vec3(0), outer, ro, rd);

    if (t2 < 0.0) return range;  // 完全没交

    float tNear = max(t1, 0.0);
    float tFar = t2;

    // 相机在云层内部
    if (length(ro) > inner && length(ro) < outer) {
        tNear = 0.0;
    }

    // 相机在云层外部但射线从外向内
    if (t1 > t2) {
        float temp = tNear;
        tNear = tFar;
        tFar = temp;
    }

    if (tNear >= tFar) return range;

    range.valid = true;
    range.tStart = tNear;
    range.tEnd = tFar;
    
    return range;
}

float CloudShadowTerm(vec3 pos)
{
    float shadow = 1.0;
    vec3 p = pos;

    // 8～12 步就够
    for (int i = 0; i < 8; ++i)
    {
        p += sun_direction * 300.0; // 300m 一步（按你单位调）
        float d = sampleCloudDensity1(p);
        shadow *= exp(-d * 0.4);    // 阴影强度
        if (shadow < 0.02) break;
    }
    return shadow;
}

vec4 marchCloudLayer(in AtmosphereParameter param, vec3 ro, vec3 rd, CloudRayRange range,inout vec3 radiance) {

    // if (!ShouldRenderCloud(ro, rd))
    //     return vec4(0.0);
    
    
    if (!range.valid) {
        return vec4(0.0);
    }

    float rayLength = range.tEnd - range.tStart;


    CloudLOD lod0 = ComputeCloudLOD(range.tStart, ro, rd);

    //int steps = int(mix(128.0, 32.0, lod0.stepLOD));
   //float stepSize = rayLength / float(steps);
    float stepSize = rayLength / float(CLOUD_STEPS);

    vec3 cloud_color = vec3(0.0); 
    float transmittance=1.0;
    vec3 scattering = vec3(0.0f);

    //蓝噪声扰动
    vec2 screenUV = gl_FragCoord.xy / iResolution.xy;
    vec2 blueNoiseUV = screenUV * 8.0;
    float blueNoise = texture(blueNoiseTexture, blueNoiseUV).r;
    float initial_jitter = blueNoise * stepSize;


    for (int i = 0; i < CLOUD_STEPS; i++) {
       

        float t = range.tStart + (float(i) + initial_jitter) * stepSize;
        vec3 pos = (ro + rd * t);

        CloudLOD lod = ComputeCloudLOD(t, ro, rd);

        float density = sampleCloudDensity1(pos);

        density *= lod.distanceFade;
        density *= lod.horizonFade;

        if (density > 0.001) {
            float absorption = density * stepSize;
            float steptransmittance = exp(-absorption);
     
         vec3 sunColor = atmosphereParams.SunLightColor * atmosphereParams.SunLightIntensity;

         float cosTheta = dot(rd,-sun_direction);
        float sunPhase = DualLobPhase(0.8, -0.3, 0.3, cosTheta);

        vec3 sunAtmosphereTransmittance = TransmittanceToAtmosphere(atmosphereParams, 
              pos, -sun_direction, transmittanceLUT);

         float cloudShadow = CloudShadowTerm(pos);

            vec3 stepScattering = sunColor *
                sunAtmosphereTransmittance *
                
                sunPhase;

        float h = getHeightFraction(pos);
        vec3 cloudAlbedo = mix(
        vec3(0.85, 0.88, 0.9),   // 云底偏冷
        vec3(1.0, 0.98, 0.95),   // 云顶偏暖
        h
        );
        stepScattering *= cloudAlbedo;

        float VoL = dot(rd, -sun_direction);
        float powder = mix(1.0, 1.5, pow(1.0 - density, 2.0));
        float forwardBoost = pow(max(VoL, 0.0), 3.0);

        stepScattering *= powder * (1.0 + forwardBoost);

        vec3 sigmaS = vec3(density);
        const float sigmaA = 0.0;
        vec3 sigmaE = max(vec3(1e-8f), sigmaA + sigmaS);
        vec3 sactterLitStep = stepScattering * sigmaS;

        sactterLitStep = transmittance * (sactterLitStep - sactterLitStep * steptransmittance);
        sactterLitStep /= sigmaE;
        scattering += sactterLitStep; 

        scattering += stepScattering * transmittance * (sigmaS * stepSize);
       
        transmittance *=  steptransmittance; 
             if (transmittance < 0.01) break;
        }

    }
    radiance = radiance * transmittance + scattering;
}
// // 云层光线步进
// vec4 marchCloudLayer(in AtmosphereParameter param, vec3 ro, vec3 rd, CloudRayRange range,inout vec3 radiance) {

//     if (!ShouldRenderCloud(ro, rd))
//         return vec4(0.0);
    
    
//     if (!range.valid) {
//         return vec4(0.0);
//     }

//     float rayLength = range.tEnd - range.tStart;


//     CloudLOD lod0 = ComputeCloudLOD(range.tStart, ro, rd);

//     int steps = int(mix(128.0, 32.0, lod0.stepLOD));
//    float stepSize = rayLength / float(steps);
//    // float stepSize = rayLength / float(CLOUD_STEPS);

//     vec3 cloud_color = vec3(0.0); 
//     float transmittance=1.0;
//     vec3 scattering = vec3(0.0f);

//     //蓝噪声扰动
//     vec2 screenUV = gl_FragCoord.xy / iResolution.xy;
//     vec2 blueNoiseUV = screenUV * 8.0;
//     float blueNoise = texture(blueNoiseTexture, blueNoiseUV).r;
//     float initial_jitter = blueNoise * stepSize;


//     for (int i = 0; i < CLOUD_STEPS; i++) {
       

//         float t = range.tStart + (float(i) + initial_jitter) * stepSize;
//         vec3 pos = (ro + rd * t);

//       CloudLOD lod = ComputeCloudLOD(t, ro, rd);

//         float density = sampleCloudDensity1(pos);

//         density *= lod.distanceFade;
//         density *= lod.horizonFade;

//         if (density > 0.001) {
//             float absorption = density * stepSize;
//             float steptransmittance = exp(-absorption);
     
//          vec3 sunColor = atmosphereParams.SunLightColor * atmosphereParams.SunLightIntensity;

//          float cosTheta = dot(rd,-sun_direction);
//         float sunPhase = DualLobPhase(0.8, -0.3, 0.3, cosTheta);

//         vec3 sunAtmosphereTransmittance = TransmittanceToAtmosphere(atmosphereParams, 
//             pos, -sun_direction, transmittanceLUT);
//          vec3 stepScattering = sunColor * sunAtmosphereTransmittance * sunPhase ;

//         vec3 sigmaS = vec3(density);
//         const float sigmaA = 0.0;
//         vec3 sigmaE = max(vec3(1e-8f), sigmaA + sigmaS);
//         vec3 sactterLitStep = stepScattering * sigmaS;

//         sactterLitStep = transmittance * (sactterLitStep - sactterLitStep * steptransmittance);
//         sactterLitStep /= sigmaE;
//         scattering += sactterLitStep; 

//         scattering += stepScattering * transmittance * (sigmaS * stepSize);
       
//         transmittance *=  steptransmittance; 
//              if (transmittance < 0.05) break;
//         }

//     }
//     radiance = radiance * transmittance + scattering;
// }

// ============================================================
// 云层渲染函数
// ============================================================


void RenderCloudLayer(in AtmosphereParameter param, vec3 ro, vec3 rd,inout vec3 radiance)
{


    CloudRayRange range = calculateCloudIntersection(param, ro, rd);
    
    // 2. 执行光线步进
    marchCloudLayer(param, ro, rd, range, radiance);
}








// #define BAYER_FACTOR 1.0/16.0
// uniform float bayerFilter[16u] = float[]
// (
// 	0.0*BAYER_FACTOR, 8.0*BAYER_FACTOR, 2.0*BAYER_FACTOR, 10.0*BAYER_FACTOR,
// 	12.0*BAYER_FACTOR, 4.0*BAYER_FACTOR, 14.0*BAYER_FACTOR, 6.0*BAYER_FACTOR,
// 	3.0*BAYER_FACTOR, 11.0*BAYER_FACTOR, 1.0*BAYER_FACTOR, 9.0*BAYER_FACTOR,
// 	15.0*BAYER_FACTOR, 7.0*BAYER_FACTOR, 13.0*BAYER_FACTOR, 5.0*BAYER_FACTOR
// );

// float raymarchToLight(
//     vec3 startPos,
//     float stepSize,
//     vec3 lightDir,
//     float originalDensity,
//     float lightDotEye
// )
// {
//     const int LIGHT_STEPS = 6;

//     float absorption = 0.5;

//     float T = 1.0;
//     float ds = stepSize * 6.0;
//     float sigma_ds = -ds * absorption;

//     // 构造一个与太阳方向正交的基
//     vec3 up = abs(lightDir.y) < 0.99 ? vec3(0,1,0) : vec3(1,0,0);
//     vec3 right = normalize(cross(lightDir, up));
//     vec3 forward = normalize(cross(right, lightDir));

//     for (int i = 0; i < LIGHT_STEPS; i++)
//     {
//         float t = float(i + 1) * ds;

//         // 锥体半径（越远越大）
//         float coneRadius = t * 0.15;

//         // world-space 抖动（不是 UV）
//         vec3 rnd = noiseKernel[i];
//         vec3 jitter =
//             right   * rnd.x * coneRadius +
//             forward * rnd.y * coneRadius;

//         vec3 pos = startPos + lightDir * t + jitter;

//         float heightFraction = getHeightFraction(pos);
//         if (heightFraction <= 0.0 || heightFraction >= 1.0)
//             continue;

//         float cloudDensity = sampleCloudDensity(pos);
//         if (cloudDensity > 0.001)
//         {
//             float Ti = exp(cloudDensity * sigma_ds);
//             T *= Ti;

//             if (T < 0.05)
//                 break;
//         }
//     }

//     return T;
// }


// uniform vec3 cloudColorTop = (vec3(169., 149., 149.)*(1.5/255.));
// uniform vec3 cloudColorBottom =  (vec3(65., 70., 80.)*(1.5/255.));

// #define CLOUDS_AMBIENT_COLOR_TOP cloudColorTop
// #define CLOUDS_AMBIENT_COLOR_BOTTOM cloudColorBottom
// #define SUN_COLOR (atmosphereParams.SunLightColor * atmosphereParams.SunLightIntensity)*vec3(1.1,1.1,0.95)

// float HG( float sundotrd, float g) {
// 	float gg = g * g;
// 	return (1. - gg) / pow( 1. + gg - 2. * g * sundotrd, 1.5);
// }

// float powder(float d){
// 	return (1. - exp(-2.*d));

// }
// vec4 raymarchToCloud(vec3 startPos, vec3 endPos) {
//     vec3 path = endPos - startPos;
//     float len = length(path);
    
//     if (len < 0.001) {
//         return vec4(0.0);
//     }

//     const int nSteps = 64;
//     float ds = len / float(nSteps);
//     vec3 rayDir = normalize(path);
//     vec3 stepVec = rayDir * ds;

//     vec4 col = vec4(0.0);

//     // 蓝噪 / Bayer 抖动
//     vec2 fragCoord = gl_FragCoord.xy;
//     int a = int(fragCoord.x) & 3;
//     int b = int(fragCoord.y) & 3;
//     float jitter = bayerFilter[a * 4 + b];
//     vec3 pos = startPos + stepVec * jitter;
    
//     float densityFactor = 0.02;
//     float T = 1.0;
//     float sigma_ds = -ds * densityFactor;
//     bool entered = false;
    
//     // 获取背景色（天空颜色），从GetSkyView获取
//     vec3 skyColor = GetSkyView(atmosphereParams, startPos, rayDir, sun_direction);
    
//     float lightDotEye = dot(normalize(sun_direction), rayDir);

//     vec4 cloudPos = vec4(0.0);
//     for(int i = 0; i < nSteps; ++i) {
//         // 云体高度裁剪
//         float heightFraction = getHeightFraction(pos);
//         if(heightFraction > 0.0 && heightFraction < 1.0) {
//             float density_sample = sampleCloudDensity(pos);
//             if(density_sample > 0.001) {
//                 if(!entered){
//                     cloudPos = vec4(pos, 1.0);
//                     entered = true;
//                 }

//                 // 环境光
//                 vec3 ambientLight = CLOUDS_AMBIENT_COLOR_BOTTOM;
                
//                 // 沿太阳方向的体积阴影
//                 float light_density = raymarchToLight(
//                     pos,
//                     ds * 0.1,
//                     sun_direction,
//                     density_sample,
//                     lightDotEye
//                 );

//                 // 相函数
//                 float scattering = mix(
//                     HG(lightDotEye, -0.08),
//                     HG(lightDotEye, 0.08),
//                     clamp(lightDotEye * 0.5 + 0.5, 0.0, 1.0)
//                 );
//                 scattering = max(scattering, 1.0);

//                 // Powder效果
//                 float powderTerm = powder(density_sample);

//                 // 入射光 - 修改：使用天空颜色作为背景
//                 vec3 S = 0.6 * (
//                     mix(
//                         mix(ambientLight * 1.8, skyColor, 0.2),
//                         scattering * SUN_COLOR,
//                         powderTerm * light_density
//                     )
//                 ) * density_sample;
                
//                 // Beer-Lambert
//                 float dTrans = exp(density_sample * sigma_ds);

//                 // 积分
//                 vec3 Sint = (S - S * dTrans) / max(density_sample, 1e-4);
//                 col.rgb += T * Sint;
//                 T *= dTrans;
//             }
//         }

//         if(T <= 0.05)
//             break;

//         pos += stepVec;
//     }

//     col.a = 1.0 - T;
//     return col;
// }

// void RenderCloudLayer(in AtmosphereParameter param, vec3 ro, vec3 rd, inout vec3 radiance) {
//     CloudRayRange range = calculateCloudIntersection(param, ro, rd);
    
//     if (!range.valid) {
//         return;
//     }
    
//     // 计算起点和终点
//     vec3 startPos = ro + rd * range.tStart;
//     vec3 endPos = ro + rd * range.tEnd;
    
//     // 光线步进计算云颜色
//     vec4 cloudColor = raymarchToCloud(startPos, endPos);
    
//     // 与天空背景混合
//     radiance = mix(radiance, cloudColor.rgb, cloudColor.a);
// }





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
    sun_direction);
    
   
    vec3 suncolor= GetSunDisk(param, eyePos, viewDir, sun_direction);

     color += suncolor;
   //RenderCloudBox(viewDir, color);

   RenderCloudLayer(param, eyePos, viewDir, color);
   
   // 应用曝光
   color *= exposure;
   
   // 简单的色调映射
   color = 1.0 - exp(-color);
   
   frag_color = vec4(color, 1.0);
}