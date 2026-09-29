#version 450
layout(location=0) in vec4 color;
layout(location=1) in vec2 uv;
layout(location=0) out vec4 output_color;
layout(set=2,binding=0) uniform sampler2D image;
layout(set=3,binding=0) uniform Alpha {uint enabled;uint comparison;float reference;float padding;};
void main(){
    vec4 result=texture(image,uv)*color;
    if(enabled!=0){
        bool keep=comparison==7 || (comparison==1&&result.a<reference) ||
            (comparison==2&&result.a==reference) || (comparison==3&&result.a<=reference) ||
            (comparison==4&&result.a>reference) || (comparison==5&&result.a!=reference) ||
            (comparison==6&&result.a>=reference);
        if(!keep)discard;
    }
    output_color=result;
}
