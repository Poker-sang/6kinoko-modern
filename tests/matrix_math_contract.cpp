#include "kinoko/matrix_math.hpp"
#include <cmath>
int main(){
    using namespace kinoko::math;
    const auto translated=multiply(translation(2,3,4),scaling(5,6,7));
    if(translated.m[3][0]!=10||translated.m[3][1]!=18||translated.m[3][2]!=28||translated.m[3][3]!=1)return 1;
    const auto yaw=rotation(1.57079632679f,0,0),pitch=rotation(0,1.57079632679f,0),roll=rotation(0,0,1.57079632679f);
    if(std::abs(yaw.m[0][2]+1)>1e-6f||std::abs(pitch.m[1][2]-1)>1e-6f||std::abs(roll.m[0][1]-1)>1e-6f)return 2;
    auto id=multiply(identity(),translated);
    for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(id.m[i][j]!=translated.m[i][j])return 3;
    return 0;
}
