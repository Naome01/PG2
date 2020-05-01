#include "pch.h"
#include "camera.h"

Camera::Camera(const int width, const int height, const float fov_y,
	const Vector3 view_from, const Vector3 view_at, float near_plane, float far_plane)
{
	width_ = width;
	height_ = height;
	fov_y_ = fov_y;

	view_from_ = view_from;
	view_at_ = view_at;

	this->near_plane = near_plane;
	this->far_plane = far_plane;
	
	Update(width, height);
}

Vector3 Camera::view_from()
{
	return view_from_;
}

Matrix3x3 Camera::M_c_w() const
{
	return M_c_w_;
}

float Camera::focal_length() const
{
	return f_y_;
}

void Camera::set_fov_y(const float fov_y)
{
	assert(fov_y > 0.0);

	fov_y_ = fov_y;
}

void Camera::Update(int widthN, int heightN)
{
	width_ = widthN;
	height_ = heightN;
	f_y_ = height_ / (2.0f * tanf(this->fov_y_ * 0.5f));

	aspect_ratio = width_ / height_;
	height_half = near_plane * tanf(fov_y_ / 2);
	width_half = height_half * aspect_ratio;

	Vector3 z_c = view_from_ - view_at_;
	z_c.Normalize();
	Vector3 x_c = up_.CrossProduct(z_c);
	x_c.Normalize();
	Vector3 y_c = z_c.CrossProduct(x_c);
	y_c.Normalize();
	M_c_w_ = Matrix3x3(x_c, y_c, z_c);

	m_view = Matrix4x4(x_c, y_c, z_c, view_from_);
	m_projection = Matrix4x4();
	m_projection.set(0, 0, near_plane / (width_half));
	m_projection.set(1, 1, 1.0f * (near_plane / (height_half)));
	m_projection.set(2, 2, (far_plane + near_plane) / (near_plane - far_plane));
	m_projection.set(2, 3, (2.0f * far_plane * near_plane) / (near_plane - far_plane));
	m_projection.set(3, 2, -1.0f);
}

void Camera::MoveForward(const float dt)
{
	Vector3 ds = view_at_ - view_from_;
	ds.Normalize();
	ds *= dt;

	view_from_ += ds;
	view_at_ += ds;
	Update(width_, height_);
}


Matrix4x4 Camera::projection() const
{
	return m_projection;;
}

Matrix4x4 Camera::view()
{
	m_view.EuclideanInverse();
	return m_view;
}
