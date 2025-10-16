#define _USE_MATH_DEFINES  // 启用数学常量定义（在Windows平台上需要）
#include "shaderpbr.h"
#include "shadercube.h"  // 添加ShaderCube头文件
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec3>
#include <osg/Uniform>
#include <osg/StateSet>
#include <osg/Program>
#include <osg/Shader>
#include <osg/StateAttribute>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Group>
#include <osg/Material>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osg/Depth>
#include <osg/TexEnv>
#include <cmath>

// 创建带PBR效果的球体阵列
osg::Node* ShaderPBR::createPBRSphere(float radius)
{
    // 创建球体阵列的根节点
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // 创建2D网格的球体阵列 (5x5)
    int rows = 5;
    int cols = 5;
    
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // 调整金属度和粗糙度的范围，避免极端值
            float metallic = (float)row / (float)(rows - 1) * 0.9f + 0.05f;  // 范围 0.05-0.95
            float roughness = (float)col / (float)(cols - 1) * 0.9f + 0.05f; // 范围 0.05-0.95
            
            // 使用不同的颜色来更好地展示金属度
            osg::Vec3 baseColor;
            if (metallic < 0.5f) {
                // 非金属：使用彩色
                baseColor = osg::Vec3(0.8f, 0.2f, 0.2f); // 红色
            } else {
                // 金属：使用白色/灰色
                baseColor = osg::Vec3(1.0f, 0.86f, 0.57f); // 金色
            }
            
            // 创建单个球体
            osg::ref_ptr<osg::Node> sphere = ShaderPBR::createSingleSphere(radius);
            
            // 获取状态集
            osg::StateSet* stateset = sphere->getOrCreateStateSet();
            
            // 设置uniform变量
            stateset->addUniform(new osg::Uniform("albedo", baseColor));
            stateset->addUniform(new osg::Uniform("metallic", metallic));
            stateset->addUniform(new osg::Uniform("roughness", roughness));
            stateset->addUniform(new osg::Uniform("ao", 1.0f));  // 完全环境光遮蔽
            
            // 设置多个光源位置 - 适中的光源
            osg::ref_ptr<osg::Uniform> lightPositions = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "lightPositions", 4);
            lightPositions->setElement(0, osg::Vec3(5.0f, 5.0f, 5.0f));
            lightPositions->setElement(1, osg::Vec3(-5.0f, 5.0f, 5.0f));
            lightPositions->setElement(2, osg::Vec3(5.0f, -5.0f, 5.0f));
            lightPositions->setElement(3, osg::Vec3(-5.0f, -5.0f, 5.0f));
            stateset->addUniform(lightPositions);
            
            // 设置多个光源颜色 - 适中的光源强度
            osg::ref_ptr<osg::Uniform> lightColors = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "lightColors", 4);
            lightColors->setElement(0, osg::Vec3(500.0f, 500.0f, 500.0f)); // 增加光源强度
            lightColors->setElement(1, osg::Vec3(500.0f, 500.0f, 500.0f));
            lightColors->setElement(2, osg::Vec3(500.0f, 500.0f, 500.0f));
            lightColors->setElement(3, osg::Vec3(500.0f, 500.0f, 500.0f));
            stateset->addUniform(lightColors);
            
            // 设置相机位置（统一设置）
            stateset->addUniform(new osg::Uniform("camPos", osg::Vec3(0.0f, -10.0f, 2.0f)));
            
            // 创建并应用简化版IBL着色器程序
            osg::ref_ptr<osg::Program> program = ShaderPBR::createPBRShaderSimpleIBL();
            stateset->setAttributeAndModes(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            
            // 添加天空盒纹理采样器uniform（纹理单元0）
            stateset->addUniform(new osg::Uniform("skybox", 0));
            
            // 设置球体位置
            osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
            float x = (col - (cols - 1) / 2.0f) * 3.0f;  // 增加间距
            float y = ((rows - 1) / 2.0f - row) * 3.0f;  // 增加间距
            transform->setMatrix(osg::Matrix::translate(x, y, 0.0f));
            transform->addChild(sphere);
            
            root->addChild(transform);
        }
    }
    
    return root.release();
}

// 创建带天空盒的PBR场景（修复版）
osg::Node* ShaderPBR::createPBRSceneWithSkybox(float sphereRadius)
{
    // 创建场景根节点
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    // ⚠️ 步骤1: 先创建天空盒，提取CubeMap纹理
    osg::ref_ptr<osg::Node> skyboxNode = ShaderPBR::createSkybox("E:/qt test/qml+osg/resource");
    
    // ⚠️ 步骤2: 从天空盒中提取TextureCubeMap
    osg::TextureCubeMap* skyboxTexture = nullptr;
    
    // 天空盒现在包裹在Transform里，需要深入查找
    if (skyboxNode.valid()) {
        // 遍历天空盒节点树，找到Geode
        class FindGeodeVisitor : public osg::NodeVisitor {
        public:
            osg::Geode* foundGeode;
            
            FindGeodeVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), foundGeode(nullptr) {}
            
            virtual void apply(osg::Geode& geode) {
                if (!foundGeode) {
                    foundGeode = &geode;
                }
                traverse(geode);
            }
        };
        
        FindGeodeVisitor visitor;
        skyboxNode->accept(visitor);
        
        if (visitor.foundGeode) {
            osg::StateSet* skyboxStateSet = visitor.foundGeode->getStateSet();
            if (skyboxStateSet) {
                skyboxTexture = dynamic_cast<osg::TextureCubeMap*>(
                    skyboxStateSet->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            }
        }
    }
    
    if (!skyboxTexture) {
        std::cout << "Error: Failed to extract skybox texture!" << std::endl;
        return root.release();
    }
    
    // ⚠️ 步骤3: 创建PBR球体并传递天空盒纹理
    osg::ref_ptr<osg::Node> pbrSpheres = ShaderPBR::createPBRSphereWithSkyboxTexture(
        sphereRadius, skyboxTexture);
    
    // ⚠️ 步骤4: 添加到场景（顺序很重要！）
    root->addChild(skyboxNode);      // 先添加天空盒（背景）
    root->addChild(pbrSpheres);      // 再添加球体（前景）
    
    return root.release();
}

// 新函数：创建带天空盒纹理的PBR球体
osg::Node* ShaderPBR::createPBRSphereWithSkyboxTexture(float radius, osg::TextureCubeMap* skyboxTexture)
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    
    int rows = 5;
    int cols = 5;
    
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // 调整金属度和粗糙度的范围，避免极端值
            float metallic = (float)row / (float)(rows - 1) * 0.9f + 0.05f;  // 范围 0.05-0.95
            float roughness = (float)col / (float)(cols - 1) * 0.9f + 0.05f; // 范围 0.05-0.95
            
            // 使用对比色
            osg::Vec3 baseColor;
            if (metallic < 0.5f) {
                baseColor = osg::Vec3(0.8f, 0.2f, 0.2f); // 红色非金属
            } else {
                baseColor = osg::Vec3(1.0f, 0.86f, 0.57f); // 金色金属
            }
            
            osg::ref_ptr<osg::Node> sphere = ShaderPBR::createSingleSphere(radius);
            osg::StateSet* stateset = sphere->getOrCreateStateSet();
            
            // 材质参数
            stateset->addUniform(new osg::Uniform("albedo", baseColor));
            stateset->addUniform(new osg::Uniform("metallic", metallic));
            stateset->addUniform(new osg::Uniform("roughness", roughness));
            stateset->addUniform(new osg::Uniform("ao", 1.0f));
            
            // 调整点光源位置，使其更容易观察到光照效果
            osg::ref_ptr<osg::Uniform> lightPositions = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "lightPositions", 4);
            // 将光源位置调整得更靠近球体阵列中心
            lightPositions->setElement(0, osg::Vec3(5.0f, 5.0f, 5.0f));
            lightPositions->setElement(1, osg::Vec3(-5.0f, 5.0f, 5.0f));
            lightPositions->setElement(2, osg::Vec3(5.0f, -5.0f, 5.0f));
            lightPositions->setElement(3, osg::Vec3(-5.0f, -5.0f, 5.0f));
            stateset->addUniform(lightPositions);
            
            // 增加光源强度，使其更明显
            osg::ref_ptr<osg::Uniform> lightColors = new osg::Uniform(osg::Uniform::FLOAT_VEC3, "lightColors", 4);
            for(int i = 0; i < 4; i++) {
                lightColors->setElement(i, osg::Vec3(500.0f, 500.0f, 500.0f)); // 增加光源强度
            }
            stateset->addUniform(lightColors);
            
            // 调整相机位置，使其更适合观察球体阵列
            stateset->addUniform(new osg::Uniform("camPos", osg::Vec3(0.0f, -10.0f, 2.0f)));
            
            // ⚠️ 关键：设置天空盒纹理到纹理单元1
            if (skyboxTexture) {
                stateset->setTextureAttributeAndModes(1, skyboxTexture, osg::StateAttribute::ON);
                stateset->addUniform(new osg::Uniform("skybox", 1));
            }
            
            // 应用PBR着色器
            osg::ref_ptr<osg::Program> program = ShaderPBR::createPBRShaderSimpleIBL();
            stateset->setAttributeAndModes(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            
            // 设置位置
            osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
            float x = (col - (cols - 1) / 2.0f) * 3.0f;
            float y = ((rows - 1) / 2.0f - row) * 3.0f;
            transform->setMatrix(osg::Matrix::translate(x, y, 0.0f));
            transform->addChild(sphere);
            
            root->addChild(transform);
        }
    }
    
    return root.release();
}

// 创建天空盒（使用ShaderCube实现）
osg::Node* ShaderPBR::createSkybox(const std::string& resourcePath)
{
    // 直接使用ShaderCube的实现
    return ShaderCube::createSkyBox(resourcePath);
}

// 创建单个球体
osg::Node* ShaderPBR::createSingleSphere(float radius)
{
    // 创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 球体参数 - 适中的细分度
    int segments = 30;
    int rings = 30;
    const double PI = 3.14159265358979323846;  // 直接定义PI常量
    
    // 生成球体顶点和法线
    for (int r = 0; r <= rings; ++r) {
        float v = (float)r / (float)rings;
        float phi = v * static_cast<float>(PI);  // 使用定义的PI常量
        
        for (int s = 0; s <= segments; ++s) {
            float u = (float)s / (float)segments;
            float theta = u * 2.0f * static_cast<float>(PI);  // 使用定义的PI常量
            
            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);
            
            // 顶点位置
            vertices->push_back(osg::Vec3(x * radius, y * radius, z * radius));
            // 法线方向（单位向量）
            float length = sqrt(x*x + y*y + z*z);
            if (length > 0) {
                normals->push_back(osg::Vec3(x/length, y/length, z/length));
            } else {
                normals->push_back(osg::Vec3(0, 1, 0));  // 默认法线
            }
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
    
    // 设置顶点属性数组
    geometry->setVertexAttribArray(0, vertices, osg::Array::BIND_PER_VERTEX);  // 位置属性
    geometry->setVertexAttribArray(1, normals, osg::Array::BIND_PER_VERTEX);   // 法线属性
    
    // 创建索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            int first = (r * (segments + 1)) + s;
            int second = first + segments + 1;
            
            indices->push_back(first);
            indices->push_back(second);
            indices->push_back(first + 1);
            
            indices->push_back(second);
            indices->push_back(second + 1);
            indices->push_back(first + 1);
        }
    }
    
    geometry->addPrimitiveSet(indices);
    
    // 创建节点
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    
    return geode.release();
}

// 简化版PBR着色器（带增强IBL）
osg::Program* ShaderPBR::createPBRShaderSimpleIBL()
{
    // 顶点着色器
    static const char* vertCode = R"(#version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        
        uniform mat4 osg_ModelViewProjectionMatrix;
        uniform mat4 osg_ModelViewMatrix;
        uniform mat3 osg_NormalMatrix;
        
        out vec3 WorldPos;
        out vec3 Normal;
        
        void main()
        {
            WorldPos = vec3(osg_ModelViewMatrix * vec4(aPos, 1.0));
            Normal = normalize(osg_NormalMatrix * aNormal);
            gl_Position = osg_ModelViewProjectionMatrix * vec4(aPos, 1.0);
        }
    )";
    
    // 片段着色器 - 简化版IBL
    static const char* fragCode = R"(#version 330 core
        out vec4 FragColor;
        in vec3 WorldPos;
        in vec3 Normal;
        
        uniform vec3 albedo;
        uniform float metallic;
        uniform float roughness;
        uniform float ao;
        uniform vec3 lightPositions[4];
        uniform vec3 lightColors[4];
        uniform vec3 camPos;
        
        // 天空盒纹理采样器
        uniform samplerCube skybox;
        
        const float PI = 3.14159265359;
        
        // PBR函数
        float DistributionGGX(vec3 N, vec3 H, float roughness)
        {
            float a = roughness*roughness;
            float a2 = a*a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH*NdotH;
            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;
            return nom / max(denom, 0.0001);
        }
        
        float GeometrySchlickGGX(float NdotV, float roughness)
        {
            float r = (roughness + 1.0);
            float k = (r*r) / 8.0;
            float nom = NdotV;
            float denom = NdotV * (1.0 - k) + k;
            return nom / max(denom, 0.0001);
        }
        
        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
        {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            float ggx2 = GeometrySchlickGGX(NdotV, roughness);
            float ggx1 = GeometrySchlickGGX(NdotL, roughness);
            return ggx1 * ggx2;
        }
        
        vec3 fresnelSchlick(float cosTheta, vec3 F0)
        {
            return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }
        
        vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
        {
            return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }
        
        // 从天空盒采样环境颜色
        vec3 sampleEnvironment(vec3 dir)
        {
            return texture(skybox, dir).rgb;
        }
        
        void main()
        {
            vec3 N = normalize(Normal);
            vec3 V = normalize(camPos - WorldPos);
            vec3 R = reflect(-V, N);
            
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);
            
            // ============ 直接光照 ============
            vec3 Lo = vec3(0.0);
            for(int i = 0; i < 4; ++i)
            {
                vec3 L = normalize(lightPositions[i] - WorldPos);
                vec3 H = normalize(V + L);
                float distance = length(lightPositions[i] - WorldPos);
                float attenuation = 1.0 / (distance * distance);
                vec3 radiance = lightColors[i] * attenuation;
                
                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
                
                vec3 kS = F;
                vec3 kD = vec3(1.0) - kS;
                kD *= 1.0 - metallic;
                
                vec3 numerator = NDF * G * F;
                float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
                vec3 specular = numerator / denominator;
                
                float NdotL = max(dot(N, L), 0.0);
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }
            
            // ============ IBL环境光 ============
            vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
            vec3 kS = F;
            vec3 kD = 1.0 - kS;
            kD *= 1.0 - metallic;
            
            // 漫反射IBL（采样法线方向）
            vec3 irradiance = sampleEnvironment(N);
            vec3 diffuse = irradiance * albedo;
            
            // 镜面反射IBL（采样反射方向）
            vec3 prefilteredColor = sampleEnvironment(R);
            
            // 简化的BRDF积分
            float NdotV = max(dot(N, V), 0.0);
            vec2 envBRDF = vec2(
                mix(1.0, 0.0, roughness),  // x: 菲涅尔比例
                roughness * 0.5             // y: 粗糙度贡献
            );
            vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
            
            vec3 ambient = (kD * diffuse + specular) * ao;
            
            // ============ 最终颜色 ============  
            vec3 color = ambient + Lo;
            
            // 增强对比度，但避免过度饱和
            color = color / (color + vec3(1.0));
            color = pow(color, vec3(1.0/2.2));
            
            FragColor = vec4(color, 1.0);
        }
    )";
    
    // 编译shader
    osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX, vertCode);
    osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT, fragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(vertShader);
    program->addShader(fragShader);
    
    return program.release();
}

// 改进版PBR着色器 - 添加简化的IBL支持
osg::Program* ShaderPBR::createPBRShaderProgramWithIBL()
{
    // 这个函数保持为空，因为我们使用了简化版的着色器
    return nullptr;
}

// 创建PBR着色器程序
osg::Program* ShaderPBR::createPBRShaderProgram()
{
    // 这个函数保持为空，因为我们使用了简化版的着色器
    return nullptr;
}