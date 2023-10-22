#include "cubic_spline.hpp"
#include "utils/utils.hpp"

CubicSpline::CubicSpline(vector<double> _x, vector<double> _y)
{
    x = _x;
    y = _y;
    nx = x.size();
    h = Utils::diff(x);
    bool not_valid = std::any_of(h.begin(), h.end(), 
                                [](double val) { return val < 0; });
    if (not_valid) {
        throw std::invalid_argument("x coordinates must be sorted in ascending order");
    }
    a = y;
    MatrixXd A = calc_A();
    VectorXd B = calc_B();
    VectorXd c_eigen = A.colPivHouseholderQr().solve(B);
    double* c_pointer = c_eigen.data();
    c.assign(c_pointer, c_pointer + c_eigen.rows());

    for (size_t idx = 0; idx < (nx - 1); ++idx) {
        double d_tmp = (c[idx + 1] - c[idx]) / (3.0 * h[idx]);
        double b_tmp = (a[idx + 1] - a[idx]) / h[idx] -
                    h[idx] * (c[idx + 1] + 2 * c[idx]) / 3.0;
        d.emplace_back(d_tmp);
        b.emplace_back(b_tmp);
    }
}

MatrixXd CubicSpline::calc_A(void)
{
    MatrixXd A = MatrixXd::Zero(nx, nx);
    A(0, 0) = 1.0;
    
    for (size_t idx = 0; idx < (nx - 1); ++idx) {
        if (idx != nx - 2) {
            A(idx + 1, idx + 1) = 2.0 * (h[idx] + h[idx + 1]);
        }
        A(idx + 1, idx) = h[idx];
        A(idx, idx + 1) = h[idx];
    } 
    A(0, 1) = 0.0;
    A(nx - 1, nx - 2) = 0.0;
    A(nx - 1, nx - 1) = 1.0;

    return A;
}

VectorXd CubicSpline::calc_B(void)
{
    VectorXd B = VectorXd::Zero(nx);

    for (size_t idx = 0; idx < (nx - 2); ++idx) {
        B(idx + 1) = 3.0 * (a[idx + 2] - a[idx + 1]) / h[idx + 1] -
                3.0 * (a[idx + 1] - a[idx]) / h[idx];
    }
    
    return B;
}

double CubicSpline::calc_position(double _x)
{
    if (_x < x[0] || _x > x.back()) {
        throw std::invalid_argument("received value out of the pre-defined range"); 
    }

    auto it = std::upper_bound(x.begin(), x.end(), _x);
    int idx = std::distance(x.begin(), it) - 1;
    double dx = _x - x[idx];
    double position = a[idx] + b[idx] * dx + c[idx] * pow(dx, 2) + d[idx] * pow(dx, 3);

    return position;
}

double CubicSpline::calc_first_derivative(double _x)
{
    if (_x < x[0] || _x > x.back()) {
        throw std::invalid_argument("received value out of the pre-defined range"); 
    }

    auto it = std::upper_bound(x.begin(), x.end(), _x);
    int idx = std::distance(x.begin(), it) - 1;
    double dx = _x - x[idx];
    double dy = b[idx] + 2.0 * c[idx] * dx + 3.0 * d[idx] * pow(dx, 2);

    return dy;
}

double CubicSpline::calc_second_derivative(double _x)
{
    if (_x < x[0] || _x > x.back()) {
        throw std::invalid_argument("received value out of the pre-defined range"); 
    }

    auto it = std::upper_bound(x.begin(), x.end(), _x);
    int idx = std::distance(x.begin(), it) - 1;
    double dx = _x - x[idx];
    double ddy = 2.0 * c[idx] + 6.0 * d[idx] * dx;

    return ddy;
}

CubicSpline2D::CubicSpline2D(vector<double> _x, vector<double> _y)
{
    s = calc_s(_x, _y);
    sx = CubicSpline(s, _x);
    sy = CubicSpline(s, _y);
}

vector<double> CubicSpline2D::calc_s(vector<double> _x, vector<double> _y)
{
    vector<double> dx = Utils::diff(_x);
    vector<double> dy = Utils::diff(_y);
    vector<double> ds;
    vector<double> s = {0};
    for (size_t idx = 0; idx < dx.size(); ++idx) {
        ds.push_back(hypot(dx[idx], dy[idx]));
    }
    vector<double> cum = Utils::cumsum(ds);
    s.insert(s.end(), cum.begin(), cum.end());

    return s;
}

Vector2d CubicSpline2D::calc_position(double _s)
{
    double _x = sx.calc_position(_s);
    double _y = sy.calc_position(_s);

    return {_x, _y};
}

double CubicSpline2D::calc_yaw(double _s)
{
    double dx = sx.calc_first_derivative(_s);
    double dy = sy.calc_first_derivative(_s);
    double yaw = atan2(dy, dx);

    return yaw;
}

double CubicSpline2D::calc_curvature(double _s)
{
    double dx = sx.calc_first_derivative(_s);
    double ddx = sx.calc_second_derivative(_s);
    double dy = sy.calc_first_derivative(_s);
    double ddy = sy.calc_second_derivative(_s);
    double k = (ddy * dx - ddx * dy) / pow(dx * dx + dy * dy, 1.50);

    return k;
}