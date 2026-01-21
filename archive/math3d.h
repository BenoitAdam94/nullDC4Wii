#ifndef MATH3D_H
#define MATH3D_H

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <limits>

// Constantes mathématiques utiles
const float PI          = 3.14159265358979323846f;
const float PI_OVER_2    = 1.57079632679489661923f;
const float PI_OVER_4    = 0.785398163397448309616f;
const float TWO_PI       = 6.28318530717958647692f;
const float DEG_TO_RAD   = PI / 180.0f;
const float RAD_TO_DEG   = 180.0f / PI;
const float EPSILON      = 1e-6f;

// Clamp une valeur entre min et max
template<typename T>
inline T clamp(T value, T min, T max) {
    return std::max(min, std::min(max, value));
}

// Interpolation linéaire
template<typename T>
inline T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

// Vecteur 2D
struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    // Opérateurs
    Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
    Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
    Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
    Vector2 operator/(float s) const { return Vector2(x / s, y / s); }

    // Produit scalaire
    float dot(const Vector2& v) const { return x * v.x + y * v.y; }

    // Longueur
    float length() const { return std::sqrt(x * x + y * y); }

    // Normalisation
    void normalize() {
        float len = length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
        }
    }

    // Retourne une version normalisée
    Vector2 normalized() const {
        Vector2 v = *this;
        v.normalize();
        return v;
    }
};

// Vecteur 3D
struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Opérateurs
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }

    // Produit scalaire
    float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }

    // Produit vectoriel
    Vector3 cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    // Longueur
    float length() const { return std::sqrt(x * x + y * y + z * z); }

    // Normalisation
    void normalize() {
        float len = length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    // Retourne une version normalisée
    Vector3 normalized() const {
        Vector3 v = *this;
        v.normalize();
        return v;
    }
};

// Vecteur 4D (utilisé pour les transformations homogènes)
struct Vector4 {
    float x, y, z, w;

    Vector4() : x(0), y(0), z(0), w(1) {}
    Vector4(float x, float y, float z, float w = 1.0f) : x(x), y(y), z(z), w(w) {}

    // Opérateurs
    Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4 operator-(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vector4 operator*(float s) const { return Vector4(x * s, y * s, z * s, w * s); }
    Vector4 operator/(float s) const { return Vector4(x / s, y / s, z / s, w / s); }

    // Produit scalaire
    float dot(const Vector4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }

    // Longueur
    float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }

    // Normalisation
    void normalize() {
        float len = length();
        if (len > EPSILON) {
            x /= len;
            y /= len;
            z /= len;
            w /= len;
        }
    }
};

// Matrice 4x4 (pour les transformations 3D)
struct Matrix4x4 {
    float m[4][4];

    // Constructeur (matrice identité par défaut)
    Matrix4x4() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    // Multiplication de matrices
    Matrix4x4 operator*(const Matrix4x4& mat) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i][j] += m[i][k] * mat.m[k][j];
                }
            }
        }
        return result;
    }

    // Transposition
    Matrix4x4 transpose() const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = m[j][i];
            }
        }
        return result;
    }

    // Matrice de translation
    static Matrix4x4 translation(float x, float y, float z) {
        Matrix4x4 mat;
        mat.m[0][3] = x;
        mat.m[1][3] = y;
        mat.m[2][3] = z;
        return mat;
    }

    // Matrice de rotation (angle en radians)
    static Matrix4x4 rotationX(float angle) {
        Matrix4x4 mat;
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[1][1] = c;
        mat.m[1][2] = -s;
        mat.m[2][1] = s;
        mat.m[2][2] = c;
        return mat;
    }

    static Matrix4x4 rotationY(float angle) {
        Matrix4x4 mat;
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c;
        mat.m[0][2] = s;
        mat.m[2][0] = -s;
        mat.m[2][2] = c;
        return mat;
    }

    static Matrix4x4 rotationZ(float angle) {
        Matrix4x4 mat;
        float c = std::cos(angle);
        float s = std::sin(angle);
        mat.m[0][0] = c;
        mat.m[0][1] = -s;
        mat.m[1][0] = s;
        mat.m[1][1] = c;
        return mat;
    }

    // Matrice de mise à l'échelle
    static Matrix4x4 scaling(float x, float y, float z) {
        Matrix4x4 mat;
        mat.m[0][0] = x;
        mat.m[1][1] = y;
        mat.m[2][2] = z;
        return mat;
    }

    // Matrice de perspective (pour la projection 3D)
    static Matrix4x4 perspective(float fov, float aspect, float near, float far) {
        Matrix4x4 mat;
        float f = 1.0f / std::tan(fov * 0.5f);
        mat.m[0][0] = f / aspect;
        mat.m[1][1] = f;
        mat.m[2][2] = (far + near) / (near - far);
        mat.m[2][3] = (2.0f * far * near) / (near - far);
        mat.m[3][2] = -1.0f;
        mat.m[3][3] = 0.0f;
        return mat;
    }
};

// Quaternion (pour les rotations avancées)
struct Quaternion {
    float x, y, z, w;

    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // Normalisation
    void normalize() {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len > EPSILON) {
            x /= len;
            y /= len;
            z /= len;
            w /= len;
        }
    }

    // Normalisation 2 (normaliZED)
    Quaternion normalized() const {
        Quaternion result = *this;
        result.normalize();
        return result;
    }


    // Addition de quaternions
    Quaternion operator+(const Quaternion& q) const {
      return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
    }
    
    // Soustraction de quaternions
    Quaternion operator-(const Quaternion& q) const {
      return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
    }

    // Multiplication de quaternions
    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }

    // Scalar float
    Quaternion operator*(float scalar) const {
      return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
    }


    // Conversion en matrice de rotation
    Matrix4x4 toMatrix() const {
        Matrix4x4 mat;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        mat.m[0][0] = 1.0f - 2.0f * (yy + zz);
        mat.m[0][1] = 2.0f * (xy - wz);
        mat.m[0][2] = 2.0f * (xz + wy);

        mat.m[1][0] = 2.0f * (xy + wz);
        mat.m[1][1] = 1.0f - 2.0f * (xx + zz);
        mat.m[1][2] = 2.0f * (yz - wx);

        mat.m[2][0] = 2.0f * (xz - wy);
        mat.m[2][1] = 2.0f * (yz + wx);
        mat.m[2][2] = 1.0f - 2.0f * (xx + yy);

        return mat;
    }

    // Création d'un quaternion à partir d'un axe et d'un angle (en radians)
    static Quaternion fromAxisAngle(const Vector3& axis, float angle) {
        float halfAngle = angle * 0.5f;
        float s = std::sin(halfAngle);
        return Quaternion(
            axis.x * s,
            axis.y * s,
            axis.z * s,
            std::cos(halfAngle)
        );
    }
};

// Fonctions utilitaires
namespace Math {
    // Interpolation sphérique (pour les quaternions)
    Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    Quaternion qb = b;
      if (dot < 0.0f) {
          qb = Quaternion(-b.x, -b.y, -b.z, -b.w);
          dot = -dot;
      }
      if (dot > 0.9995f) {
          return (a + (qb - a) * t).normalized();
      }
      float theta = std::acos(dot);
      float sinTheta = std::sin(theta);
      float invSinTheta = 1.0f / sinTheta;
      Quaternion result = a * std::sin((1.0f - t) * theta) * invSinTheta + qb * std::sin(t * theta) * invSinTheta;
      return result;
    }
}

#endif // MATH3D_H
