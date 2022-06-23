#include <gtest/gtest.h>
#include <rally/math/vec.h>
#include <stdlib.h>

using namespace rally;

TEST(Vec, Alignment) {
  EXPECT_EQ(alignof(Vec2), 8);
  EXPECT_EQ(alignof(Vec3), 16);
  EXPECT_EQ(alignof(Vec4), 16);
  EXPECT_EQ(alignof(Mat4), 16);
}

TEST(Vec, Size) {
  EXPECT_EQ(sizeof(Vec2), 8);
  EXPECT_EQ(sizeof(Vec3), 16);
  EXPECT_EQ(sizeof(Vec4), 16);
  EXPECT_EQ(sizeof(Mat4), 64);
}

TEST(Vec, VNear) {
  Vec4 a{0.0f, 1.0f, 2.0f, 3.0f};
  Vec4 b{4.0f, 2.0f, 4.3f, 2.0f};
  Vec4 c{4.0f, 3.0f, 6.3f, 5.0f};
  EXPECT_EQ(VNear(a, a), true);
  EXPECT_EQ(VNear(a, b), false);
  EXPECT_EQ(VNear(VAdd(a, b), c), true);
}

inline r32 RandR32(const r32 minf, const r32 maxf) {
  r32 r = ((r32)rand()) / RAND_MAX;
  r = (r * (maxf - minf)) + minf;
  return r;
}

inline Vec4 RandVec4() {
  constexpr r32 kMagnitude = 100.0f;
  return {RandR32(-kMagnitude, kMagnitude), RandR32(-kMagnitude, kMagnitude),
          RandR32(-kMagnitude, kMagnitude), RandR32(-kMagnitude, kMagnitude)};
}

TEST(Vec, VAdd) {
  u32 iters = 100;
  srand(0);
  float af[4], bf[4], cf[4];
  while (iters--) {
    Vec4 a = RandVec4();
    Vec4 b = RandVec4();
    VStore(a, af);
    VStore(b, bf);
    for (u32 i = 0; i < 4; i++) cf[i] = af[i] + bf[i];
    Vec4 cdef = VLoad(cf);
    Vec4 c = VAdd(a, b);
    EXPECT_EQ(VNear(c, cdef), true);
  }
}

TEST(Vec, VDot) {
  u32 iters = 100;
  srand(0);
  float af[4], bf[4];
  while (iters--) {
    Vec4 a = RandVec4();
    Vec4 b = RandVec4();
    VStore(a, af);
    VStore(b, bf);
    r32 exp_ans = 0;
    for (u32 i = 0; i < 4; i++) exp_ans += af[i] * bf[i];
    r32 ans = VDot(a, b);
    EXPECT_LE(abs(ans - exp_ans), eps);
  }
}

inline Mat4 RandMat4() {
  Mat4 mat = {RandVec4(), RandVec4(), RandVec4(), RandVec4()};
  return mat;
}

TEST(Vec, VMul) {
  u32 iters = 100;
  srand(0);
  float af[16], bf[4], cf[4];
  while (iters--) {
    Mat4 A = RandMat4();
    MStore(A, (r32*)af);
    Vec4 b = RandVec4();
    VStore(b, bf);

    Vec4 Ab = VMul(A, b);

    for (u32 i = 0; i < 4; i++) {
      cf[i] = 0.0f;
      for (u32 j = 0; j < 4; j++) {
        cf[i] += af[j * 4 + i] * bf[j];
      }
    }
    Vec4 manual_Ab = VLoad(cf);

    EXPECT_EQ(VNear(manual_Ab, Ab), true);
  }
}

TEST(Mat, MMul) {
  u32 iters = 100;
  srand(0);
  r32 af[16], bf[16], cf[16];
  while (iters--) {
    Mat4 A = RandMat4();
    MStore(A, af);
    Mat4 B = RandMat4();
    MStore(B, bf);

    Mat4 AB = MMul(A, B);

    for (u32 i = 0; i < 4; i++) {
      for (u32 j = 0; j < 4; j++) {
        cf[4 * i + j] = 0.0f;
        for (u32 k = 0; k < 4; k++) {
          cf[4 * i + j] += af[k * 4 + j] * bf[i * 4 + k];
        }
      }
    }
    Mat4 manual_AB = MLoad(cf);

    EXPECT_EQ(MNear(AB, manual_AB), true);
  }
}

TEST(Mat, LUDecomposition) {
  u32 iters = 100;
  srand(0);
  r32 lf[16], uf[16];
  while (iters--) {
    Mat4 A = RandMat4();
    Mat4 L, U;
    LUDecomposition(A, L, U);
    MStore(L, lf);
    MStore(U, uf);
    Mat4 LU = MMul(L, U);
    for (u32 i = 0; i < 4; i++) {
      for (u32 j = 0; j < 4; j++) {
        u32 p = i * 4 + j;
        if (i < j) {
          EXPECT_LE(abs(uf[p]), eps);
        } else if (i == j) {
          EXPECT_LE(abs(lf[p] - 1.0f), eps);
        } else {
          EXPECT_LE(abs(lf[p]), eps);
        }
      }
    }
    EXPECT_EQ(MNear(LU, A), true);
  }
}

inline Mat4 RandL() {
  constexpr r32 kMagnitude = 100.0f;
  r32 lf[16];
  for (u32 i = 0; i < 4; i++) {
    for (u32 j = 0; j < 4; j++) {
      if (i <= j) {
        lf[i * 4 + j] = RandR32(-kMagnitude, kMagnitude);
      } else
        lf[i * 4 + j] = 0;
    }
  }
  return MLoad(lf);
}

TEST(Mat, ForwardSubstitution) {
  u32 iters = 100;
  srand(0);
  r32 lf[16];
  while (iters--) {
    Mat4 L = RandL();
    Vec4 b = RandVec4();
    Vec4 x = ForwardSubstitution(L, b);
    Vec4 Lx = VMul(L, x);
    EXPECT_EQ(VNear(Lx, b), true);
  }
}

inline Mat4 RandU() {
  constexpr r32 kMagnitude = 100.0f;
  r32 uf[16];
  for (u32 i = 0; i < 4; i++) {
    for (u32 j = 0; j < 4; j++) {
      if (i >= j) {
        uf[i * 4 + j] = RandR32(-kMagnitude, kMagnitude);
      } else {
        uf[i * 4 + j] = 0;
      }
    }
  }
  return MLoad(uf);
}

TEST(Mat, BackSubstitution) {
  u32 iters = 100;
  srand(0);
  r32 lf[16];
  while (iters--) {
    Mat4 U = RandU();
    Vec4 b = RandVec4();
    Vec4 x = BackSubstitution(U, b);
    Vec4 Ux = VMul(U, x);
    EXPECT_EQ(VNear(Ux, b), true);
  }
}

TEST(Mat, MInverse) {
  u32 iters = 100;
  srand(0);
  while (iters--) {
    Mat4 A = RandMat4();
    Mat4 AI = MInverse(A);
    Mat4 AAI = MMul(A, AI);
    Mat4 AIA = MMul(AI, A);
    EXPECT_EQ(MNear(AAI, kIdentity), true);
    EXPECT_EQ(MNear(AIA, kIdentity), true);
  }
}