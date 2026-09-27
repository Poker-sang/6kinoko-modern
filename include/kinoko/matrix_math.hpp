#pragma once
#include <cmath>
#include <cstring>
namespace kinoko::math {
struct Matrix {float m[4][4]{};};
inline Matrix identity(){Matrix r;for(int i=0;i<4;++i)r.m[i][i]=1;return r;}
inline Matrix multiply(const Matrix& a,const Matrix& b){Matrix r;for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)r.m[i][j]+=a.m[i][k]*b.m[k][j];return r;}
inline Matrix translation(float x,float y,float z){auto r=identity();r.m[3][0]=x;r.m[3][1]=y;r.m[3][2]=z;return r;}
inline Matrix scaling(float x,float y,float z){Matrix r;r.m[0][0]=x;r.m[1][1]=y;r.m[2][2]=z;r.m[3][3]=1;return r;}
inline Matrix rotation(float yaw,float pitch,float roll){
    auto x=identity(),y=x,z=x;
    x.m[1][1]=x.m[2][2]=std::cos(pitch);x.m[1][2]=std::sin(pitch);x.m[2][1]=-x.m[1][2];
    y.m[0][0]=y.m[2][2]=std::cos(yaw);y.m[2][0]=std::sin(yaw);y.m[0][2]=-y.m[2][0];
    z.m[0][0]=z.m[1][1]=std::cos(roll);z.m[0][1]=std::sin(roll);z.m[1][0]=-z.m[0][1];
    return multiply(multiply(z,x),y);
}
}
