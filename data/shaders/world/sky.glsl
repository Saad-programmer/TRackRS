// Analytic Preetham/Perez daylight distribution in CIE xyY, linear RGB output.
uniform vec3 uSunSkyRadiance;
uniform float uOvercast=0.;
uniform vec4 uSceneDesert=vec4(0.);   // x heat (dry desert air), y airborne dust (chergui)
const float SKY_PI=3.14159265359;
vec3 perez(float theta,float gamma,vec3 A,vec3 B,vec3 C,vec3 D,vec3 E) {
 return (vec3(1)+A*exp(B/max(cos(theta),.025)))*(vec3(1)+C*exp(D*gamma)+E*cos(gamma)*cos(gamma));
}
vec3 analyticSky(vec3 d,vec3 sun,vec3 radiance,float daylight) {
 // Dry desert air is clearer (lower turbidity, deeper blue); blowing sand is hazier.
 float T=clamp(2.5-.55*clamp(uSceneDesert.x*2.,0.,1.)+5.5*uSceneDesert.y,1.7,8.),thetaSun=acos(clamp(sun.z,.001,1.)),t=thetaSun;
 vec3 A=vec3(.1787*T-1.463,-.0193*T-.2592,-.0167*T-.2608);
 vec3 B=vec3(-.3554*T+.4275,-.0665*T+.0008,-.095*T+.0092);
 vec3 C=vec3(-.0227*T+5.3251,-.0004*T+.2125,-.0079*T+.2102);
 vec3 D=vec3(.1206*T-2.5771,-.0641*T-.8989,-.0441*T-1.6537);
 vec3 E=vec3(-.067*T+.3703,-.0033*T+.0452,-.0109*T+.0529);
 float x=(.00165*t*t*t-.00374*t*t+.00208*t)*T*T+(-.02902*t*t*t+.06377*t*t-.03202*t+.00394)*T+(.11693*t*t*t-.21196*t*t+.06052*t+.25885);
 float y=(.00275*t*t*t-.00610*t*t+.00316*t)*T*T+(-.04214*t*t*t+.08970*t*t-.04153*t+.00515)*T+(.15346*t*t*t-.26756*t*t+.06669*t+.26688);
 float Y=max((4.0453*T-4.971)*tan((4./9.-T/120.)*(SKY_PI-2.*t))-.2155*T+2.4192,.05);
 float theta=acos(clamp(d.z,.001,1.)),gamma=acos(clamp(dot(d,sun),-1.,1.));
 vec3 xyY=vec3(Y,x,y)*perez(theta,gamma,A,B,C,D,E)/perez(0.,thetaSun,A,B,C,D,E);
 vec3 xyz=vec3(xyY.y*xyY.x/max(xyY.z,.001),xyY.x,(1.-xyY.y-xyY.z)*xyY.x/max(xyY.z,.001));
 vec3 rgb=max(mat3(3.2406,-.9689,.0557,-1.5372,1.8758,-.2040,-.4986,.0415,1.057)*xyz,vec3(0))*.035;
 rgb*=uSunSkyRadiance/vec3(3.,2.91,2.7);
 // Cloud cover: a brighter, desaturated dome with a softened horizon gradient.
 float lum=dot(rgb,vec3(.2126,.7152,.0722));
 rgb=mix(rgb,vec3(lum*1.15)*vec3(.94,.97,1.)*(.75+.25*clamp(d.z*2.+.5,0.,1.)),uOvercast*.85);
 // Sunrise/sunset: warm forward scatter around a low sun (desert dawns are vivid).
 float low=smoothstep(.35,0.,sun.z)*smoothstep(-.12,.02,sun.z);
 rgb+=radiance*vec3(1.,.55,.25)*.045*low*exp(-gamma*2.2)*(1.+2.*uSceneDesert.x)*(1.-uOvercast);
 // Chergui: suspended sand turns the whole dome a dim orange-brown.
 float dustLum=dot(rgb,vec3(.2126,.7152,.0722));
 rgb=mix(rgb,dustLum*vec3(1.25,.82,.48)*(.6+.4*clamp(d.z*3.+.4,0.,1.)),.85*uSceneDesert.y);
 rgb=mix(vec3(.015,.022,.05),rgb,daylight);
 if(d.z<0.)rgb=mix(rgb,vec3(.025,.022,.018)*daylight,clamp(-d.z*4.,0.,1.));
 // Solar disc uses the exact direct-light colour/intensity; no separate sun tint.
 rgb+=radiance*8.*(1.-uOvercast)*smoothstep(cos(.006),cos(.0045),dot(d,sun));
 // Stable direction-space stars. They do not move with camera translation.
 vec2 starUv=vec2(atan(d.y,d.x)/6.2831853+.5,asin(clamp(d.z,-1.,1.))/SKY_PI+.5)*vec2(2048,1024);
 vec2 cell=floor(starUv);float hash=fract(sin(dot(cell,vec2(127.1,311.7)))*43758.5453);
 float radius=length(fract(starUv)-.5);
 float aa=max(length(fwidth(starUv))*.35,.03);
 // Milky Way: a fixed great circle (tilted galactic plane) with dark-lane
 // mottling; star density rises towards it. Clear desert air shows it best.
 vec3 galacticPole=normalize(vec3(.35,-.6,.72));
 float band=exp(-pow(dot(d,galacticPole)/.18,2.));
 vec2 bandUv=starUv/vec2(2048,1024)*vec2(18.,9.);
 float mottle=fract(sin(dot(floor(bandUv),vec2(41.3,289.1)))*15731.7)*.5
             +fract(sin(dot(floor(bandUv*2.3),vec2(91.7,131.3)))*24631.1)*.5;
 float darkSky=(1.-daylight)*(1.-uOvercast)*(1.-uSceneDesert.y)*step(.015,d.z);
 rgb+=vec3(.55,.6,.75)*.022*band*(.55+.45*mottle)*darkSky*(1.+uSceneDesert.x);
 float threshold=mix(.995,.988,band);
 float star=step(threshold,hash)*(1.-smoothstep(.13-aa,.13+aa,radius));
 rgb+=vec3(star)*(.6+.8*fract(hash*91.3))*step(.015,d.z)*(1.-daylight)*1.5*(1.-uOvercast)*(1.-uSceneDesert.y);
 vec3 moon=normalize(-sun+vec3(.15,0,.18));
 float disc=smoothstep(cos(.006),cos(.0045),dot(d,moon));
 rgb+=vec3(.3,.34,.4)*disc*(1.-daylight)*(1.-uOvercast);
 return max(rgb,vec3(0));
}
