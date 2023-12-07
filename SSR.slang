import Scene.Camera.Camera;
import Utils.Math.MatrixUtils;
cbuffer ssrBuf
{
    Texture2D<float4> tex;
    Texture2D<float4> worldNormalTex;
    Texture2D<float> depthTex;
    Texture2D<float4> worldPosTex;
    Camera gCamera;
	float4x4 invProj;
    SamplerState gSampler;
};
float gauss[49] = 
{
    0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067,
    0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292,
    0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117,
    0.00038771, 0.01330373, 0.11098164, 0.22508352, 0.11098164, 0.01330373, 0.00038771,
    0.00019117, 0.00655965, 0.05472157, 0.11098164, 0.05472157, 0.00655965, 0.00019117,
    0.00002292, 0.00078633, 0.00655965, 0.01330373, 0.00655965, 0.00078633, 0.00002292,
    0.00000067, 0.00002292, 0.00019117, 0.00038771, 0.00019117, 0.00002292, 0.00000067
};
 
 
float3 GetGaussColor(float2 uv)
{
    const int size = 7;
 
    float3 finalColor = float3(0,0,0);
 
    uint2 texDim;
    uint levels;
    tex.GetDimensions(0, texDim.x, texDim.y, levels);
    int idx = 0;
    for(int i = -3;i <= 3;i++)
    {
        for(int j = -3; j <= 3;j++)
        {
            float2 offset_uv = uv + float2(5.0 * i /texDim.x, 5.0 * j /texDim.y);
            float3 color = tex.Sample(gSampler, offset_uv).xyz;
            float weight = gauss[idx++];
            finalColor = finalColor + weight * color;
        }
    }
 
    return finalColor;
}
#ifndef _YRC_SCREEN_SPACE_RAYTRACE_
#define _YRC_SCREEN_SPACE_RAYTRACE_

#define RAY_LENGTH 40.0	//maximum ray length.
#define STEP_COUNT 16	//maximum sample count.
#define PIXEL_STRIDE 16 //sample multiplier. it's recommend 16 or 8.
#define PIXEL_THICKNESS (0.03 * PIXEL_STRIDE)	//how thick is a pixel. correct value reduces noise.

bool RayIntersect(float raya, float rayb, float2 sspt) {
	if (raya > rayb) {
		float t = raya;
		raya = rayb;
		rayb = t;
	}

	float screenPCameraDepth = -depthTex.SampleLevel(gSampler, float2(sspt / 2 + 0.5),0).r;
	return raya < screenPCameraDepth && rayb > screenPCameraDepth - PIXEL_THICKNESS;

}

bool traceRay(float3 start, float3 direction, float jitter, float4 texelSize, out float2 hitPixel, out float marchPercent,out float hitZ) {
	//clamp raylength to near clip plane.
	float rayLength = ((start.z + direction.z * RAY_LENGTH) > - gCamera.data.nearZ) ?
		(-gCamera.data.nearZ - start.z) / direction.z : RAY_LENGTH;

	float3 end = start + direction * rayLength;

	float4 H0 = mul(gCamera.data.projMat, float4(start, 1));
	float4 H1 = mul(gCamera.data.projMat, float4(end, 1));

	float2 screenP0 = H0.xy / H0.w;
	float2 screenP1 = H1.xy / H1.w;	

	float k0 = 1.0 / H0.w;
	float k1 = 1.0 / H1.w;

	float Q0 = start.z * k0;
	float Q1 = end.z * k1;

	if (abs(dot(screenP1 - screenP0, screenP1 - screenP0)) < 0.00001) {
		screenP1 += texelSize.xy;
	}
	float2 deltaPixels = (screenP1 - screenP0) * texelSize.zw;
	float step;	//the sample rate.
	step = min(1 / abs(deltaPixels.y), 1 / abs(deltaPixels.x)); //make at least one pixel is sampled every time.

	//make sample faster.
	step *= PIXEL_STRIDE;		
	float sampleScaler = 1.0 - min(1.0, -start.z / 100); //sample is slower when far from the screen.
	step *= 1.0 + sampleScaler;	

	float interpolationCounter = step;	//by default we use step instead of 0. this avoids some glitch.

	float4 pqk = float4(screenP0, Q0, k0);
	float4 dpqk = float4(screenP1 - screenP0, Q1 - Q0, k1 - k0) * step;

	pqk += jitter * dpqk;

	float prevZMaxEstimate = start.z;

	bool intersected = false;
		//the logic here is a little different from PostProcessing or (casual-effect). but it's all about raymarching.
		for (int i = 1;
			i <= STEP_COUNT && interpolationCounter <= 1 && !intersected;
			i++,
			interpolationCounter += step
			) {
		pqk += dpqk;
		float rayZMin = prevZMaxEstimate;
		float rayZMax = ( pqk.z) / ( pqk.w);

		if (RayIntersect(rayZMin, rayZMax, pqk.xy - dpqk.xy / 2)) {
			hitPixel = (pqk.xy - dpqk.xy / 2) / 2 + 0.5;
			marchPercent = (float)i / STEP_COUNT;
			intersected = true;
		}
		else {
			prevZMaxEstimate = rayZMax;
		}
	}

#if 1	  //binary search
	if (intersected) {
		pqk -= dpqk;	//one step back
		
			for (float gapSize = PIXEL_STRIDE; gapSize > 1.0; gapSize /= 2) {
				dpqk /= 2;
				float rayZMin = prevZMaxEstimate;
				float rayZMax = (pqk.z) / ( pqk.w);

				if (RayIntersect(rayZMin, rayZMax, pqk.xy - dpqk.xy / 2)) {		//hit, stay the same.(but ray length is halfed)

				}
				else {							//miss the hit. we should step forward
					pqk += dpqk;
					prevZMaxEstimate = rayZMax;
				}
			}
		hitPixel = (pqk.xy - dpqk.xy / 2) / 2 + 0.5;
	}
#endif
	hitZ = pqk.z / pqk.w;

	return intersected;
}

#endif
#define SCREEN_EDGE_MASK 0.9
			float alphaCalc(float3 rayDirection, float2 hitPixel, float marchPercent, float hitZ) {
				float res = 1;
				res *= saturate(-5 * (rayDirection.z - 0.2));
				float2 screenPCurrent = 2 * (hitPixel - 0.5);
				res *= 1 - max(
					(clamp(abs(screenPCurrent.x), SCREEN_EDGE_MASK, 1.0) - SCREEN_EDGE_MASK) / (1 - SCREEN_EDGE_MASK),
					(clamp(abs(screenPCurrent.y), SCREEN_EDGE_MASK, 1.0) - SCREEN_EDGE_MASK) / (1 - SCREEN_EDGE_MASK)
				);
				res *= 1 - marchPercent;
				res *= 1 - (-(hitZ - 0.2) * (1.0/gCamera.data.farZ));
				return res;
			}

float4 main(float2 texC: TEXCOORD) : SV_Target
{   
    float3 viewDir = gCamera.data.target;
    float3 wsNormal = worldNormalTex.Sample(gSampler,texC).xyz * 2.0 - 1.0;
    float3 csNormal = normalize(mul((float3x3)gCamera.data.viewMat, wsNormal)); 

    uint2 texDim;
    uint levels;
    tex.GetDimensions(0, texDim.x, texDim.y, levels);

	float4 cameraRay = float4(texC * 2.0 - 1.0, 1.0, 1.0);
	cameraRay = mul(invProj, cameraRay);
	float3 csRay = cameraRay.xyz / cameraRay.w;
    //float2 texC2 = float2(texC.x, 1.0 - texC.y);
	//texC.y = 1.0 - texC.y;
    float2 uv2 = texC * texDim;
    float c = (uv2.x + uv2.y) * 0.25;
    float jitter = fmod(c,1.0);

    float marchPercent;
    float hitZ;
    float3 reflection = float3(0);
    float alpha = 0;
    float2 hitPixel;
	Ray ray = gCamera.computeRayPinhole(uint2(texC * texDim),texDim,true);
	//float3 worldPos = worldPosTex.Sample(gSampler,texC).xyz;
	//worldPos = normalize(mul((float3x3)gCamera.data.viewMat, worldPos)); 
	//float3 v = normalize(gCamera.data.posW - worldPos);
	//v = normalize(mul((float3x3)gCamera.data.viewMat, v)); 
	float depth = depthTex.SampleLevel(gSampler, texC, 0).r;

	float3 csRayOrigin = ray.dir;
    float3 reflectDir = normalize(reflect(ray.dir, csNormal));
    float rayBump = max(-0.018*csRayOrigin.z, 0.001);
    if (traceRay(
        csRayOrigin + csNormal * rayBump,
        reflectDir,
        jitter,
        float4(1.0/texDim,texDim),
        hitPixel,
        marchPercent,
        hitZ)) {
        alpha = alphaCalc(reflectDir, hitPixel, marchPercent,hitZ);
    }
    reflection = tex.Sample(gSampler, hitPixel).xyz;	
    // return float4(hitPixel,0, 1);
	// return float4(reflectDir,1);
    return float4(reflection,1); //* alpha;  + tex.Sample(gSampler,texC);
}
