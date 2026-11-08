// Shared world-space light, stabilised CSM and analytic exponential height fog.
uniform bool uSceneEnabled=false;
uniform bool uDepthOnly=false;
uniform bool uSceneToneMap=true;
uniform mat4 uSceneFromLocal,uSceneView,uSceneLight[4];
uniform vec3 uSceneCamera,uSolarDirection,uSolarRadiance,uSolarSkyRadiance,uSceneAmbient;
uniform sampler2DArrayShadow uSceneShadow;
uniform int uSceneShadowCount=0;
uniform vec4 uSceneSplits,uSceneBias;
uniform vec4 uSceneWeather=vec4(0);      // wetness, puddles, snow, overcast (0..1)
uniform float uSceneFogDensity=.003;     // extinction per metre at sea level
// Smooth 2D value noise (world metres in, 0..1 out) for weather masks.
float sceneHash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float sceneNoise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
 return mix(mix(sceneHash(i),sceneHash(i+vec2(1,0)),f.x),mix(sceneHash(i+vec2(0,1)),sceneHash(i+vec2(1)),f.x),f.y);}
float sceneShadowLayer(vec3 world,vec3 normal,int layer) {
 vec4 p=uSceneLight[layer]*vec4(world+normal*.025,1);
 vec3 q=p.xyz/p.w*.5+.5;
 if(any(lessThan(q,vec3(0)))||any(greaterThan(q,vec3(1))))return 1.;
 vec2 texel=1./vec2(textureSize(uSceneShadow,0).xy);
 float bias=uSceneBias[layer]*(1.+2.*(1.-max(dot(normal,uSolarDirection),0.)));
 float sum=0.;for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)
  sum+=texture(uSceneShadow,vec4(q.xy+vec2(x,y)*texel,float(layer),q.z-bias));
 return sum/9.;
}
float sceneVisibility(vec3 world,vec3 normal) {
 if(!uSceneEnabled||uSceneShadowCount==0)return 1.;
 float depth=-(uSceneView*vec4(world,1)).z;
 int layer=0;for(int i=0;i<3;++i)if(i<uSceneShadowCount-1&&depth>uSceneSplits[i])layer=i+1;
 float visibility=sceneShadowLayer(world,normal,layer);
 float blend=smoothstep(.9*uSceneSplits[layer],uSceneSplits[layer],depth);
 if(layer+1<uSceneShadowCount)visibility=mix(visibility,sceneShadowLayer(world,normal,layer+1),blend);
 else visibility=mix(visibility,1.,blend);
 return visibility;
}
vec3 sceneFog(vec3 colour,vec3 world) {
 if(!uSceneEnabled)return colour;
 vec3 delta=world-uSceneCamera;float distance=length(delta);
 float x=clamp(.006*delta.z,-50.,50.);
 float integral=abs(x)<.0001?1.-.5*x+x*x/6.:(1.-exp(-x))/x;
 float optical=uSceneFogDensity*distance*exp(clamp(-.006*uSceneCamera.z,-50.,50.))*integral;
 float transmission=exp(-max(optical,0.));
 float cosine=dot(delta/max(distance,.0001),uSolarDirection);
 float phase=.08+.12*pow(max(cosine,0.),8.);
 vec3 scattering=uSceneAmbient*1.5+uSolarSkyRadiance*.02+uSolarRadiance*phase;
 return colour*transmission+scattering*(1.-transmission);
}
