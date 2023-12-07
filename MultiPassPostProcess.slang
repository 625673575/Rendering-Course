cbuffer PerFrameCB
{
    float2 iResolution;
    float iGlobalTime;
    SamplerState gSampler;
};
#define glsl_mod(x,y) (((x)-(y)*floor((x)/(y))))
#define SMOOTH(r, R) (1.-smoothstep(R-1., R+1., r))
#define RANGE(a, b, x) (step(a, x)*(1.-step(b, x)))
#define RS(a, b, x) (smoothstep(a-1., a+1., x)*(1.-smoothstep(b-1., b+1., x)))
#define M_PI 3.1415927
#define blue1 float3(0.74, 0.95, 1.)
#define blue2 float3(0.87, 0.98, 1.)
#define blue3 float3(0.35, 0.76, 0.83)
#define blue4 float3(0.953, 0.969, 0.89)
#define red float3(1., 0.38, 0.227)
#define MOV(a, b, c, d, t) (float2(a*cos(t)+b*cos(0.1*t), c*sin(t)+d*cos(0.1*t)))
float movingLine(float2 uv, float2 center, float radius)
{
    float theta0 = 90.*iGlobalTime;
    float2 d = uv-center;
    float r = sqrt(dot(d, d));
    if (r<radius)
    {
        float2 p = radius*float2(cos(theta0*M_PI/180.), -sin(theta0*M_PI/180.));
        float l = length(d-p*clamp(dot(d, p)/dot(p, p), 0., 1.));
        d = normalize(d);
        float theta = glsl_mod(180.*atan2(d.y, d.x)/M_PI+theta0, 360.);
        float gradient = clamp(1.-theta/90., 0., 1.);
        return SMOOTH(l, 1.)+0.5*gradient;
    }
    else return 0.;
}

float circle(float2 uv, float2 center, float radius, float width)
{
    float r = length(uv-center);
    return SMOOTH(r-width/2., radius)-SMOOTH(r+width/2., radius);
}

float circle2(float2 uv, float2 center, float radius, float width, float opening)
{
    float2 d = uv-center;
    float r = sqrt(dot(d, d));
    d = normalize(d);
    if (abs(d.y)>opening)
    return SMOOTH(r-width/2., radius)-SMOOTH(r+width/2., radius);
    else return 0.;
}

float circle3(float2 uv, float2 center, float radius, float width)
{
    float2 d = uv-center;
    float r = sqrt(dot(d, d));
    d = normalize(d);
    float theta = 180.*(atan2(d.y, d.x)/M_PI);
    return smoothstep(2., 2.1, abs(glsl_mod(theta+2., 45.)-2.))*lerp(0.5, 1., step(45., abs(glsl_mod(theta, 180.)-90.)))*(SMOOTH(r-width/2., radius)-SMOOTH(r+width/2., radius));
}

float triangles(float2 uv, float2 center, float radius)
{
    float2 d = uv-center;
    return RS(-8., 0., d.x-radius)*(1.-smoothstep(7.+d.x-radius, 9.+d.x-radius, abs(d.y)))+RS(0., 8., d.x+radius)*(1.-smoothstep(7.-d.x-radius, 9.-d.x-radius, abs(d.y)))+RS(-8., 0., d.y-radius)*(1.-smoothstep(7.+d.y-radius, 9.+d.y-radius, abs(d.x)))+RS(0., 8., d.y+radius)*(1.-smoothstep(7.-d.y-radius, 9.-d.y-radius, abs(d.x)));
}

float _cross(float2 uv, float2 center, float radius)
{
    float2 d = uv-center;
    int x = int(d.x);
    int y = int(d.y);
    float r = sqrt(dot(d, d));
    if (r<radius&&(x==y||x==-y))
    return 1.;
    else return 0.;
}

float dots(float2 uv, float2 center, float radius)
{
    float2 d = uv-center;
    float r = sqrt(dot(d, d));
    if (r<=2.5)
        return 1.;
        
    if (r<=radius&&(abs(d.y+0.5)<=1.&&glsl_mod(d.x+1., 50.)<2.))
    return 1.;
    else if (abs(d.y+0.5)<=1.&&r>=50.&&r<115.)
    return 0.5;
    else return 0.;
}

float bip1(float2 uv, float2 center)
{
    return SMOOTH(length(uv-center), 3.);
}

float bip2(float2 uv, float2 center)
{
    float r = length(uv-center);
    float R = 8.+glsl_mod(87.*iGlobalTime, 80.);
    return (0.5-0.5*cos(30.*iGlobalTime))*SMOOTH(r, 5.)+SMOOTH(6., r)-SMOOTH(8., r)+smoothstep(max(8., R-20.), R, r)-SMOOTH(R, r);
}
float4 mainImage(float2 fragCoord)
{
    float3 finalColor;
    float2 uv = fragCoord.xy;
    float2 c = iResolution.xy/2.;
    finalColor = ((float3)0.3*_cross(uv, c, 240.));
    finalColor += (circle(uv, c, 100., 1.)+circle(uv, c, 165., 1.))*blue1;
    finalColor += circle(uv, c, 240., 2.);
    finalColor += circle3(uv, c, 313., 4.)*blue1;
    finalColor += triangles(uv, c, 315.+30.*sin(iGlobalTime))*blue2;
    finalColor += movingLine(uv, c, 240.)*blue3;
    finalColor += circle(uv, c, 10., 1.)*blue3;
    finalColor += 0.7*circle2(uv, c, 262., 1., 0.5+0.2*cos(iGlobalTime))*blue3;
    if (length(uv-c)<240.)
    {
        float2 p = 130.*MOV(1.3, 1., 1., 1.4, 3.+0.1*iGlobalTime);
        finalColor += bip1(uv, c+p)*float3(1, 1, 1);
        p = 130.*MOV(0.9, -1.1, 1.7, 0.8, -2.+sin(0.1*iGlobalTime)+0.15*iGlobalTime);
        finalColor += bip1(uv, c+p)*float3(1, 1, 1);
        p = 50.*MOV(1.54, 1.7, 1.37, 1.8, sin(0.1*iGlobalTime+7.)+0.2*iGlobalTime);
        finalColor += bip2(uv, c+p)*red;
    }
    return float4(finalColor, 1.);
}

float4 main(float2 texC: TEXCOORD) : SV_TARGET
{
    return mainImage(texC * iResolution);
}
