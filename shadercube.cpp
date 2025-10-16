#include "shadercube.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec3>
#include <osg/Uniform>
#include <osg/StateSet>
#include <osg/Program>
#include <osg/Shader>
#include <osg/StateAttribute>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Options>
#include <osg/Texture2D>
#include <osg/TextureCubeMap>
#include <osg/TexEnv>
#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/Notify>
#include <iostream>
#include<osg/Vec3>
#include<osg/Vec4>
#include<osg/Quat>
#include<osg/Matrix>
#include<osg/ShapeDrawable>

#include<osg/Geode>
#include<osg/Geometry>
#include<osg/Transform>
#include<osg/Material>
#include<osg/NodeCallback>
#include<osg/Depth>
#include<osg/CullFace>
#include<osg/TexMat>
#include<osg/TexGen>
#include<osg/TexEnv>
#include<osg/TextureCubeMap>
#include<osgDB/WriteFile>
#include<osgDB/ReadFile>
#include<osgUtil/Optimizer>
#include<osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

// 天空盒回调：使天空盒始终跟随摄像机位置但不旋转
class SkyboxTransformCallback : public osg::NodeCallback
{
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* transform = dynamic_cast<osg::MatrixTransform*>(node);
        if (transform)
        {
            // 使用单位矩阵，确保天空盒不会旋转
            transform->setMatrix(osg::Matrix::identity());
        }
        traverse(node, nv);
    }
};

// 实现创建Shader程序的方法
osg::Program* ShaderCube::createShaderProgram()
{
    // 创建顶点着色器 (Blinn-Phong光照模型)
    const char* vertexShaderSource = R"(
        #version 330 core
        
        // 输入属性
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 texCoord;
        
        // 输出到片元着色器
        out vec3 fragNormal;
        out vec3 fragPosition;
        out vec2 fragTexCoord;
        
        // 使用OSG内置的统一变量
        uniform mat4 osg_ModelViewProjectionMatrix;
        uniform mat4 osg_ModelViewMatrix;
        uniform mat3 osg_NormalMatrix;
        
        void main()
        {
            // 使用OSG内置的MVP矩阵变换顶点位置
            gl_Position = osg_ModelViewProjectionMatrix * vec4(position, 1.0);
            
            // 计算世界空间中的法线和位置
            // 修复法线计算：正确使用法线矩阵变换输入法线
            fragNormal = normalize(osg_NormalMatrix * normal);
            fragPosition = vec3(osg_ModelViewMatrix * vec4(position, 1.0));
            fragTexCoord = texCoord;
        }
    )";
    
    // 创建片元着色器 (Blinn-Phong光照模型)
    const char* fragmentShaderSource = R"(
        #version 330 core
        
        // 输入变量
        in vec3 fragNormal;
        in vec3 fragPosition;
        in vec2 fragTexCoord;
        
        // 输出变量
        out vec4 FragColor;
        
        // 统一变量
        uniform vec4 baseColor;
        uniform bool isSelected;
        uniform bool useLighting;
        uniform sampler2D textureSampler;
        
        void main()
        {
            if (isSelected) {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0); // 红色高亮
            } else {
                // 基础颜色计算
                vec4 finalColor = baseColor;
                
                // 如果基础颜色是默认值，则使用白色
                if (finalColor.r == 0.0 && finalColor.g == 0.0 && finalColor.b == 0.0 && finalColor.a == 0.0) {
                    finalColor = vec4(1.0, 1.0, 1.0, 1.0);
                }
                
                // 获取纹理颜色
                vec4 textureColor = texture(textureSampler, fragTexCoord);
                // 如果纹理有效，则使用纹理颜色
                if (textureColor.a > 0.1) {
                    finalColor = textureColor;
                }
                
                FragColor = finalColor;
                if (!useLighting) {
                    FragColor = finalColor;
                    return;
                }
                
                // 定义光照参数
                vec3 lightPos   = vec3(5.0, 5.0, 10.0);  // 光源位置
                vec3 viewPos    = vec3(0.0, -10.0, 2.0);  // 观察者位置
                vec3 lightColor = vec3(1.0, 1.0, 1.0); // 白光
                
                // 定义光照截断参数
                float cutOff = 0.8;      // 内边缘
                float outerCutOff = 0.5; // 外边缘
                
                // 计算光照方向和表面到光源的方向
                vec3 pos = fragPosition;
                vec3 normal = normalize(fragNormal);
                vec3 lightDir = normalize(lightPos - pos);
                
                // 计算当前点的光照角度
                float theta = dot(normal, lightDir);
                
                // 使用smoothstep进行强度插值，实现平滑边缘过渡
                float intensity = smoothstep(outerCutOff, cutOff, theta);
                
                // 环境光
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * lightColor;
                
                // 漫反射
                float diff = max(dot(normal, lightDir), 0.0);
                vec3 diffuse = diff * lightColor;
                
                // Blinn-Phong镜面反射
                vec3 viewDir = normalize(viewPos - pos);
                vec3 halfwayDir = normalize(lightDir + viewDir);  // 半角向量
                float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
                float specularStrength = 0.5;
                vec3 specular = specularStrength * spec * lightColor;
                
                // 应用光照强度到漫反射和镜面反射
                diffuse *= intensity;
                specular *= intensity;
                
                // 合成最终颜色
                vec3 result = (ambient + diffuse + specular) * finalColor.rgb;
                FragColor = vec4(result, finalColor.a);
            }
        }
    )";
    
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;
    
    // 创建并添加顶点着色器
    osg::ref_ptr<osg::Shader> vertexShader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
    program->addShader(vertexShader);
    
    // 创建并添加片元着色器
    osg::ref_ptr<osg::Shader> fragmentShader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
    program->addShader(fragmentShader);
    
    // 添加程序名称，便于调试
    program->setName("BlinnPhongShaderCubeProgram");
    
    return program.release();
}

// 创建天空盒着色器程序
osg::Program* ShaderCube::createSkyBoxShaderProgram()
{
    // 顶点着色器源码
    const char* vertexShaderSource = R"(
        #version 330 core
        
        layout(location = 0) in vec3 position;
        
        out vec3 TexCoords;
        
        uniform mat4 osg_ViewMatrix;
        uniform mat4 osg_ProjectionMatrix;
        
        void main()
        {
            TexCoords = position;
            // 移除视图矩阵的平移部分，确保天空盒始终围绕相机
            mat4 view = mat4(mat3(osg_ViewMatrix));
            vec4 pos = osg_ProjectionMatrix * view * vec4(position, 1.0);
            gl_Position = pos.xyww; // 使用.xyww而不是.xyzw来确保深度测试正确
        }
    )";
    
    // 片元着色器源码
    const char* fragmentShaderSource = R"(
        #version 330 core
        
        in vec3 TexCoords;
        
        out vec4 FragColor;
        
        uniform samplerCube skybox;
        
        void main()
        {    
            FragColor = texture(skybox, TexCoords);
        }
    )";
    
    // 创建着色器程序
    osg::ref_ptr<osg::Program> program = new osg::Program;
    
    // 创建并添加顶点着色器
    osg::ref_ptr<osg::Shader> vertexShader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
    program->addShader(vertexShader);
    
    // 创建并添加片元着色器
    osg::ref_ptr<osg::Shader> fragmentShader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
    program->addShader(fragmentShader);
    
    return program.release();
}


//实现创建Shader立方体的方法
osg::Node* ShaderCube::createCube(float size)
{
    // 创建顶点数组（24个顶点，每个面4个独立顶点）
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 创建法线数组（24个法线，与顶点一一对应）
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 创建纹理坐标数组（24个纹理坐标，与顶点一一对应）
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
    
    float s = size;
    
    // 重新定义立方体的顶点，确保正确的顺序和方向
    // 前面 (Z轴正方向)
    vertices->push_back(osg::Vec3(-s, -s,  s)); // 0
    vertices->push_back(osg::Vec3( s, -s,  s)); // 1
    vertices->push_back(osg::Vec3( s,  s,  s)); // 2
    vertices->push_back(osg::Vec3(-s,  s,  s)); // 3
    
    // 后面 (Z轴负方向)
    vertices->push_back(osg::Vec3( s, -s, -s)); // 4
    vertices->push_back(osg::Vec3(-s, -s, -s)); // 5
    vertices->push_back(osg::Vec3(-s,  s, -s)); // 6
    vertices->push_back(osg::Vec3( s,  s, -s)); // 7
    
    // 上面 (Y轴正方向)
    vertices->push_back(osg::Vec3(-s,  s, -s)); // 8
    vertices->push_back(osg::Vec3(-s,  s,  s)); // 9
    vertices->push_back(osg::Vec3( s,  s,  s)); // 10
    vertices->push_back(osg::Vec3( s,  s, -s)); // 11
    
    // 下面 (Y轴负方向)
    vertices->push_back(osg::Vec3(-s, -s, -s)); // 12
    vertices->push_back(osg::Vec3( s, -s, -s)); // 13
    vertices->push_back(osg::Vec3( s, -s,  s)); // 14
    vertices->push_back(osg::Vec3(-s, -s,  s)); // 15
    
    // 右面 (X轴正方向)
    vertices->push_back(osg::Vec3( s, -s, -s)); // 16
    vertices->push_back(osg::Vec3( s, -s,  s)); // 17
    vertices->push_back(osg::Vec3( s,  s,  s)); // 18
    vertices->push_back(osg::Vec3( s,  s, -s)); // 19
    
    // 左面 (X轴负方向)
    vertices->push_back(osg::Vec3(-s, -s, -s)); // 20
    vertices->push_back(osg::Vec3(-s, -s,  s)); // 21
    vertices->push_back(osg::Vec3(-s,  s,  s)); // 22
    vertices->push_back(osg::Vec3(-s,  s, -s)); // 23
    
    // 定义每个面的法线（24个法线，与顶点一一对应）
    // 前面法线
    normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f)); // 0
    normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f)); // 1
    normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f)); // 2
    normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f)); // 3
    
    // 后面法线
    normals->push_back(osg::Vec3(0.0f, 0.0f, -1.0f)); // 4
    normals->push_back(osg::Vec3(0.0f, 0.0f, -1.0f)); // 5
    normals->push_back(osg::Vec3(0.0f, 0.0f, -1.0f)); // 6
    normals->push_back(osg::Vec3(0.0f, 0.0f, -1.0f)); // 7
    
    // 上面法线
    normals->push_back(osg::Vec3(0.0f, 1.0f, 0.0f)); // 8
    normals->push_back(osg::Vec3(0.0f, 1.0f, 0.0f)); // 9
    normals->push_back(osg::Vec3(0.0f, 1.0f, 0.0f)); // 10
    normals->push_back(osg::Vec3(0.0f, 1.0f, 0.0f)); // 11
    
    // 下面法线
    normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f)); // 12
    normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f)); // 13
    normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f)); // 14
    normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f)); // 15
    
    // 右面法线
    normals->push_back(osg::Vec3(1.0f, 0.0f, 0.0f)); // 16
    normals->push_back(osg::Vec3(1.0f, 0.0f, 0.0f)); // 17
    normals->push_back(osg::Vec3(1.0f, 0.0f, 0.0f)); // 18
    normals->push_back(osg::Vec3(1.0f, 0.0f, 0.0f)); // 19
    
    // 左面法线
    normals->push_back(osg::Vec3(-1.0f, 0.0f, 0.0f)); // 20
    normals->push_back(osg::Vec3(-1.0f, 0.0f, 0.0f)); // 21
    normals->push_back(osg::Vec3(-1.0f, 0.0f, 0.0f)); // 22
    normals->push_back(osg::Vec3(-1.0f, 0.0f, 0.0f)); // 23
    
    // 定义纹理坐标（24个纹理坐标，与顶点一一对应）
    // 前面
    texcoords->push_back(osg::Vec2(0.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 1.0f));
    texcoords->push_back(osg::Vec2(0.0f, 1.0f));
    
    // 后面
    texcoords->push_back(osg::Vec2(0.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 1.0f));
    texcoords->push_back(osg::Vec2(0.0f, 1.0f));
    
    // 上面
    texcoords->push_back(osg::Vec2(0.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 1.0f));
    texcoords->push_back(osg::Vec2(0.0f, 1.0f));
    
    // 下面
    texcoords->push_back(osg::Vec2(0.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 1.0f));
    texcoords->push_back(osg::Vec2(0.0f, 1.0f));
    
    // 右面
    texcoords->push_back(osg::Vec2(0.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 1.0f));
    texcoords->push_back(osg::Vec2(0.0f, 1.0f));
    
    // 左面
    texcoords->push_back(osg::Vec2(0.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 0.0f));
    texcoords->push_back(osg::Vec2(1.0f, 1.0f));
    texcoords->push_back(osg::Vec2(0.0f, 1.0f));
    
    // 创建索引数组（使用三角形绘制立方体的6个面，共36个索引）
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 前面 (0,1,2,3)
    indices->push_back(0); indices->push_back(1); indices->push_back(2);
    indices->push_back(2); indices->push_back(3); indices->push_back(0);
    
    // 后面 (4,5,6,7)
    indices->push_back(4); indices->push_back(5); indices->push_back(6);
    indices->push_back(6); indices->push_back(7); indices->push_back(4);
    
    // 上面 (8,9,10,11)
    indices->push_back(8); indices->push_back(9); indices->push_back(10);
    indices->push_back(10); indices->push_back(11); indices->push_back(8);
    
    // 下面 (12,13,14,15)
    indices->push_back(12); indices->push_back(13); indices->push_back(14);
    indices->push_back(14); indices->push_back(15); indices->push_back(12);
    
    // 右面 (16,17,18,19)
    indices->push_back(16); indices->push_back(17); indices->push_back(18);
    indices->push_back(18); indices->push_back(19); indices->push_back(16);
    
    // 左面 (20,21,22,23)
    indices->push_back(20); indices->push_back(21); indices->push_back(22);
    indices->push_back(22); indices->push_back(23); indices->push_back(20);
    
    // 创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 设置几何体属性
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setTexCoordArray(0, texcoords);
    geometry->addPrimitiveSet(indices);
    
    // 设置顶点属性 (使用标准属性位置)
    geometry->setVertexAttribArray(0, vertices);  // 顶点位置
    geometry->setVertexAttribBinding(0, osg::Geometry::BIND_PER_VERTEX);
    geometry->setVertexAttribArray(1, normals);   // 法线
    geometry->setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
    geometry->setVertexAttribArray(2, texcoords); // 纹理坐标
    geometry->setVertexAttribBinding(2, osg::Geometry::BIND_PER_VERTEX);
    
    // 创建Geode节点
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    
    // 获取状态集
    osg::StateSet* stateset = geode->getOrCreateStateSet();
    
    // 启用着色器
    osg::ref_ptr<osg::Program> program = createShaderProgram();
    stateset->setAttributeAndModes(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 设置默认颜色uniform
    stateset->addUniform(new osg::Uniform("baseColor", osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f))); // 白色
    stateset->addUniform(new osg::Uniform("isSelected", false));
    stateset->addUniform(new osg::Uniform("useLighting", true)); // 启用光照
    
    // 加载并应用纹理
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile("E://1.png");
    if (image.valid()) {
        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setImage(image);
        texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
        texture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
        texture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        
        stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
        stateset->addUniform(new osg::Uniform("textureSampler", 0));
    } else {
        // 如果没有找到纹理文件，确保uniform仍然被创建以避免错误
        stateset->addUniform(new osg::Uniform("textureSampler", 0));
        // 输出错误信息
        std::cout << "Failed to load texture: E://1.png" << std::endl;
    }
    
    // 确保启用深度测试
    stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 禁用背面剔除，确保所有面都被渲染
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    
    return geode.release(); 
}

// osg::Node* ShaderCube::createSkySSSBox(const std::string& resourcePath)
// {
//     // 创建状态集
//     osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet();

//     // 设置纹理映射方式，指定为替代方式，即纹理中的颜色代替原来的颜色
//     osg::ref_ptr<osg::TexEnv> te = new osg::TexEnv;
//     te->setMode(osg::TexEnv::REPLACE);
//     stateset->setTextureAttributeAndModes(0, te.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

//     // 自动生成纹理坐标，反射方式(NORMAL_MAP)
//     osg::ref_ptr<osg::TexGen> tg = new osg::TexGen;
//     tg->setMode(osg::TexGen::NORMAL_MAP);
//     stateset->setTextureAttributeAndModes(0, tg.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

//     // 加载立方体贴图
//     osg::ref_ptr<osg::TextureCubeMap> cubemap = new osg::TextureCubeMap;

//     // 加载6个面的纹理
//     std::string faces[6] = {
//         resourcePath + "/px.png", // 右
//         resourcePath + "/nx.png", // 左
//         resourcePath + "/py.png", // 上
//         resourcePath + "/ny.png", // 下
//         resourcePath + "/pz.png", // 前
//         resourcePath + "/nz.png"  // 后
//     };

//     osg::ref_ptr<osg::Image> imagePosX = osgDB::readImageFile(faces[0]);
//     osg::ref_ptr<osg::Image> imageNegX = osgDB::readImageFile(faces[1]);
//     osg::ref_ptr<osg::Image> imagePosY = osgDB::readImageFile(faces[2]);
//     osg::ref_ptr<osg::Image> imageNegY = osgDB::readImageFile(faces[3]);
//     osg::ref_ptr<osg::Image> imagePosZ = osgDB::readImageFile(faces[4]);
//     osg::ref_ptr<osg::Image> imageNegZ = osgDB::readImageFile(faces[5]);

//     bool allImagesLoaded = imagePosX.valid() && imageNegX.valid() && 
//                           imagePosY.valid() && imageNegY.valid() && 
//                           imagePosZ.valid() && imageNegZ.valid();

//     if (allImagesLoaded) {
//         // 设置立方图的六个面的贴图
//         cubemap->setImage(osg::TextureCubeMap::POSITIVE_X, imagePosX);
//         cubemap->setImage(osg::TextureCubeMap::NEGATIVE_X, imageNegX);
//         cubemap->setImage(osg::TextureCubeMap::POSITIVE_Y, imagePosY);
//         cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Y, imageNegY);
//         cubemap->setImage(osg::TextureCubeMap::POSITIVE_Z, imagePosZ);
//         cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Z, imageNegZ);

//         // 设置纹理环绕模式
//         cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
//         cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
//         cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

//         // 设置滤波：线形
//         cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
//         cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
//     } else {
//         std::cout << "Failed to load one or more skybox textures" << std::endl;
//         return new osg::Group;
//     }

//     // 应用立方体贴图
//     stateset->setTextureAttributeAndModes(0, cubemap, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

//     // 关闭光照和背面剔除
//     stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
//     stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

//     // 将深度设置为远平面
//     osg::ref_ptr<osg::Depth> depth = new osg::Depth;
//     depth->setFunction(osg::Depth::ALWAYS);
//     depth->setRange(1.0, 1.0); // 远平面   
//     stateset->setAttributeAndModes(depth, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

//     // 设置渲染顺序为-1，先渲染
//     stateset->setRenderBinDetails(-1, "RenderBin");

//     // 创建一个球体作为天空盒
//     osg::ref_ptr<osg::Drawable> drawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 500.0f));

//     // 把球体加入到叶节点
//     osg::ref_ptr<osg::Geode> geode = new osg::Geode;
//     geode->setCullingActive(false);
//     geode->setStateSet(stateset);
//     geode->addDrawable(drawable);

//     return geode.release();
// }

osg::Node* ShaderCube::createSkyBox(const std::string& resourcePath)
{
    // 创建状态集
    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet();

    // 加载立方体贴图
    osg::ref_ptr<osg::TextureCubeMap> cubemap = new osg::TextureCubeMap;

    std::string faces[6] = {
        resourcePath + "/px.png",
        resourcePath + "/nx.png",
        resourcePath + "/py.png",
        resourcePath + "/ny.png",
        resourcePath + "/pz.png",
        resourcePath + "/nz.png"
    };

    osg::ref_ptr<osg::Image> imagePosX = osgDB::readImageFile(faces[0]);
    osg::ref_ptr<osg::Image> imageNegX = osgDB::readImageFile(faces[1]);
    osg::ref_ptr<osg::Image> imagePosY = osgDB::readImageFile(faces[2]);
    osg::ref_ptr<osg::Image> imageNegY = osgDB::readImageFile(faces[3]);
    osg::ref_ptr<osg::Image> imagePosZ = osgDB::readImageFile(faces[4]);
    osg::ref_ptr<osg::Image> imageNegZ = osgDB::readImageFile(faces[5]);

    bool allImagesLoaded = imagePosX.valid() && imageNegX.valid() && 
                          imagePosY.valid() && imageNegY.valid() && 
                          imagePosZ.valid() && imageNegZ.valid();

    if (!allImagesLoaded) {
        std::cout << "Failed to load skybox textures" << std::endl;
        return new osg::Group;
    }

    cubemap->setImage(osg::TextureCubeMap::POSITIVE_X, imagePosX);
    cubemap->setImage(osg::TextureCubeMap::NEGATIVE_X, imageNegX);
    cubemap->setImage(osg::TextureCubeMap::POSITIVE_Y, imagePosY);
    cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Y, imageNegY);
    cubemap->setImage(osg::TextureCubeMap::POSITIVE_Z, imagePosZ);
    cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Z, imageNegZ);

    cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // ⚠️ 确保纹理设置在Geode的StateSet上（纹理单元0）
    stateset->setTextureAttributeAndModes(0, cubemap, osg::StateAttribute::ON);

    // 使用REFLECTION_MAP
    osg::ref_ptr<osg::TexGen> tg = new osg::TexGen;
    tg->setMode(osg::TexGen::REFLECTION_MAP);
    stateset->setTextureAttributeAndModes(0, tg.get(), osg::StateAttribute::ON);

    osg::ref_ptr<osg::TexEnv> te = new osg::TexEnv;
    te->setMode(osg::TexEnv::REPLACE);
    stateset->setTextureAttributeAndModes(0, te.get(), osg::StateAttribute::ON);

    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    // 深度设置
    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setFunction(osg::Depth::LEQUAL);
    depth->setRange(1.0, 1.0);
    depth->setWriteMask(false);  // ⚠️ 禁止写深度
    stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);

    stateset->setRenderBinDetails(-1, "RenderBin");

    // 创建立方体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    float size = 500.0f;
    
    vertices->push_back(osg::Vec3(-size, -size, -size));
    vertices->push_back(osg::Vec3( size, -size, -size));
    vertices->push_back(osg::Vec3( size,  size, -size));
    vertices->push_back(osg::Vec3(-size,  size, -size));
    vertices->push_back(osg::Vec3(-size, -size,  size));
    vertices->push_back(osg::Vec3( size, -size,  size));
    vertices->push_back(osg::Vec3( size,  size,  size));
    vertices->push_back(osg::Vec3(-size,  size,  size));
    
    geometry->setVertexArray(vertices);
    
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    indices->push_back(0); indices->push_back(3); indices->push_back(2);
    indices->push_back(2); indices->push_back(1); indices->push_back(0);
    indices->push_back(4); indices->push_back(5); indices->push_back(6);
    indices->push_back(6); indices->push_back(7); indices->push_back(4);
    indices->push_back(0); indices->push_back(4); indices->push_back(7);
    indices->push_back(7); indices->push_back(3); indices->push_back(0);
    indices->push_back(1); indices->push_back(2); indices->push_back(6);
    indices->push_back(6); indices->push_back(5); indices->push_back(1);
    indices->push_back(0); indices->push_back(1); indices->push_back(5);
    indices->push_back(5); indices->push_back(4); indices->push_back(0);
    indices->push_back(3); indices->push_back(7); indices->push_back(6);
    indices->push_back(6); indices->push_back(2); indices->push_back(3);
    
    geometry->addPrimitiveSet(indices);
    
    // ⚠️ Geode携带StateSet（包含纹理）
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->setCullingActive(false);
    geode->setStateSet(stateset);  // 纹理在这里！
    geode->addDrawable(geometry);

    // 天空盒回调
    class SkyboxTransformCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::MatrixTransform* transform = dynamic_cast<osg::MatrixTransform*>(node);
            if (transform)
            {
                // 使用单位矩阵，确保天空盒不会旋转
                transform->setMatrix(osg::Matrix::identity());
            }
            traverse(node, nv);
        }
    };

    // ⚠️ 包裹在Transform中
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
    transform->addChild(geode);
    transform->setUpdateCallback(new SkyboxTransformCallback);
    transform->setReferenceFrame(osg::Transform::ABSOLUTE_RF);

    return transform.release();
}
