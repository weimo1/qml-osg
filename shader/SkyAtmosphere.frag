#version 330
in vec3 vWorldPosition;
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;
in vec3 vCameraPosition;

out vec4 color;

uniform float mieDirectionalG;
uniform vec3 up;
uniform vec3 cameraPosition;

// 云层uniforms
uniform float cloudDensity;
uniform float cloudHeight;
uniform float cloudBaseHeight;
uniform sampler2D cloudMap;  // 云图纹理采样器
uniform sampler2D blueNoise;   // 蓝噪声纹理采样器

// constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

const float n = 1.0003;
const float N = 2.545E25;

// optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
// 66 arc seconds -> degrees, and the cosine of that
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
// 1.0 / ( 4.0 * pi )
const float ONE_OVER_FOURPI = 0.07957747154594767;

// 云层参数 - 基于统一球体
const float cloudMinHeight = 10.0;  // 云层最小高度
const float cloudMaxHeight = 200.0;  // 云层最大高度

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

// 采样云图纹理
float sampleCloudMap(vec2 coord) {
    // 使用fract确保纹理坐标在[0,1]范围内
    vec2 clampedCoord = fract(coord);
    
    // 使用线性插值采样以减少锯齿
    return texture2D(cloudMap, clampedCoord).x;
}

// 分形布朗运动噪声 - 使用纹理
float fbm(vec3 p) {
    vec2 coord1 = p.xz * 0.0025;  // 调整缩放因子以控制云的大小
    vec2 coord2 = p.xy * 0.0025;  // 第二组坐标用于多样性
    
    // 增强各层噪声的权重，使云层更浓密
    float noise = clamp(sampleCloudMap(coord1), 0.0, 1.0) * 0.5;       // 增强权重
    noise += clamp(sampleCloudMap(coord1 * 2.0), 0.0, 1.0) * 0.3;     // 增强权重
    noise += clamp(sampleCloudMap(coord1 * 4.0), 0.0, 1.0) * 0.15;    // 增强权重
    noise += clamp(sampleCloudMap(coord2), 0.0, 1.0) * 0.08;         // 使用不同的坐标
    noise += clamp(sampleCloudMap(coord2 * 2.0), 0.0, 1.0) * 0.04;  // 使用不同的坐标
    noise += clamp(sampleCloudMap(coord2 * 4.0), 0.0, 1.0) * 0.02; // 使用不同的坐标
    
    return noise;
}

// 计算 pos 点的云密度
float getCloudDensity(vec3 pos) {
    // 使用世界坐标x,z来采样噪声，确定哪些地方有云
    vec3 cloudCoord = pos * 0.001;  // 调整缩放因子以控制云的大小
    
    // 生成噪声值
    float noiseValue = fbm(cloudCoord);
    
    // 高度衰减 - 云层中部密度最大
    float mid = (cloudMinHeight + cloudMaxHeight) / 200000.0;
    float h = cloudMaxHeight - cloudMinHeight;
    float heightWeight = 1.0 - 2.0 * abs(mid - pos.y) / h;
    heightWeight = pow(max(heightWeight, 0.0), 0.5);
    
    // 应用高度权重
    noiseValue *= heightWeight;
    
    // 应用厚度阈值，低于该阈值的都映射到0
    float cloudThickness = 0.2;  // 降低阈值
    noiseValue = clamp((noiseValue - cloudThickness) / (1.0 - cloudThickness), 0.0, 1.0);
    
    return noiseValue * cloudDensity * 0.0005;  // 增加密度系数
}

// 获取云层颜色（使用简单的alpha混合方式）
float getCloudAlpha(vec3 direction) {
    // 旋转90度，使云层铺在天空上（绕X轴旋转90度）
    vec3 rotatedDirection = vec3(direction.x, direction.z, -direction.y);
    
    // 从相机位置沿视线方向进行ray marching
    vec3 rayOrigin = vCameraPosition;
    vec3 rayDir = rotatedDirection;
    
    // 设置云层的边界
    float cloudBottom = cloudMinHeight;
    float cloudTop = cloudMaxHeight;
    
    // 计算与云层的交点
    float tBottom = (cloudBottom - rayOrigin.y) / rayDir.y;
    float tTop = (cloudTop - rayOrigin.y) / rayDir.y;
    
    // 确保tBottom是进入点，tTop是离开点
    float tMin = min(tBottom, tTop);
    float tMax = max(tBottom, tTop);
    
    // 如果光线与云层不相交，则没有云
    if (tMax < 0.0 || tMin > 100000.0) {
        return 0.0;  // 完全透明
    }
    
    // 确保进入点在相机前方
    tMin = max(tMin, 0.0);
    
    // 计算步长和步数
    float stepLength = 50.0;
    int maxSteps = 300;
    
    // 初始化累积值
    float alpha = 0.0;  // 不透明度
    
    // 计算屏幕UV坐标用于蓝噪声采样
    vec2 screenUV = gl_FragCoord.xy / vec2(1920.0, 1080.0);  // 假设屏幕分辨率为1920x1080
    
    // 采样蓝噪声纹理
    float blueNoiseValue = texture2D(blueNoise, screenUV).r;
    
    // 使用蓝噪声对步进起始点做偏移，解决分层问题
    vec3 point = rayOrigin + rayDir * (tMin + stepLength * blueNoiseValue);
    
    // 从进入点开始ray marching
    for(int i = 0; i < maxSteps; i++) {
        float t = tMin + float(i) * stepLength;
        // 应用蓝噪声偏移
        t += stepLength * blueNoiseValue * 0.5;
        vec3 point = rayOrigin + rayDir * t;
        
        // 检查是否超出云层范围
        if(point.y < cloudBottom || point.y > cloudTop || t > tMax) {
            break;
        }
        
        // 采样云密度
        float density = getCloudDensity(point);
        
        // 增加密度以获得更明显的云效果
        density *= 1.0;
        
        // 累积不透明度
        alpha += density * stepLength * 0.02;
        
        // 限制不透明度在合理范围内
        alpha = min(alpha, 1.0);
        
        // 如果不透明度接近1，可以提前退出循环
        if(alpha >= 0.99) {
            alpha = 1.0;
            break;
        }
    }
    
    return alpha;
}

// 计算大气散射光照
vec3 calculateAtmosphericLight(vec3 direction, vec3 sunDir) {
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
    float theta = acos(direction.y);
    float phi = atan(direction.z, direction.x);
    vec2 uv = vec2(phi, theta) / vec2(2.0 * pi, pi) + vec2(0.5, 0.0);
    vec3 L0 = vec3(0.1) * Fex;

    // composition + solar disc
    float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (vSunE * 19000.0 * Fex) * sundisk;

    vec3 texColor = (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);

    vec3 retColor = pow(texColor, vec3(1.0 / (1.2 + (1.2 * vSunfade))));

    return retColor;
}

void main() 
{
    // 归一化世界坐标，使其落在单位球上
    vec3 direction = normalize(vWorldPosition);
    vec3 sunDir = vSunDirection;

    // 计算大气散射颜色 - 使用归一化方向
    vec3 skyColor = calculateAtmosphericLight(direction, sunDir);
    
    // 获取云层不透明度 - 使用归一化方向
    float cloudAlpha = getCloudAlpha(direction);
    
    // 使用简单的alpha混合公式: finalColor = (1 - cloudAlpha) * skyColor + cloudAlpha * whiteColor
    vec3 whiteColor = vec3(1.0, 1.0, 1.0);  // 纯白色云层
    vec3 finalColor = (1.0 - cloudAlpha) * skyColor + cloudAlpha * whiteColor;
    
    // 确保颜色不会全黑
    finalColor = max(finalColor, vec3(0.02));
    
    color = vec4(finalColor, 1.0);
}