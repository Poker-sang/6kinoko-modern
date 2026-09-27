// FXC SM5.1 / DXBC, SDL GPU vertex uniform space 1.
cbuffer View : register(b0, space1) { float2 extent; float2 padding; };
struct Input { float4 position : TEXCOORD0; float4 color : TEXCOORD1; float2 uv : TEXCOORD2; };
struct Output { float4 position : SV_Position; float4 color : TEXCOORD0; float2 uv : TEXCOORD1; };
Output main(Input v) {
    Output o;
    float w=1.0/v.position.w;
    // Legacy positions already subtract 0.5. Modern pixel centers need +0.5.
    float2 xy=(v.position.xy+0.5)/extent;
    o.position=float4((xy.x*2.0-1.0)*w,(1.0-xy.y*2.0)*w,v.position.z*w,w);
    o.color=v.color;o.uv=v.uv;
    return o;
}
