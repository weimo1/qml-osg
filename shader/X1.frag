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
uniform float sunZenithAngle;  // 新增：太阳天顶角度
uniform float sunAzimuthAngle;  // 新增：太阳方位角度
uniform sampler2D iChannel0;   // 新增：噪声纹理
uniform float iTime;           // 新增：时间变量

// constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

const float n = 1.0003; // refractive index of air
const float N = 2.545E25; // number of molecules per unit volume for air at 288.15K and 1013mb (sea level -45 celsius)

// optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
// 66 arc seconds -> degrees, and the cosine of that
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
// 1.0 / ( 4.0 * pi )
const float ONE_OVER_FOURPI = 0.07957747154594767;

// 云彩参数 - 调整参数使云层更明显但不过于厚重
const float cloudscale = 1.1;
const float speed = 0.03;
const float clouddark = 0.6;   // 增加暗部
const float cloudlight = 0.6;  // 增加光照影响
const float cloudcover = 0.4;  // 增加云层覆盖
const float cloudalpha = 7.0;  // 增加云层密度
const float skytint = 0.8;     // 增加天空色调影响

const vec3 skycolour1 = vec3(0.2, 0.4, 0.6);
const vec3 skycolour2 = vec3(0.4, 0.7, 1.0);

// 更亮的云颜色
const vec3 cloudcolourBase = vec3(1.3, 1.3, 1.2);  // 更亮的云颜色

const mat2 m = mat2(1.6, 1.2, -1.2, 1.6);

// 哈希函数
vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

// 噪声函数
float noise(in vec2 p) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;
    
    vec2 i = floor(p + (p.x + p.y) * K1);    
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;

    vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3 n = h * h * h * h * vec3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
    
    return dot(n, vec3(70.0));    
}

// 分形布朗运动
float fbm(vec2 n) {
    float total = 0.0, amplitude = 0.1;
    for (int i = 0; i < 7; i++) {
        total += noise(n) * amplitude;
        n = m * n;
        amplitude *= 0.4;
    }
    return total;
}

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

void main() 
{
    vec3 worldPos = vWorldPosition;
    vec3 direction = normalize(worldPos - cameraPosition);
  
    vec3 sunDir = vSunDirection;

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
    float theta = acos(direction.y); // elevation --> y-axis, [-pi/2, pi/2]
    float phi = atan(direction.z, direction.x); // azimuth --> x-axis [-pi/2, pi/2]
    vec2 uv = vec2(phi, theta) / vec2(2.0 * pi, pi) + vec2(0.5, 0.0);
    vec3 L0 = vec3(0.1) * Fex;

    // composition + solar disc
    float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (vSunE * 19000.0 * Fex) * sundisk;

    vec3 texColor = (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);

    texColor *= 2.0 ;
    vec3 retColor = pow(texColor, vec3(1.0 / (1.2 + (1.2 * vSunfade))));

    retColor*=0.2;

    // 只在太阳以上的位置生成云朵
    // 计算太阳的仰角
    float sunElevation = sunDir.y; // 太阳的y分量表示其仰角
    
    // 只在高于太阳仰角的位置生成云朵
    if (direction.y > sunElevation - 0.1) { // 稍微放宽一点条件，让云朵可以在太阳附近生成
        // 根据与太阳仰角的差值调整云朵密度
        float elevationDiff = direction.y - (sunElevation - 0.1);
        float cloudFactor = smoothstep(0.0, 0.3, elevationDiff);
        
        // 添加云彩效果
        // 计算片段坐标（模拟全屏效果）
        vec2 fragCoord = worldPos.xz;
        vec2 iResolution = vec2(800.0, 600.0); // 假设的分辨率
        vec2 p = fragCoord.xy / iResolution.xy;
        vec2 uv_cloud = p * vec2(iResolution.x / iResolution.y, 1.0);    
        
        float time = iTime * speed;
        float q = fbm(uv_cloud * cloudscale * 0.5);
        
        // ridged noise shape
        float r = 0.0;
        uv_cloud *= cloudscale;
        uv_cloud -= q - time;
        float weight = 0.8;
        for (int i = 0; i < 8; i++) {
            r += abs(weight * noise(uv_cloud));
            uv_cloud = m * uv_cloud + time;
            weight *= 0.7;
        }
        
        // noise shape
        float f = 0.0;
        uv_cloud = p * vec2(iResolution.x / iResolution.y, 1.0);
        uv_cloud *= cloudscale;
        uv_cloud -= q - time;
        weight = 0.7;
        for (int i = 0; i < 8; i++) {
            f += weight * noise(uv_cloud);
            uv_cloud = m * uv_cloud + time;
            weight *= 0.6;
        }
        
        f *= r + f;
        
        // noise colour
        float c = 0.0;
        time = iTime * speed * 2.0;
        uv_cloud = p * vec2(iResolution.x / iResolution.y, 1.0);
        uv_cloud *= cloudscale * 2.0;
        uv_cloud -= q - time;
        weight = 0.4;
        for (int i = 0; i < 7; i++) {
            c += weight * noise(uv_cloud);
            uv_cloud = m * uv_cloud + time;
            weight *= 0.6;
        }
        
        // noise ridge colour
        float c1 = 0.0;
        time = iTime * speed * 3.0;
        uv_cloud = p * vec2(iResolution.x / iResolution.y, 1.0);
        uv_cloud *= cloudscale * 3.0;
        uv_cloud -= q - time;
        weight = 0.4;
        for (int i = 0; i < 7; i++) {
            c1 += abs(weight * noise(uv_cloud));
            uv_cloud = m * uv_cloud + time;
            weight *= 0.6;
        }
        
        c += c1;
        
        vec3 skycolour = mix(skycolour2, skycolour1, p.y);
        vec3 cloudcolour = cloudcolourBase * clamp((clouddark + cloudlight * c), 0.0, 1.0);
       
        f = cloudcover + cloudalpha * f * r;
        
        // 限制云的覆盖范围并调整密度
        f = clamp(f + c, 0.0, 1.0) * 0.9;  // 增加云层密度但保持透明度
        
        // 根据与太阳位置的关系调整云朵密度
        f *= cloudFactor;
        
        // 混合天空和云，使用适中的混合方式
        vec3 result = mix(retColor, clamp(skytint * retColor + cloudcolour, 0.0, 1.0), f * 0.85);  // 适中的云层混合强度

        color = vec4(result, 1.0);
    } else {
        // 不生成云彩，直接使用大气散射颜色
        color = vec4(retColor, 1.0);
    }
}