// Original nine-surface splat material. Array layer order is surfaces.json.
uniform sampler2DArray uTerrainAlbedo;
uniform sampler2DArray uTerrainNormal;
uniform sampler2DArray uTerrainOrm;
uniform usampler2D uTerrainTypes;
uniform float uWorldPeriod;
struct TerrainSurface { vec3 albedo; vec3 normal; vec3 orm; float puddleable; };
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
    float surface[9];for(int i=0;i<9;++i)surface[i]=0.0;
    for(int y=0;y<2;++y)for(int x=0;x<2;++x) {
        ivec2 p=((base+ivec2(x,y))%size+size)%size;
        int layer=clamp(int(texelFetch(uTerrainTypes,p,0).r),0,8);
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
    for(int layer=0;layer<9;++layer)if(surface[layer]>0.00001) {
        float w=surface[layer];
        result.albedo+=terrainTri(uTerrainAlbedo,uv,float(layer),tri,dx,dy)*w;
        result.orm+=terrainTri(uTerrainOrm,uv,float(layer),tri,dx,dy)*w;
        vec2 nx=textureGrad(uTerrainNormal,vec3(uv.yz,float(layer)),dx.yz,dy.yz).rg*2.0-1.0;
        vec2 ny=textureGrad(uTerrainNormal,vec3(uv.xz,float(layer)),dx.xz,dy.xz).rg*2.0-1.0;
        vec2 nz=textureGrad(uTerrainNormal,vec3(uv.xy,float(layer)),dx.xy,dy.xy).rg*2.0-1.0;
        detail+=(vec3(0,nx)*tri.x+vec3(ny.x,0,ny.y)*tri.y+vec3(nz,0)*tri.z)*w;
    }
    result.albedo*=macro;
    // Standing water collects on tarmac, gravel, dirt and mud, not grass/sand/snow/ice.
    result.puddleable=surface[0]+surface[1]+surface[2]+surface[7];
    result.normal=normalize(normal+detail*0.55);
    return result;
}
