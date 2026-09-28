#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 uv;
layout(set=1,binding=0) uniform View {vec2 extent;float clip_space;float padding;};
layout(location=0) out vec4 out_color;
layout(location=1) out vec2 out_uv;
void main(){
    float w=1.0/position.w;vec2 xy=(position.xy+0.5)/extent;
    gl_Position=clip_space!=0.0?position:vec4((xy.x*2.0-1.0)*w,(1.0-xy.y*2.0)*w,position.z*w,w);
    out_color=color;out_uv=uv;
}
