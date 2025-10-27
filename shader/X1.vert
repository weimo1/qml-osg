#version 330
layout(location = 0) in vec3 aPos;

uniform vec3 sunPosition;
uniform float rayleigh;
uniform float turbidity;
uniform float mieCoefficient;
uniform vec3 up;
uniform float sunZenithAngle;  // 添加太阳天顶角度uniform
uniform float sunAzimuthAngle;  // 添加太阳方位角度uniform

uniform mat4 viewInverse;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;

out vec3 vWorldPosition;
out vec3 vSunDirection;
out float vSunfade;
out vec3 vBetaR;
out vec3 vBetaM;
out float vSunE;




// constants for atmospheric scattering
const float e = 2.71828182845904523536028747135266249775724709369995957;
const float pi = 3.141592653589793238462643383279502884197169;

// wavelength of used primaries, according to preetham
const vec3 lambda = vec3(680E-9, 550E-9, 450E-9);
// this pre-calculation replaces older TotalRayleigh(vec3 lambda) function:
// (8.0 * pow(pi, 3.0) * pow(pow(n, 2.0) - 1.0, 2.0) * (6.0 + 3.0 * pn)) / (3.0 * N * pow(lambda, vec3(4.0)) * (6.0 - 7.0 * pn))
const vec3 totalRayleigh = vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5);

// mie stuff
// K coefficient for the primaries
const float v = 4.0;
const vec3 K = vec3(0.686, 0.678, 0.666);
// MieConst = pi * pow( ( 2.0 * pi ) / lambda, vec3( v - 2.0 ) ) * K
const vec3 MieConst = vec3(1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14);

// earth shadow hack
// cutoffAngle = pi / 1.95;
const float cutoffAngle = 1.6110731556870734;
const float steepness = 1.5;
const float EE = 1000.0;

float sunIntensity(float zenithAngleCos) {
    zenithAngleCos = clamp(zenithAngleCos, -1.0, 1.0);
    return EE * max(0.0, 1.0 - pow(e, -( ( cutoffAngle - acos(zenithAngleCos) ) / steepness )));
}

vec3 totalMie(float T){
    float c = (0.2 * T) * 10E-18;
    return 0.434 * c * MieConst;
}

void main() 
{
    mat4 modelMatrix = viewInverse * osg_ModelViewMatrix;

    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    vWorldPosition = worldPosition.xyz;

    // 处理投影和视图矩阵以解决坐标系问题
    gl_Position = osg_ProjectionMatrix * osg_ModelViewMatrix * vec4(aPos, 1.0);
    
    // 翻转Y轴以匹配Qt坐标系
    gl_Position.y = -gl_Position.y;
    
    gl_Position.z = gl_Position.w; // set z to camera.far

    // 根据太阳天顶角度和方位角计算太阳方向
    vec3 computedSunDirection = vec3(
        sin(sunZenithAngle) * cos(sunAzimuthAngle),
        sin(sunZenithAngle) * sin(sunAzimuthAngle),
        cos(sunZenithAngle)
    );
    
    // 使用计算出的太阳方向
    vSunDirection = normalize(computedSunDirection);

    vSunE = sunIntensity(dot(vSunDirection, up));

    vSunfade = 1.0 - clamp(1.0 - exp((computedSunDirection.z / 450000.0)), 0.0, 1.0);

    float rayleighCoefficient = rayleigh - (1.0 * (1.0 - vSunfade));

    // extinction (absorption + out scattering)
    // rayleigh coefficients
    vBetaR = totalRayleigh * rayleighCoefficient;

    // mie coefficients
    vBetaM = totalMie(turbidity) * mieCoefficient;
    
    // 调整坐标系以匹配OpenGL/OSG的约定
    // 在OpenGL中，Y轴向上为正，但可能需要根据具体实现调整
    vWorldPosition.y = -vWorldPosition.y;
    vSunDirection.y = -vSunDirection.y;
}