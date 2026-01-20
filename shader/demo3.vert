
    #version 330
    uniform mat4 viewInverse;
    uniform mat4 projectInverse;

    layout(location = 0) in vec4 vertex;
	layout(location = 1) in vec2 uv;

    out vec3 view_ray;
	out vec2 auv;  

    void main() 
	{
      view_ray =(viewInverse * vec4((projectInverse * vertex).xyz, 0.0)).xyz;
	  auv=uv;

      gl_Position = vertex;
    
    }
