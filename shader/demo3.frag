
#version 330
in vec3 view_ray;
in vec2 auv;

out vec4 fragColor;

uniform vec4 iMouse;
uniform vec3 iResolution;
uniform float iTime;
uniform int iFrame;

uniform sampler2D iChannelC0;
uniform sampler2D iChannelC1;
uniform sampler2D iChannelB2;

uniform vec3 cameraEye;      // 相机位置
uniform vec3 cameraCenter;   // 相机看向的点
uniform vec3 cameraUp;       // 相机 UP 向量
uniform vec3 cameraRight;    // 相机右向量
uniform float cameraFOV;     // 视锥体 FOV (弧度)
uniform float cameraAspect;  // 宽高比


// Lazy HLSL -> GLSL porting. Remove if you intend to use it with HLSL.
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define lerp mix

// Config
#define DRAW_PLANET                // Draw planet ground sphere.
#define PREVENT_CAMERA_GROUND_CLIP // Force camera to stay above horizon. Useful for certain games.
#define LIGHT_COLOR_IS_RADIANCE    // Comment out if light color is not in radiometric units.
#define AERIAL_SCALE               3.0 // Higher value = more aerial perspective. A value of 1 is tuned to match reference implementation.
#define NIGHT_LIGHT                2e-3 // Optional, cheap (free) non-physical night lighting. Makes twilight a bit purple which can look nice.
#define SUN_DISC_SIZE              1.0 // 1 is physical sun size (0.5 degrees).

// Math
#define INFINITY 3.402823466e38
#define PI       3.14159265359

// Atmosphere parameters (physical)
#define ATMOSPHERE_HEIGHT  100000.0
#define ATMOSPHERE_DENSITY 1.0
#define PLANET_RADIUS      6371000.0
#define PLANET_CENTER      float3(0, -PLANET_RADIUS, 0)
#define C_RAYLEIGH         (float3(5.802, 13.558, 33.100) * 1e-6)
#define C_MIE              (float3(3.996, 3.996, 3.996) * 1e-6)
#define C_OZONE            (float3(0.650, 1.881, 0.085) * 1e-6)

// Atmosphere parameters (approximation)
#define RAYLEIGH_MAX_LUM   2.5
#define MIE_MAX_LUM        0.5

// Magic numbers
#define M_EXPOSURE_MUL        0.23 // Tuned to match physical reference.
#define M_FAKE_MS             0.3 // Physical multiple scattering results in ~30% increase in energy.
#define M_AERIAL              2.5
#define M_TRANSMITTANCE       0.25
#define M_LIGHT_TRANSMITTANCE 1e6
#define M_MIN_LIGHT_ELEVATION -0.3
#define M_DENSITY_HEIGHT_MOD  1e-12
#define M_DENSITY_CAM_MOD     10.0
#define M_OZONE               1.5
#define M_OZONE2              5.0
#define M_MIE                 float3(0.95, 0.85, 0.75)

float sq(float x) { return x*x; }
float pow4(float x) { return sq(x)*sq(x); }
float pow8(float x) { return pow4(x)*pow4(x); }
float saturate(float x) { return clamp(x, 0., 1.); }


vec2 fragCoord;

// https://iquilezles.org/articles/intersectors/
float2 SphereIntersection(float3 rayStart, float3 rayDir, float3 sphereCenter, float sphereRadius)
{
	float3 oc = rayStart - sphereCenter;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - sq(sphereRadius);
    float h = sq(b) - c;
    if (h < 0.0)
    {
        return float2(-1.0, -1.0);
    }
    else
    {
        h = sqrt(h);
        return float2(-b-h, -b+h);
    }
}

float2 PlanetIntersection(float3 rayStart, float3 rayDir)
{
	return SphereIntersection(rayStart, rayDir, PLANET_CENTER, PLANET_RADIUS);
}

float2 AtmosphereIntersection(float3 rayStart, float3 rayDir)
{
	return SphereIntersection(rayStart, rayDir, PLANET_CENTER, PLANET_RADIUS + ATMOSPHERE_HEIGHT);
}

float PhaseR(float costh)
{
	return (1.0+sq(costh))*0.06;
}

float PhaseM(float costh, float g)
{
	g = min(g, 0.9381);
	float k = 1.55*g-0.55*sq(g)*g;
	float a = 1.0-sq(k);
	float b = 12.57*sq(1.0-k*costh);
	return a/b;
}

float3 GetLightTransmittance(float3 position, float3 lightDir, float multiplier, float ozoneMultiplier)
{
    float lightExtinctionAmount = exp(-(saturate(lightDir.y + 0.05) * 40.0)) +
        exp(-(saturate(lightDir.y + 0.5) * 5.0)) * 0.4 +
        sq(saturate(1.0-lightDir.y)) * 0.02 +
        0.002;
	return exp(-(C_RAYLEIGH + C_MIE + C_OZONE * ozoneMultiplier) * lightExtinctionAmount * ATMOSPHERE_DENSITY * multiplier * M_LIGHT_TRANSMITTANCE);
}

float3 GetLightTransmittance(float3 position, float3 lightDir)
{
	return GetLightTransmittance(position, lightDir, 1.0, 1.0);
}

void GetRayleighMie(float opticalDepth, float densityR, float densityM, out float3 R, out float3 M)
{
    // Approximate marched Rayleigh + Mie scattering with some exp magic.
    R = (1.0 - exp(-opticalDepth * densityR * C_RAYLEIGH / RAYLEIGH_MAX_LUM)) * RAYLEIGH_MAX_LUM;
	M = (1.0 - exp(-opticalDepth * densityM * C_MIE / MIE_MAX_LUM)) * MIE_MAX_LUM;
}

// Main atmosphere function
float3 GetAtmosphere(
    float3 rayStart,      // Camera position
    float3 rayDir,        // View direction
    float  rayLength,     // View distance
    float3 lightDir,      // Light (sun) direction
	float3 lightColor,    // Light (sun) color. Usually white
out float4 transmittance, // Atmospheric transmittance in xyz, planet intersection flag in w
    float4 fogFactor,     // (Optional) Fog "fade" factor. Can be used to add your own height fog or to fade the world out
    float  occlusion      // (Optional) Scattering occlusion (god rays)
) {
#ifdef PREVENT_CAMERA_GROUND_CLIP
	rayStart.y = max(rayStart.y, 1.0);
#endif

	// Planet and atmosphere intersection to get optical depth
	// TODO: Could simplify to circle intersection test if flat horizon is acceptable
	float2 t1 = PlanetIntersection(rayStart, rayDir);
	float2 t2 = AtmosphereIntersection(rayStart, rayDir);
    
    // Note: This only works if camera XZ is at 0. Otherwise, swap for the line below.
    float altitude = rayStart.y;
    //float altitude = (length(rayStart - PLANET_CENTER) - PLANET_RADIUS);
    float normAltitude = rayStart.y / ATMOSPHERE_HEIGHT;

	if (t2.y < 0.0)
	{
		// Outside of atmosphere looking into space, return nothing
		transmittance = float4(1, 1, 1, 1);
		return float3(0, 0, 0);
	}
    else
    {
        // In case camera is outside of atmosphere, subtract distance to entry.
        t2.y -= max(0.0, t2.x);

#ifdef DRAW_PLANET
        float opticalDepth = t1.x > 0.0 ? min(t1.x, t2.y) : t2.y;
#else
        float opticalDepth = t2.y;
#endif

        // Optical depth modulators
        opticalDepth = min(rayLength, opticalDepth);
        opticalDepth = min(opticalDepth * M_AERIAL * AERIAL_SCALE, t2.y);

        // Altitude-based density modulators
        float hbias = 1.0-1.0/(2.0+sq(t2.y)*M_DENSITY_HEIGHT_MOD);
        hbias = pow(hbias, 1.0+normAltitude*M_DENSITY_CAM_MOD); // Really need a pow here, bleh
        float sqhbias = sq(hbias);
        float densityR = sqhbias * ATMOSPHERE_DENSITY;
        float densityM = sq(sqhbias)*hbias * ATMOSPHERE_DENSITY;

        // Apply light transmittance (makes sky red as sun approaches horizon)
        float ly = lightDir.y;
        ly += saturate(-lightDir.y + 0.02) * saturate(lightDir.y + 0.7);
        ly = clamp(ly, -1.0, 1.0);
        lightColor *= GetLightTransmittance(rayStart, float3(lightDir.x, ly, lightDir.z), hbias, M_OZONE2);

#ifndef LIGHT_COLOR_IS_RADIANCE
        // If used in an environment where light "color" is not defined in radiometric units
        // we need to multiply with PI to correct the output.
        lightColor *= PI;
#endif

        float3 R, M;
        GetRayleighMie(opticalDepth, densityR, densityM, R, M);
        
        float3 E = (C_RAYLEIGH * densityR + C_MIE * densityM + C_OZONE * densityR * M_OZONE) * pow4(1.0 - normAltitude) * M_TRANSMITTANCE;

        float costh = dot(rayDir, lightDir);
        float phaseR = PhaseR(costh);
        float phaseM = PhaseM(costh, 0.88);
        
#ifdef NIGHT_LIGHT
        float nightLight = NIGHT_LIGHT;
#else
        float nightLight = 0.0;
#endif
        
        // Combined scattering
        float3 rayleigh = (phaseR * occlusion + phaseR * M_FAKE_MS) * lightColor + nightLight * phaseR;
        float3 mie = ((phaseM * occlusion + phaseR * M_FAKE_MS) * lightColor + nightLight * phaseR) * M_MIE;
        float3 scattering = mie * M + rayleigh * R;

        // View extinction, matched to reference
        transmittance.xyz = exp(-(opticalDepth + pow8(opticalDepth * 4.5e-6)) * E);
        // Store planet intersection flag in transmittance.w, useful for occluding clouds, celestial bodies etc.
        transmittance.w = step(t1.x, 0.0);

        if (fogFactor.w > 0.0)
        {
            // 2nd sample (all the way to atmosphere exit), used for fog fade.
            opticalDepth = t2.y;
            GetRayleighMie(opticalDepth, densityR, densityM, R, M);
            float3 scattering2 = mie * M + rayleigh * R;
            float3 transmittance2 = exp(-opticalDepth * E);

            scattering2 *= lerp(fogFactor.xyz, float3(1, 1, 1), sq(fogFactor.w)); // Fog color test
            scattering = lerp(scattering, scattering2, fogFactor.w);
            transmittance.xyz = lerp(transmittance.xyz, transmittance2, fogFactor.w);
        }
        
        if (t1.y > 0.0 && t1.y < rayLength)
        {
            // Darken planet
            float3 planetColor = float3(0.4, 0.4, 0.4);
            float planetOpticalDepth = t1.y - max(0.0, t1.x);
            float skyWeight = exp(-planetOpticalDepth * 1e-6);
            scattering *= lerp(planetColor, float3(1, 1, 1), skyWeight);
        }

        return scattering * M_EXPOSURE_MUL;
    }
}

// Overloaded functions
float3 GetAtmosphere(
    float3 rayStart,
    float3 rayDir,
    float rayLength,
    float3 lightDir,
	float3 lightColor,
out float4 transmittance
) 
{
    return GetAtmosphere(rayStart, rayDir, rayLength, lightDir, lightColor, transmittance, vec4(0.0), 1.0);
}
float3 GetAtmosphere(
    float3 rayStart,
    float3 rayDir,
    float rayLength,
    float3 lightDir,
    float3 lightColor
) 
{
    float4 transmittance;
    return GetAtmosphere(rayStart, rayDir, rayLength, lightDir, lightColor, transmittance, vec4(0.0), 1.0);
}

float3 GetSunDisc(float3 rayDir, float3 lightDir)
{
    const float A = cos(0.00436 * SUN_DISC_SIZE);
	float costh = dot(rayDir, lightDir);
	float disc = sqrt(smoothstep(A, 1.0, costh));
	return float3(disc, disc, disc);
}




 
#define ENABLE_ATMOSPHERE

// Disable all defines below to preview only the atmosphere function

#define ENABLE_AMBIENT_OCCLUSION
#define ENABLE_ATMOSPHERE_OCCLUSION
#define ENABLE_HEIGHT_FOG

//#define ENABLE_TERRAIN
#define ENABLE_CLOUDS
//#define ENABLE_STARS
#define ENABLE_CELESTIAL_BODIES
//#define ENABLE_MOON_LIGHT

//#define ENABLE_WATER

#define CAM_HEIGHT         1800.0
#define CAM_Z              1.4
#define TERRAIN_HEIGHT     3000.0
#define TERRAIN_CENTER     0.3
#define TERRAIN_SCALE      4000.0
#define TERRAIN_OFFSET     vec2(0.0, 20.0)
#define TERRAIN_OCTAVES    12
#define TERRAIN_GAIN       0.45
#define TERRAIN_LACUNARITY 2.0
#define CACHE_TERRAIN
#define TEMPORAL_ACCUMULATION_LIGHTING 0.98
#define TEMPORAL_ACCUMULATION_CLOUDS 0.85

#define CLOUD_COVERAGE 0.3 // Fixed cloud coverage
//#define CLOUD_COVERAGE (0.35 - sin(iTime * 0.02 + 3.1415) * 0.2) // Animated cloud coverage
#define CLOUD_DENSITY 1.0
#define CLOUD_PLANE_BOT 3000.0
#define CLOUD_PLANE_TOP 5000.0

#define HEIGHT_FOG_PLANE 1000.0
#define HEIGHT_FOG_DENSITY 5e-5

#define WATER_HEIGHT 100.0

#define CAM_EXPOSURE 10.0
#define CAM_AUTO_EXPOSURE
#define CAM_AUTO_EXPOSURE_EV_LIMIT 4.0
#define CAM_TONEMAP
#define CAM_DITHER
#define CAM_GAMMA 2.0
#define CAM_FXAA

//#define DEBUG_SHADOWS
//#define DEBUG_AMBIENT_OCCLUSION
//#define DEBUG_ATMOSPHERE_OCCLUSION

//#define DEBUG_ALBEDO
//#define DEBUG_NORMAL
//#define DEBUG_LIGHTING

 
void GetCamera(vec2 uv, vec3 res, out vec3 ro, out vec3 rd)
{
//    ro = vec3(0, CAM_HEIGHT, 0);
//    rd = normalize(vec3((uv - 0.5) * vec2(res.x / res.y, 1.0), CAM_Z));
//	return;
//
//       // ro: 使用真实相机位置
    ro = cameraEye;
    
    // 计算基于相机宽高比的 UV 坐标
    vec2 uv_centered = uv - 0.5;  // [-0.5, 0.5]
    
    // 根据 FOV 计算视锥体
    float vlen = tan(cameraFOV / 2.0);
    float hlen = vlen * cameraAspect;
    
    // 在相机坐标系中构建光线
    vec3 forward = normalize(cameraCenter - cameraEye);
    
    // 光线方向 = forward + right * hlen * uv.x + up * vlen * uv.y
    rd = normalize(
        forward +
        cameraRight * (uv_centered.x * hlen * 2.0) +
        cameraUp * (uv_centered.y * vlen * 2.0)
    );

}
vec2 GetUV(vec3 rd, vec3 res)
{
    return rd.xy / (rd.z / CAM_Z) / vec2(res.x / res.y, 1.0) + 0.5;
}
vec3 ExpandPackedNormal(vec2 n)
{
    return vec3(n.x, sqrt(1.0 - clamp(dot(n, n), 0.0, 1.0)), n.y);
}
float GetTemporalStability(int frame, float hash, float prevHash, float accumulation, float scale)
{
    return frame == 0 ? 0.0 : accumulation * exp(-abs(hash - prevHash) * scale);
}

// https://www.shadertoy.com/view/4djSRW
float hash11(float p)
{
    p = fract((p+1.0)*0.1031);
    p *= p+33.33;
    return fract(p*p*2.0);
}
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
float hash14(vec4 p4)
{
	p4 = fract(p4  * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return fract((p4.x + p4.y) * (p4.z + p4.w));
}
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}
vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}

// http://iquilezles.org/articles/morenoise/
const mat2 m = mat2(0.8,-0.6,0.6,0.8);
vec3 Noised(vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    vec2 u = f*f*(3.0-2.0*f);
    float a = hash12(p+vec2(0,0));
    float b = hash12(p+vec2(1,0));
    float c = hash12(p+vec2(0,1));
    float d = hash12(p+vec2(1,1));
	return vec3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,
				6.0*f*(1.0-f)*(vec2(b-a,c-a)+(a-b-c+d)*u.yx));   
}

///////
// Buffer A renders the terrain during the first frame, or whenever the resolution changes.

#define MAX_SAMPLES 512
#define MAX_DIST 200000.0

float Terrain(vec2 p)
{
    p /= TERRAIN_SCALE;
    p += TERRAIN_OFFSET;
    
    float a = 0.0;
    float b = 1.0;
    vec2 d = vec2(0.0);
    vec3 n = vec3(0.0);
    
    float na = 0.5;
    
    vec2 pf = p;
    for (int i = 0; i < TERRAIN_OCTAVES; i++)
    {
        vec3 nl = Noised(pf);
        //nl.x = 1.0 - abs(nl.x - 0.5) * 2.0;
        n += nl * na / (1.0+dot(d,d));
        d += nl.yz;       
        na *= TERRAIN_GAIN;
        pf = m*pf*TERRAIN_LACUNARITY;
        pf += vec2(-d.y,d.x) * 0.1; // curl warp
    }

    return (n.x - TERRAIN_CENTER) * TERRAIN_HEIGHT;
}
vec3 TerrainNormal(vec3 p)
{
    float d = 1.0;
    vec3 p0 = p;
    vec3 p1 = p + vec3(d, 0, 0);
    vec3 p2 = p + vec3(0, 0, d);
    p0.y = Terrain(p0.xz);
    p1.y = Terrain(p1.xz);
    p2.y = Terrain(p2.xz);
    
    return normalize(cross(p2 - p0, p1 - p0));
}
float MarchTerrain(vec3 ro, vec3 rd, float maxDist, out vec3 normal)
{
    float t = 0.0;
    float ds = 0.0;
    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        vec3 p = ro + rd * t;
        float h = p.y - Terrain(p.xz);
        if (h < 0.0)
        {
            t -= ds * 0.5;
            break;
        }
        ds = t * 1e-3 + h * 0.7;
        t += ds;
        if (t > maxDist)
        {
            t = -1.0;
            break;
        }
    }
    normal = TerrainNormal(ro + rd * t);
    return t;
}

vec4 Render(vec2 pixel)
{
    vec2 uv = pixel / iResolution.xy;
    
    vec3 ro, rd;
    GetCamera(uv, iResolution, ro, rd);
    
    vec3 normal;
    float t = MarchTerrain(ro, rd, MAX_DIST, normal);
    
    vec2 t_planet = PlanetIntersection(ro, rd);
    if (t_planet.x > 0.0 && t < 0.0)
    {
        t = t_planet.x;
    }
    
    vec4 color = vec4(t, normal.xz, 0.0);
    
    if (t > 0.0)
    {
        vec3 p = ro + rd * t;
        float d = 100.0;
        vec3 n1 = TerrainNormal(p - vec3(d, 0, 0));
        vec3 n2 = TerrainNormal(p + vec3(0, 0, d));
        vec3 n3 = TerrainNormal(p - vec3(d, 0, 0));
        vec3 n4 = TerrainNormal(p + vec3(0, 0, d));
        color.w = (dot(n1, n2) + dot(n3, n4)) / 2.0;
    }
    
    if (pixel.x < 1.0 && pixel.y < 1.0)
    {
        color.w = iResolution.x;
    }
    
    return color;
}

vec4 mainImage_Terrain(  in vec2 fragCoord )
{    
    return  Render(fragCoord);
}


///////////////
// Buffer B renders god rays, shadows and ambient occlusion
// (by marching the terrain buffer depth).

#define SAMPLE_COUNT_ATMOSPHERE_OCCLUSION 4.0
#define SAMPLE_COUNT_SHADOWS 4.0
#define SAMPLE_COUNT_CLOUD_SHADOWS 2.0
#define SAMPLE_COUNT_AMBIENT_OCCLUSION 4.0
#define SHADOW_RANGE 1000.0
#define AMBIENT_OCCLUSION_RANGE 750.0
#define CLOUD_SHADOW_RANGE 20000.0

uint Hash(uint s)
{
    s ^= 2747636419u;
    s *= 2654435769u;
    s ^= s >> 16;
    s *= 2654435769u;
    s ^= s >> 16;
    s *= 2654435769u;
    return s;
}

float Random(uint seed)
{
    return float(Hash(seed)) / 4294967295.0;
}

float3 RandomUnitVector(uint seed)
{
    float PI2 = 6.28318530718;
    float z = 1.0 - 2.0 * Random(seed);
    float xy = sqrt(1.0 - z * z);
    float r = Random(seed + 1u);
    float sn = sin(PI2 * r);
    float cs = cos(PI2 * r);
    return float3(sn * xy, cs * xy, z);
}

float GetCloudShadow(vec2 uv, float rl, float thickness)
{
     return 1.0;
 }
 
vec4 mainImage_B(in vec2 fragCoord )
{
    vec4 fragColor;
    vec4 mouse = iMouse / iResolution.xyxy;
    float time = -iTime * 0.2 + 3.5;
    vec2 sunPos = 0.5 + vec2(cos(time) * 0.4, sin(time) * 0.45);
 
    vec3 sunDir;
    vec3 foo;
    GetCamera(sunPos, iResolution, foo, sunDir);
    
    const vec3 moonDir = normalize(vec3(0.5, 0.25, 1));
    
    vec2 uv = fragCoord / iResolution.xy;
    
    vec3 ro, rd;
    GetCamera(uv, iResolution, ro, rd);
    
    float dither = texture(iChannelB2, uv * iResolution.xy / vec2(1024), 0.0).x;
    dither = fract(dither + (0.61803398875 * float(iFrame & 255)));
    
    float atmosphereOcclusion = 1.0;
    float shadow = 1.0;
    float shadow2 = 1.0;
    float ambientOcclusion = 1.0;
    
#ifdef ENABLE_ATMOSPHERE_OCCLUSION
    atmosphereOcclusion = 0.0;
    for (float i = 0.0; i < SAMPLE_COUNT_ATMOSPHERE_OCCLUSION; i++)
    {
        float delta = (i + dither) / SAMPLE_COUNT_ATMOSPHERE_OCCLUSION;
        vec2 localUV = mix(uv, sunPos, delta);
        
        float cloud = GetCloudShadow(localUV, delta * 1e10, 1e12);
        
         float terrain = 1.0;
         
        atmosphereOcclusion += terrain * cloud;
    }
    atmosphereOcclusion /= SAMPLE_COUNT_ATMOSPHERE_OCCLUSION;
#endif

    vec4 terrain =vec4(0);
    vec3 p = ro + rd * terrain.x * 0.998;
    vec3 n = ExpandPackedNormal(terrain.yz);
    
 
#ifdef ENABLE_AMBIENT_OCCLUSION
    ambientOcclusion = 0.0;
    float cloudOcclusion = 1.0;
    if (terrain.x > 0.0)
    {
        ivec2 iFragCoord = ivec2(fragCoord.xy);
        vec3 sampleDir = RandomUnitVector(uint(iFrame * iFragCoord.x * iFragCoord.y + iFragCoord.x + iFragCoord.y * int(iResolution.x)));
        sampleDir *= sign(dot(sampleDir, n));
        sampleDir = normalize(sampleDir);
        for (float i = 0.0; i < SAMPLE_COUNT_AMBIENT_OCCLUSION; i++)
        {
            float delta = (i + dither) / SAMPLE_COUNT_AMBIENT_OCCLUSION;
            vec3 localP = p + sampleDir * delta * AMBIENT_OCCLUSION_RANGE;
            vec3 dir = localP - ro;
            float dist = length(dir);
            dir /= dist;
            vec2 localUV = GetUV(dir, iResolution);
            vec4 localTerrain =vec4(0);
            float occluder = localTerrain.x - dist;
            float cloud = pow(GetCloudShadow(localUV, dist, 1e5), 0.05);
            ambientOcclusion += (localTerrain.x < 0.0 || occluder > 0.0 || occluder < -AMBIENT_OCCLUSION_RANGE ? 1.0 : 0.0) * cloud;
        }
        ambientOcclusion /= SAMPLE_COUNT_AMBIENT_OCCLUSION;
        ambientOcclusion = ambientOcclusion;
    }
    else
    {
        ambientOcclusion = 1.0;
    }
#endif

    vec4 prev =vec4(0);    
    vec4 prevData =vec4(0);
    float screenHash = iResolution.x + iResolution.y;
    float screenHashPrev = prevData.z;
    float sunHash = sunPos.x + sunPos.y;
    float sunHashPrev = prevData.w;
    
    float temporalStability = GetTemporalStability(iFrame, sunHash, sunHashPrev, TEMPORAL_ACCUMULATION_LIGHTING, 50.0);
    atmosphereOcclusion = mix(atmosphereOcclusion, prev.x, temporalStability);
    
    temporalStability = GetTemporalStability(iFrame, sunHash, sunHashPrev, TEMPORAL_ACCUMULATION_LIGHTING, 20.0);
    shadow = mix(shadow, prev.y, temporalStability);
    shadow2 = mix(shadow2, prev.z, temporalStability);
    
    temporalStability = GetTemporalStability(iFrame, screenHash, screenHashPrev, TEMPORAL_ACCUMULATION_LIGHTING, 20.0);
    ambientOcclusion = mix(ambientOcclusion, prev.w, temporalStability);
    
    fragColor = vec4(atmosphereOcclusion, shadow, shadow2, ambientOcclusion);
    if (fragCoord.x < 1.0 && fragCoord.y < 1.0)
    {
        // Store some data in corner pixel
        fragColor.z = screenHash;
        fragColor.w = sunHash;
    }

    return fragColor;
}

 
//////////////////////
// Buffer C renders clouds (final lighting is composited in Image)

float Noise(vec2 x)
{
    return textureLod(iChannelC0, x * 0.002, 0.0).x;   
}

// https://www.shadertoy.com/view/csSfRK
#define PARAMS_LINEAR_RAMP  vec2(0.00, 0.00)
#define PARAMS_CUMULUS      vec2(0.4, 0.6)
#define PARAMS_CUMULONIMBUS vec2(0.70, 0.98)
float CloudShape(float x, float y, vec2 shapeParams)
{  
    shapeParams.x *= shapeParams.y;
    shapeParams.y = 1.0 / (1.0 - shapeParams.y);
    float anvil = 1.0 - sq(abs(y - 0.5) * 2.0);
	return saturate(x - anvil * shapeParams.x - pow(y, shapeParams.y));
}

#define SCALE 1000.0
#define OFFSET vec2(4.0, 8.0)
#define SHAPE PARAMS_CUMULUS

float remap01(float x, float a, float b) { return clamp((x - a) / (b - a), 0.0, 1.0); }

float Height(vec3 p)
{
    return length(p - PLANET_CENTER) - PLANET_RADIUS;
}

float Cloud(vec3 p)
{
    vec2 wind = vec2(0, iTime);
    float y = (Height(p) - CLOUD_PLANE_BOT) / (CLOUD_PLANE_TOP - CLOUD_PLANE_BOT);
    const float scale = 1.0 / SCALE;
    float n = Noise(p.xz * scale + OFFSET + vec2(-0.2, 0.3) * iTime);
    float d = CloudShape(n - 1.0 + CLOUD_COVERAGE * 2.0, y, SHAPE);
    
    float n2 = Noise(p.xz * scale * 8.0 - vec2(0.2, 0.0) * iTime);
    float n3 = Noise(p.xz * scale * 40.0 + vec2(1.0, 0.0) * iTime);
    
    d = remap01(d, (1.0 - n2) * 0.3, 1.0);
    d = remap01(d, (1.0 - n3) * 0.1, 1.0);
    
    d *= smoothstep(0.0, 0.75, y);
    
    return sq(d) * CLOUD_DENSITY;
}

vec3 Lum(vec3 p, vec3 ld, float costh, float ext, float dither)
{
    float le = 0.0;
    float ae = 0.0;
    int sc = 4;
    float ss = (CLOUD_PLANE_TOP - CLOUD_PLANE_BOT) / float(sc);
    for (int j = 0; j < sc; j++)
    {
        vec3 lp = p + ld * (float(j) + dither) * ss;
        le += Cloud(lp) * ss;
    }
    sc = 2;
    for (int j = 0; j < sc; j++)
    {
        vec3 lp = p + vec3(0, 1, 0) * (float(j) + dither) * ss;
        ae += Cloud(lp) * ss;
    }
    float single = exp(-le) * PhaseM(costh, 0.85);
    float multi = exp(-le * 0.05) * PhaseR(costh);
    multi *= 1.0 - exp(-ext * 6e2);
    return vec3(single + multi, (exp(-ae) + exp(-ae * 0.05)) * 0.5, 0);
}

vec4 mainImage_C( in vec2 fragCoord )
{
 
vec4 fragColor;

#ifndef ENABLE_CLOUDS
    fragColor = vec4(0, 0, 0, 0);
    return fragColor;
#endif

    vec2 uv = fragCoord / iResolution.xy;
    
    vec3 dither = textureLod(iChannelC1, fragCoord / vec2(1024), 0.0).xyz;

    dither = fract(dither + (0.61803398875 * float(iFrame & 255)));
    
    vec4 mouse = iMouse / iResolution.xyxy;
    float time = -iTime * 0.2 + 3.5;
    vec2 sunPos = 0.5 + vec2(cos(time) * 0.4, sin(time) * 0.45);
    if (iMouse.x > 10.0)
    {
        sunPos = mouse.xy;
    }
    vec3 sunDir;
    vec3 foo;
    GetCamera(sunPos, iResolution, foo, sunDir);

    vec3 ro, rd;
    GetCamera(uv, iResolution, ro, rd);
    
    float costh = dot(rd, sunDir);
    
    vec2 t1 = SphereIntersection(ro, rd, PLANET_CENTER, PLANET_RADIUS + CLOUD_PLANE_BOT);
    vec2 t2 = SphereIntersection(ro, rd, PLANET_CENTER, PLANET_RADIUS + CLOUD_PLANE_TOP);
    vec2 tp = PlanetIntersection(ro, rd);
    
    float enter = t1.y;
    float exit = t2.y;
    if (t1.x > 0.0)
    {
        exit = t1.x;
    }
    if (t2.x > 0.0)
    {
        enter = t2.x;
    }
    if (ro.y > CLOUD_PLANE_BOT && ro.y < CLOUD_PLANE_TOP)
    {
        enter = 0.0;
    }
    if (tp.x > 0.0)
    {
        enter = min(enter, tp.x);
        exit = min(exit, tp.x);
    }
    
    enter = max(0.0, enter);

    
    vec2 shape = PARAMS_CUMULUS;
    float coverage = 0.9;
    
    float depth = 0.0;
    
    vec3 s = vec3(0.0);
    float tsm = 1.0;
    int sc = 64;
    float ss = 100.0;
    float t = enter + ss * 0.5;
    for (int i = 0; i < sc; i++)
    {
        vec3 p = ro + rd * (t + ss * dither.x);
        
        float h = Height(p);
        if (h < CLOUD_PLANE_BOT || h > CLOUD_PLANE_TOP)
        {
            break;
        }
        
        float le = Cloud(p);
        vec3 ll = vec3(0.0);
        if (le > 0.0)
        {
            ll = Lum(p, sunDir, costh, le, dither.y);
            vec4 at;
        }
        
        float lt = exp(-le * ss);
        float is = tsm * (1.0 - lt);

        depth = mix(depth, t, sq(tsm));
        
        s += ll * is;
		tsm *= lt;
        
        if (tsm < 1e-5)
        {
            break;
        }
        
        t += ss;
        ss *= 1.05;
        
        if (t > exit)
        {
            break;
        }
    }
    
    vec4 prev = vec4(0.0);
    
    float temporalStability = iMouse.z > 0.0 || iFrame < 1 ? 0.0 : TEMPORAL_ACCUMULATION_CLOUDS;


    float screenHash = iResolution.x + iResolution.y;

    float sunHash = sunPos.x + sunPos.y;
    
    fragColor = max(vec4(0.0), mix(vec4(s.xy / (1.0 - tsm), 1.0 - tsm, depth), prev, temporalStability));
    
    if (fragCoord.x < 1.0 && fragCoord.y < 1.0)
    {
        fragColor.z = screenHash;
        fragColor.w = sunHash;
    }

    return fragColor;
}

////////////////////////////
// Render the image

// Sample Buffer A (terrain), Buffer B (god rays/shadow/ambient occlusion) and Buffer C (clouds),
// then composite with atmosphere from Common and apply color grading etc.

#define HEIGHT_FOG_DIST_WEIGHT 1e-4
#define HEIGHT_FOG_COLOR vec3(0.8, 0.9, 1.0)

#define AMBIENT_DIR_BIAS 4.0

struct Light
{
    vec3 direction;
    vec3 radiance;
};

vec3 TonemapACES(vec3 color)
{
    return (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
}

float GetFogWeight(vec3 ro, vec3 rd, float rl, float h)
{
#ifdef ENABLE_HEIGHT_FOG
    vec2 fogt = SphereIntersection(ro, rd, PLANET_CENTER, PLANET_RADIUS + HEIGHT_FOG_PLANE);
    fogt.x = max(0.0, fogt.x);
    fogt.y = min(fogt.y, rl);
    int sc = 4;
    float ss = (fogt.y - fogt.x) / float(sc);
    float od = 0.0;
    for (int i = 0; i < sc; i++)
    {
        float delta = (float(i) + 0.5) / float(sc);
        vec3 p = ro + rd * mix(fogt.x, fogt.y, delta);
        float y = distance(p, PLANET_CENTER) - PLANET_RADIUS;
        float f = max(0.0, sq(1.0 - y / HEIGHT_FOG_PLANE));
        od += f * ss;
    }
    return 1.0 - exp(-od * HEIGHT_FOG_DENSITY);
#else
    return 0.0;
#endif
}

 
vec4 ResolveClouds(vec4 clouds, vec3 ro, vec3 rd, Light light1, Light light2, float atmosphereOcclusion)
{
    float cloudDepth = clouds.w;
    float cloudAlpha = clamp(clouds.z + 1e-5, 0.0, 1.0);
    vec3 cloudPos = ro + rd * cloudDepth;
    
    float fogWeight = GetFogWeight(ro, rd, cloudDepth, cloudPos.y);
    vec4 transmittance;
    vec3 scattering = GetAtmosphere(ro, rd, cloudDepth, light1.direction, light1.radiance, transmittance, vec4(HEIGHT_FOG_COLOR, fogWeight), atmosphereOcclusion);
#ifdef ENABLE_ATMOSPHERE
    vec3 al = GetAtmosphere(cloudPos, vec3(0, 1, 0), INFINITY, light1.direction, light1.radiance);
#else
    vec3 al = vec3(0.0);
#endif
 
    clouds.x *= sqrt(atmosphereOcclusion);
    float costh = dot(rd, light1.direction);
    vec3 cloudColor = (GetLightTransmittance(cloudPos, light1.direction) * light1.radiance * clouds.x + al * clouds.y * 2.0) * cloudAlpha;
    //cloudColor = vec3(0.0);
#ifdef ENABLE_ATMOSPHERE
    cloudColor = cloudColor * transmittance.xyz + scattering * cloudAlpha;
#endif
    return vec4(cloudColor, 1.0 - cloudAlpha);
}


/////////////////////
void main()
{
    fragCoord = gl_FragCoord.xy;


    vec4 mouse = iMouse / iResolution.xyxy;

    float time = -iTime * 0.2 + 3.5;
    vec2 sunPos = 0.5 + vec2(cos(time) * 0.4, sin(time) * 0.45);
 //   if (iMouse.x > 10.0)
    {
 //       sunPos = mouse.xy;
    }
    
    Light sun, moon;
    
    vec3 foo;
    GetCamera(sunPos, iResolution, foo, sun.direction);
    
    vec2 moonPos = vec2(0.8, 0.7);
    GetCamera(moonPos, iResolution, foo, moon.direction);

    vec3 moonCenter = moon.direction * 384400e3;
    const float moonRadius = 1737e3 * SUN_DISC_SIZE * 0.9; // Scale moon size with sun size
    
    sun.radiance = vec3(1.0);
    moon.radiance = vec3(0.05) * (dot(sun.direction, -moon.direction) * 0.5 + 0.5);
    
    vec3 moonRadiance = vec3(0.05) * (dot(sun.direction, -moon.direction) * 0.5 + 0.5);
    
    float eclipse = smoothstep(1.0, 0.9999, dot(sun.direction, moon.direction));
    eclipse = sqrt(eclipse);
    sun.radiance *= mix(1.0, eclipse, 0.999);
    
    vec2 jitter = vec2(0.0);
     
    vec2 uv = (fragCoord + jitter) / iResolution.xy;
    vec2 uv0 = fragCoord / iResolution.xy;
    
    vec3 ro, rd;
    GetCamera(uv, iResolution, ro, rd);
    
    float rl = INFINITY;
    
    vec3 color = vec3(0.0);
    

    vec4 occlusionTex = mainImage_B(fragCoord);
    float atmosphereOcclusion = occlusionTex.x;
    float shadow = occlusionTex.y;
    float shadow2 = occlusionTex.z;
    float ambientOcclusion = pow8(occlusionTex.w); 
      
    float temporalWeight = 0.0;
	
    // Get atmosphere (sun)
    vec4 transmittance;
    float altitude = max(0.0, (ro + rd * rl).y);
    float fogWeight = GetFogWeight(ro, rd, rl, altitude);

    vec3 scattering = GetAtmosphere(ro, rd, rl, sun.direction, sun.radiance, transmittance, vec4(HEIGHT_FOG_COLOR, fogWeight), atmosphereOcclusion);
   
  
    
#ifdef ENABLE_ATMOSPHERE
    // Apply atmosphere (sun)
    color = color * transmittance.xyz + scattering;
 #endif

#ifdef ENABLE_CLOUDS
    vec4 clouds = mainImage_C( fragCoord);
    clouds = ResolveClouds(clouds, ro, rd, sun, moon, atmosphereOcclusion);
    color = color * clouds.a + clouds.rgb;
#endif

#ifdef CAM_AUTO_EXPOSURE
    // Auto-exposure
    vec3 radiance = GetAtmosphere(ro, vec3(0, 1, 0), INFINITY, sun.direction, sun.radiance) +
        GetAtmosphere(ro, vec3(0, 1, 0), INFINITY, moon.direction, moon.radiance);
    float lum = dot(radiance, vec3(0.3, 0.59, 0.11));
    color *= min(CAM_AUTO_EXPOSURE_EV_LIMIT, 0.003 / clamp(lum, 0.0002, 1.0));
#endif

#ifdef CAM_EXPOSURE
    color *= CAM_EXPOSURE;
#endif

#ifdef CAM_TONEMAP
    color = TonemapACES(color);
#endif

#ifdef CAM_GAMMA
    color = pow(color, vec3(1.0 / CAM_GAMMA));
#endif
   

 
 
     
    fragColor = vec4(color, 1.0);
}