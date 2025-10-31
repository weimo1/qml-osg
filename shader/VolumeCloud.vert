#version 330
layout(location = 0) in vec3 aPos;

out vec3 vWorldPosition;
out vec3 vSunDirection;
out float vSunfade;
out vec3 vBetaR;
out vec3 vBetaM;
out float vSunE;

uniform vec3 sunPosition;
uniform float rayleigh;
uniform float turbidity;
uniform float mieCoefficient;
uniform vec3 up;
uniform float sunZenithAngle;
uniform float sunAzimuthAngle;
uniform float cloudDensity;
uniform float cloudHeight;

// 噪声纹理和时间变量
uniform sampler2D iChannel0;
uniform float iTime;

uniform mat4 viewInverse;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;

const float pi = 3.141592653589793238462643383279502884197169;
const float e = 2.718281828459045;

const vec3 lambda = vec3(680E-9, 550E-9, 450E-9);
const vec3 totalRayleigh = vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5);

const float v = 4.0;
const vec3 K = vec3(0.686, 0.678, 0.666);
const vec3 MieConst = vec3(1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14);

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

    gl_Position = osg_ProjectionMatrix * osg_ModelViewMatrix * vec4(aPos, 1.0);
    gl_Position.z = gl_Position.w;

    vec3 computedSunDirection = vec3(
        sin(sunZenithAngle) * cos(sunAzimuthAngle),
        sin(sunZenithAngle) * sin(sunAzimuthAngle),
        cos(sunZenithAngle)
    );
    
    vSunDirection = normalize(computedSunDirection);
    vSunE = sunIntensity(dot(vSunDirection, up));
    vSunfade = 1.0 - clamp(1.0 - exp((computedSunDirection.z / 450000.0)), 0.0, 1.0);

    float rayleighCoefficient = rayleigh - (1.0 * (1.0 - vSunfade));
    vBetaR = totalRayleigh * rayleighCoefficient;
    vBetaM = totalMie(turbidity) * mieCoefficient;
}