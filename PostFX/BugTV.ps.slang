
cbuffer PerFrameCB : register(b0)
{
    Texture2D gTexture;
    SamplerState gSampler;
};


cbuffer BugTVCB : register(b1)
{
    float iGlobalTime;
    float Frequency;
    float2 iResolution;
};

//#define CURVE 
//#define SCANS
//#define FLICKS
//#define GRAINS 
//#define YBUG 
//#define DIRTY
//#define STRIP
//#define COLOR
//#define BLINK
//#define VIG

float2 uv_curve(float2 uv)
{
    uv = (uv - 0.5) * 2.0;
    uv *= 1.2;
    uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
    uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
    uv /= 1.15;
    uv = (uv / 2.0) + 0.5;
    return uv;
}

float3 color(Texture2D tex, float2 uv)
{
    float3 color = gTexture.Sample(gSampler, uv).rgb;
#ifdef COLOR
    float bw = (color.r + color.g + color.b) / 3.0;
    color = lerp(color, float3(bw, bw, bw), .95);
    float p = 1.5;
    color.r = pow(color.r, p);
    color.g = pow(color.g, p - 0.1);
    color.b = pow(color.b, p);
#endif
    return color;
}
float rand2d(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float rand(float n)
{
    return frac(sin(n) * 43758.5453123);
}

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float noise(float p)
{
    float fl = floor(p);
    float fc = frac(p);
    return lerp(rand(fl), rand(fl + 1.0), fc);
}
float map(float val, float amin, float amax, float bmin, float bmax)
{
    float n = (val - amin) / (amax - amin);
    float m = bmin + n * (bmax - bmin);
    return m;
}
float snoise(float p)
{
    return noise(p)*2.0-1.0;
}

float threshold(float val, float cut)
{
    float v = clamp(abs(val) - cut, 0.0, 1.0);
    v = sign(val) * v;
    float scale = 1.0 / (1.0 - cut);
    return v * scale;
}

float3 ghost(Texture2D tex, float2 uv)
{
#ifdef FLICKS
    
    float n1 = threshold(snoise(iGlobalTime * 10.), .85);
    float n2 = threshold(snoise(2000.0 + iGlobalTime * 10.), .85);
    float n3 = threshold(snoise(3000.0 + iGlobalTime * 10.), .85);
    
    float2 or = float2(0., 0.);
    float2 og = float2(0, 0.);
    float2 ob = float2(0., 0);

    float os = .05;
    or += float2(n1 * os, 0.);
    og += float2(n2 * os, 0.);
    ob += float2(0., n3 * os);
  
    float r = color(gTexture, uv + or).r;
    float g = color(gTexture, uv + og).g;
    float b = color(gTexture, uv + ob).b;
    return float3(r, g, b);
#else 
    return gTexture.Sample(gSampler, uv).rgb;
#endif
}

float2 uv_ybug(float2 uv)
{
    float n4 = clamp(noise(200.0 + iGlobalTime * 2.) * 14., 0., 2.);
    uv.y += n4;
    uv.y = mod(uv.y, 1.);
    return uv;
}

float2 uv_hstrip(float2 uv)
{
    float vnoise = snoise(iGlobalTime * 6.);
    float hnoise = threshold(snoise(iGlobalTime * 10.), .5);

    float line = (sin(uv.y * 10. + vnoise) + 1.) / 2.;
    line = (clamp(line, .9, 1.) - .9) * 10.;
    
    uv.x += line * 0.03 * hnoise;
    uv.x = mod(uv.x, 1.);
    return uv;
}

float4 main(float2 uv : TEXCOORD) : SV_TARGET0
{
    float t = float(int(iGlobalTime * Frequency));

#ifdef CURVE
    uv = uv_curve(uv);
#endif

    float2 ouv = uv;
    
#ifdef GRAINS
    float xn = threshold(snoise(iGlobalTime * 10.), .7) * 0.05;
    float yn = threshold(snoise((500.0 + iGlobalTime) * 10.), .7) * 0.05;
    
    float r = rand2d(uv + (t + 100.0) * .01);
    uv = uv + float2(xn, yn) * r;
#endif
    
     
#ifdef YBUG
    uv = uv_ybug(uv);
#endif

#ifdef STRIP
    uv = uv_hstrip(uv);
#endif
    
   
    float2 onePixel = float2(0.0, 1.0) / iResolution.xy * 3.;
#ifdef BLUR
    float3 colorA = ghost(gTexture,uv + onePixel,or,og,ob);
    float3 colorB = ghost(gTexture,uv - onePixel,or,og,ob);
    float3 colorC = ghost(gTexture,uv,or,og,ob);
    float3 color = (colorA+colorB+colorC)/3.0;
#else
    float3 color = ghost(gTexture, uv);
#endif

    //color = colorC;
    
    float scanA = (sin(uv.y * 3.1415 * iResolution.y / 2.7) + 1.) / 2.;
    float scanB = (sin(uv.y * 3.1415 * 1.) + 1.) / 2.;
#ifdef SCANS
    color *= .75 + scanA * .25;
    //color *= .5 + scanC * .5;
    //color *= scanB;    
#endif
    
#ifdef BLINK
    float blink = .96 + .04 * (sin(iGlobalTime * 100.) + 1.) / 2.;
    color *= blink;
#endif
    
#ifdef VIG
    float vig = 44.0 * (ouv.x * (1.0 - ouv.x) * ouv.y * (1.0 - ouv.y));
    vig *= lerp(0.7, 1.0, rand(t + 0.5));
    color *= .6 + .4 * vig;
#endif
     
#ifdef DIRTY
    color *= 1.0 + rand2d(uv + t * .01) * 0.2;
#endif

    float3 backColor = float3(.4, .4, .4);
    if (ouv.x < 0.0 || ouv.x > 1.0)
        color = backColor;
    if (ouv.y < 0.0 || ouv.y > 1.0)
        color = backColor;

    return float4(color, 1.0);
}
