 /* ************************************************************************* *\
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or 
 nondisclosure agreement with Intel Corporation and may not be copied 
 or disclosed except in accordance with the terms of that agreement. 
 Copyright (C) 2009 Intel Corporation. All Rights Reserved.
 \* ************************************************************************* */

// CONVERSION: w-element ignored

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// helper
float minf(const float a, const float b) {
	return a < b ? a : b;
}
float maxf(const float a, const float b) {
	return a > b ? a : b;
}

/* we need to use this structure to express float3 types in memory */
struct _int3 { int x,y,z; };
struct _float3 { float x,y,z; };

/* triangle structure */
struct Triangle { 
  struct _float3 p0,p1,p2;
};

/* BVH node */
struct BVH {
  struct _float3 lower, upper;
  int left, right;
};

/* ray structure */
struct Ray {
  //float4 org, dir, rcp_dir;
  float orgX, orgY, orgZ;
  float dirX, dirY, dirZ;
  float rcp_dirX, rcp_dirY, rcp_dirZ;
  float min, max;
};

struct Ray create_ray(const float orgX, const float orgY, const float orgZ, const float dirX, const float dirY, const float dirZ) {
	struct Ray ray;
	ray.orgX = orgX;
	ray.orgY = orgY;
	ray.orgZ = orgZ;
	ray.dirX = dirX;
	ray.dirY = dirY;
	ray.dirZ = dirZ;
	ray.min = 0.0001f;
	ray.max = 1E10;
	ray.rcp_dirX = 1.0f/ray.dirX;
	ray.rcp_dirY = 1.0f/ray.dirY;
	ray.rcp_dirZ = 1.0f/ray.dirZ;
	return ray;
}

/* hit structure */
struct Hit { float t,u,v; int tri; };

/* ray/box intersection */
bool intersect_box(const struct Ray ray, const float lowerX, const float lowerY, const float lowerZ, const float upperX, const float upperY, const float upperZ, float* dist) 
{
  //float4 clipMin = (lower - ray.org) * ray.rcp_dir;
  //float4 clipMax = (upper - ray.org) * ray.rcp_dir;
  //float4 minClip = min(clipMin,clipMax);
  //float4 maxClip = max(clipMin,clipMax);
  //float _min = maxf(maxf(minClip.x,maxf(minClip.y,minClip.z)),ray.min);
  //float _max = minf(minf(minf(maxClip.x,minf(maxClip.y,maxClip.z)),ray.max),*dist);
  //bool hit = _min <= _max;
  //*dist = _min;
  //return hit;
  float clipMinX = (lowerX - ray.orgX) * ray.rcp_dirX;
  float clipMinY = (lowerY - ray.orgY) * ray.rcp_dirY;
  float clipMinZ = (lowerZ - ray.orgZ) * ray.rcp_dirZ;
  float clipMaxX = (upperX - ray.orgX) * ray.rcp_dirX;
  float clipMaxY = (upperY - ray.orgY) * ray.rcp_dirY;
  float clipMaxZ = (upperZ - ray.orgZ) * ray.rcp_dirZ;
  float minClipX = minf(clipMinX, clipMaxX);
  float minClipY = minf(clipMinY, clipMaxY);
  float minClipZ = minf(clipMinZ, clipMaxZ);
  float maxClipX = maxf(clipMinX, clipMaxX);
  float maxClipY = maxf(clipMinY, clipMaxY);
  float maxClipZ = maxf(clipMinZ, clipMaxZ);
  float _min = maxf(maxf(minClipX,maxf(minClipY,minClipZ)),ray.min);
  float _max = minf(minf(minf(maxClipX,minf(maxClipY,maxClipZ)),ray.max),*dist);
  bool hit = _min <= _max;
  *dist = _min;
  return hit;
}

/* ray/triangle intersection */
bool intersect_triangle(const struct Ray ray, int tri, float p0X, float p0Y, float p0Z, float p1X, float p1Y, float p1Z, float p2X, float p2Y, float p2Z, struct Hit* hit) 
{
  //float4 E1 = p1 - p0;
  //float4 E2 = p2 - p0;
  //float4 T = ray.org - p0;
  //float4 P = cross(ray.dir,E2);
  //float4 Q = cross(T,E1);
  //float det = dot(P,E1);
  //float rcp_det = 1.0f/det;
  //float t = dot(Q,E2) * rcp_det;
  //float u = dot(P,T) * rcp_det;
  //float v = dot(Q,ray.dir) * rcp_det;
  //bool miss = det == 0.0f | t < ray.min | minf(ray.max,hit->t) < t | minf(minf(u,v),1-u-v) < 0;
  //if (miss) return false;
  //hit->t = t;
  //hit->u = u;
  //hit->v = v;
	//hit->tri = tri;
  //return true;

  float E1X = p1X - p0X;
  float E1Y = p1Y - p0Y;
  float E1Z = p1Z - p0Z;
  float E2X = p2X - p0X;
  float E2Y = p2Y - p0Y;
  float E2Z = p2Z - p0Z;
  float TX = ray.orgX - p0X;
  float TY = ray.orgY - p0Y;
  float TZ = ray.orgZ - p0Z;
  float PX = ray.dirY * E2Z - ray.dirZ * E2Y;
  float PY = ray.dirZ * E2X - ray.dirX * E2Z;
  float PZ = ray.dirX * E2Y - ray.dirY * E2X;
  float QX = TY * E1Z - TZ * E1Y;
  float QY = TZ * E1X - TX * E1Z;
  float QZ = TX * E1Y - TY * E1X;
  float det = PX*E1X + PY*E1Y + PZ*E1Z;
  float rcp_det = 1.0f/det;
  float t = ( QX*E2X + QY*E2Y + QZ*E2Z )  * rcp_det;
  float u = ( PX*TX + PY*TY + PZ*TZ ) * rcp_det;
  float v = ( QX*ray.dirX + QY*ray.dirY + QZ*ray.dirZ ) * rcp_det;
  bool miss = det == 0.0f | t < ray.min | minf(ray.max,hit->t) < t | minf(minf(u,v),1-u-v) < 0;
  if (miss) return false;
  hit->t = t;
  hit->u = u;
  hit->v = v;
	hit->tri = tri;
  return true;
}

/* traversal function */
void traverse(const struct Ray ray, __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos,struct Hit* hit) 
{
  int stack_ptr = 0;
  int stack[64];
  stack[stack_ptr++] = 0;

  while (stack_ptr) 
  {
    struct BVH node = nodes[stack[--stack_ptr]];
    
    if (node.left < 0) { 
      for (int i=0; i<-node.left; i++) {
        struct _int3 tri = tris[node.right+i];
		//intersect_triangle(ray,node.right+i,pos[tri.x],pos[tri.y],pos[tri.z],hit);
		struct _float3 posX = pos[tri.x];
		struct _float3 posY = pos[tri.y];
		struct _float3 posZ = pos[tri.z];
		intersect_triangle(ray,node.right+i,posX.x,posX.y,posX.z,posY.x,posY.y,posY.z,posZ.x,posZ.y,posZ.z,hit);
      }
    } else {
      float ldist = hit->t, rdist = hit->t;
      //bool lhit = intersect_box(ray,nodes[node.left].lower,nodes[node.left].upper,&ldist);
      //bool rhit = intersect_box(ray,nodes[node.right].lower,nodes[node.right].upper,&rdist);
	  struct _float3 nll = nodes[node.left].lower;
	  struct _float3 nlu = nodes[node.left].upper;
	  struct _float3 nrl = nodes[node.right].lower;
	  struct _float3 nru = nodes[node.right].upper;
      bool lhit = intersect_box(ray,nll.x,nll.y,nll.z,nlu.x,nlu.y,nlu.z,&ldist);
      bool rhit = intersect_box(ray,nrl.x,nrl.y,nrl.z,nru.x,nru.y,nru.z,&rdist);
      
      if (lhit && rhit) 
      {
				if (ldist < rdist) {
					stack[stack_ptr++] = node.right;
					stack[stack_ptr++] = node.left;
				} else {
					stack[stack_ptr++] = node.left;
					stack[stack_ptr++] = node.right;
				}
      } else {
				if (lhit) stack[stack_ptr++] = node.left;
				if (rhit) stack[stack_ptr++] = node.right;
      }
    }
  }
}

/* occlusion function */
bool occluded(const struct Ray ray, __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos) 
{
  struct Hit hit; hit.t = 1E10; hit.u = hit.v = 0; hit.tri = -1;
  int stack_ptr = 0;
  int stack[64];
  stack[stack_ptr++] = 0;

  while (stack_ptr) 
  {
    struct BVH node = nodes[stack[--stack_ptr]];
    
    if (node.left < 0) { 
      for (int i=0; i<-node.left; i++) {
        struct _int3 tri = tris[node.right+i];
		struct _float3 posX = pos[tri.x];
		struct _float3 posY = pos[tri.y];
		struct _float3 posZ = pos[tri.z];
		if (intersect_triangle(ray,node.right+i,posX.x,posX.y,posX.z,posY.x,posY.y,posY.z,posZ.x,posZ.y,posZ.z,&hit)) return true;
      }
    } else {
      float ldist = hit.t, rdist = hit.t;
	  struct _float3 nll = nodes[node.left].lower;
	  struct _float3 nlu = nodes[node.left].upper;
	  struct _float3 nrl = nodes[node.right].lower;
	  struct _float3 nru = nodes[node.right].upper;
      bool lhit = intersect_box(ray,nll.x,nll.y,nll.z,nlu.x,nlu.y,nlu.z,&ldist);
      bool rhit = intersect_box(ray,nrl.x,nrl.y,nrl.z,nru.x,nru.y,nru.z,&rdist);
      
      if (lhit && rhit) 
      {
				if (ldist < rdist) {
					stack[stack_ptr++] = node.right;
					stack[stack_ptr++] = node.left;
				} else {
					stack[stack_ptr++] = node.left;
					stack[stack_ptr++] = node.right;
				}
      } else {
				if (lhit) stack[stack_ptr++] = node.left;
				if (rhit) stack[stack_ptr++] = node.right;
      }
    }
  }
	return false;
}

void main_SIMD(__global uint4* dstX, __global uint4* dstY, __global uint4* dstZ, int width, int height, __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand, float4 PX, float4 PY, float4 PZ, float4 UX, float4 UY, float4 UZ, float4 VX, float4 VY, float4 VZ, float4 WX, float4 WY, float4 WZ, int4 ambientOcclusion) {
	pos->x = 3.f;
	return;
};

/* ray generation and framebuffer writeback */
__kernel void main(__global uint* dstX, __global uint* dstY, __global uint* dstZ, int width, int height,
									 __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand,
									 float PX, float PY, float PZ, float UX, float UY, float UZ, float VX, float VY, float VZ, float WX, float WY, float WZ, int ambientOcclusion) 
{
  /* ignore pixels outside the image */
	int ix = get_global_id(0);
	int iy = get_global_id(1);
  if (ix > width || iy > height) return;
  
  /* compute normalized screen position */
  float x = (float)ix/(float)width;
  float y = (float)iy/(float)height;
  
  /* generate and traverse ray */
  struct Hit hit; hit.t = 1E10; hit.u = hit.v = 0; hit.tri = -1;
  float xdirX = x*UX + y*VX + WX;
  float xdirY = x*UY + y*VY + WY;
  float xdirZ = x*UZ + y*VZ + WZ;
  const float dirlen = xdirX*xdirX + xdirY*xdirY + xdirZ*xdirZ;
  const float ld = 1.0f/sqrt(dirlen);
  struct Ray ray = create_ray(PX, PY, PZ, xdirX*ld, xdirY*ld, xdirZ*ld);
  traverse(ray,nodes,tris,pos,&hit);

  /* framebuffer writeback */
  //dst[width * iy + ix] = (uchar4)(255*hit.u,255*hit.v,255*(1.0f-hit.u-hit.v),0);
  dstX[width * iy + ix] = 255*hit.u;
  dstY[width * iy + ix] = 255*hit.v;
  dstZ[width * iy + ix] = 255*(1.0f-hit.u-hit.v);
}

/* hemisphere sampling */
void sample_hemisphere_cosine_distribution(const float NX, const float NY, const float NZ, float r1, float r2, float *retX, float *retY, float *retZ) 
{
	/* create coordinate frame */
  float dxX, dxY, dxZ, dyX, dyY, dyZ; 
  //float4 dx0 = cross((float4)(1.0f,0.0f,0.0f,0.0f),N);
  //float4 dx1 = cross((float4)(0.0f,1.0f,0.0f,0.0f),N);
  float dx0X = 0.0f * NZ - 0.0f * NY;
  float dx0Y = 0.0f * NX - 1.0f * NZ;
  float dx0Z = 1.0f * NY - 0.0f * NX;
  float dx1X = 1.0f * NZ - 0.0f * NY;
  float dx1Y = 0.0f * NX - 0.0f * NZ;
  float dx1Z = 0.0f * NY - 1.0f * NX;
  //bool b = dot(dx0,dx0) > dot(dx1,dx1);
  const float lendx0 = ( dx0X*dx0X + dx0Y*dx0Y + dx0Z*dx0Z );
  bool b = lendx0 > ( dx1X*dx1X + dx1Y*dx1Y + dx1Z*dx1Z );
  //if (b) dx = normalize(dx0); else dx = dx1;
  if (b) {
	const float l = 1.0f/sqrt(lendx0);
	dxX = dx0X * l;
	dxY = dx0Y * l;
	dxZ = dx0Z * l;
  } else {
	dxX = dx1X;
	dxY = dx1Y;
	dxZ = dx1Z;
  }
  //dy = normalize(cross(dx,N));
  float crossdxX = dxY * NZ - dxZ * NY;
  float crossdxY = dxZ * NX - dxX * NZ;
  float crossdxZ = dxX * NY - dxY * NX;
  const float crosslen = crossdxX*crossdxX + crossdxY*crossdxY + crossdxZ*crossdxZ;
  const float l = 1.0f/sqrt(crosslen);
  dyX = crossdxX * l;
  dyY = crossdxY * l;
  dyZ = crossdxZ * l; // not required?

  float k = sqrt(1.0f-r2);
  //float4 s = (float4)(cos(2.0f*M_PI*r1)*k,sin(2.0f*M_PI*r1)*k,sqrt(r2),0.0f);
  //return normalize(s.x*dx + s.y*dy + s.z*N);
  float sX = (cos(2.0f*M_PI*r1)*k);
  float sY = (sin(2.0f*M_PI*r1)*k);
  float sZ = sqrt(r2);
  float svecX = sX * dxX + sY * dyX + sZ * NX;
  float svecY = sY * dxY + sY * dyY + sZ * NY;
  float svecZ = sZ * dxZ + sY * dyZ + sZ * NZ;
  const float slen = sX * sX + sY * sY + sZ * sZ;
  const float sl = 1.0f/sqrt(slen);
  *retX = svecX * sl;
  *retY = svecY * sl;
  *retZ = svecZ * sl;
}

/* ray generation and framebuffer writeback */
__kernel void ambientOcclusion(__global uint* dstX, __global uint* dstY, __global uint* dstZ, int width, int height, 
															 __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand,
															 float PX, float PY, float PZ, float UX, float UY, float UZ, float VX, float VY, float VZ, float WX, float WY, float WZ, int ambientOcclusion) 
{
  /* ignore pixels outside the image */
  int ix = get_global_id(0);
  int iy = get_global_id(1);
  if (ix > width || iy > height) return;
  
  /* compute normalized screen position */
  float x = (float)ix/(float)width;
  float y = (float)iy/(float)height;
  
  /* generate and traverse ray */
  struct Hit hit; hit.t = 1E10; hit.u = hit.v = 0; hit.tri = -1;
  float xdirX = x*UX + y*VX + WX;
  float xdirY = x*UY + y*VY + WY;
  float xdirZ = x*UZ + y*VZ + WZ;
  const float dirlen = xdirX*xdirX + xdirY*xdirY + xdirZ*xdirZ;
  const float ld = 1.0f/sqrt(dirlen);
  struct Ray ray = create_ray(PX, PY, PZ, xdirX*ld, xdirY*ld, xdirZ*ld);
  traverse(ray,nodes,tris,pos,&hit);
  float colorX = hit.u;
  float colorY = hit.v;
  float colorZ = 1.0f-hit.u-hit.v;
  
  /* shoot ambient occlusion rays */
  if (hit.tri >= 0) 
  {
		int num_miss = 0;
		struct _int3 tri = tris[hit.tri];
		struct _float3 posX = pos[tri.x];
		struct _float3 posY = pos[tri.y];
		struct _float3 posZ = pos[tri.z];
		float pos0X = posZ.x - posX.x;
		float pos0Y = posZ.y - posX.y;
		float pos0Z = posZ.z - posX.z;
		float pos1X = posY.x - posX.x;
		float pos1Y = posY.y - posX.y;
		float pos1Z = posY.z - posX.z;
		//float4 N = normalize(cross(pos0,pos1));
		float crosspos0X = pos0Y * pos1Z - pos0Z * pos1Y;
		float crosspos0Y = pos0Z * pos1X - pos0X * pos1Z;
		float crosspos0Z = pos0X * pos1Y - pos0Y * pos1X;
		const float poslen = crosspos0X*crosspos0X + crosspos0Y*crosspos0Y + crosspos0Z*crosspos0Z;
		const float pl = 1.0f/poslen;
		float NX = crosspos0X * pl;
		float NY = crosspos0Y * pl;
		float NZ = crosspos0Z * pl;

		for (int i=0; i<ambientOcclusion; i++) {
			//int r0 = ((2*i+0)*(13*ix+17*iy))%1024, r1 = ((2*i+1)*(13*ix+17*iy))%1024;
			//float4 dir = sample_hemisphere_cosine_distribution(N,rand[r0],rand[r1]);
			//struct Ray ray1 = create_ray(ray.org+0.999f*hit.t*ray.dir,100000.0f*dir);
			//if (!occluded(ray1,nodes,tris,pos)) num_miss++;
			int r0 = ((2*i+0)*(13*ix+17*iy))%1024, r1 = ((2*i+1)*(13*ix+17*iy))%1024;
			float dirX, dirY, dirZ;
			sample_hemisphere_cosine_distribution(NX, NY, NZ, rand[r0],rand[r1], &dirX, &dirY, &dirZ);
			float orgX = ray.orgX+0.99f*hit.t*ray.dirX;
			float orgY = ray.orgY+0.99f*hit.t*ray.dirY;
			float orgZ = ray.orgZ+0.99f*hit.t*ray.dirZ;
			struct Ray ray1 = create_ray(orgX,orgY,orgZ,100000.0f*dirX, 100000.0f*dirY, 100000.0f*dirZ);
			if (!occluded(ray1,nodes,tris,pos)) num_miss++;
		}
		float c = (float)num_miss/(float)ambientOcclusion;
		colorX = c;
		colorY = c;
		colorZ = c;
  }
  
  /* framebuffer writeback */
  //dst[width * iy + ix] = (uchar4)(255*colorX,255*colorY,255*colorZ,0);
  dstX[width * iy + ix] = 255*colorX;
  dstY[width * iy + ix] = 255*colorY;
  dstZ[width * iy + ix] = 255*colorZ;
}

/* ray generation and framebuffer writeback */
__kernel void simplePathTracing(__global uint* dstX, __global uint* dstY, __global uint* dstZ, int width, int height, 
															 __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand,
															 float PX, float PY, float PZ, float UX, float UY, float UZ, float VX, float VY, float VZ, float WX, float WY, float WZ, int maxDepth) 
{
  /* ignore pixels outside the image */
	int ix = get_global_id(0);
	int iy = get_global_id(1);
  if (ix > width || iy > height) return;
  
  /* compute normalized screen position */
  float x = (float)ix/(float)width;
  float y = (float)iy/(float)height;
  
  /* generate and traverse ray */
  struct Hit hit;
  //struct Ray ray = create_ray(P.xyzz,normalize(x*U + y*V + W).xyzz);
  float xdirX = x*UX + y*VX + WX;
  float xdirY = x*UY + y*VY + WY;
  float xdirZ = x*UZ + y*VZ + WZ;
  const float dirlen = xdirX*xdirX + xdirY*xdirY + xdirZ*xdirZ;
  const float ld = 1.0f/sqrt(dirlen);
  struct Ray ray = create_ray(PX, PY, PZ, xdirX*ld, xdirY*ld, xdirZ*ld);

  for (int depth=0; depth<maxDepth; depth++) {
    //hit.t = 1E10; hit.u = hit.v = 0; hit.tri = -1;
    //traverse(ray,nodes,tris,pos,&hit);
    //if (hit.tri == -1) break;
  	//struct _int3 tri = tris[hit.tri];
    //float4 P = ray.org+0.999f*hit.t*ray.dir;
	//float4 N = normalize(cross(_float3_to_float4(pos[tri.z])-_float3_to_float4(pos[tri.x]),_float3_to_float4(pos[tri.y])-_float3_to_float4(pos[tri.x])));
    //float4 Nf; if (dot(-ray.dir,N) < 0) Nf = -N; else Nf = N;
  	//int r0 = ((2*depth+0)*(13*ix+17*iy)+3*depth)%1024, r1 = ((2*depth+1)*(13*ix+17*iy)+5*depth)%1024;
    //float4 D = sample_hemisphere_cosine_distribution(Nf,rand[r0],rand[r1]);
	//if (depth+1<maxDepth) ray = create_ray(P,D);
    
	hit.t = 1E10; hit.u = hit.v = 0; hit.tri = -1;
    traverse(ray,nodes,tris,pos,&hit);
    if (hit.tri == -1) break;
  	struct _int3 tri = tris[hit.tri];
    float PX = ray.orgX+0.999f*hit.t*ray.dirX;
    float PY = ray.orgY+0.999f*hit.t*ray.dirY;
    float PZ = ray.orgZ+0.999f*hit.t*ray.dirZ;
	struct _float3 posX = pos[tri.x];
	struct _float3 posY = pos[tri.y];
	struct _float3 posZ = pos[tri.z];
	float pos0X = posZ.x - posX.x;
	float pos0Y = posZ.y - posX.y;
	float pos0Z = posZ.z - posX.z;
	float pos1X = posY.x - posX.x;
	float pos1Y = posY.y - posX.y;
	float pos1Z = posY.z - posX.z;
	//float4 N = normalize(cross(pos0,pos1));
	float crosspos0X = pos0Y * pos1Z - pos0Z * pos1Y;
	float crosspos0Y = pos0Z * pos1X - pos0X * pos1Z;
	float crosspos0Z = pos0X * pos1Y - pos0Y * pos1X;
	const float poslen = crosspos0X*crosspos0X + crosspos0Y*crosspos0Y + crosspos0Z*crosspos0Z;
	const float pl = 1.0f/poslen;
	float NX = crosspos0X * pl;
	float NY = crosspos0Y * pl;
	float NZ = crosspos0Z * pl;
	float dotdN = (-ray.dirX*NX) + (-ray.dirY*NY) + (-ray.dirZ*NZ);
	if (dotdN < 0) {
		NX = -NX;
		NY = -NY;
		NZ = -NZ;
	}
  	int r0 = ((2*depth+0)*(13*ix+17*iy)+3*depth)%1024, r1 = ((2*depth+1)*(13*ix+17*iy)+5*depth)%1024;
	float DX, DY, DZ;
	sample_hemisphere_cosine_distribution(NX, NY, NZ, rand[r0],rand[r1], &DX, &DY, &DZ);
	if (depth+1<maxDepth) ray = create_ray(PX,PY,PZ,DX,DY,DZ);
  }

  /* framebuffer writeback */
  //dst[width * iy + ix] = (uchar4)(255*hit.u,255*hit.v,255*(1-hit.u-hit.v),0);
  dstX[width * iy + ix] = 255*hit.u;
  dstY[width * iy + ix] = 255*hit.v;
  dstZ[width * iy + ix] = 255*(1.0f-hit.u-hit.v);
}
