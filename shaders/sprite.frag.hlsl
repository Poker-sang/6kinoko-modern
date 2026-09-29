Texture2D image : register(t0, space2);
SamplerState image_sampler : register(s0, space2);
cbuffer Alpha : register(b0, space3) { uint enabled; uint comparison; float reference; float depth_unorm24; };
struct Output { float4 color : SV_Target0; float depth : SV_Depth; };
Output main(float4 position : SV_Position,float4 color : TEXCOORD0,float2 uv : TEXCOORD1) {
    float4 result=image.Sample(image_sampler,uv)*color;
    if(enabled!=0) {
        bool keep=comparison==7 || (comparison==1 && result.a<reference) ||
            (comparison==2 && result.a==reference) || (comparison==3 && result.a<=reference) ||
            (comparison==4 && result.a>reference) || (comparison==5 && result.a!=reference) ||
            (comparison==6 && result.a>=reference);
        if(!keep) discard;
    }
    Output output;
    output.color=result;
    output.depth=depth_unorm24!=0 ? round(saturate(position.z)*16777215.0)/16777215.0 : position.z;
    return output;
}
