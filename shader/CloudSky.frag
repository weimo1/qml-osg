#version 330
in vec3 vWorldPosition;
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;
in vec3 cameraPosition;

out vec4 color;

uniform float mieDirectionalG;
uniform vec3 up;
uniform float sunZenithAngle;
uniform float sunAzimuthAngle;
uniform float cloudDensity;
uniform float cloudHeight;
uniform vec3 atmosphereColor;
uniform float iTime;           // 时间变量
uniform sampler2D blueNoise;   // 蓝噪声纹理

// 体积云uniforms
uniform sampler2D cloudMap;    // 云噪声纹理
uniform vec3 sunDirection;     // 太阳方向

// 新增的云层控制参数uniforms
uniform float densityThreshold;
uniform float contrast;
uniform float densityFactor;
uniform float stepSize;

// 基础颜色和光照颜色定义
#define baseBright  vec3(1.26,1.25,1.29)    // 基础颜色 -- 亮部
#define baseDark    vec3(0.31,0.31,0.32)    // 基础颜色 -- 暗部
#define lightBright vec3(1.29, 1.17, 1.05)  // 光照颜色 -- 亮部
#define lightDark   vec3(0.7,0.75,0.8)      // 光照颜色 -- 暗部

const float pi = 3.141592653589793238462643383279502884197169;
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
const float ONE_OVER_FOURPI = 0.07957747154594767;

#define bottom 130  // 云层底部
#define top 200     // 云层顶部
#define width 400    // 云层 xz 坐标范围 [-width, width]

// 计算 pos 点的云密度
float getDensity(vec3 pos) {
    // 高度衰减 - 云层中部密度最大
    float mid = (bottom + top) / 2.0;
    float h = top - bottom;
    float weight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    weight = pow(max(weight, 0.0), 0.5);  // 开根号使过渡更平滑

    vec2  coord1 = pos.xz * 0.00025;
    float noise = texture2D(cloudMap, coord1).x * 0.5;
    noise += texture2D(cloudMap, coord1 * 2.0).x * 0.25;
    noise += texture2D(cloudMap, coord1 * 4.0).x * 0.125;
    noise += texture2D(cloudMap, coord1 * 8.0).x * 0.0625;
    noise += texture2D(cloudMap, coord1 * 16.0).x * 0.03125;
    noise += texture2D(cloudMap, coord1 * 32.0).x * 0.015625;

    noise *= weight;

    // 使用uniform参数控制密度阈值
    if(noise < densityThreshold) {
        noise = 0.0;
    } else {
        // 使用uniform参数控制对比度
        noise = pow(noise, contrast);
    }

    // 使用uniform参数控制密度因子
    noise *= densityFactor;

    return noise;
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

// 获取体积云颜色
vec4 getCloud(vec3 worldPos, vec3 cameraPos, vec3 lightPos) {
    vec3 direction = normalize(worldPos - cameraPos);   // 视线射线方向
    // 使用uniform参数控制步长
    vec3 step = direction * stepSize;
    vec4 colorSum = vec4(0);        // 积累的颜色
    vec3 point = cameraPos;         // 从相机出发开始测试

    // 计算屏幕UV坐标用于蓝噪声采样
    vec2 screenUV = gl_FragCoord.xy / vec2(1920.0, 1080.0);  // 假设屏幕分辨率为1920x1080
    
    // 采样蓝噪声纹理
    float blueNoiseValue = texture2D(blueNoise, screenUV).r;

    // 如果相机在云层下，将测试起始点移动到云层底部
    if(point.y < bottom) {
        point += direction * (abs(bottom - cameraPos.y) / abs(direction.y));
    }
    // 如果相机在云层上，将测试起始点移动到云层顶部
    if(top < point.y) {
        point += direction * (abs(cameraPos.y - top) / abs(direction.y));
    }

    // 如果目标像素遮挡了云层则放弃测试
    float len1 = length(point - cameraPos);     // 云层到眼距离
    float len2 = length(worldPos - cameraPos);  // 目标像素到眼距离
    if(len2 < len1) {
        return vec4(0);
    }

    // 使用蓝噪声对步进起始点做偏移，解决分层问题
    point += step * blueNoiseValue * 0.5;

    // ray marching
    for(int i=0; i<300; i++) {  // 使用固定步数
        point += step;
        if(bottom>point.y || point.y>top || -width>point.x || point.x>width || -width>point.z || point.z>width) {
            break;
        }
        
        // 采样
        float density = getDensity(point);                // 当前点云密度
        
        // 根据距离眼睛的距离进行线性插值，使密度缓慢减为0
        float distanceToCamera = length(point - cameraPos);
        float maxDistance = 100000.0;  // 最大影响距离
        float distanceFactor = max(0.0, 1.0 - distanceToCamera / maxDistance);
        density *= distanceFactor;

        // 控制透明度
        density *= cloudDensity;  // 使用cloudDensity uniform控制密度

        // 简单的光照模型
        vec3 lightDir = normalize(sunDirection);
        vec3 normal = normalize(point - cameraPos);
        float NdotL = max(0.0, dot(normal, lightDir));
        
        // 颜色计算 - 使用基础颜色和光照
        vec3 baseColor = mix(baseBright, baseDark, density) * density * 0.7;   // 基础颜色
        vec3 litColor = baseColor * (0.5 + 0.5 * NdotL);  // 简单的光照计算

        // 混合
        vec4 color = vec4(litColor, density);                     // 当前点的最终颜色
        colorSum = color * (1.0 - colorSum.a) + colorSum;           // 与累积的颜色混合
    }

    return colorSum;
}

void main() 
{
    vec3 worldPos = vWorldPosition;
    // 使用归一化的世界坐标方向来采样天空盒
    vec3 direction = normalize(worldPos);

    // 获取体积云颜色
    vec4 cloud = getCloud(worldPos, cameraPosition, sunDirection * 1000.0);
    
    // 简单的降噪处理 - 平滑颜色
    cloud.rgb = mix(cloud.rgb, vec3(0.5), 0.1);
    
    // 使用深蓝色背景
    vec3 backgroundColor = vec3(0.1, 0.1, 0.3);
    vec3 finalColor = mix(backgroundColor, cloud.rgb, cloud.a);
    
    color = vec4(finalColor, 1.0);
}