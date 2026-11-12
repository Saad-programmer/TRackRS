// Original splat material: layers 0-8 = data/terrain/pbr/surfaces.json, layers
// 9-17 = the desert set (data/terrain/desert, amendment A3.1): sand_dune,
// fesh_fesh, hard_sand, hamada_stone, oued_gravel, dry_lake_clay,
// palmeraie_dirt, rock, tarmac_n9. Desert sand gets wind ripples (normal
// detail aligned with uDesertWind), slip-face darkening (steep leeward
// slopes) and grain sparkle (terrainGlint, view-dependent sun glints).
uniform sampler2DArray uTerrainAlbedo;
uniform sampler2DArray uTerrainNormal;
uniform sampler2DArray uTerrainOrm;
uniform usampler2D uTerrainTypes;
uniform float uWorldPeriod;
uniform vec4 uDesertWind;   // xy downwind direction, z strength 0..1
const int TERRAIN_LAYERS = 18;
struct TerrainSurface { vec3 albedo; vec3 normal; vec3 orm; float puddleable; float sparkle; };
vec3 terrainTri(sampler2DArray map, vec3 p, float layer, vec3 weights, vec3 dx, vec3 dy) {
    return textureGrad(map,vec3(p.yz,layer),dx.yz,dy.yz).rgb*weights.x
          +textureGrad(map,vec3(p.xz,layer),dx.xz,dy.xz).rgb*weights.y
          +textureGrad(map,vec3(p.xy,layer),dx.xy,dy.xy).rgb*weights.z;
}
TerrainSurface terrainMaterial(vec3 world, vec3 normal) {
    ivec2 size=textureSize(uTerrainTypes,0);
    vec2 pixel=world.xy/uWorldPeriod*vec2(size);
    ivec2 base=ivec2(floor(pixel));
    vec2 blend=fract(pixel);
    float surface[TERRAIN_LAYERS];for(int i=0;i<TERRAIN_LAYERS;++i)surface[i]=0.0;
    for(int y=0;y<2;++y)for(int x=0;x<2;++x) {
        ivec2 p=((base+ivec2(x,y))%size+size)%size;
        int layer=clamp(int(texelFetch(uTerrainTypes,p,0).r),0,TERRAIN_LAYERS-1);
        surface[layer]+=(x==0?1.0-blend.x:blend.x)*(y==0?1.0-blend.y:blend.y);
    }
    vec3 tri=pow(abs(normal),vec3(4));tri/=max(dot(tri,vec3(1)),0.00001);
    TerrainSurface result;result.albedo=vec3(0);result.orm=vec3(0);
    vec3 detail=vec3(0);
    // Low-frequency world-space variation is continuous across tiles/LODs.
    float macro=0.88+0.08*sin(dot(world.xy,vec2(0.009,0.013)))+0.04*cos(dot(world.xy,vec2(0.031,-0.017)));
    vec3 uv=world*2.0; // half-metre grain tiles; macro variation remains in world space
    // Gradients are computed before the non-uniform material loop. Implicit
    // derivatives inside divergent splat branches are undefined in GLSL.
    vec3 dx=dFdx(uv),dy=dFdy(uv);
    // Wind ripples (5-25 cm crests across the wind) are evaluated in uniform
    // control flow so their derivative-based distance fade is well defined.
    vec2 wind=uDesertWind.xy/max(length(uDesertWind.xy),1e-4);
    float warp=sin(dot(world.xy,vec2(0.37,0.11)))*1.7+sin(dot(world.xy,vec2(-0.13,0.29)))*1.3;
    float ripplePhase=dot(world.xy,wind)*(6.2831853/0.16)+warp;
    float rippleFade=1.0-smoothstep(0.7,2.4,length(vec2(dFdx(ripplePhase),dFdy(ripplePhase))));
    for(int layer=0;layer<TERRAIN_LAYERS;++layer)if(surface[layer]>0.00001) {
        float w=surface[layer];
        result.albedo+=terrainTri(uTerrainAlbedo,uv,float(layer),tri,dx,dy)*w;
        result.orm+=terrainTri(uTerrainOrm,uv,float(layer),tri,dx,dy)*w;
        vec2 nx=textureGrad(uTerrainNormal,vec3(uv.yz,float(layer)),dx.yz,dy.yz).rg*2.0-1.0;
        vec2 ny=textureGrad(uTerrainNormal,vec3(uv.xz,float(layer)),dx.xz,dy.xz).rg*2.0-1.0;
        vec2 nz=textureGrad(uTerrainNormal,vec3(uv.xy,float(layer)),dx.xy,dy.xy).rg*2.0-1.0;
        detail+=(vec3(0,nx)*tri.x+vec3(ny.x,0,ny.y)*tri.y+vec3(nz,0)*tri.z)*w;
    }
    result.albedo*=macro;
    // Desert sand (amendment A3.1).
    float duneSand=surface[9]+0.6*surface[11];
    float slope=1.0-normal.z;
    float lee=dot(normal.xy/max(length(normal.xy),1e-4),wind);
    float strength=mix(0.6,1.0,uDesertWind.z);
    detail+=vec3(wind*cos(ripplePhase)*0.32*strength*rippleFade*duneSand*(1.0-smoothstep(0.3,0.5,slope)),0.0);
    result.albedo*=1.0-0.24*duneSand*smoothstep(0.22,0.5,slope)*smoothstep(0.0,0.6,lee);   // slip faces
    result.sparkle=(surface[9]+surface[11]+0.3*surface[10])*(1.0-smoothstep(0.4,0.7,slope));
    // Standing water collects on tarmac, gravel, dirt and mud, not grass/sand/snow/ice.
    result.puddleable=surface[0]+surface[1]+surface[2]+surface[7];
    result.normal=normalize(normal+detail*0.55);
    return result;
}
// Sun glints from individual quartz grains: ~8 % of 3 cm cells carry a facet
// tilted at random; a facet reflecting the sun into the eye flashes. Fades as
// cells shrink below a pixel (no shimmer at range). Returns radiance scale.
float terrainGlint(vec3 world, vec3 N, vec3 V, vec3 L, float mask) {
    if (mask <= 0.0) return 0.0;
    vec2 grain = world.xy * 33.0;
    float footprint = length(fwidth(grain));
    vec2 cell = floor(grain);
    vec3 h = fract(sin(vec3(dot(cell, vec2(127.1, 311.7)), dot(cell, vec2(269.5, 183.3)), dot(cell, vec2(419.2, 371.9)))) * 43758.5453);
    if (h.x < 0.92) return 0.0;
    vec3 facet = normalize(N + (h - 0.5) * 1.1);
    float glint = pow(max(dot(reflect(-L, facet), V), 0.0), 900.0);
    return glint * mask * (1.0 - smoothstep(0.35, 1.2, footprint)) * max(dot(N, L), 0.0);
}
