#include "pch.h"
#include "triangle.h"

Vector3 Tangent(const Vector3& t, const Vector3& b, const Vector3& n)
{
	auto res = t - (t.DotProduct(n) * n);
	res.Normalize();
	if (n.CrossProduct(res).DotProduct(b) < 0.0f) {
		res = res * -1.0f;
	}
	res.Normalize();
	return res;
}
Triangle::Triangle( const Vertex & v0, const Vertex & v1, const Vertex & v2, Surface * surface )
{
	vertices_[0] = v0;
	vertices_[1] = v1;
	vertices_[2] = v2;	

	const Vector3 e1 = v1.position - v0.position;
	const Vector3 e2 = v2.position - v0.position;

	const float du1 = v1.texture_coords->u - v0.texture_coords->u;
	const float du2 = v2.texture_coords->u - v0.texture_coords->u;
	const float dv1 = v1.texture_coords->v - v0.texture_coords->v;
	const float dv2 = v2.texture_coords->v - v0.texture_coords->v;


	float d = 1.0f/ (du1 * dv2 - du2 * dv1);

	const Vector3 t = d * (dv2 * e1 - dv1 * e2);
	const Vector3 b = d * (-du2 * e1 + du1 * e2);

	vertices_[0].tangent = Tangent(t, b, v0.normal);
	vertices_[1].tangent = Tangent(t, b, v1.normal);
	vertices_[2].tangent = Tangent(t, b, v2.normal);
}

Vertex Triangle::vertex( const int i )
{
	return vertices_[i];
}
