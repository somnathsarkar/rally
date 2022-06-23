#include <math.h>
#include <rally/math/vec.h>

namespace rally {
Vec4 VAdd(const Vec4& a, const Vec4& b) {
  Vec4 out_c;
  VAdd(a, b, out_c);
  return out_c;
}
void VAdd(const Vec4& a, const Vec4& b, Vec4& out_c) {
  out_c.data = _mm_add_ps(a.data, b.data);
}

bool VNear(const Vec4& a, const Vec4& b) {
  __m128 diff = _mm_sub_ps(a.data, b.data);
  r32 fdiff[4];
  _mm_store_ps(fdiff, diff);
  for (u32 i = 0; i < 4; i++) {
    if (fdiff[i] > eps || fdiff[i] < -eps) return false;
  }
  return true;
}

bool MNear(const Mat4& a, const Mat4& b) {
  bool is_near = true;
  for (u32 col_i = 0; col_i < 4; col_i++) {
    is_near &= VNear(a.cols[col_i], b.cols[col_i]);
  }
  return is_near;
}

r32 VDot(const Vec4& a, const Vec4& b) {
  __m128 prod = _mm_mul_ps(a.data, b.data);
  r32 fprod[4];
  _mm_store_ps(fprod, prod);
  r32 ans = 0;
  for (u32 i = 0; i < 4; i++) {
    ans += fprod[i];
  }
  return ans;
}

inline static constexpr unsigned int ShuffleMask(u32 x, u32 y, u32 z, u32 w) {
  return x | (y << 2) | (z << 4) | (w << 6);
}

Vec4 VMul(const Mat4& A, const Vec4& b) {
  Vec4 out_Ab;
  VMul(A, b, out_Ab);
  return out_Ab;
}

void VMul(const Mat4& A, const Vec4& b, Vec4& out_Ab) {
  const __m128 bx = _mm_shuffle_ps(b.data, b.data, ShuffleMask(0, 0, 0, 0));
  const __m128 by = _mm_shuffle_ps(b.data, b.data, ShuffleMask(1, 1, 1, 1));
  const __m128 bz = _mm_shuffle_ps(b.data, b.data, ShuffleMask(2, 2, 2, 2));
  const __m128 bw = _mm_shuffle_ps(b.data, b.data, ShuffleMask(3, 3, 3, 3));
  const __m128 abx = _mm_mul_ps(bx, A.cols[0].data);
  const __m128 aby = _mm_mul_ps(by, A.cols[1].data);
  const __m128 abz = _mm_mul_ps(bz, A.cols[2].data);
  const __m128 abw = _mm_mul_ps(bw, A.cols[3].data);
  out_Ab.data = _mm_add_ps(abx, aby);
  out_Ab.data = _mm_add_ps(out_Ab.data, abz);
  out_Ab.data = _mm_add_ps(out_Ab.data, abw);
}

void VStore(const Vec4& a, r32* const out_arr) {
  _mm_store_ps(out_arr, a.data);
}

Vec4 VLoad(const r32* const arr) {
  Vec4 vec;
  VLoad(arr, vec);
  return vec;
}

void VLoad(const r32* const arr, Vec4& out_V) { out_V.data = _mm_load_ps(arr); }

void MStore(const Mat4& a, r32* const out_arr) {
  for (u32 col_i = 0; col_i < 4; col_i++)
    _mm_store_ps(out_arr + (sizeof(Vec4) / sizeof(r32)) * col_i,
                 a.cols[col_i].data);
}

Mat4 MLoad(const r32* const arr) {
  Mat4 mat;
  MLoad(arr, mat);
  return mat;
}

void MLoad(const r32* const arr, Mat4& out_M) {
  for (u32 col_i = 0; col_i < 4; col_i++) {
    out_M.cols[col_i].data = _mm_load_ps(arr + 4 * col_i);
  }
}

Mat4 MMul(const Mat4& A, const Mat4& B) {
  Mat4 out_AB;
  MMul(A, B, out_AB);
  return out_AB;
}

void MMul(const Mat4& A, const Mat4& B, Mat4& AB) {
  for (u32 i = 0; i < 4; i++) VMul(A, B.cols[i], AB.cols[i]);
}

// Gaussian Elimination without pivoting, numerical stability could be improved
// by adding pivoting. This is a relatively expensive operation, SIMD
// instructions might improve performance.
void LUDecomposition(const Mat4& A, Mat4& out_L, Mat4& out_U) {
  r32 U[16], L[16];
  MStore(A, U);
  MStore(kIdentity, L);
  for (u32 k = 0; k < 3; k++) {
    for (u32 j = k + 1; j < 4; j++) {
      L[j + k * 4] = U[j + k * 4] / U[k + k * 4];
      for (u32 i = k; i < 4; i++) {
        U[j + i * 4] -= L[j + k * 4] * U[k + i * 4];
      }
    }
  }
  MLoad(L, out_L);
  MLoad(U, out_U);
}

// Solve Ux=b, where U is upper triangular
Vec4 BackSubstitution(const Mat4& U, const Vec4& b) {
  Vec4 out_x;
  BackSubstitution(U, b, out_x);
  return out_x;
}

void BackSubstitution(const Mat4& U, const Vec4& b, Vec4& out_x) {
  r32 uf[16];
  r32 bf[4];
  r32 xf[4];
  VStore(b, bf);
  MStore(U, uf);
  for (i32 j = 3; j >= 0; j--) {
    xf[j] = bf[j];
    for (u32 k = j + 1; k < 4; k++) {
      xf[j] -= xf[k] * uf[j + k * 4];
    }
    xf[j] /= uf[j + j * 4];
  }
  VLoad(xf, out_x);
}

Vec4 ForwardSubstitution(const Mat4& L, const Vec4& b) {
  Vec4 out_x;
  ForwardSubstitution(L, b, out_x);
  return out_x;
}

void ForwardSubstitution(const Mat4& L, const Vec4& b, Vec4& out_x) {
  r32 lf[16];
  r32 bf[4];
  r32 xf[4];
  VStore(b, bf);
  MStore(L, lf);
  for (u32 j = 0; j < 4; j++) {
    xf[j] = bf[j];
    for (u32 k = 0; k < j; k++) {
      xf[j] -= xf[k] * lf[j + k * 4];
    }
    xf[j] /= lf[j + j * 4];
  }
  VLoad(xf, out_x);
}

// Solve Ax = b, where A = LU is LU decomposition of A
static void MSolveLU(const Mat4& L, const Mat4& U, const Vec4& b, Vec4& out_x) {
  Vec4 y;
  ForwardSubstitution(L, b, y);
  BackSubstitution(U, y, out_x);
}

// Find Inverse of Matrix A
Mat4 MInverse(const Mat4& A) {
  Mat4 AI;
  MInverse(A, AI);
  return AI;
}

void MInverse(const Mat4& A, Mat4& AI) {
  Mat4 L, U;
  LUDecomposition(A, L, U);
  for (u32 col_i = 0; col_i < 4; col_i++) {
    MSolveLU(L, U, kIdentity.cols[col_i], AI.cols[col_i]);
  }
}

// View-to-Projection transform for a Perspective camera with fovx radians, and
// width:height aspect_ratio
Mat4 MPerspective(const r32 fovx, const r32 aspect_ratio, const r32 near_plane,
                  const r32 far_plane) {
  Mat4 out_P;
  MPerspective(fovx, aspect_ratio, near_plane, far_plane, out_P);
  return out_P;
}
void MPerspective(const r32 fovx, const r32 aspect_ratio, const r32 near_plane,
                  const r32 far_plane, Mat4& out_P) {
  r32 halfcot = 1.0f / tanf(fovx / 2.0f);
  float P[16] = {halfcot,
                 0.0f,
                 0.0f,
                 0.0f,  // Column 1
                 0.0f,
                 aspect_ratio * halfcot,
                 0.0f,
                 0.0f,  // Column 2
                 0.0f,
                 0.0f,
                 far_plane / (far_plane - near_plane),
                 1.0f,  // Column 3
                 0.0f,
                 0.0f,
                 -(near_plane * far_plane) / (far_plane - near_plane),
                 0.0f};
  MLoad(P, out_P);
}

// Rotation matrix along x,y,z with rotations applied in that order
Mat4 MRotation(const r32 rotx, const r32 roty, const r32 rotz) {
  Mat4 out_R;
  MRotation(rotx, roty, rotz, out_R);
  return out_R;
}

void MRotation(const r32 rotx, const r32 roty, const r32 rotz, Mat4& out_R) {
  r32 cz = cos(rotz), sz = sin(rotz);
  r32 cy = cos(roty), sy = sin(roty);
  r32 cx = cos(rotx), sx = sin(rotx);
  r32 R[16] = {cz * cy, //cy
               sz * cy, // 0
               -sy, // -sy
               0.0f,  // Column 1
               cz * sy * sx - sz * cx, // 0
               sz * sy * sx + cz * cx, // 1
               cy * sx, // 0
               0.0f,  // Column 2 // 0
               cz * sy * cx + sz * sx, // sy
               sz * sy * cx - cz * sx, // 0
               cy * cx, // cy
               0.0f,  // Column 3
               0.0f,
               0.0f,
               0.0f,
               1.0f};
  MLoad(R, out_R);
}

// Translation matrix along world x,y,z
Mat4 MTranslation(const r32 tx, const r32 ty, const r32 tz) {
  Mat4 out_T;
  MTranslation(tx, ty, tz, out_T);
  return out_T;
}
void MTranslation(const r32 tx, const r32 ty, const r32 tz, Mat4& out_T) {
  r32 tcol[4] = {tx, ty, tz, 1.0f};
  out_T = kIdentity;
  VLoad(tcol, out_T.cols[3]);
}

// Uniform scale matrix
Mat4 MScale(const r32 x) {
  Mat4 out_S;
  MScale(x, out_S);
  return out_S;
}
void MScale(const r32 x, Mat4& out_S) {
  r32 S[16] = {x, 0, 0, 0, 0, x, 0, 0, 0, 0, x, 0, 0, 0, 0, 1};
  MLoad(S, out_S);
}
}  // namespace rally