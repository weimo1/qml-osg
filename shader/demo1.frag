#version 330 core

uniform vec3 cameraPosition; //unit m
uniform vec3 sun_direction;

uniform mat4 viewInverse; //m
uniform mat4 projectInverse;//m

uniform vec3 earth_center;//m

uniform sampler2D depthTexture;
uniform sampler2D groundTexture;  // 地球表面模型，原始材质

in vec2 vUV;
in vec3 view_ray;

out vec4 fragColor;

// === 大气参数（单位：km）===
const float kPlanetRadius     = 6371000;
const float kAtmosphereRadius = 6471000;

const vec3 kBetaR = vec3(5.8e-6, 1.35e-5, 2.95e-5); // 瑞利
const vec3 kBetaM = vec3(2.0e-5);                   // 米氏
const float kScaleHeightR = 8000;
const float kScaleHeightM = 1200;

const float kGroundAlbedo = 0.2;
const float kFogDensity = 0.00001;
const float kFogHeightFalloff = 0.0003;
const vec3 kFogColor = vec3(0.8, 0.9, 1.0);

const float g_mie = 0.76;
 
 // 辅助：求射线与以 earth_center 为中心半径为 R 的球相交 (返回最小正 t 或 -1)
float RaySphereIntersectT(vec3 ro_world, vec3 rd_world, vec3 sphereCenter, float radius) {
    // ro_world = 起点在世界坐标
    vec3 L = ro_world - sphereCenter;

    float a = dot(rd_world, rd_world);
    float b = 2.0 * dot(L, rd_world);
    float c = dot(L, L) - radius * radius;

    float disc = b * b - 4.0 * a * c;

    if (disc < 0.0) return -1.0;

    float sqrtD = sqrt(disc);
    float t0 = (-b - sqrtD) / (2.0 * a);
    float t1 = (-b + sqrtD) / (2.0 * a);
    // 返回最近的正根（若存在）
    if (t0 > 0.0) return t0;
    if (t1 > 0.0) return t1;

    return -1.0;
}

// === 工具函数 ===
float rayleighPhase(float cosTheta) {
    return 0.75 * (1.0 + cosTheta * cosTheta);
}
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / pow(denom, 1.5);
}
 
// 修正的 transmittance: start,end 均为 world-space（单位要与 kPlanetRadius/kAtmosphereRadius 一致，假设为 km）
vec3 transmittance(vec3 startWorld, vec3 endWorld) 
{
    vec3 dir  = normalize(endWorld - startWorld);
    float len = length(endWorld - startWorld);
	if (len < 1e-3) return vec3(1.0); // 避免除零和 NaN

    const int steps = 16; // 稍增精度
    vec3 sum = vec3(0.0);

    for (int i = 0; i < steps; ++i) {
        float t = (float(i) + 0.5) / float(steps) * len;
        vec3 p = startWorld + dir * t;                 // world-space sample
        float h = length(p - earth_center) - kPlanetRadius;
        h = max(h, 0.0);
        vec3 tau = kBetaR * exp(-h / kScaleHeightR) + kBetaM * exp(-h / kScaleHeightM);
        sum += tau;
    }
    return exp(-sum * (len / float(steps)));
}


 
// 修正的 skyRadiance：所有位置用 world-space（km）；sun 路径追踪到大气顶点
vec3 skyRadiance(vec3 camPos, vec3 rd)
{ 
    // 从相机到大气外壳交点（世界坐标空间）
    float tAtm = RaySphereIntersectT(camPos, rd, earth_center, kAtmosphereRadius);
    if (tAtm < 0.0) return vec3(0.0);

    // 地面交点（若有）
    float tGround = RaySphereIntersectT(camPos, rd, earth_center, kPlanetRadius);

	float tMax = (tGround > 0.0) ? min(tGround, tAtm) : tAtm;
 

    const int SAMPLES = 12; // 增加精度
    vec3 total = vec3(0.0);
    for (int i = 0; i < SAMPLES; ++i)
	{
        float t = (float(i) + 0.5) * tMax / float(SAMPLES);
        vec3 sampleWorld = camPos + rd * t; // world-space sample

        // 计算从 sample 出发沿太阳方向到大气顶点的距离
        float tSun = RaySphereIntersectT(sampleWorld + sun_direction * 1e-6, sun_direction, earth_center, kAtmosphereRadius);
        if (tSun < 0.0) {
            // 如果不相交（理论上应相交），退回长距离
            tSun = 1000000.0;
        }

        vec3 sunTrans   = transmittance(sampleWorld, sampleWorld + sun_direction * tSun);
        vec3 viewTrans = transmittance(camPos, sampleWorld);

        vec3 attenuation = sunTrans * viewTrans;

        float cosTheta = dot(rd, sun_direction);

        vec3 phase = kBetaR * rayleighPhase(cosTheta) + kBetaM * miePhase(cosTheta, g_mie);
        total += phase * attenuation;
    }
    return total * (tMax / float(SAMPLES));
}

vec3 applyHeightFog(vec3 color, float dist, float height)
{
    float fogFactor = exp(-kFogDensity * dist * exp(-kFogHeightFalloff * height));
    return mix(kFogColor, color, clamp(fogFactor, 0.0, 1.0));
}


// 从深度重建世界坐标,使用km为单位
vec3 reconstructWorldPosition(vec2 uv, float depth) {
    // 将 [0,1] 深度转为 [-1,1] NDC Z
    float z = depth * 2.0 - 1.0;
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, z, 1.0);
    
    // 投影逆变换 → 相机空间
    vec4 viewSpace = projectInverse * clipSpace;
    viewSpace /= viewSpace.w;
    
    // 视图逆变换 → 世界空间
    vec4 worldSpace = viewInverse * viewSpace;
    return  worldSpace.xyz / worldSpace.w;
}

void main() 
{
	float depth        = texture(depthTexture, vUV).r;  
	  // 处理背景（天空）

	vec3 baseColor = texture(groundTexture,vUV).rgb;

    vec3 worldPos = reconstructWorldPosition(vUV, depth);
 
    vec3 viewDir = normalize(worldPos - cameraPosition);


    vec3 skyColor = skyRadiance(cameraPosition, viewDir);

    vec3 finalColor;

        // 大气中
    finalColor = skyColor;

        // 太阳光晕
    float cosTheta = dot(viewDir, sun_direction);
    float angle = acos(cosTheta);
    float sunDisk = max(0.0, 1.0 - angle / 0.0047); // 太阳视角半径 ≈ 0.27°
    finalColor += vec3(10.0, 6.0, 2.0) * sunDisk * sunDisk;
    

	// float dist = length(worldPos - cameraPosition);
  //  finalColor = applyHeightFog(finalColor, dist, height);

    // 色调映射（Reinhard）
    finalColor = finalColor / (1.0 + finalColor);

    fragColor = vec4(finalColor, 1.0);
}