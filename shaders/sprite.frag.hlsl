// SDL GPU fragment resources use space 2.
Texture2D image : register(t0, space2);
SamplerState image_sampler : register(s0, space2);
float4 main(float4 position : SV_Position,float4 color : TEXCOORD0,float2 uv : TEXCOORD1) : SV_Target0 {
    return image.Sample(image_sampler,uv)*color;
}
