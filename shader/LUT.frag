#version 460

        in vec2 v_texCoord;
        out vec4 frag_color;

        // 添加time uniform变量
        uniform float time;

        const float PI = 3.14159265358979323846;

        uniform sampler2D transmittanceLut;
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

        // 函数声明
        void UvToTransmittanceLutParams(float bottomRadius, float topRadius, vec2 uv, out float mu, out float r);
        vec2 GetTransmittanceLutUv(float bottomRadius, float topRadius, float mu, float r);
        vec3 TransmittanceToAtmosphere(in AtmosphereParameter param, vec3 p, vec3 dir, sampler2D lut);
        vec3 RayleighCoefficient(in AtmosphereParameter param, float h);
        float RayleiPhase(in AtmosphereParameter param, float cos_theta);
        vec3 MieCoefficient(in AtmosphereParameter param, float h);
        float MiePhase(in AtmosphereParameter param, float cos_theta);
        vec3 Scattering(in AtmosphereParameter param, vec3 p, vec3 lightDir, vec3 viewDir);
        vec3 MieAbsorption(in AtmosphereParameter param, float h);
        vec3 OzoneAbsorption(in AtmosphereParameter param, float h);
        float RayIntersectSphere(vec3 center, float radius, vec3 rayStart, vec3 rayDir);
        vec3 Transmittance(in AtmosphereParameter param, vec3 p1, vec3 p2);

        // 函数定义
        void UvToTransmittanceLutParams(float bottomRadius, float topRadius, vec2 uv, out float mu, out float r)
        {
            float x_mu = uv.x;
            float x_r = uv.y;

            float H = sqrt(max(0.0f, topRadius * topRadius - bottomRadius * bottomRadius));
            float rho = H * x_r;
            r = sqrt(max(0.0f, rho * rho + bottomRadius * bottomRadius));

            float d_min = topRadius - r;
            float d_max = rho + H;
            float d = d_min + x_mu * (d_max - d_min);
            mu = d == 0.0f ? 1.0f : (H * H - rho * rho - d * d) / (2.0f * r * d);
            mu = clamp(mu, -1.0f, 1.0f);
        }

vec2 GetTransmittanceLutUv(float bottomRadius, float topRadius, float mu, float r)
{
    r = clamp(r, bottomRadius, topRadius);
    mu = clamp(mu, -1.0, 1.0);
    float H = sqrt(max(0.0f, topRadius * topRadius - bottomRadius * bottomRadius));
    float rho = sqrt(max(0.0f, r * r - bottomRadius * bottomRadius));

    float discriminant = r * r * (mu * mu - 1.0f) + topRadius * topRadius;
	float d = max(0.0f, (-r * mu + sqrt(discriminant)));

    float d_min = topRadius - r;
    float d_max = rho + H;

    float x_mu = (d - d_min) / (d_max - d_min);
    float x_r = rho / H;

    float invWidth = 1.0 / float(256);
    float invHeight = 1.0 / float(64);

    x_mu=clamp(x_mu, invWidth, 1.0 - invWidth);
    x_r=clamp(x_r, invHeight, 1.0 - invHeight);
    return vec2(x_mu, x_r);
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

vec3 IntegralMultiScattering(
             in AtmosphereParameter param, vec3 samplePoint, vec3 lightDir,
                        sampler2D transmittanceLut)
        {
    const int N_DIRECTION = 64;
    const int N_SAMPLE = 32;
    vec3 RandomSphereSamples[64] = {
        vec3(-0.7838,-0.620933,0.00996137),
        vec3(0.106751,0.965982,0.235549),
        vec3(-0.215177,-0.687115,-0.693954),
        vec3(0.318002,0.0640084,-0.945927),
        vec3(0.357396,0.555673,0.750664),
        vec3(0.866397,-0.19756,0.458613),
        vec3(0.130216,0.232736,-0.963783),
        vec3(-0.00174431,0.376657,0.926351),
        vec3(0.663478,0.704806,-0.251089),
        vec3(0.0327851,0.110534,-0.993331),
        vec3(0.0561973,0.0234288,0.998145),
        vec3(0.0905264,-0.169771,0.981317),
        vec3(0.26694,0.95222,-0.148393),
        vec3(-0.812874,-0.559051,-0.163393),
        vec3(-0.323378,-0.25855,-0.910263),
        vec3(-0.1333,0.591356,-0.795317),
        vec3(0.480876,0.408711,0.775702),
        vec3(-0.332263,-0.533895,-0.777533),
        vec3(-0.0392473,-0.704457,-0.708661),
        vec3(0.427015,0.239811,0.871865),
        vec3(-0.416624,-0.563856,0.713085),
        vec3(0.12793,0.334479,-0.933679),
        vec3(-0.0343373,-0.160593,-0.986423),
        vec3(0.580614,0.0692947,0.811225),
        vec3(-0.459187,0.43944,0.772036),
        vec3(0.215474,-0.539436,-0.81399),
        vec3(-0.378969,-0.31988,-0.868366),
        vec3(-0.279978,-0.0109692,0.959944),
        vec3(0.692547,0.690058,0.210234),
        vec3(0.53227,-0.123044,-0.837585),
        vec3(-0.772313,-0.283334,-0.568555),
        vec3(-0.0311218,0.995988,-0.0838977),
        vec3(-0.366931,-0.276531,-0.888196),
        vec3(0.488778,0.367878,-0.791051),
        vec3(-0.885561,-0.453445,0.100842),
        vec3(0.71656,0.443635,0.538265),
        vec3(0.645383,-0.152576,-0.748466),
        vec3(-0.171259,0.91907,0.354939),
        vec3(-0.0031122,0.9457,0.325026),
        vec3(0.731503,0.623089,-0.276881),
        vec3(-0.91466,0.186904,0.358419),
        vec3(0.15595,0.828193,-0.538309),
        vec3(0.175396,0.584732,0.792038),
        vec3(-0.0838381,-0.943461,0.320707),
        vec3(0.305876,0.727604,0.614029),
        vec3(0.754642,-0.197903,-0.62558),
        vec3(0.217255,-0.0177771,-0.975953),
        vec3(0.140412,-0.844826,0.516287),
        vec3(-0.549042,0.574859,-0.606705),
        vec3(0.570057,0.17459,0.802841),
        vec3(-0.0330304,0.775077,0.631003),
        vec3(-0.938091,0.138937,0.317304),
        vec3(0.483197,-0.726405,-0.48873),
        vec3(0.485263,0.52926,0.695991),
        vec3(0.224189,0.742282,-0.631472),
        vec3(-0.322429,0.662214,-0.676396),
        vec3(0.625577,-0.12711,0.769738),
        vec3(-0.714032,-0.584461,-0.385439),
        vec3(-0.0652053,-0.892579,-0.446151),
        vec3(0.408421,-0.912487,0.0236566),
        vec3(0.0900381,0.319983,0.943135),
        vec3(-0.708553,0.483646,0.513847),
        vec3(0.803855,-0.0902273,0.587942),
        vec3(-0.0555802,-0.374602,-0.925519),
    };
    const float uniform_phase = 1.0 / (4.0 * PI);
    const float sphereSolidAngle = 4.0 * PI / float(N_DIRECTION);
    
    vec3 G_2 = vec3(0, 0, 0);
    vec3 f_ms = vec3(0, 0, 0);

    for(int i=0; i<N_DIRECTION; i++)
    {
        // 光线和大气层求交
        vec3 viewDir = RandomSphereSamples[i];
        float dis = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, samplePoint, viewDir);
        float d = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius, samplePoint, viewDir);
        if(d > 0) dis = min(dis, d);
        float ds = dis / float(N_SAMPLE);

        vec3 p = samplePoint + (viewDir * ds) * 0.5;
        vec3 opticalDepth = vec3(0, 0, 0);

        for(int j=0; j<N_SAMPLE; j++)
        {
            float h = length(p) - param.PlanetRadius;
            vec3 sigma_s = RayleighCoefficient(param, h) + MieCoefficient(param, h);  // scattering
            vec3 sigma_a = OzoneAbsorption(param, h) + MieAbsorption(param, h);       // absorption
            vec3 sigma_t = sigma_s + sigma_a;                                         // extinction
            opticalDepth += sigma_t * ds;

            vec3 t1 = TransmittanceToAtmosphere(param, p, lightDir, transmittanceLut);
            vec3 s  = Scattering(param, p, lightDir, viewDir);
            vec3 t2 = exp(-opticalDepth);
            
            // 用 1.0 代替太阳光颜色, 该变量在后续的计算中乘上去
            G_2  += t1 * s * t2 * uniform_phase * ds *50;  
            f_ms += t2 * sigma_s * uniform_phase * ds ;

            p += viewDir * ds;
        }
    }

    G_2 *= sphereSolidAngle;
    f_ms *= sphereSolidAngle;
    return G_2 * (1.0 / (1.0 - f_ms));
}


        void main()
        {
            // 直接在着色器中写死大气参数值
            AtmosphereParameter param;
            param.SeaLevel = 0.0;
            param.PlanetRadius = 6360000;
            param.AtmosphereHeight = 60000.0;
            param.SunLightIntensity = 31.4;
            param.SunLightColor = vec3(1.0, 1.0, 1.0);
            param.SunDiskAngle = 1;
            param.RayleighScatteringScale = 1.0;
            param.RayleighScatteringScalarHeight = 8000.0;
            param.MieScatteringScale = 1.0;
            param.MieAnisotropy = 0.8;
            param.MieScatteringScalarHeight = 1200;
            param.OzoneAbsorptionScale = 1.0;
            param.OzoneLevelCenterHeight = 25000.0;
            param.OzoneLevelWidth = 15000.0;
            
            vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
            vec2 uv = v_texCoord;

            // // 多次计算天空颜色
            float mu_s = uv.x * 2.0 - 1.0;
            float r = uv.y * param.AtmosphereHeight + param.PlanetRadius;

            float cos_theta = mu_s;
            float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
            vec3 lightDir = vec3(sin_theta, cos_theta, 0);
            vec3 p = vec3(0, r, 0);

            color.rgb = IntegralMultiScattering(param, p, lightDir, transmittanceLut);


            //color = texture(transmittanceLut, uv);

            //单次计算
            // float bottomRadius = param.PlanetRadius;
            // float topRadius = param.PlanetRadius + param.AtmosphereHeight;
            // float cos_theta = 0.0;
            // float r = 0.0;
            // UvToTransmittanceLutParams(bottomRadius, topRadius, uv, cos_theta, r);

            // float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
            // vec3 viewDir = vec3(sin_theta, cos_theta, 0);
            // vec3 eyePos = vec3(0, r, 0);

            // float dis = RayIntersectSphere(vec3(0,0,0), param.PlanetRadius + param.AtmosphereHeight, eyePos, viewDir);
            // vec3 hitPoint = eyePos + viewDir * dis;

            // color.rgb = Transmittance(param, eyePos, hitPoint);

            frag_color = vec4(color);
        }