
    #version 330
    uniform mat4 model_from_view;
    uniform mat4 view_from_clip;
    layout(location = 0) in vec4 vertex;
	layout(location = 1) in vec2 uv;

    out vec3 view_ray;
	out vec2 auv;

    void main() 
	{
      view_ray =(model_from_view * vec4((view_from_clip * vertex).xyz, 0.0)).xyz*0.001;
	  auv=uv;

      gl_Position = vertex;
    }
