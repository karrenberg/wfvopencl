// AO Bench by OpenCL
// imported by kioku @ System K 2009
// 
// about AO Bench[http://lucille.atso-net.jp/aobench/]
//

// scalarized version (replaced all float4 by scalar values)

//const __constant int hwidth = 128;
//const __constant int hheight = 128;
//const __constant float hwidth = 1024.0f*0.5f;
//const __constant float hheight = 1024.0f*0.5f;
struct Ray
{
	float orgX, orgY, orgZ;
	float dirX, dirY, dirZ;
};
struct Sphere
{
	float centerX, centerY, centerZ;
	float radius;
};
struct Plane
{
	float pX, pY, pZ;
	float nX, nY, nZ;
};

struct Intersection
{
    float t;
    float pX, pY, pZ;     // hit point
    float nX, nY, nZ;     // normal
    int hit;
};

//__constant struct Sphere sphere[3];
//__constant struct Plane plane;

void sphere_intersect(const struct Sphere* s, const struct Ray* ray, struct Intersection* isect)
{
    const float rsX = ray->orgX - s->centerX;
    const float rsY = ray->orgY - s->centerY;
    const float rsZ = ray->orgZ - s->centerZ;
    const float B = rsX*ray->dirX + rsY*ray->dirY + rsZ*ray->dirZ; //dot(rs, ray->dir);
    const float C = (rsX*rsX + rsY*rsY + rsZ*rsZ) - (s->radius * s->radius); //dot(rs, rs) - (s->radius * s->radius);
    const float D = B * B - C;

    if (D > 0.0f)
    {
		const float t = -B - sqrt(D);
		if ( (t > 0.0f) && (t < isect->t) )
		{
			isect->t = t;
			isect->hit = 1;

			// calculate normal.
			const float pX = ray->orgX + ray->dirX * t;
			const float pY = ray->orgY + ray->dirY * t;
			const float pZ = ray->orgZ + ray->dirZ * t;
			float nX = pX - s->centerX;
			float nY = pY - s->centerY;
			float nZ = pZ - s->centerZ;
			//n = normalize(n);
			const float ls = nX*nX + nY*nY + nZ*nZ;
			const float l = 1.0f/sqrt(ls);
			nX = nX * l;
			nY = nY * l;
			nZ = nZ * l;
			isect->nX = nX;
			isect->nY = nY;
			isect->nZ = nZ;
			isect->pX = pX;
			isect->pY = pY;
			isect->pZ = pZ;
		}
	}
}

void plane_intersect(const struct Plane* pl, const struct Ray* ray, struct Intersection* isect)
{
  	const float d = 1.0f - (pl->pX*pl->nX + pl->pY*pl->nY + pl->pZ*pl->nZ); //-dot(pl->p, pl->n);
	const float v = ray->dirX*pl->nX + ray->dirY*pl->nY + ray->dirZ*pl->nZ; //dot(ray->dir, pl->n);

	if (fabs(v) < 1.0e-6f)
		return; // the plane is parallel to the ray.

    //const float t = -(dot(ray->org, pl->n) + d) / v;
    const float t = (1.0f - (ray->orgX*pl->nX + ray->orgY*pl->nY + ray->orgZ*pl->nZ + d)) / v; //-(dot(ray->org, pl->n) + d) / v;

    if ( (t > 0.0f) && (t < isect->t) )
    {
		isect->hit = 1;
		isect->t   = t;
		isect->nX   = pl->nX;
		isect->nY   = pl->nY;
		isect->nZ   = pl->nZ;

		const float pX = ray->orgX + t * ray->dirX;
		const float pY = ray->orgY + t * ray->dirY;
		const float pZ = ray->orgZ + t * ray->dirZ;
		isect->pX = pX;
		isect->pY = pY;
		isect->pZ = pZ;
	}
}


void Intersect(const struct Ray* r, struct Intersection* i)
{
	struct Sphere sphere[3];
	sphere[0].centerX = -2.0f;
	sphere[0].centerY = 0.0f;
	sphere[0].centerZ = -3.5f;
	sphere[0].radius = 0.5f;
	sphere[1].centerX = -0.5f;
	sphere[1].centerY = 0.0f;
	sphere[1].centerZ = -3.0f;
	sphere[1].radius = 0.5f;
	sphere[2].centerX = 1.0f;
	sphere[2].centerY = 0.0f;
	sphere[2].centerZ = -2.2f;
	sphere[2].radius = 0.5f;
	struct Plane pl;
    pl.pX = 0.0f;
	pl.pY = -0.5f;
	pl.pZ = 0.0f;
    pl.nX = 0.0f;
	pl.nY = 1.0f;
	pl.nZ = 0.0f;

	//for (int c = 0; c < 3; c++)
	//{
		//sphere_intersect(&sphere[c], r, i);
	//}
	sphere_intersect(&sphere[0], r, i);
	sphere_intersect(&sphere[1], r, i);
	sphere_intersect(&sphere[2], r, i);
	plane_intersect(&pl, r, i);
}

void orthoBasis(float basisX[3], float basisY[3], float basisZ[3], const float nX, const float nY, const float nZ)
{
	basisX[2] = nX;
	basisY[2] = nY;
	basisZ[2] = nZ;
	basisX[1] = 0.0f;
	basisY[1] = 0.0f;
	basisZ[1] = 0.0f;

	if ((nX < 0.6f) && (nX > -0.6f))
		basisX[1] = 1.0f;
	else if ((nY < 0.6f) && (nY > -0.6f))
		basisY[1] = 1.0f;
	else if ((nZ < 0.6f) && (nZ > -0.6f))
		basisZ[1] = 1.0f;
	else
		basisX[1] = 1.0f;


	//basis[0] = cross(basis[1], basis[2]);
	basisX[0] = basisY[1] * basisZ[2] - basisZ[1] * basisY[2];
	basisY[0] = basisZ[1] * basisX[2] - basisX[1] * basisZ[2];
	basisZ[0] = basisX[1] * basisY[2] - basisY[1] * basisX[2];
	//basis[0] = normalize(basis[0]);
	const float ls0 = basisX[0]*basisX[0] + basisY[0]*basisY[0] + basisZ[0]*basisZ[0];
	const float l0 = 1.0f/sqrt(ls0);
	basisX[0] = basisX[0] * l0;
	basisY[0] = basisY[0] * l0;
	basisZ[0] = basisZ[0] * l0;

	//basis[1] = cross(basis[2], basis[0]);
	basisX[1] = basisY[2] * basisZ[0] - basisZ[2] * basisY[0];
	basisY[1] = basisZ[2] * basisX[0] - basisX[2] * basisZ[0];
	basisZ[1] = basisX[2] * basisY[0] - basisY[2] * basisX[0];
	//basis[1] = normalize(basis[1]);
	const float ls1 = basisX[1]*basisX[1] + basisY[1]*basisY[1] + basisZ[1]*basisZ[1];
	const float l1 = 1.0f/sqrt(ls1);
	basisX[1] = basisX[1] * l1;
	basisY[1] = basisY[1] * l1;
	basisZ[1] = basisZ[1] * l1;

}

//float random(float* seed)
//{
//	*seed = (int)(fmod((float)(*seed)*1364.0f+626.0f, 509.0f));
//	return (float)(*seed)/509.0f;
//}

float computeAO(struct Intersection* isect, float* sd)
{
	int i, j;
    const int ntheta = 16;
    const int nphi   = 16;
    float eps  = 0.0001f;

    // Slightly move ray org towards ray dir to avoid numerical probrem.
    float pX = isect->pX + eps * isect->nX;
	float pY = isect->pY + eps * isect->nY;
	float pZ = isect->pZ + eps * isect->nZ;

    // Calculate orthogonal basis.
    float basisX[3], basisY[3], basisZ[3];
    orthoBasis(basisX, basisY, basisZ, isect->nX, isect->nY, isect->nZ);
	
    float occlusion = 0.0f;

    for (j = 0; j < ntheta; j++)
    {
		for (i = 0; i < nphi; i++)
		{
			// Pick a random ray direction with importance sampling.
			// p = cos(theta) / 3.141592f
			*sd = (int)(fmod((float)(*sd)*1364.0f+626.0f, 509.0f));
			const float r = *sd/509.0f;    //(float)(seed)/509.0f;//random(seed);
			*sd = (int)(fmod((float)(*sd)*1364.0f+626.0f, 509.0f));
			const float phi = *sd/509.0f * 2.0f * 3.141592f;  //2.0f * 3.141592f * random();

			const float refX = cos(phi) * sqrt(1.0f - r);
			const float refY = sin(phi) * sqrt(1.0f - r);
			const float refZ = sqrt(r);

			// local -> global
			const float rrayX = refX * basisX[0] + refY * basisX[1] + refZ * basisX[2];
			const float rrayY = refX * basisY[0] + refY * basisY[1] + refZ * basisY[2];
			const float rrayZ = refX * basisZ[0] + refY * basisZ[1] + refZ * basisZ[2];

			struct Ray ray;
			ray.orgX = pX;
			ray.orgY = pY;
			ray.orgZ = pZ;
			ray.dirX = rrayX;
			ray.dirY = rrayY;
			ray.dirZ = rrayZ;

			struct Intersection occIsect;
			occIsect.hit = 0;
			occIsect.t = 1.0e+30f;
			occIsect.nX = occIsect.pX = 0.0f;
			occIsect.nY = occIsect.pY = 0.0f;
			occIsect.nZ = occIsect.pZ = 0.0f;
			Intersect(&ray, &occIsect);
			if (occIsect.hit != 0)
				occlusion += 1.0f;

		}
	}

	// [0.0, 1.0]
	occlusion = ((float)(ntheta * nphi) - occlusion) / (float)(ntheta * nphi);
	return occlusion;//(float4)(occlusion, occlusion, occlusion, 1.0f);
}

__kernel void AmbientOcclusionRenderer(__global uint * out, uint width, uint height) {

	//uint nIndex = get_global_id(0) + get_global_id(1) * get_global_size(0);
	const float hwidth = width/2.0f;
	const float hheight = height/2.0f;

	struct Intersection i;
	i.hit = 0;
	i.t = 1.0e+30f;
	i.nX = i.pX = 0;
	i.nY = i.pY = 0;
	i.nZ = i.pZ = 0;
	
	//float px = ((float)(int)(get_local_id(0) + get_group_id(0) * get_local_size(0)) - hwidth) / hwidth;
	//float py = ((float)(int)(get_local_id(1) + get_group_id(1) * get_local_size(1)) - hheight) / hheight;
	const float px = ((float)(int)(get_global_id(0)) - hwidth) / hwidth;
	const float py = ((float)(int)(get_global_id(1)) - hheight) / hheight;
	//float px = (int)((get_global_id(0)) - hwidth) / hwidth;
	//float py = (int)((get_global_id(1)) - hheight) / hheight;
	//float4 dir = normalize((float4)(px, py, -1.0f, 0.0f));
	const float ls = px*px + py*py + 1.0f;
	const float l = 1.0f/sqrt(ls);
	const float dirX = px * l;
	const float dirY = py * l;
	const float dirZ = -1.0f * l;
	struct Ray r;
	r.orgX = 0;
	r.orgY = 0;
	r.orgZ = 0;
	r.dirX = dirX;
	r.dirY = dirY;
	r.dirZ = dirZ;
	int seed = (int)(fmod((dirX+hwidth) * (dirY+hheight) * 4525434.0f, 65536.0f));
	
	int rcol = 0;
	Intersect(&r, &i);
	if (i.hit != 0)
	{
		float s = seed;
		rcol = (int)(computeAO(&i, &s) * 255);
		seed = s;
	}
	
	//float cx = dirX + 1.0f;
	//float cy = dirY + 1.0f;
	//out[nIndex] = i.hit;
	//out[nIndex] = (int)(cx*128.0f) + (int)(cy*128.0f)*256;
	out[get_global_id(0) + get_global_id(1) * get_global_size(0)] = (int)(rcol | (rcol<<8) | (rcol<<16) | (255<<24));
}
