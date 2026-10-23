// Test-only visibility lets finite differences inspect the actual ANN parameters.
#include <vector>
#include <iostream>
#include <iomanip>
#include "NeuralNetwork.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
double loss(ANN& net, const Matrix& x, const Matrix& y) {
    Matrix out=net.feedforward(x);
    double value=0;
    for(int i=0;i<out.getRows();++i) value+=0.5*std::pow(out.data[i][0]-y.data[i][0],2);
    return value;
}
int main(int argc,char**argv) {
    if(argc>1 && std::string(argv[1])=="gradient") {
        std::srand(7);
        ANN net({2,3,2},{SIGMOID,SIGMOID});
        Matrix x(std::vector<std::vector<double>>{{0.2},{0.7}}), y(std::vector<std::vector<double>>{{0.1},{0.9}});
        ANN before=net;
        net.feedforward(x); net.update(y,0.5);
        double worst=0;
        for(size_t l=0;l<before.layers.size();++l) {
            for(int kind=0;kind<2;++kind) {
                Matrix& params=kind ? before.layers[l].bias : before.layers[l].weights;
                Matrix& actual=kind ? net.layers[l].bias : net.layers[l].weights;
                for(int r=0;r<params.getRows();++r) for(int c=0;c<params.getCol();++c) {
                    double v=params.data[r][c], eps=1e-5;
                    params.data[r][c]=v+eps; double plus=loss(before,x,y);
                    params.data[r][c]=v-eps; double minus=loss(before,x,y);
                    params.data[r][c]=v;
                    double numeric=(plus-minus)/(2*eps);
                    double analytic=(v-actual.data[r][c])/0.5;
                    worst=std::max(worst,std::abs(numeric-analytic));
                }
            }
        }
        std::cout<<"Gradient max absolute error: "<<worst<<std::endl;
        assert(worst<1e-7);
    } else {
        Matrix m(std::vector<std::vector<double>>{{-2,0,2}});
        Matrix r=nn_utils::relu(m), t=nn_utils::tanh(m), dr=nn_utils::drelu(r), dt=nn_utils::dtanh(t);
        assert(r.data[0][0]==0 && r.data[0][1]==0 && r.data[0][2]==2);
        assert(std::abs(t.data[0][0]+0.9640275800758169)<1e-12);
        assert(dr.data[0][0]==0 && dr.data[0][1]==0 && dr.data[0][2]==1);
        assert(std::abs(dt.data[0][0]-0.07065082485316443)<1e-12 && dt.data[0][1]==1);
        std::cout<<"Activation values and derivatives PASS\n";
    }
}
