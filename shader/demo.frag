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
float DistanceToTopAtmosphereBoundary(IN(AtmosphereParameters) atmosphere, float r, float mu) 
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

    scattering = scattering * shadow_transmittance;
    single_mie_scattering = single_mie_scattering * shadow_transmittance;
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
const float kSphereRadius = 1000.0 / kLengthUnitInMeters;
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

// ============ 局部云盒参数 ============

// 使用地心坐标系定义云盒位置

// 云盒中心位置（地心坐标系，单位：km）
const vec3 kCloudBoxCenter = vec3(0.0, 0.0, 6364.0);  // 地表上方4km

// 云盒尺寸（单位：km）
const vec3 kCloudBoxSize = vec3(500.0, 500.0, 6.0);  // 50x50x6 km的云盒

// 云的外观参数
const vec3 kCloudAlbedo = vec3(0.95, 0.95, 0.95);
const float kCloudExtinction = 0.08;

// 云纹理采样器
uniform sampler2D cloudTexture;      // 2D云纹理图（天气图）
uniform sampler2D cloudShapeTexture; // 主体形状2D噪声图
uniform sampler2D blueNoiseTexture;  // 蓝噪声纹理，用于消除云渲染分层


// ============ 云密度函数 ============
float GetCloudDensity(vec3 point_earth_space) {
    // 1. 将点转换到云盒局部坐标系
    vec3 local_pos = point_earth_space - kCloudBoxCenter;
    
    // 2. 检查是否在盒子内
    vec3 half_size = kCloudBoxSize * 0.5;
    if (abs(local_pos.x) > half_size.x || 
        abs(local_pos.y) > half_size.y || 
        abs(local_pos.z) > half_size.z) {
        return 0.0;  // 在盒子外，密度为0
    }
    
    // 3. 计算UV坐标用于采样天气图
    vec2 weatherUV = local_pos.xz / kCloudBoxSize.xz + 0.5;  // 转换到[0,1]范围
    // 增加天气图的缩放，使云分布更加分散
    weatherUV *= 0.5;  // 缩放UV坐标，使天气图的特征更大，云朵更分散
    
    // 4. 采样天气图控制云的分布
    vec4 weatherMap = texture2D(cloudTexture, weatherUV);
    
    // 5. 采样形状纹理生成云的细节形状 - 多重采样
    // 使用更复杂的UV坐标组合来增强细节
    vec2 shapeUV1 = local_pos.xy * 0.0025;
    vec2 shapeUV2 = local_pos.yz * 0.0025;
    vec2 shapeUV3 = (local_pos.xz + local_pos.xy * 0.5) * 0.0025;
    
    // 多重采样不同坐标的纹理
    float shape1 = texture2D(cloudShapeTexture, shapeUV1).x;
    float shape2 = texture2D(cloudShapeTexture, shapeUV2).x;
    float shape3 = texture2D(cloudShapeTexture, shapeUV3).x;
    
    // 混合不同坐标的采样结果
    float shape = (shape1 * 0.5 + shape2 * 0.5 + shape3 * 0.2);
    
    // 增加更多的细节层次
    shape += texture2D(cloudShapeTexture, shapeUV1 * 2.0).x * 0.25;
    shape += texture2D(cloudShapeTexture, shapeUV1 * 4.0).x * 0.125;
    shape += texture2D(cloudShapeTexture, shapeUV1 * 8.0).x * 0.0625;


    
    
    // 6. 边缘软化（让云边缘柔和过渡）
    vec3 edge_factor = 1.0 - abs(local_pos) / half_size;  // 0(边缘) -> 1(中心)
    float edge_fade = min(min(edge_factor.x, edge_factor.y), edge_factor.z);
    edge_fade = smoothstep(0.0, 0.05, edge_fade);  // 进一步减少边缘软化区域，使中心更实
    
    // 7. 高度衰减（底部密集，顶部稀疏）
    float height_fraction = (local_pos.z + half_size.z) / kCloudBoxSize.z;  // 0(底部) -> 1(顶部)
    // 调整高度衰减参数，使云的中部更密集
    float height_gradient = smoothstep(0.0, 0.15, height_fraction) *  // 底部渐入
                           smoothstep(1.0, 0.85, height_fraction);    // 顶部渐出，保留更多中部区域
    
    // 8. 组合所有因素
    // 分别控制天气图和形状纹理的影响
    float weatherInfluence = weatherMap.r;  // 天气图控制云的分布
    float shapeInfluence = shape;  // 形状纹理控制云的细节
    
    // 调整天气图的影响，使云分布更加分散但中心更实
    weatherInfluence = pow(weatherInfluence, 0.3);  // 适度降低天气图的对比度
    weatherInfluence *= 0.7;  // 适度降低整体密度
    
    // 增强形状纹理的影响
    shapeInfluence = pow(shapeInfluence, 0.75);  // 增强形状纹理对比度
    
    float density = weatherInfluence * shapeInfluence * 2.0;  // 适度调整整体密度
    density *= height_gradient;  // 应用高度衰减
    density *= edge_fade;        // 应用边缘软化
    
    // 9. 增加对比度使云朵更清晰
    density = pow(density, 2.2);
    
    // 10. 设置密度阈值，过滤掉太小的密度值
    if (density < 0.1) {  // 提高阈值，过滤掉更小的云朵
        density = 0.0;
    }
    
    return clamp(density, 0.0, 1.0);

}

// ============ 射线与云盒相交测试（轴对齐包围盒AABB） ============
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

// ============ 云层渲染主函数 ============
void RenderCloudBox(vec3 view_direction, inout vec3 radiance) {
    // 第1步：相交测试（完全对应球体代码的相交测试）
    vec3 camera_earth_space = camera - earth_center;
    float t_min, t_max;
    
    if (!RayIntersectCloudBox(camera_earth_space, view_direction, t_min, t_max)) {
        return;  // 未击中云盒，直接返回（对应球体的 discriminant < 0.0）
    }
    
    // 获取蓝噪声值用于步进偏移
    vec2 screenUV = gl_FragCoord.xy / iResolution;
    vec2 blueNoiseUV = screenUV * 8.0; // 调整蓝噪声纹理的缩放
    float blueNoise = texture2D(blueNoiseTexture, blueNoiseUV).r;
    
    // 第2步：光线步进（对应球体的单点着色，但这里是区间积分）
    const int STEPS = 64;  // 使用较低的步进次数，通过蓝噪声消除分层
    float step_size = (t_max - t_min) / float(STEPS);
    
    vec3 cloud_light = vec3(0.0);  // 初始化为黑色
    float cloud_transmittance = 1.0;
    
    for (int i = 0; i < STEPS; i++) {
        // 提前退出优化（云已经完全不透明）
        if (cloud_transmittance < 0.01) break;
        
        // 使用蓝噪声对步进起始点做偏移
        float t = t_min + (float(i) + blueNoise) * step_size;
        vec3 sample_pos = camera_earth_space + view_direction * t;
        
        // 2.2 采样云密度（对应球体的隐式密度=1.0）
        float density = GetCloudDensity(sample_pos);
        
        if (density > 0.01) {  // 降低阈值使更多细节可见
            // 2.3 简单的白色云渲染（无光照）
            vec3 point_light = vec3(1.0); // 纯白色
            
            // 2.4 Beer-Lambert衰减定律（体积渲染核心）
            float optical_depth = density * step_size * kCloudExtinction;
            float absorption = exp(-optical_depth);
            
            // 2.5 累积光照和透明度
            // 使用更平滑的累积方式
            vec3 contribution = point_light * (1.0 - absorption) * cloud_transmittance;
            cloud_light += contribution;
            cloud_transmittance *= absorption;
        }
    }
    
    // 第3步：混合到场景
    radiance = mix(radiance, cloud_light, 1.0 - cloud_transmittance);
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



  RenderCloudBox(view_direction, radiance);
  radiance = mix(radiance, sphere_radiance, sphere_alpha);

  color.rgb = 
      pow(vec3(1.0) - exp(-radiance / white_point * exposure), vec3(1.0 / 2.2));
  color.a = 1.0;
}
