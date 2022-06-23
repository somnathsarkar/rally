#pragma once
#include <rally/types.h>
#include <xmmintrin.h>
namespace rally {
// LU Decomposition is the limiting factor for this epsilon
constexpr r32 eps = 1e-2;

struct Vec2 {
  alignas(8) r32 x;
  r32 y;
};
struct Vec3 {
  __m128 data;
};
struct Vec4 {
  __m128 data;
};
struct Mat4 {
  Vec4 cols[4];
};

constexpr Mat4 kIdentity = {
    {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};

// Are all elements in the containers close? (determined by epsilon)
bool VNear(const Vec4& a, const Vec4& b);
bool MNear(const Mat4& A, const Mat4& B);

// Element-wise addition
Vec4 VAdd(const Vec4& a, const Vec4& b);
void VAdd(const Vec4& a, const Vec4& b, Vec4& out_c);

// Dot product of two vectors
r32 VDot(const Vec4& a, const Vec4& b);

// Matrix-Vector multiplication
Vec4 VMul(const Mat4& A, const Vec4& b);
void VMul(const Mat4& A, const Vec4& b, Vec4& out_Ab);

// Matrix-Matrix multiplication
Mat4 MMul(const Mat4& A, const Mat4& B);
void MMul(const Mat4& A, const Mat4& B, Mat4& out_AB);

// Move vector/matrix contents to float array
// Matrix is stored in column order!
void VStore(const Vec4& a, r32* const out_arr);
void MStore(const Mat4& a, r32* const out_arr);

// Initialize vector/matrix with float array
// Matrix array must be in column order!
Vec4 VLoad(const r32* const arr);
void VLoad(const r32* const arr, Vec4& out_V);
Mat4 MLoad(const r32* const arr);
void MLoad(const r32* const arr, Mat4& out_M);

// Numerical Linear Algebra Routines
// Decompose matrix A into lower-triangular matrix L, upper-triangular matrix U
// Convention: L has ones along diagonal
void LUDecomposition(const Mat4& A, Mat4& out_L, Mat4& out_U);
// Solve Ux=b where U is upper-triangular
Vec4 BackSubstitution(const Mat4& U, const Vec4& b);
void BackSubstitution(const Mat4& U, const Vec4& b, Vec4& out_x);
// Solve Lx=b where L is lower-triangular
Vec4 ForwardSubstitution(const Mat4& L, const Vec4& b);
void ForwardSubstitution(const Mat4& L, const Vec4& b, Vec4& out_x);
// Find Inverse of Matrix A
Mat4 MInverse(const Mat4& A);
void MInverse(const Mat4& A, Mat4& out_AI);

// Matrix Transformation Routines
// View-to-Projection transform for a Perspective camera with fovx radians, and
// width:height aspect_ratio
Mat4 MPerspective(const r32 fovx, const r32 aspect_ratio, const r32 near_plane,
                  const r32 far_plane);
void MPerspective(const r32 fovx, const r32 aspect_ratio, const r32 near_plane,
                  const r32 far_plane, Mat4& out_P);
// Rotation matrix along world x,y,z with rotations applied in that order
Mat4 MRotation(const r32 rotx, const r32 roty, const r32 rotz);
void MRotation(const r32 rotx, const r32 roty, const r32 rotz, Mat4& out_R);
// Translation matrix along world x,y,z
Mat4 MTranslation(const r32 tx, const r32 ty, const r32 tz);
void MTranslation(const r32 tx, const r32 ty, const r32 tz, Mat4& out_T);
// Uniform scale matrix
Mat4 MScale(const r32 x);
void MScale(const r32 x, Mat4& out_S);
}  // namespace rally