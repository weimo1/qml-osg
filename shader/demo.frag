#version 330

uniform float exposure;
uniform vec3 camera;

uniform vec3 white_point;
uniform vec3 earth_center;
uniform vec3 sun_direction;
uniform vec2 sun_size;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;
uniform sampler2D groundTexture;  // 地球表面模型，原始材质

// 体积云纹理
uniform sampler3D _ShapeNoiceTex;      // 3D基础形状纹理（包含Perlin和Worley噪声）
uniform sampler3D _DetailNoiceTex;     // 3D细节纹理（高频Worley噪声）
uniform sampler2D _WeatherNoiceTex;    // 2D天气纹理（控制云的覆盖率等属性）
uniform sampler2D blueNoiseTexture;    // 蓝噪声纹理，用于消除云渲染分层


// 时间
uniform float time;
// 屏幕分辨率
uniform vec2 iResolution;

in vec3 view_ray;
in vec2 auv;
out vec4 color;


#define IN(x) const in x
#define OUT(x) out x

#define assert(x)

const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

const int SCATTERING_TEXTURE_R_SIZE = 32;

const int SCATTERING_TEXTURE_MU_SIZE = 128;
const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
const int SCATTERING_TEXTURE_NU_SIZE = 8;

const int IRRADIANCE_TEXTURE_WIDTH = 64;
const int IRRADIANCE_TEXTURE_HEIGHT = 16;

#define COMBINED_SCATTERING_TEXTURES

const float m  = 1.0;
const float nm = 1.0;
const float rad = 1.0;
const float sr  = 1.0;
const float watt = 1.0;
const float lm = 1.0;
const float PI = 3.14159265358979323846;
const float km = 1000.0 * m;
const float m2 = m * m;
const float m3 = m * m * m;
const float pi = PI * rad;
const float deg = pi / 180.0;

const float watt_per_square_meter = watt / m2;
const float watt_per_square_meter_per_sr = watt / (m2 * sr);
const float watt_per_square_meter_per_nm = watt / (m2 * nm);
const float watt_per_square_meter_per_sr_per_nm =  watt / (m2 * sr * nm);
const float watt_per_cubic_meter_per_sr_per_nm = watt / (m3 * sr * nm);

const float cd  = lm / sr;
const float kcd = 1000.0 * cd;
const float cd_per_square_meter = cd / m2;
const float kcd_per_square_meter = kcd / m2;

struct DensityProfileLayer {
  float width;
  float exp_term;
  float exp_scale;
  float linear_term;
  float constant_term;
};

struct DensityProfile {
  DensityProfileLayer layers[2];
};

struct AtmosphereParameters {
  vec3 solar_irradiance;
  float sun_angular_radius;
  float bottom_radius;
  float top_radius;
  DensityProfile rayleigh_density;
  vec3 rayleigh_scattering;
  DensityProfile mie_density;
  vec3 mie_scattering;
  vec3 mie_extinction;
  float mie_phase_function_g;
  DensityProfile absorption_density;
  vec3 absorption_extinction;
  vec3 ground_albedo;
  float mu_s_min;
};

const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(
vec3(1.474000,1.850400,1.911980),
0.004675,
6360.000000,
6420.000000,
DensityProfile(DensityProfileLayer[2](DensityProfileLayer(0.000000,0.000000,0.000000,0.000000,0.000000),DensityProfileLayer(0.000000,1.000000,-0.125000,0.000000,0.000000))),
vec3(0.005802,0.013558,0.033100),
DensityProfile(DensityProfileLayer[2](DensityProfileLayer(0.000000,0.000000,0.000000,0.000000,0.000000),DensityProfileLayer(0.000000,1.000000,-0.833333,0.000000,0.000000))),
vec3(0.003996,0.003996,0.003996),
vec3(0.004440,0.004440,0.004440),
0.800000,
DensityProfile(DensityProfileLayer[2](DensityProfileLayer(25.000000,0.000000,0.000000,0.066667,-0.666667),DensityProfileLayer(0.000000,0.000000,0.000000,-0.066667,2.666667))),
vec3(0.000650,0.001881,0.000085),
vec3(0.100000,0.100000,0.100000),
-0.207912);


const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(114974.916437,71305.954816,65310.548555);
const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(98242.786222,69954.398112,66475.012354);

float ClampCosine(float mu) {
  return clamp(mu, float(-1.0), float(1.0));
}

float ClampDistance(float d) {
  return max(d, 0.0 * m);
}

float ClampRadius(IN(AtmosphereParameters) atmosphere, float r) 
{
  return clamp(r, atmosphere.bottom_radius, atmosphere.top_radius);
}

float SafeSqrt(float a) {
  return sqrt(max(a, 0.0 * m2));
}

///////////r p点的高程， mu cos sita角， 视线方向和竖直线的
float DistanceToTopAtmosphereBoundary(IN(AtmosphereParameters) atmosphere,  float r, float mu) 
{
  assert(r <= atmosphere.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);

  float discriminant = r * r * (mu * mu - 1.0) + atmosphere.top_radius * atmosphere.top_radius;
  return ClampDistance(-r * mu + SafeSqrt(discriminant));
}


bool RayIntersectsGround(IN(AtmosphereParameters) atmosphere, float r, float mu) 
{
  assert(r >= atmosphere.bottom_radius);
  assert(mu >= -1.0 && mu <= 1.0);

  return mu < 0.0 && r * r * (mu * mu - 1.0) + atmosphere.bottom_radius * atmosphere.bottom_radius >= 0.0 * m2;
}

float GetLayerDensity(IN(DensityProfileLayer) layer, float altitude) 
{
  float density = layer.exp_term * exp(layer.exp_scale * altitude) + layer.linear_term * altitude + layer.constant_term;

  return clamp(density, float(0.0), float(1.0));
}

float GetProfileDensity(IN(DensityProfile) profile, float altitude)
{
  return altitude < profile.layers[0].width ?
      GetLayerDensity(profile.layers[0], altitude) :
      GetLayerDensity(profile.layers[1], altitude);
}

 


float GetTextureCoordFromUnitRange(float x, int texture_size)
{
  return 0.5 / float(texture_size) + x * (1.0 - 1.0 / float(texture_size));
}


float GetUnitRangeFromTextureCoord(float u, int texture_size)
{
  return (u - 0.5 / float(texture_size)) / (1.0 - 1.0 / float(texture_size));
}

vec2 GetTransmittanceTextureUvFromRMu(IN(AtmosphereParameters) atmosphere,  float r, float mu) 
{
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);

  float H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);

  float rho = SafeSqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);

  float d = DistanceToTopAtmosphereBoundary(atmosphere, r, mu);
  float d_min = atmosphere.top_radius - r;
  float d_max = rho + H;
  float x_mu = (d - d_min) / (d_max - d_min);
  float x_r = rho / H;

  return vec2(GetTextureCoordFromUnitRange(x_mu, TRANSMITTANCE_TEXTURE_WIDTH),
              GetTextureCoordFromUnitRange(x_r, TRANSMITTANCE_TEXTURE_HEIGHT));
}

void GetRMuFromTransmittanceTextureUv(IN(AtmosphereParameters) atmosphere, IN(vec2) uv, OUT(float) r, OUT(float) mu)
{
  assert(uv.x >= 0.0 && uv.x <= 1.0);
  assert(uv.y >= 0.0 && uv.y <= 1.0);

  float x_mu = GetUnitRangeFromTextureCoord(uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
  float x_r = GetUnitRangeFromTextureCoord(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);

  float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -  atmosphere.bottom_radius * atmosphere.bottom_radius);
  float rho = H * x_r;
  r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);
  float d_min = atmosphere.top_radius - r;
  float d_max = rho + H;
  float d = d_min + x_mu * (d_max - d_min);

  mu = d == 0.0 * m ? float(1.0) : (H * H - rho * rho - d * d) / (2.0 * r * d);
  mu = ClampCosine(mu);
}


vec3 GetTransmittanceToTopAtmosphereBoundary(IN(AtmosphereParameters) atmosphere,IN(sampler2D) transmittance_texture,float r, float mu) 
{
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  vec2 uv = GetTransmittanceTextureUvFromRMu(atmosphere, r, mu);
  return vec3(texture(transmittance_texture, uv));
}

vec3 GetTransmittance(IN(AtmosphereParameters) atmosphere,IN(sampler2D) transmittance_texture,float r, float mu, float d, bool ray_r_mu_intersects_ground) 
{
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  assert(d >= 0.0 * m);

  float r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
  float mu_d = ClampCosine((r * mu + d) / r_d);

  if (ray_r_mu_intersects_ground) 
  {
    return min(
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r_d, -mu_d) /
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r, -mu),
        vec3(1.0));
  } 
  else
  {
    return min(
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r, mu) /
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r_d, mu_d),
        vec3(1.0));
  }
}

vec3 GetTransmittanceToSun(IN(AtmosphereParameters) atmosphere,IN(sampler2D) transmittance_texture, float r, float mu_s) 
{
  float sin_theta_h = atmosphere.bottom_radius / r;
  float cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));

  return GetTransmittanceToTopAtmosphereBoundary(
          atmosphere, transmittance_texture, r, mu_s) *
      smoothstep(-sin_theta_h * atmosphere.sun_angular_radius / rad,
                 sin_theta_h * atmosphere.sun_angular_radius / rad,
                 mu_s - cos_theta_h);
}

  

float RayleighPhaseFunction(float nu)
{
  float k = 3.0 / (16.0 * PI * sr);
  return k * (1.0 + nu * nu);
}

float MiePhaseFunction(float g, float nu)
{
  float k = 3.0 / (8.0 * PI * sr) * (1.0 - g * g) / (2.0 + g * g);
  return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

vec4 GetScatteringTextureUvwzFromRMuMuSNu(IN(AtmosphereParameters) atmosphere, float r, float mu, float mu_s, float nu, bool ray_r_mu_intersects_ground) 
{
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  assert(mu_s >= -1.0 && mu_s <= 1.0);
  assert(nu >= -1.0 && nu <= 1.0);

  float H = sqrt(atmosphere.top_radius * atmosphere.top_radius -  atmosphere.bottom_radius * atmosphere.bottom_radius);
  float rho = SafeSqrt(r * r - atmosphere.bottom_radius * atmosphere.bottom_radius);

  float u_r = GetTextureCoordFromUnitRange(rho / H, SCATTERING_TEXTURE_R_SIZE);
  float r_mu = r * mu;
  float discriminant = r_mu * r_mu - r * r + atmosphere.bottom_radius * atmosphere.bottom_radius;
  float u_mu;

  if (ray_r_mu_intersects_ground) 
  {
    float d = -r_mu - SafeSqrt(discriminant);
    float d_min = r - atmosphere.bottom_radius;
    float d_max = rho;
    u_mu = 0.5 - 0.5 * GetTextureCoordFromUnitRange(d_max == d_min ? 0.0 :
        (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
  } 
  else 
  {
    float d = -r_mu + SafeSqrt(discriminant + H * H);
    float d_min = atmosphere.top_radius - r;
    float d_max = rho + H;
    u_mu = 0.5 + 0.5 * GetTextureCoordFromUnitRange(
        (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
  }

  float d = DistanceToTopAtmosphereBoundary(atmosphere, atmosphere.bottom_radius, mu_s);

  float d_min = atmosphere.top_radius - atmosphere.bottom_radius;
  float d_max = H;
  float a = (d - d_min) / (d_max - d_min);
  float D = DistanceToTopAtmosphereBoundary(atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
  float A = (D - d_min) / (d_max - d_min);
  float u_mu_s = GetTextureCoordFromUnitRange(max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);
  float u_nu = (nu + 1.0) / 2.0;
  return vec4(u_nu, u_mu_s, u_mu, u_r);
}


void GetRMuMuSNuFromScatteringTextureUvwz(IN(AtmosphereParameters) atmosphere,IN(vec4) uvwz, OUT(float) r, OUT(float) mu, OUT(float) mu_s,OUT(float) nu, OUT(bool) ray_r_mu_intersects_ground) 
{
  assert(uvwz.x >= 0.0 && uvwz.x <= 1.0);
  assert(uvwz.y >= 0.0 && uvwz.y <= 1.0);
  assert(uvwz.z >= 0.0 && uvwz.z <= 1.0);
  assert(uvwz.w >= 0.0 && uvwz.w <= 1.0);

  float H = sqrt(atmosphere.top_radius * atmosphere.top_radius - atmosphere.bottom_radius * atmosphere.bottom_radius);
  float rho = H * GetUnitRangeFromTextureCoord(uvwz.w, SCATTERING_TEXTURE_R_SIZE);
  r = sqrt(rho * rho + atmosphere.bottom_radius * atmosphere.bottom_radius);

  if (uvwz.z < 0.5)
  {
    float d_min = r - atmosphere.bottom_radius;
    float d_max = rho;
    float d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(1.0 - 2.0 * uvwz.z, SCATTERING_TEXTURE_MU_SIZE / 2);
    mu = d == 0.0 * m ? float(-1.0) : ClampCosine(-(rho * rho + d * d) / (2.0 * r * d));
    ray_r_mu_intersects_ground = true;
  } 
  else 
  {
    float d_min = atmosphere.top_radius - r;
    float d_max = rho + H;
    float d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(2.0 * uvwz.z - 1.0, SCATTERING_TEXTURE_MU_SIZE / 2);
    mu = d == 0.0 * m ? float(1.0) : ClampCosine((H * H - rho * rho - d * d) / (2.0 * r * d));
    ray_r_mu_intersects_ground = false;
  }

  float x_mu_s = GetUnitRangeFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
  float d_min = atmosphere.top_radius - atmosphere.bottom_radius;
  float d_max = H;

  float D = DistanceToTopAtmosphereBoundary(atmosphere, atmosphere.bottom_radius, atmosphere.mu_s_min);
  float A = (D - d_min) / (d_max - d_min);

  float a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
  float d = d_min + min(a, A) * (d_max - d_min);
  mu_s = d == 0.0 * m ? float(1.0) :
     ClampCosine((H * H - d * d) / (2.0 * atmosphere.bottom_radius * d));
  nu = ClampCosine(uvwz.x * 2.0 - 1.0);
}

void GetRMuMuSNuFromScatteringTextureFragCoord(
    IN(AtmosphereParameters) atmosphere, IN(vec3) frag_coord,
    OUT(float) r, OUT(float) mu, OUT(float) mu_s, OUT(float) nu,
    OUT(bool) ray_r_mu_intersects_ground)
{
  const vec4 SCATTERING_TEXTURE_SIZE = vec4(
      SCATTERING_TEXTURE_NU_SIZE - 1,
      SCATTERING_TEXTURE_MU_S_SIZE,
      SCATTERING_TEXTURE_MU_SIZE,
      SCATTERING_TEXTURE_R_SIZE);

  float frag_coord_nu =  floor(frag_coord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));
  float frag_coord_mu_s = mod(frag_coord.x, float(SCATTERING_TEXTURE_MU_S_SIZE));

  vec4 uvwz = vec4(frag_coord_nu, frag_coord_mu_s, frag_coord.y, frag_coord.z) /SCATTERING_TEXTURE_SIZE;

  GetRMuMuSNuFromScatteringTextureUvwz(atmosphere, uvwz, r, mu, mu_s, nu, ray_r_mu_intersects_ground);

  nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)), mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}


vec3 GetScattering(IN(AtmosphereParameters) atmosphere, IN(sampler3D) scattering_texture, float r, float mu, float mu_s, float nu, bool ray_r_mu_intersects_ground) 
{
  vec4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(
      atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);

  float tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
  float tex_x = floor(tex_coord_x);
  float lerp = tex_coord_x - tex_x;

  vec3 uvw0 = vec3((tex_x + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);

  return vec3(texture(scattering_texture, uvw0) * (1.0 - lerp) +  texture(scattering_texture, uvw1) * lerp);
}

vec3 GetScattering(
    IN(AtmosphereParameters) atmosphere,
    IN(sampler3D) single_rayleigh_scattering_texture,
    IN(sampler3D) single_mie_scattering_texture,
    IN(sampler3D) multiple_scattering_texture,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground,
    int scattering_order) 
{
  if (scattering_order == 1)
  {
    vec3 rayleigh = GetScattering(atmosphere, single_rayleigh_scattering_texture, r, mu, mu_s, nu,ray_r_mu_intersects_ground);
    vec3 mie = GetScattering(atmosphere, single_mie_scattering_texture, r, mu, mu_s, nu,ray_r_mu_intersects_ground);

  return rayleigh * RayleighPhaseFunction(nu) +mie * MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
  }
  else 
  {
   return GetScattering(atmosphere, multiple_scattering_texture, r, mu, mu_s, nu,ray_r_mu_intersects_ground);
  }
}

vec3 GetIrradiance(IN(AtmosphereParameters) atmosphere, IN(sampler2D) irradiance_texture,float r, float mu_s);

  

vec2 GetIrradianceTextureUvFromRMuS(IN(AtmosphereParameters) atmosphere, float r, float mu_s) 
{
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  assert(mu_s >= -1.0 && mu_s <= 1.0);

  float x_r = (r - atmosphere.bottom_radius) / (atmosphere.top_radius - atmosphere.bottom_radius);
  float x_mu_s = mu_s * 0.5 + 0.5;
  return vec2(GetTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH), GetTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
}


const vec2 IRRADIANCE_TEXTURE_SIZE =  vec2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

  

vec3 GetIrradiance(IN(AtmosphereParameters) atmosphere, IN(sampler2D) irradiance_texture, float r, float mu_s) 
{
  vec2 uv = GetIrradianceTextureUvFromRMuS(atmosphere, r, mu_s);
  return vec3(texture(irradiance_texture, uv));
}

 vec3 GetExtrapolatedSingleMieScattering(IN(AtmosphereParameters) atmosphere, IN(vec4) scattering) 
{
  if (scattering.r <= 0.0) 
  {
    return vec3(0.0);
  }

  return scattering.rgb * scattering.a / scattering.r * (atmosphere.rayleigh_scattering.r / atmosphere.mie_scattering.r) *  (atmosphere.mie_scattering / atmosphere.rayleigh_scattering);
}

vec3 GetCombinedScattering(
    IN(AtmosphereParameters) atmosphere,
    IN(sampler3D) scattering_texture,
    IN(sampler3D) single_mie_scattering_texture,
    float r, float mu, float mu_s, float nu,
    bool ray_r_mu_intersects_ground,
    OUT(vec3) single_mie_scattering) 
{
  vec4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);

  float tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
  float tex_x = floor(tex_coord_x);
  float lerp = tex_coord_x - tex_x;

  vec3 uvw0 = vec3((tex_x + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);


  vec4 combined_scattering =    texture(scattering_texture, uvw0) * (1.0 - lerp) +   texture(scattering_texture, uvw1) * lerp;

  vec3 scattering = vec3(combined_scattering);
  single_mie_scattering = GetExtrapolatedSingleMieScattering(atmosphere, combined_scattering);

  return scattering;
}

vec3 GetSkyRadiance(
    IN(AtmosphereParameters) atmosphere,
    IN(sampler2D) transmittance_texture,
    IN(sampler3D) scattering_texture,
    IN(sampler3D) single_mie_scattering_texture,
    vec3 camera,
  IN(vec3) view_ray,
  float shadow_length,
    IN(vec3) sun_direction,
  OUT(vec3) transmittance) 
{
  float r = length(camera);
  float rmu = dot(camera, view_ray);
  float distance_to_top_atmosphere_boundary = -rmu -  sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);

  if (distance_to_top_atmosphere_boundary > 0.0 * m) 
  {
    camera = camera + view_ray * distance_to_top_atmosphere_boundary;
    r = atmosphere.top_radius;
    rmu += distance_to_top_atmosphere_boundary;
  } 
  else if (r > atmosphere.top_radius) 
  {
    transmittance = vec3(1.0);
    return vec3(0.0 * watt_per_square_meter_per_sr_per_nm);
  }

  float mu = rmu / r;
  float mu_s = dot(camera, sun_direction) / r;
  float nu = dot(view_ray, sun_direction);
  bool ray_r_mu_intersects_ground = RayIntersectsGround(atmosphere, r, mu);

  transmittance = ray_r_mu_intersects_ground ? vec3(0.0) : GetTransmittanceToTopAtmosphereBoundary(atmosphere, transmittance_texture, r, mu);
  vec3 single_mie_scattering;
  vec3 scattering;

  if (shadow_length == 0.0 * m) 
  {
    scattering = GetCombinedScattering(
        atmosphere, scattering_texture, single_mie_scattering_texture,
        r, mu, mu_s, nu, ray_r_mu_intersects_ground,
        single_mie_scattering);
  }
  else 
  {
    float d = shadow_length;
    float r_p =  ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_p = (r * mu + d) / r_p;
    float mu_s_p = (r * mu_s + d * nu) / r_p;
    scattering = GetCombinedScattering(atmosphere, scattering_texture, single_mie_scattering_texture, r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,single_mie_scattering);

   vec3 shadow_transmittance = GetTransmittance(atmosphere, transmittance_texture,r, mu, shadow_length, ray_r_mu_intersects_ground);

   scattering = scattering * shadow_transmittance;   single_mie_scattering = single_mie_scattering * shadow_transmittance;
  }
  return scattering * RayleighPhaseFunction(nu) + single_mie_scattering *  MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
}

vec3 GetSkyRadianceToPoint(
    IN(AtmosphereParameters) atmosphere,
    IN(sampler2D) transmittance_texture,
    IN(sampler3D) scattering_texture,
    IN(sampler3D) single_mie_scattering_texture,
    vec3 camera, 
  IN(vec3) point,
  float shadow_length,
    IN(vec3) sun_direction,
  OUT(vec3) transmittance) 
{
  vec3 view_ray = normalize(point - camera);
  float r = length(camera);
  float rmu = dot(camera, view_ray);
  float distance_to_top_atmosphere_boundary = -rmu - sqrt(rmu * rmu - r * r + atmosphere.top_radius * atmosphere.top_radius);

  if (distance_to_top_atmosphere_boundary > 0.0 * m) 
  {
    camera = camera + view_ray * distance_to_top_atmosphere_boundary;
    r = atmosphere.top_radius;
    rmu += distance_to_top_atmosphere_boundary;
  }

  float mu = rmu / r;
  float mu_s = dot(camera, sun_direction) / r;
  float nu = dot(view_ray, sun_direction);
  float d = length(point - camera);
  bool ray_r_mu_intersects_ground = RayIntersectsGround(atmosphere, r, mu);

  transmittance = GetTransmittance(atmosphere, transmittance_texture, r, mu, d, ray_r_mu_intersects_ground);
  vec3 single_mie_scattering;

  vec3 scattering = GetCombinedScattering(atmosphere, scattering_texture, single_mie_scattering_texture,r, mu, mu_s, nu, ray_r_mu_intersects_ground,single_mie_scattering);
  d = max(d - shadow_length, 0.0 * m);
  float r_p = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
  float mu_p = (r * mu + d) / r_p;
  float mu_s_p = (r * mu_s + d * nu) / r_p;
  vec3 single_mie_scattering_p;

  vec3 scattering_p = GetCombinedScattering(
      atmosphere, scattering_texture, single_mie_scattering_texture,
      r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
      single_mie_scattering_p);

  vec3 shadow_transmittance = transmittance;
  if (shadow_length > 0.0 * m) 
  {
    shadow_transmittance = GetTransmittance(atmosphere, transmittance_texture, r, mu, d, ray_r_mu_intersects_ground);
  }

  scattering = scattering - shadow_transmittance * scattering_p;
  single_mie_scattering =  single_mie_scattering - shadow_transmittance * single_mie_scattering_p;

   single_mie_scattering = GetExtrapolatedSingleMieScattering( atmosphere, vec4(scattering, single_mie_scattering.r));
   single_mie_scattering = single_mie_scattering *  smoothstep(float(0.0), float(0.01), mu_s);

  return scattering * RayleighPhaseFunction(nu) + single_mie_scattering * MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
}

vec3 GetSunAndSkyIrradiance(
    IN(AtmosphereParameters) atmosphere,
    IN(sampler2D) transmittance_texture,
    IN(sampler2D) irradiance_texture,
    IN(vec3) point, 
  IN(vec3) normal,
  IN(vec3) sun_direction,
    OUT(vec3) sky_irradiance) 
{
  float r = length(point);
  float mu_s = dot(point, sun_direction) / r;

  sky_irradiance = GetIrradiance(atmosphere, irradiance_texture, r, mu_s) *   (1.0 + dot(normal, point) / r) * 0.5;
  return atmosphere.solar_irradiance *  GetTransmittanceToSun( atmosphere, transmittance_texture, r, mu_s) * max(dot(normal, sun_direction), 0.0);
}

 vec3 GetSolarRadiance() 
{
    return ATMOSPHERE.solar_irradiance /(PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
}

vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, float shadow_length, vec3 sun_direction, out vec3 transmittance) 
{
    return GetSkyRadiance(ATMOSPHERE, transmittance_texture,scattering_texture, single_mie_scattering_texture,camera, view_ray, shadow_length, sun_direction, transmittance);
}

vec3 GetSkyRadianceToPoint( vec3 camera, vec3 point, float shadow_length, vec3 sun_direction, out vec3 transmittance) 
{
    return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture,scattering_texture, single_mie_scattering_texture,camera, point, shadow_length, sun_direction, transmittance);
}

vec3 GetSunAndSkyIrradiance(vec3 p, vec3 normal, vec3 sun_direction,out vec3 sky_irradiance) 
{
    return GetSunAndSkyIrradiance(ATMOSPHERE, transmittance_texture,irradiance_texture, p, normal, sun_direction, sky_irradiance);
}  












const float kLengthUnitInMeters = 1000.000000;
const vec3  kSphereCenter = vec3(0.0, 0.0, 1000.0) / kLengthUnitInMeters;
const float kSphereRadius = 0.0 / kLengthUnitInMeters;
const vec3 kSphereAlbedo = vec3(0.8);
const vec3 kGroundAlbedo = vec3(0.0, 0.0, 0.04);

float GetSunVisibility(vec3 point, vec3 sun_direction) 
{
  vec3 p = point - kSphereCenter;
  float p_dot_v = dot(p, sun_direction);
  float p_dot_p = dot(p, p);
  float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;

  float distance_to_intersection = -p_dot_v - sqrt(kSphereRadius * kSphereRadius - ray_sphere_center_squared_distance);

  if (distance_to_intersection > 0.0) 
  {
    float ray_sphere_distance = kSphereRadius - sqrt(ray_sphere_center_squared_distance);
   float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
   return smoothstep(1.0, 0.0, ray_sphere_angular_distance / sun_size.x);
  }
  return 1.0;
}

float GetSkyVisibility(vec3 point) 
{
  vec3 p = point - kSphereCenter;
  float p_dot_p = dot(p, p);
  return 1.0 + p.z / sqrt(p_dot_p) * kSphereRadius * kSphereRadius / p_dot_p;
}

void GetSphereShadowInOut(vec3 view_direction, vec3 sun_direction, out float d_in, out float d_out) 
{
  vec3 pos           = camera - kSphereCenter;
  float pos_dot_sun  = dot(pos, sun_direction);
  float view_dot_sun = dot(view_direction, sun_direction);

  float k = sun_size.x;
  float l = 1.0 + k * k;
  float a = 1.0 - l * view_dot_sun * view_dot_sun;
  float b = dot(pos, view_direction) - l * pos_dot_sun * view_dot_sun -   k * kSphereRadius * view_dot_sun;
  float c = dot(pos, pos) - l * pos_dot_sun * pos_dot_sun -  2.0 * k * kSphereRadius * pos_dot_sun - kSphereRadius * kSphereRadius;
  float discriminant = b * b - a * c;

  if (discriminant > 0.0)
  {
    d_in = max(0.0, (-b - sqrt(discriminant)) / a);
    d_out = (-b + sqrt(discriminant)) / a;
    float d_base = -pos_dot_sun / view_dot_sun;
    float d_apex = -(pos_dot_sun + kSphereRadius / k) / view_dot_sun;
    if (view_dot_sun > 0.0) 
  {
      d_in = max(d_in, d_apex);
      d_out = a > 0.0 ? min(d_out, d_base) : d_base;
    } 
  else
  {
      d_in = a > 0.0 ? max(d_in, d_base) : d_base;
      d_out = min(d_out, d_apex);
    }
  } 
  else 
  {
    d_in = 0.0;
    d_out = 0.0;
  }
}
// ============================================================
// 与大气散射一致的地平线雾系统
// ============================================================
const float HEIGHT_FOG_THICKNESS = 2.0 * km;    // 雾层厚度
const float HEIGHT_FOG_BASE_DENSITY = 0.3;     // 基础密度
const vec3 HEIGHT_FOG_COLOR = vec3(135, 206, 245) / 255.0; // 雾颜色
// ============================================================
// 球体相交
// ============================================================
vec2 SphereIntersection(vec3 rayStart, vec3 rayDir, vec3 sphereCenter, float sphereRadius) {
    vec3 oc = rayStart - sphereCenter;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float h = b * b - c;
    
    if (h < 0.0) {
        return vec2(-1.0, -1.0);
    } else {
        h = sqrt(h);
        return vec2(-b - h, -b + h);
    }
}

// ============================================================
// 计算雾权重
// ============================================================
float GetFogWeight(vec3 ro, vec3 rd) {
    // 只在看向地面时计算雾
    vec2 ground_hit = SphereIntersection(ro, rd, vec3(0.0), ATMOSPHERE.bottom_radius);
    if (ground_hit.x <= 0.0) {
        return 0.0;
    }
    
    float fog_top = ATMOSPHERE.bottom_radius + HEIGHT_FOG_THICKNESS;
    float camera_r = length(ro);
    
    // 计算射线穿过雾层的范围
    float t_start, t_end;
    
    if (camera_r > fog_top) {
        vec2 fog_hit = SphereIntersection(ro, rd, vec3(0.0), fog_top);
        if (fog_hit.x < 0.0) return 0.0;
        t_start = fog_hit.x;
        t_end = min(fog_hit.y, ground_hit.x);
    } else if (camera_r > ATMOSPHERE.bottom_radius) {
        t_start = 0.0;
        vec2 fog_hit = SphereIntersection(ro, rd, vec3(0.0), fog_top);
        t_end = (fog_hit.y > 0.0) ? min(fog_hit.y, ground_hit.x) : ground_hit.x;
    } else {
        return 0.0;
    }
    
    if (t_end <= t_start) return 0.0;
    
    // 光学深度积分
    const int SAMPLES = 6;
    float step = (t_end - t_start) / float(SAMPLES);
    float optical_depth = 0.0;
    
    for (int i = 0; i < SAMPLES; i++) {
        float t = t_start + (float(i) + 0.5) * step;
        vec3 pos = ro + rd * t;
        float r = length(pos);
        float altitude = r - ATMOSPHERE.bottom_radius;
        float h = clamp(altitude / HEIGHT_FOG_THICKNESS, 0.0, 1.0);
        float density = exp(-h * 4.0);
        optical_depth += density * step;
    }
    
    float extinction = optical_depth * HEIGHT_FOG_BASE_DENSITY;
    float transmittance = exp(-extinction);
    
    return clamp(1.0 - transmittance, 0.0, 0.95);
}

// ============================================================
// 地平线增强版本
// ============================================================
float GetFogWeightHorizon(vec3 ro, vec3 rd) {
    float base_fog = GetFogWeight(ro, rd);
    if (base_fog < 0.001) return 0.0;
    
    vec3 up = normalize(ro);
    float elevation = dot(rd, up);
    float horizon = 1.0 - abs(elevation);
    horizon = pow(horizon, 2.0);
    
    float final_fog = base_fog * (0.2 + 0.8 * horizon);
    return clamp(final_fog, 0.0, 0.95);
}

vec3 GetFogScatteringc(vec3 ro, vec3 rd, vec3 sun_dir, float fog_weight) {
    // 基础雾颜色
    vec3 fog_base = HEIGHT_FOG_COLOR;
    
    // 太阳光方向影响
    float sun_dot = dot(rd, sun_dir);
    float phase = 0.5 + 0.5 * max(sun_dot, 0.0);
    
    // 采样雾层中点获取大气照明
    vec2 ground_hit = SphereIntersection(ro, rd, vec3(0.0), ATMOSPHERE.bottom_radius);
    float sample_t = ground_hit.x * 0.2; // 采样靠近相机的位置
    vec3 sample_pos = ro + rd * sample_t;
    
    float r = length(sample_pos);
    float mu_s = dot(normalize(sample_pos), sun_dir);
    
    // 获取太阳透射率
    vec3 sun_transmittance = GetTransmittanceToSun(
        ATMOSPHERE,
        transmittance_texture,
        r,
        mu_s
    );
    
    // 组合光照
    vec3 ambient = vec3(0.1, 0.12, 0.15);  // 弱环境光
    vec3 sun = sun_transmittance * phase * 0.4;
    
    vec3 fog_radiance = fog_base * (ambient + sun);
    
    return fog_radiance * fog_weight;
}


// ============================================================
// 🔥 关键修改：使用大气散射颜色作为雾色
// ============================================================
vec3 GetFogScattering(vec3 ro, vec3 rd, vec3 sun_dir, float fog_weight) {
    // 1. 在雾层中采样一个代表点
    vec2 ground_hit = SphereIntersection(ro, rd, vec3(0.0), ATMOSPHERE.bottom_radius);
    
    // 采样距离：在雾层的前30%位置（靠近相机，颜色更接近天空）
    float fog_top = ATMOSPHERE.bottom_radius + HEIGHT_FOG_THICKNESS;
    vec2 fog_hit = SphereIntersection(ro, rd, vec3(0.0), fog_top);
    
    float t_start = max(0.0, fog_hit.x);
    float t_end = min(fog_hit.y, ground_hit.x);
    float sample_t = mix(t_start, t_end, 0.3); // 采样靠近相机的位置
    
    vec3 sample_pos = ro + rd * sample_t;
    
    // 2. 🔥 直接使用大气散射系统计算该点的天空颜色
    float r = length(sample_pos);
    float rmu = dot(sample_pos, rd);
    float mu = rmu / r;
    float mu_s = dot(sample_pos, sun_dir) / r;
    float nu = dot(rd, sun_dir);
    
    bool ray_intersects_ground = RayIntersectsGround(ATMOSPHERE, r, mu);
    
    // 获取大气散射颜色（这就是雾应该有的颜色）
    vec3 single_mie;
    vec3 fog_color = GetCombinedScattering(
        ATMOSPHERE,
        scattering_texture,
        single_mie_scattering_texture,
        r, mu, mu_s, nu,
        ray_intersects_ground,
        single_mie
    );
    
    // 应用相位函数
    fog_color = fog_color * RayleighPhaseFunction(nu) + 
                single_mie * MiePhaseFunction(ATMOSPHERE.mie_phase_function_g, nu);
    
    // 3. 归一化并调整强度
    // 雾的强度应该比天空稍弱，避免过曝
    fog_color *= fog_weight * 0.8;
    
    return fog_color;
}

// ============================================================
// 简化版：直接采样天空颜色
// ============================================================
vec3 GetFogScatteringSimple(vec3 ro, vec3 rd, vec3 sun_dir, float fog_weight) {
    // 在雾层中采样点
    vec2 ground_hit = SphereIntersection(ro, rd, vec3(0.0), ATMOSPHERE.bottom_radius);
    float sample_distance = ground_hit.x * 0.2; // 20%的距离处
    
    vec3 sample_pos = ro + rd * sample_distance;
    
    // 🔥 直接调用天空散射函数
    vec3 dummy_transmittance;
    vec3 fog_color = GetSkyRadiance(
        sample_pos,     // 采样点位置
        rd,             // 视线方向
        0.0,            // 无阴影
        sun_dir,        // 太阳方向
        dummy_transmittance
    );
    
    return fog_color * fog_weight;
}

// ============================================================
// 高级版本：多点采样平均（更平滑的颜色过渡）
// ============================================================
vec3 GetFogScatteringAdvanced(vec3 ro, vec3 rd, vec3 sun_dir, float fog_weight) {
    vec2 ground_hit = SphereIntersection(ro, rd, vec3(0.0), ATMOSPHERE.bottom_radius);
    if (ground_hit.x <= 0.0) return vec3(0.0);
    
    // 在雾层内采样3个点，平均它们的散射颜色
    vec3 fog_color_sum = vec3(0.0);
    const int SAMPLES = 3;
    
    for (int i = 0; i < SAMPLES; i++) {
        float t = ground_hit.x * (0.1 + 0.3 * float(i) / float(SAMPLES - 1));
        vec3 sample_pos = ro + rd * t;
        
        vec3 dummy_trans;
        vec3 sample_color = GetSkyRadiance(
            sample_pos,
            rd,
            0.0,
            sun_dir,
            dummy_trans
        );
        
        fog_color_sum += sample_color;
    }
    
    vec3 fog_color = fog_color_sum / float(SAMPLES);
    return fog_color * fog_weight;
}



// ============================================================
// 应用体积雾
// ============================================================
void ApplyVolumetricFog(vec3 view_direction, inout vec3 radiance, inout vec3 transmittance) {
    vec3 ro = camera - earth_center;
    
    float fog_weight = GetFogWeight(ro, view_direction);
    
    if (fog_weight > 0.001) {
        // 🔥 使用与大气散射一致的雾颜色
        vec3 fog_scatter = GetFogScatteringc(ro, view_direction, sun_direction, fog_weight);
        
        // 混合
        radiance = radiance * (1.0 - fog_weight) + fog_scatter;
        transmittance *= (1.0 - fog_weight);
    }
}

// ============================================================
// 调试可视化
// ============================================================
vec3 DebugFogDensity(vec3 view_direction) {
    vec3 ro = camera - earth_center;
    
    vec2 ground_hit = SphereIntersection(ro, view_direction, vec3(0.0), ATMOSPHERE.bottom_radius);
    float fog_weight = GetFogWeightHorizon(ro, view_direction);
    
    vec3 color = vec3(0.0);
    
    if (ground_hit.x > 0.0) {
        // 绿色 = 击中地面
        color.g = 0.3;
        // 红色 = 雾密度
        color.r = fog_weight;
        
        // 如果雾密度很高，显示为白色
        if (fog_weight > 0.5) {
            color = vec3(fog_weight);
        }
    } else {
        // 蓝色 = 看向天空
        color.b = 0.5;
    }
    
    return color;
}






// 前向声明云相关的函数
float GetCloudDensity(vec3 point_earth_space);
bool RayIntersectCloudBox(vec3 ray_origin, vec3 ray_dir, out float t_min, out float t_max);
void RenderCloudBox(vec3 view_direction, inout vec3 radiance);
float ComputeCloudSelfShadowing(vec3 sample_pos, vec3 sun_dir);
vec3 ComputeStepScattering(vec3 sample_pos, vec3 view_dir, vec3 sun_dir);
float HenyeyGreensteinPhase(float g, float cosTheta);
float DualLobPhase(float g0, float g1, float w, float cosTheta);
float Remap(float value, float lo, float ho, float ln, float hn);


// ============ 云的外观参数 ============
// 云的外观参数
const vec3 kCloudColor = vec3(1.0, 1.0, 1.0);         // 纯白色
const vec3 kCloudShade = vec3(0.8, 0.8, 0.8);         // 浅灰色阴影
const float kCloudExtinction = 0.8;                   // 增加消光系数

// ============ 改进的云盒参数 ============

// 云盒中心和尺寸（增加高度）
const vec3 kCloudBoxCenter = vec3(0.0, 0.0, 6361.0);  // 提高1km
const vec3 kCloudBoxSize = vec3(5, 5.0, 0.3);   // 增加到8km厚度

// ============ 云密度函数 ============

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
    vec3 camera_earth_space = camera - earth_center;
    float t_min, t_max;
    
    if (!RayIntersectCloudBox(camera_earth_space, view_direction, t_min, t_max)) {
        return;
    }
    
    const int STEPS = 256;
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
        vec3 curr_pos = camera - earth_center + view_direction * t;
        
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

void RenderCloudBox1(vec3 view_direction, inout vec3 radiance) {
    vec3 camera_earth_space = camera - earth_center;
    float t_min, t_max;
    
    // 1. 边界相交测试
    if (!RayIntersectCloudBox(camera_earth_space, view_direction, t_min, t_max)) {
        return;
    }
    
    // 🔥 初始参数调整 (您应该在主文件头部定义这些常量)
    const float INITIAL_SAMPLES = 256.0; // 基础步数 (用于计算初始步长)
    float MAX_STEP_SIZE = (t_max - t_min) / INITIAL_SAMPLES; // 约等于平均步长
    const float MIN_STEP_SCALE = 0.2; // 最小步长系数 (例如 0.2 * MAX_STEP_SIZE)
    const int MAX_STEPS = 256; // 安全阈值，防止无限循环或步数过多
    
    // 2. 蓝噪声抖动 (Jitter)
    vec2 screenUV = gl_FragCoord.xy / iResolution;
    // 使用不同的缩放和平移来获取噪声
    vec2 blueNoiseUV = screenUV * 8.0 + vec2(time * 0.1); 
    float blueNoise = texture(blueNoiseTexture, blueNoiseUV).r;
    
    vec3 cloud_color = vec3(0.0);
    float transmittance = 1.0;
    
    // 3. 初始化 Raymarch 距离
    float t = t_min;
    // 抖动起始点，范围 [0, MAX_STEP_SIZE]
    t += blueNoise * MAX_STEP_SIZE;
    
    int step_count = 0;
    
    // 🔥 4. 使用 WHILE 循环进行距离迭代 (自适应步长的核心)
    while (t < t_max && step_count < MAX_STEPS) {
        if (transmittance < 0.005) break; // 提前退出阈值稍微收紧
        
        vec3 curr_pos = camera_earth_space + view_direction * t;
        
        float density = GetCloudDensity(curr_pos);
        
        // --- 计算自适应步长 ---
        // density: [0, 1]
        // mix(MAX_STEP_SIZE, MAX_STEP_SIZE * MIN_STEP_SCALE, smoothstep(0.0, 0.8, density))
        // 密度高 (0.8+) 时，步长缩小到 MIN_STEP_SCALE (例如 0.2)
        // 密度低 (0.0) 时，使用 MAX_STEP_SIZE (加速)
        float current_step_size = mix(MAX_STEP_SIZE, MAX_STEP_SIZE * MIN_STEP_SCALE, 
                                      smoothstep(0.0, 0.8, density));
        
        // 确保不会意外增大步长
        current_step_size = min(current_step_size, MAX_STEP_SIZE * 2.0); 
        
        if (density > 0.005) {
            // 🔥 使用改进的光照计算
            vec3 step_color = ComputeStepScattering(curr_pos, view_direction, sun_direction);
            
            // 密度 * 消光系数 = Extinction Coefficient
            float extinction = density * kCloudExtinction;
            
            // Beer-Lambert 定律计算当前步的透射率
            float step_T = exp(-extinction * current_step_size);
            
            // 体积积分 (Accumulate Radiance): (In-Scattering) * Transmittance
            // (1.0 - step_T) 近似当前步的吸收/散射 (Extinction)
            cloud_color += step_color * transmittance * (1.0 - step_T);
            
            // 更新射线透射率 (Out-Scattering)
            transmittance *= step_T;
        }
        
        // 迭代到下一步
        t += current_step_size;
        step_count++;
    }
    
    // 5. 最终混合
    // radiance = 穿透云层的背景光 * 剩余透射率 + 云体自身光照
    radiance = radiance * transmittance + cloud_color;
}
// ============ 重映射函数 ============
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

void main() 
{

  vec3 view_direction = normalize(view_ray);
  float fragment_angular_size =
      length(dFdx(view_ray) + dFdy(view_ray)) / length(view_ray);
  float shadow_in;
  float shadow_out;
  GetSphereShadowInOut(view_direction, sun_direction, shadow_in, shadow_out);
  float lightshaft_fadein_hack = smoothstep(
      0.02, 0.04, dot(normalize(camera - earth_center), sun_direction));


  vec3 p = camera - kSphereCenter;
  float p_dot_v = dot(p, view_direction);
  float p_dot_p = dot(p, p);
  float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
  float discriminant =
      kSphereRadius * kSphereRadius -ray_sphere_center_squared_distance;
  float sphere_alpha = 0.0;
  vec3 sphere_radiance = vec3(0.0);
  if (discriminant >= 0.0) {
    float distance_to_intersection = -p_dot_v - sqrt(discriminant);
    if (distance_to_intersection > 0.0) {
      float ray_sphere_distance =
          kSphereRadius - sqrt(ray_sphere_center_squared_distance);
      float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
      sphere_alpha =
          min(ray_sphere_angular_distance / fragment_angular_size, 1.0);
      vec3 point = camera + view_direction * distance_to_intersection;
      vec3 normal = normalize(point - kSphereCenter);
      vec3 sky_irradiance;
      vec3 sun_irradiance = GetSunAndSkyIrradiance(
          point - earth_center, normal, sun_direction, sky_irradiance);
      sphere_radiance =
          kSphereAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);
      float shadow_length =
          max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) *
          lightshaft_fadein_hack;
      vec3 transmittance;
      vec3 in_scatter = GetSkyRadianceToPoint(camera - earth_center,
          point - earth_center, shadow_length, sun_direction, transmittance);
      sphere_radiance = sphere_radiance * transmittance + in_scatter;
    }
  }

  float shadow_length = max(0.0, shadow_out - shadow_in) *
      lightshaft_fadein_hack;
  vec3 transmittance;
  vec3 radiance = GetSkyRadiance(
      camera - earth_center, view_direction, shadow_length, sun_direction,
      transmittance);
  if (dot(view_direction, sun_direction) > sun_size.y) {
    radiance = radiance + transmittance * GetSolarRadiance();
  }

  //ApplyVolumetricFog(view_direction, radiance, transmittance);

  RenderCloudBox(view_direction, radiance);
   radiance = mix(radiance, sphere_radiance, sphere_alpha);

  // 检查纹理是否有效（使用较低的阈值）
  vec4 groundTextureColor = texture(groundTexture, auv);
  // 检查纹理是否有效（使用较低的阈值）
  if (length(groundTextureColor.rgb) > 0.01) {
    // 直接使用纹理颜色，不经过大气处理
    color.rgb = groundTextureColor.rgb;
  } else {
    // 没有纹理的地方使用原来的辐射度处理
    color.rgb = pow(vec3(1.0) - exp(-radiance / white_point * exposure), vec3(1.0 / 2.2));
  }
  color.a = 1.0;
}
