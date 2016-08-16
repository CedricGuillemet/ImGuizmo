#include "imgui.h"
#include "imgui_internal.h"

namespace ImGuizmo
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// utility and math

	void FPU_MatrixF_x_MatrixF(const float *a, const float *b, float *r)
	{
		r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
		r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
		r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
		r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

		r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
		r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
		r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
		r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

		r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
		r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
		r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
		r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

		r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
		r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
		r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
		r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
	}
	template <typename T> T LERP(T x, T y, float z) { return (x + (y - x)*z); }

	struct matrix_t;
	struct vec_t
	{
	public:
		vec_t(const vec_t& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
		vec_t() {}
		vec_t(float _x, float _y, float _z = 0.f, float _w = 0.f) : x(_x), y(_y), z(_z), w(_w) {}
		vec_t(int _x, int _y, int _z = 0, int _w = 0) : x((float)_x), y((float)_y), z((float)_z), w((float)_w) {}
		vec_t(float v) : x(v), y(v), z(v), w(v) {}

		float x, y, z, w;

		void Lerp(const vec_t& v, float t)
		{
			x += (v.x - x) * t;
			y += (v.y - y) * t;
			z += (v.z - z) * t;
			w += (v.w - w) * t;
		}
		/*
		void LerpColor(const vec_t& v, float t)
		{
			for (int i = 0; i < 4; i++)
				(*this)[i] = sqrtf(((*this)[i] * (*this)[i]) * (1.f - t) + (v[i] * v[i]) * (t));
		}
		void Lerp(const vec_t& v, const vec_t& v2, float t)
		{
			*this = v;
			Lerp(v2, t);
		}
		*/
		void set(float v) { x = y = z = w = v; }
		void set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

		vec_t& operator -= (const vec_t& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
		vec_t& operator += (const vec_t& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
		vec_t& operator *= (const vec_t& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
		vec_t& operator *= (float v) { x *= v;	y *= v;	z *= v;	w *= v;	return *this; }

		vec_t operator * (float f) const;
		vec_t operator - () const;
		vec_t operator - (const vec_t& v) const;
		vec_t operator + (const vec_t& v) const;
		vec_t operator * (const vec_t& v) const;

		const vec_t& operator + () const { return (*this); }
		float Length() const { return sqrtf(x*x + y*y + z*z); };
		float LengthSq() const { return (x*x + y*y + z*z); };
		vec_t Normalize() { (*this) *= (1.f / Length()); return (*this); }
		vec_t Normalize(const vec_t& v) { this->set(v.x, v.y, v.z, v.w); this->Normalize(); return (*this); }

		void Cross(const vec_t& v)
		{
			vec_t res;
			res.x = y * v.z - z * v.y;
			res.y = z * v.x - x * v.z;
			res.z = x * v.y - y * v.x;

			x = res.x;
			y = res.y;
			z = res.z;
			w = 0.f;
		}
		void Cross(const vec_t& v1, const vec_t& v2)
		{
			x = v1.y * v2.z - v1.z * v2.y;
			y = v1.z * v2.x - v1.x * v2.z;
			z = v1.x * v2.y - v1.y * v2.x;
			w = 0.f;
		}
		float Dot(const vec_t &v) const
		{
			return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
		}
		float Dot3(const vec_t &v) const
		{
			return (x * v.x) + (y * v.y) + (z * v.z);
		}
		/*
		void isMaxOf(const vec_t& v)
		{
			x = (v.x>x) ? v.x : x;
			y = (v.y>y) ? v.y : y;
			z = (v.z>z) ? v.z : z;
			w = (v.w>w) ? v.z : w;
		}
		void isMinOf(const vec_t& v)
		{
			x = (v.x>x) ? x : v.x;
			y = (v.y>y) ? y : v.y;
			z = (v.z>z) ? z : v.z;
			w = (v.w>w) ? z : v.w;
		}

		bool isInside(const vec_t& min, const vec_t& max) const
		{
			if (min.x > x || max.x < x ||
				min.y > y || max.y < y ||
				min.z > z || max.z < z)
				return false;
			return true;
		}

		vec_t symetrical(const vec_t& v) const
		{
			vec_t res;
			float dist = signedDistanceTo(v);
			res = v;
			res -= (*this)*dist*2.f;

			return res;
		}
		*/
		void Transform(const matrix_t& matrix);
		void Transform(const vec_t & s, const matrix_t& matrix);

		void TransformVector(const matrix_t& matrix);
		void TransformPoint(const matrix_t& matrix);
		void TransformVector(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformVector(matrix); }
		void TransformPoint(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformPoint(matrix); }

		// quaternion slerp
		//void slerp(const vec_t &q1, const vec_t &q2, float t );

		//float signedDistanceTo(const vec_t& point) const;
		//vec_t interpolateHermite(const vec_t &nextKey, const vec_t &nextKeyP1, const vec_t &prevKey, float ratio) const;
		//static float d(const vec_t& v1, const vec_t& v2) { return (v1 - v2).Length(); }
		//static float d2(const vec_t& v1, const vec_t& v2) { return (v1 - v2).LengthSq(); }
		/*
		static vec_t zero;

		uint16 toUInt5551() const { return (uint16_t)(((int)(w*1.f) << 15) + ((int)(z*31.f) << 10) + ((int)(y*31.f) << 5) + ((int)(x*31.f))); }
		void fromUInt5551(unsigned short v) {
			w = (float)((v & 0x8000) >> 15); z = (float)((v & 0x7C00) >> 10) * (1.f / 31.f);
			y = (float)((v & 0x3E0) >> 5) * (1.f / 31.f); x = (float)((v & 0x1F)) * (1.f / 31.f);
		}

		uint32_t toUInt32() const { return ((int)(w*255.f) << 24) + ((int)(z*255.f) << 16) + ((int)(y*255.f) << 8) + ((int)(x*255.f)); }
		void fromUInt32(uint32_t v) {
			w = (float)((v & 0xFF000000) >> 24) * (1.f / 255.f); z = (float)((v & 0xFF0000) >> 16) * (1.f / 255.f);
			y = (float)((v & 0xFF00) >> 8) * (1.f / 255.f); x = (float)((v & 0xFF)) * (1.f / 255.f);
		}

		vec_t swapedRB() const;
		*/
		float& operator [] (size_t index) { return ((float*)&x)[index]; }
		const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
	};

	vec_t vec_t::operator * (float f) const { return vec_t(x * f, y * f, z * f, w *f); }
	vec_t vec_t::operator - () const { return vec_t(-x, -y, -z, -w); }
	vec_t vec_t::operator - (const vec_t& v) const { return vec_t(x - v.x, y - v.y, z - v.z, w - v.w); }
	vec_t vec_t::operator + (const vec_t& v) const { return vec_t(x + v.x, y + v.y, z + v.z, w + v.w); }
	vec_t vec_t::operator * (const vec_t& v) const { return vec_t(x * v.x, y * v.y, z * v.z, w * v.w); }
	//float vec_t::signedDistanceTo(const vec_t& point) const { return (point.Dot(vec_t(x, y, z))) - w; }

	ImVec2 operator+ (const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }

	vec_t Normalized(const vec_t& v) { vec_t res; res = v; res.Normalize(); return res; }
	vec_t Cross(const vec_t& v1, const vec_t& v2)
	{
		vec_t res;
		res.x = v1.y * v2.z - v1.z * v2.y;
		res.y = v1.z * v2.x - v1.x * v2.z;
		res.z = v1.x * v2.y - v1.y * v2.x;
		res.w = 0.f;
		return res;
	}

	float Dot(const vec_t &v1, const vec_t &v2)
	{
		return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
	}

	vec_t BuildPlan(const vec_t & p_point1, const vec_t & p_normal)
	{
		vec_t normal, res;
		normal.Normalize(p_normal);
		res.w = normal.Dot(p_point1);
		res.x = normal.x;
		res.y = normal.y;
		res.z = normal.z;

		return res;
	}

	struct matrix_t
	{
	public:
		union
		{
			float m[4][4];
			float m16[16];
			struct
			{
				vec_t right, up, dir, position;
			};
			struct
			{
				vec_t line[4];
			};
		};
/*
		matrix_t(float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11, float v12, float v13, float v14, float v15, float v16)
		{
			m16[0] = v1; m16[1] = v2; m16[2] = v3; m16[3] = v4; m16[4] = v5; m16[5] = v6; m16[6] = v7; m16[7] = v8; m16[8] = v9; m16[9] = v10; m16[10] = v11; m16[11] = v12; m16[12] = v13;
			m16[13] = v14; m16[14] = v15; m16[15] = v16;
		}
		*/
		matrix_t(const matrix_t& other) { memcpy(&m16[0], &other.m16[0], sizeof(float) * 16); }
		//matrix_t(const vec_t & r, const vec_t &u, const vec_t& d, const vec_t& p) { set(r, u, d, p); }
		matrix_t() {}
		/*
		void set(const vec_t & r, const vec_t &u, const vec_t& d, const vec_t& p) { right = r; up = u; dir = d; position = p; }
		void set(float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11, float v12, float v13, float v14, float v15, float v16)
		{
			*this = matrix_t(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16);
		}
		static matrix_t GetIdentity() {
			return matrix_t(1.f, 0.f, 0.f, 0.f,
				0.f, 1.f, 0.f, 0.f,
				0.f, 0.f, 1.f, 0.f,
				0.f, 0.f, 0.f, 1.f);
		}
		*/
		operator float * () { return m16; }
		operator const float* () const { return m16; }
		void translation(float _x, float _y, float _z) { this->translation(vec_t(_x, _y, _z)); }

		void translation(const vec_t& vt)
		{
			right.set(1.f, 0.f, 0.f, 0.f);
			up.set(0.f, 1.f, 0.f, 0.f);
			dir.set(0.f, 0.f, 1.f, 0.f);
			position.set(vt.x, vt.y, vt.z, 1.f);
		}
		/*
		void translationScale(const vec_t& vt, const vec_t& scale)
		{
			right.set(scale.x, 0.f, 0.f, 0.f);
			up.set(0.f, scale.y, 0.f, 0.f);
			dir.set(0.f, 0.f, scale.z, 0.f);
			position.set(vt.x, vt.y, vt.z, 1.f);
		}

		void rotationY(const float angle)
		{
			float c = cosf(angle);
			float s = sinf(angle);

			right.set(c, 0.f, -s, 0.f);
			up.set(0.f, 1.f, 0.f, 0.f);
			dir.set(s, 0.f, c, 0.f);
			position.set(0.f, 0.f, 0.f, 1.f);
		}

		void rotationX(const float angle)
		{
			float c = cosf(angle);
			float s = sinf(angle);

			right.set(1.f, 0.f, 0.f, 0.f);
			up.set(0.f, c, s, 0.f);
			dir.set(0.f, -s, c, 0.f);
			position.set(0.f, 0.f, 0.f, 1.f);
		}

		void rotationZ(const float angle)
		{
			float c = cosf(angle);
			float s = sinf(angle);

			right.set(c, s, 0.f, 0.f);
			up.set(-s, c, 0.f, 0.f);
			dir.set(0.f, 0.f, 1.f, 0.f);
			position.set(0.f, 0.f, 0, 1.f);
		}
		void scale(float _s)
		{
			right.set(_s, 0.f, 0.f, 0.f);
			up.set(0.f, _s, 0.f, 0.f);
			dir.set(0.f, 0.f, _s, 0.f);
			position.set(0.f, 0.f, 0.f, 1.f);
		}
		void scale(float _x, float _y, float _z)
		{
			right.set(_x, 0.f, 0.f, 0.f);
			up.set(0.f, _y, 0.f, 0.f);
			dir.set(0.f, 0.f, _z, 0.f);
			position.set(0.f, 0.f, 0.f, 1.f);
		}
		void scale(const vec_t& s) { scale(s.x, s.y, s.z); }
		*/
		matrix_t& operator *= (const matrix_t& mat)
		{
			matrix_t tmpMat;
			tmpMat = *this;
			tmpMat.Multiply(mat);
			*this = tmpMat;
			return *this;
		}
		matrix_t operator * (const matrix_t& mat) const
		{
			matrix_t matT;
			matT.Multiply(*this, mat);
			return matT;
		}

		void Multiply(const matrix_t &matrix)
		{
			matrix_t tmp;
			tmp = *this;

			FPU_MatrixF_x_MatrixF((float*)&tmp, (float*)&matrix, (float*)this);
		}

		void Multiply(const matrix_t &m1, const matrix_t &m2)
		{
			FPU_MatrixF_x_MatrixF((float*)&m1, (float*)&m2, (float*)this);
		}
		/*
		void glhPerspectivef2(float fovyInDegrees, float aspectRatio, float znear, float zfar);
		void glhFrustumf2(float left, float right, float bottom, float top, float znear, float zfar);
		void PerspectiveFovLH2(const float fovy, const float aspect, const float zn, const float zf);
		void OrthoOffCenterLH(const float l, float r, float b, const float t, float zn, const float zf);
		void lookAtRH(const vec_t &eye, const vec_t &at, const vec_t &up);
		void lookAtLH(const vec_t &eye, const vec_t &at, const vec_t &up);
		void LookAt(const vec_t &eye, const vec_t &at, const vec_t &up);
		void rotationQuaternion(const vec_t &q);
		*/
		float GetDeterminant() const
		{
			return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
				m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
		}

		float Inverse(const matrix_t &srcMatrix, bool affine = false);
		float Inverse(bool affine = false);
		void identity() {
			right.set(1.f, 0.f, 0.f, 0.f);
			up.set(0.f, 1.f, 0.f, 0.f);
			dir.set(0.f, 0.f, 1.f, 0.f);
			position.set(0.f, 0.f, 0.f, 1.f);
		}
		void transpose()
		{
			matrix_t tmpm;
			for (int l = 0; l < 4; l++)
			{
				for (int c = 0; c < 4; c++)
				{
					tmpm.m[l][c] = m[c][l];
				}
			}
			(*this) = tmpm;
		}
		/*
		void rotationAxis(const vec_t & axis, float angle);
		void lerp(const matrix_t& r, const matrix_t& t, float s)
		{
			right = LERP(r.right, t.right, s);
			up = LERP(r.up, t.up, s);
			dir = LERP(r.dir, t.dir, s);
			position = LERP(r.position, t.position, s);
		}
		void rotationYawPitchRoll(const float yaw, const float pitch, const float roll);

		void orthoNormalize()
		{
			right.Normalize();
			up.Normalize();
			dir.Normalize();
		}
		*/
		//static matrix_t Identity;
	};

	void vec_t::Transform(const matrix_t& matrix)
	{
		vec_t out;

		out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + w * matrix.m[3][0];
		out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + w * matrix.m[3][1];
		out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + w * matrix.m[3][2];
		out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + w * matrix.m[3][3];

		x = out.x;
		y = out.y;
		z = out.z;
		w = out.w;
	}

	void vec_t::Transform(const vec_t & s, const matrix_t& matrix)
	{
		*this = s;
		Transform(matrix);
	}

	void vec_t::TransformPoint(const matrix_t& matrix)
	{
		vec_t out;

		out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
		out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
		out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
		out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];

		x = out.x;
		y = out.y;
		z = out.z;
		w = out.w;
	}

	float matrix_t::Inverse(const matrix_t &srcMatrix, bool affine)
	{
		float det = 0;

		if (affine)
		{
			det = GetDeterminant();
			float s = 1 / det;
			m[0][0] = (srcMatrix.m[1][1] * srcMatrix.m[2][2] - srcMatrix.m[1][2] * srcMatrix.m[2][1]) * s;
			m[0][1] = (srcMatrix.m[2][1] * srcMatrix.m[0][2] - srcMatrix.m[2][2] * srcMatrix.m[0][1]) * s;
			m[0][2] = (srcMatrix.m[0][1] * srcMatrix.m[1][2] - srcMatrix.m[0][2] * srcMatrix.m[1][1]) * s;
			m[1][0] = (srcMatrix.m[1][2] * srcMatrix.m[2][0] - srcMatrix.m[1][0] * srcMatrix.m[2][2]) * s;
			m[1][1] = (srcMatrix.m[2][2] * srcMatrix.m[0][0] - srcMatrix.m[2][0] * srcMatrix.m[0][2]) * s;
			m[1][2] = (srcMatrix.m[0][2] * srcMatrix.m[1][0] - srcMatrix.m[0][0] * srcMatrix.m[1][2]) * s;
			m[2][0] = (srcMatrix.m[1][0] * srcMatrix.m[2][1] - srcMatrix.m[1][1] * srcMatrix.m[2][0]) * s;
			m[2][1] = (srcMatrix.m[2][0] * srcMatrix.m[0][1] - srcMatrix.m[2][1] * srcMatrix.m[0][0]) * s;
			m[2][2] = (srcMatrix.m[0][0] * srcMatrix.m[1][1] - srcMatrix.m[0][1] * srcMatrix.m[1][0]) * s;
			m[3][0] = -(m[0][0] * srcMatrix.m[3][0] + m[1][0] * srcMatrix.m[3][1] + m[2][0] * srcMatrix.m[3][2]);
			m[3][1] = -(m[0][1] * srcMatrix.m[3][0] + m[1][1] * srcMatrix.m[3][1] + m[2][1] * srcMatrix.m[3][2]);
			m[3][2] = -(m[0][2] * srcMatrix.m[3][0] + m[1][2] * srcMatrix.m[3][1] + m[2][2] * srcMatrix.m[3][2]);
		}
		else
		{
			// transpose matrix
			float src[16];
			for (int i = 0; i<4; ++i)
			{
				src[i] = srcMatrix.m16[i * 4];
				src[i + 4] = srcMatrix.m16[i * 4 + 1];
				src[i + 8] = srcMatrix.m16[i * 4 + 2];
				src[i + 12] = srcMatrix.m16[i * 4 + 3];
			}

			// calculate pairs for first 8 elements (cofactors)
			float tmp[12]; // temp array for pairs
			tmp[0] = src[10] * src[15];
			tmp[1] = src[11] * src[14];
			tmp[2] = src[9] * src[15];
			tmp[3] = src[11] * src[13];
			tmp[4] = src[9] * src[14];
			tmp[5] = src[10] * src[13];
			tmp[6] = src[8] * src[15];
			tmp[7] = src[11] * src[12];
			tmp[8] = src[8] * src[14];
			tmp[9] = src[10] * src[12];
			tmp[10] = src[8] * src[13];
			tmp[11] = src[9] * src[12];

			// calculate first 8 elements (cofactors)
			m16[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
			m16[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
			m16[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
			m16[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
			m16[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
			m16[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
			m16[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
			m16[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

			// calculate pairs for second 8 elements (cofactors)
			tmp[0] = src[2] * src[7];
			tmp[1] = src[3] * src[6];
			tmp[2] = src[1] * src[7];
			tmp[3] = src[3] * src[5];
			tmp[4] = src[1] * src[6];
			tmp[5] = src[2] * src[5];
			tmp[6] = src[0] * src[7];
			tmp[7] = src[3] * src[4];
			tmp[8] = src[0] * src[6];
			tmp[9] = src[2] * src[4];
			tmp[10] = src[0] * src[5];
			tmp[11] = src[1] * src[4];

			// calculate second 8 elements (cofactors)
			m16[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
			m16[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
			m16[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
			m16[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
			m16[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
			m16[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
			m16[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
			m16[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

			// calculate determinant
			det = src[0] * m16[0] + src[1] * m16[1] + src[2] * m16[2] + src[3] * m16[3];

			// calculate matrix inverse
			float invdet = 1 / det;
			for (int j = 0; j<16; ++j)
			{
				m16[j] *= invdet;
			}
		}

		return det;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 
	enum MOVETYPE
	{
		MOVE_NONE,
		MOVE_X,
		MOVE_Y,
		MOVE_Z,
		MOVE_XY,
		MOVE_XZ,
		MOVE_YZ,
		MOVE_SCREEN
	};

	struct Context
	{
		Context() : mbMoving(false), mbEnable(true)
		{
		}

		ImDrawList* mDrawList;

		matrix_t mViewMat;
		matrix_t mProjectionMat;
		matrix_t mModel;
		matrix_t mMVP;
		matrix_t mViewProjection;

		vec_t mCameraEye;
		vec_t mCameraRight;
		vec_t mCameraDir;
		vec_t mRayOrigin;
		vec_t mRayVector;

		ImVec2 mScreenSquareCenter;
		ImVec2 mScreenSquareMin;
		ImVec2 mScreenSquareMax;

		float mScreenFactor;
		vec_t mRelativeOrigin;

		bool mbMoving;
		bool mbEnable;

		vec_t mTranslationPlan;
		vec_t mTranslationPlanOrigin;
		vec_t mMatrixOrigin;

		int mCurrentOperation;
	};

	static Context gContext;

	static const float angleLimit = 0.96f;
	static const float planeLimit = 0.2f;

	static const vec_t direction[3] = { vec_t(1.f,0.f,0.f), vec_t(0.f,1.f,0.f), vec_t(0.f,0.f,1.f) };
	static const ImU32 directionColor[3] = { 0xFF0000AA, 0xFF00AA00, 0xFFAA0000 };
	static const ImU32 selectionColor = 0xFF1080FF;
	static const ImU32 inactiveColor = 0x99999999;
	static const ImU32 translationLineColor = 0xAAAAAAAA;
	static const char *translationInfoMask[] = { "X : %5.2f", "Y : %5.2f", "Z : %5.2f", "X : %5.2f Y : %5.2f", "X : %5.2f Z : %5.2f", "Y : %5.2f Z : %5.2f", "X : %5.2f Y : %5.2f Z : %5.2f" };
	static const int translationInfoIndex[] = { 0,0,0, 1,0,0, 2,0,0, 0,1,0, 0,2,0, 1,2,0, 0,1,2 };
	static const float quadMin = 0.5f;
	static const float quadMax = 0.8f;
	static const float quadUV[8] = { quadMin, quadMin, quadMin, quadMax, quadMax, quadMax, quadMax, quadMin };

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 
	int GetMoveType(vec_t *gizmoHitProportion);

	static ImVec2 worldToPos(const vec_t& worldPos, const matrix_t& mat)
	{
		ImGuiIO& io = ImGui::GetIO();

		vec_t trans;
		trans.TransformPoint(worldPos, mat);
		trans *= 0.5f / trans.w;
		trans += vec_t(0.5f);
		trans.y = 1.f - trans.y;
		trans.x *= io.DisplaySize.x;
		trans.y *= io.DisplaySize.y;
		return ImVec2(trans.x, trans.y);
	}

	static void ComputeCameraRay(vec_t &rayOrigin, vec_t &rayDir)
	{
		ImGuiIO& io = ImGui::GetIO();

		matrix_t mViewProjInverse;
		mViewProjInverse.Inverse(gContext.mViewMat * gContext.mProjectionMat);

		float mox = (io.MousePos.x / io.DisplaySize.x) * 2.f - 1.f;
		float moy = (1.f - (io.MousePos.y / io.DisplaySize.y)) * 2.f - 1.f;

		rayOrigin.Transform(vec_t(mox, moy, 0.f, 1.f), mViewProjInverse);
		rayOrigin *= 1.f / rayOrigin.w;
		vec_t rayEnd;
		rayEnd.Transform(vec_t(mox, moy, 1.f, 1.f), mViewProjInverse);
		rayEnd *= 1.f / rayEnd.w;
		rayDir = Normalized(rayEnd - rayOrigin);
	}

	static float IntersectRayPlane(const vec_t & rOrigin, const vec_t& rVector, const vec_t& plan)
	{
		float numer = plan.Dot3(rOrigin) - plan.w;
		float denom = plan.Dot3(rVector);

		if (fabsf(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
			return -1.0f;

		return -(numer / denom);
	}

	void BeginFrame()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::Begin("gizmo", NULL, io.DisplaySize, 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
		gContext.mDrawList = ImGui::GetWindowDrawList();

		ImGui::End();
	}

	bool IsUsing()
	{
		return gContext.mbMoving;
	}

	bool IsOver()
	{
		return (GetMoveType(NULL) != MOVE_NONE) || IsUsing();
	}

	void Enable(bool enable)
	{
		gContext.mbEnable = enable;
		if (!enable)
			gContext.mbMoving = false;
	}

	float GetUniform(const vec_t& position, const matrix_t& mat)
	{
		vec_t trf(position.x, position.y, position.z, 1.f);
		trf.Transform(gContext.mViewProjection);
		return trf.w;
	}

	static void ComputeContext(const float *view, const float *projection, float *matrix)
	{
		gContext.mViewMat = *(matrix_t*)view;
		gContext.mProjectionMat = *(matrix_t*)projection;
		gContext.mModel = *(matrix_t*)matrix;
		gContext.mViewProjection = gContext.mViewMat * gContext.mProjectionMat;
		gContext.mMVP = gContext.mModel * gContext.mViewProjection;

		matrix_t viewInverse;
		viewInverse.Inverse(gContext.mViewMat);
		gContext.mCameraDir = viewInverse.dir;
		gContext.mCameraEye = viewInverse.position;
		gContext.mCameraRight = viewInverse.right;

		gContext.mScreenFactor = 0.1f * GetUniform(gContext.mModel.position, gContext.mViewProjection);

		ImVec2 centerSSpace = worldToPos(vec_t(0.f), gContext.mMVP);
		gContext.mScreenSquareCenter = centerSSpace;
		gContext.mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
		gContext.mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);

		ComputeCameraRay(gContext.mRayOrigin, gContext.mRayVector);
	}

	static void DrawTranslationGizmo(int type)
	{
		ImDrawList* drawList = gContext.mDrawList;

		// colors
		ImU32 colors[7];
		if (gContext.mbEnable)
		{
			colors[0] = (type == MOVE_SCREEN) ? selectionColor : 0xFFFFFFFF;
			for (int i = 0; i < 3; i++)
			{
				int colorPlaneIndex = (i + 2) % 3;
				colors[i + 1] = (type == (int)(MOVE_X + i)) ? selectionColor : directionColor[i];
				colors[i + 4] = (type == (int)(MOVE_XY + i)) ? selectionColor : directionColor[colorPlaneIndex];
			}
		}
		else
		{
			for (int i = 0; i < sizeof(colors) / sizeof(ImU32); i++)
				colors[i] = inactiveColor;
		}

		// draw screen quad
		drawList->AddCircle(gContext.mScreenSquareCenter, 8.f, colors[0]);

		// draw
		for (unsigned int i = 0; i < 3; i++)
		{
			const int planNormal = (i + 2) % 3;
			const vec_t& dirPlaneX = direction[i];
			const vec_t& dirPlaneY = direction[(i + 1) % 3];
			const vec_t& dirPlaneNormal = direction[planNormal];

			vec_t cameraEyeToGizmo = Normalized(gContext.mModel.position - gContext.mCameraEye);
			const bool belowAxisLimit = (fabsf(cameraEyeToGizmo.Dot3(dirPlaneX)) < angleLimit);
			const bool belowPlaneLimit = (fabsf(cameraEyeToGizmo.Dot3(dirPlaneNormal)) > planeLimit);

			// draw axis
			if (belowAxisLimit)
			{
				ImVec2 baseSSpace = worldToPos(dirPlaneX * 0.1f * gContext.mScreenFactor, gContext.mMVP);
				ImVec2 worldDirSSpace = worldToPos(dirPlaneX * gContext.mScreenFactor, gContext.mMVP);

				drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 6.f);
			}

			// draw plane
			if (belowPlaneLimit)
			{
				ImVec2 screenQuadPts[4];
				for (int j = 0; j < 4; j++)
				{
					vec_t cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY  * quadUV[j * 2 + 1]) * gContext.mScreenFactor;
					screenQuadPts[j] = worldToPos(cornerWorldPos, gContext.mMVP);
				}
				drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4], true);
			}
		}
		if (gContext.mbMoving)
		{
			ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
			ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.position, gContext.mViewProjection);
			vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
			dif.Normalize();
			dif *= 5.f;
			drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
			drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
			drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);

			char tmps[512];
			vec_t deltaInfo = gContext.mModel.position - gContext.mMatrixOrigin;
			int componentInfoIndex = (type - MOVE_X) * 3;
			ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
			drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
			drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 

	int GetMoveType(vec_t *gizmoHitProportion)
	{
		ImGuiIO& io = ImGui::GetIO();
		int type = MOVE_NONE;

		// screen
		if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
			io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y)
			type = MOVE_SCREEN;

		// compute
		for (unsigned int i = 0; i < 3 && type == MOVE_NONE; i++)
		{
			const int planNormal = (i + 2) % 3;
			const int nextPlan = (i + 1) % 3;

			vec_t cameraEyeToGizmo = Normalized(gContext.mModel.position - gContext.mCameraEye);
			const bool belowAxisLimit = (fabsf(cameraEyeToGizmo.Dot3(direction[i])) < angleLimit);
			const bool belowPlaneLimit = (fabsf(cameraEyeToGizmo.Dot3(direction[planNormal])) > planeLimit);

			const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModel.position, direction[planNormal]));
			vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

			const float dx = direction[i].Dot3((posOnPlan - gContext.mModel.position) * (1.f / gContext.mScreenFactor));
			const float dy = direction[nextPlan].Dot3((posOnPlan - gContext.mModel.position) * (1.f / gContext.mScreenFactor));
			if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.f)
				type = MOVE_X + i;

			if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
				type = MOVE_XY + i;

			if (gizmoHitProportion)
				*gizmoHitProportion = vec_t(dx, dy, 0.f);
		}
		return type;
	}

	void Translate(const float *view, const float *projection, float *matrix, float *deltaMatrix)
	{
		ImGuiIO& io = ImGui::GetIO();
		ComputeContext(view, projection, matrix);

		// set delta to identity 
		if (deltaMatrix)
			((matrix_t*)deltaMatrix)->identity();
		// -- 
		int type = MOVE_NONE;
		if (gContext.mbEnable)
		{
			// move
			if (gContext.mbMoving)
			{
				const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
				vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;
				vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
				vec_t delta = newOrigin - gContext.mModel.position;
				
				// 1 axis constraint
				if (gContext.mCurrentOperation >= MOVE_X && gContext.mCurrentOperation <= MOVE_Z)
				{
					int axisIndex = gContext.mCurrentOperation - MOVE_X;
					const vec_t& axisValue = gContext.mModel.line[axisIndex];
					float lengthOnAxis = Dot(axisValue, delta);
					delta = axisValue * lengthOnAxis;
				}
				 
				// compute matrix & delta
				gContext.mTranslationPlanOrigin += delta;
				matrix_t deltaMatrixTranslation;
				deltaMatrixTranslation.translation(delta);
				if (deltaMatrix)
					memcpy(deltaMatrix, deltaMatrixTranslation.m16, sizeof(float) * 16);
				matrix_t res = *(matrix_t*)matrix * deltaMatrixTranslation;
				*(matrix_t*)matrix = res;

				if (!io.MouseDown[0])
					gContext.mbMoving = false;

				type = gContext.mCurrentOperation;
			}
			else
			{
				// find new possible way to move
				vec_t gizmoHitProportion;
				type = GetMoveType(&gizmoHitProportion);
				if (io.MouseDown[0] && type != MOVE_NONE)
				{
					gContext.mbMoving = true;
					gContext.mCurrentOperation = type;
					const vec_t movePlanNormal[] = { gContext.mModel.up, gContext.mModel.dir, gContext.mModel.right, gContext.mModel.dir, gContext.mModel.up, gContext.mModel.right, -gContext.mCameraDir };
					// pickup plan
					gContext.mTranslationPlan = BuildPlan(gContext.mModel.position, movePlanNormal[type - MOVE_X]);
					const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
					gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
					gContext.mMatrixOrigin = gContext.mModel.position;

					gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.position) * (1.f / gContext.mScreenFactor);
				}
			}
		}

		DrawTranslationGizmo(type);
	}

};

