 /* ************************************************************************* *\
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or 
 nondisclosure agreement with Intel Corporation and may not be copied 
 or disclosed except in accordance with the terms of that agreement. 
 Copyright (C) 2009 Intel Corporation. All Rights Reserved.
 \* ************************************************************************* */

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* we need to use this structure to express float3 types in memory */
struct _int3 { int x,y,z; };
struct _float3 { float x,y,z; };
float4 _float3_to_float4(struct _float3 v) { return (float4)(v.x,v.y,v.z,0.0f); };

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
  float4 org, dir, rcp_dir;
  float min, max;
};

struct Ray create_ray(const float4 org, const float4 dir) {
	struct Ray ray; ray.org = org; ray.dir = dir; ray.min = 0.0001f; ray.max = 1E10;
	ray.rcp_dir = 1.0f/ray.dir;
	return ray;
}

/* hit structure */
struct Hit { float t,u,v; int tri; };

/* ray/box intersection */
bool intersect_box(const struct Ray ray, const float4 lower, const float4 upper, float* dist) 
{
  float4 clipMin = (lower - ray.org) * ray.rcp_dir;
  float4 clipMax = (upper - ray.org) * ray.rcp_dir;
  float4 minClip = min(clipMin,clipMax);
  float4 maxClip = max(clipMin,clipMax);
  float _min = max(max(minClip.x,max(minClip.y,minClip.z)),ray.min);
  float _max = min(min(min(maxClip.x,min(maxClip.y,maxClip.z)),ray.max),*dist);
  bool hit = _min <= _max;
  *dist = _min;
  return hit;
}

/* ray/triangle intersection */
bool intersect_triangle(const struct Ray ray, int tri, float4 p0, float4 p1, float4 p2, struct Hit* hit) 
{
  float4 E1 = p1 - p0;
  float4 E2 = p2 - p0;
  float4 T = ray.org - p0;
  float4 P = cross(ray.dir,E2);
  float4 Q = cross(T,E1);
  float det = dot(P,E1);
  float rcp_det = 1.0f/det;
  float t = dot(Q,E2) * rcp_det;
  float u = dot(P,T) * rcp_det;
  float v = dot(Q,ray.dir) * rcp_det;
  bool miss = det == 0.0f | t < ray.min | min(ray.max,hit->t) < t | min(min(u,v),1-u-v) < 0;
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
				intersect_triangle(ray,node.right+i,_float3_to_float4(pos[tri.x]),_float3_to_float4(pos[tri.y]),_float3_to_float4(pos[tri.z]),hit);
      }
    } else {
      float ldist = hit->t, rdist = hit->t;
      bool lhit = intersect_box(ray,_float3_to_float4(nodes[node.left].lower),_float3_to_float4(nodes[node.left].upper),&ldist);
      bool rhit = intersect_box(ray,_float3_to_float4(nodes[node.right].lower),_float3_to_float4(nodes[node.right].upper),&rdist);
      
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
				if (intersect_triangle(ray,node.right+i,_float3_to_float4(pos[tri.x]),_float3_to_float4(pos[tri.y]),_float3_to_float4(pos[tri.z]),&hit)) return true;
      }
    } else {
      float ldist = hit.t, rdist = hit.t;
      bool lhit = intersect_box(ray,_float3_to_float4(nodes[node.left].lower),_float3_to_float4(nodes[node.left].upper),&ldist);
      bool rhit = intersect_box(ray,_float3_to_float4(nodes[node.right].lower),_float3_to_float4(nodes[node.right].upper),&rdist);
      
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

/* ray generation and framebuffer writeback */
__kernel void main(__global uchar4 *dst, int width, int height,
									 __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand,
									 float4 P, float4 U, float4 V, float4 W, int ambientOcclusion) 
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
  struct Ray ray = create_ray(P.xyzz,normalize(x*U + y*V + W).xyzz);
  traverse(ray,nodes,tris,pos,&hit);

  /* framebuffer writeback */
  dst[width * iy + ix] = (uchar4)(255*hit.u,255*hit.v,255*(1.0f-hit.u-hit.v),0);
}

/* hemisphere sampling */
float4 sample_hemisphere_cosine_distribution(const float4 N, float r1, float r2) 
{
	/* create coordinate frame */
  float4 dx, dy; 
  float4 dx0 = cross((float4)(1.0f,0.0f,0.0f,0.0f),N);
  float4 dx1 = cross((float4)(0.0f,1.0f,0.0f,0.0f),N);
  bool b = dot(dx0,dx0) > dot(dx1,dx1);
  if (b) dx = normalize(dx0); else dx = dx1;
  dy = normalize(cross(dx,N));

  float k = sqrt(1.0f-r2);
  float4 s = (float4)(cos(2.0f*M_PI*r1)*k,sin(2.0f*M_PI*r1)*k,sqrt(r2),0.0f);
  return normalize(s.x*dx + s.y*dy + s.z*N);
}

/* ray generation and framebuffer writeback */
__kernel void ambientOcclusion(__global uchar4* dst, int width, int height, 
															 __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand,
															 float4 P, float4 U, float4 V, float4 W, int ambientOcclusion) 
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
  struct Ray ray = create_ray(P.xyzz,normalize(x*U + y*V + W).xyzz);
  traverse(ray,nodes,tris,pos,&hit);
	float4 color = (float4)(hit.u,hit.v,1.0f-hit.u-hit.v,0);

	/* shoot ambient occlusion rays */
	if (hit.tri >= 0) 
	{
		int num_miss = 0;
		struct _int3 tri = tris[hit.tri];
		float4 N = normalize(cross(_float3_to_float4(pos[tri.z])-_float3_to_float4(pos[tri.x]),_float3_to_float4(pos[tri.y])-_float3_to_float4(pos[tri.x])));
		for (int i=0; i<ambientOcclusion; i++) {
			int r0 = ((2*i+0)*(13*ix+17*iy))%1024, r1 = ((2*i+1)*(13*ix+17*iy))%1024;
			float4 dir = sample_hemisphere_cosine_distribution(N,rand[r0],rand[r1]);
			struct Ray ray1 = create_ray(ray.org+0.999f*hit.t*ray.dir,100000.0f*dir);
			if (!occluded(ray1,nodes,tris,pos)) num_miss++;
		}
		float c = (float)num_miss/(float)ambientOcclusion;
		color = (float4)(c,c,c,c);
	}

  /* framebuffer writeback */
  dst[width * iy + ix] = (uchar4)(255*color.x,255*color.y,255*color.z,0);
}

/* ray generation and framebuffer writeback */
__kernel void simplePathTracing(__global uchar4* dst, int width, int height, 
															 __global struct BVH* nodes, __global struct _int3* tris, __global struct _float3* pos, __global float* rand,
															 float4 P, float4 U, float4 V, float4 W, int maxDepth) 
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
  struct Ray ray = create_ray(P.xyzz,normalize(x*U + y*V + W).xyzz);

  for (int depth=0; depth<maxDepth; depth++) {
    hit.t = 1E10; hit.u = hit.v = 0; hit.tri = -1;
    traverse(ray,nodes,tris,pos,&hit);
    if (hit.tri == -1) break;
  	struct _int3 tri = tris[hit.tri];
    float4 P = ray.org+0.999f*hit.t*ray.dir;
		float4 N = normalize(cross(_float3_to_float4(pos[tri.z])-_float3_to_float4(pos[tri.x]),_float3_to_float4(pos[tri.y])-_float3_to_float4(pos[tri.x])));
    float4 Nf; if (dot(-ray.dir,N) < 0) Nf = -N; else Nf = N;
  	int r0 = ((2*depth+0)*(13*ix+17*iy)+3*depth)%1024, r1 = ((2*depth+1)*(13*ix+17*iy)+5*depth)%1024;
    float4 D = sample_hemisphere_cosine_distribution(Nf,rand[r0],rand[r1]);
		if (depth+1<maxDepth) ray = create_ray(P,D);
  }

  /* framebuffer writeback */
  dst[width * iy + ix] = (uchar4)(255*hit.u,255*hit.v,255*(1-hit.u-hit.v),0);
}
