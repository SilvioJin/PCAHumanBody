#ifndef MYTOOLS
#define MYTOOLS

#define NOMINMAX

#include <vector>
#include <iostream>
#include <Eigen/Dense>
#include <igl/vertex_triangle_adjacency.h>
#include "nanoflann.hpp"
#include <igl/fit_plane.h>
#include <Eigen/SVD>

#include <random>
#include <Eigen/LU>
#include <igl/boundary_loop.h>
#include <igl/boundary_facets.h>
#include <igl/setdiff.h>
#include <igl/slice.h>
#include <igl/slice_into.h>
#include <igl/cotmatrix.h>
#include <igl/massmatrix.h>
#include <igl/invert_diag.h>
#include <ANN/ANN.h>

void get_axis(Eigen::MatrixXd & axis);


void calculate_vertex_normal(
	Eigen::MatrixXd const & V,
	Eigen::MatrixXi const & F,
	Eigen::MatrixXd const & FN,
	Eigen::MatrixXd & out_VN);

void calculate_vertex_normal_flann(
	Eigen::MatrixXd const & V, 
	Eigen::MatrixXi const & F,  
	Eigen::MatrixXd & out_VN);

Eigen::MatrixXd findNN(
            Eigen::MatrixXd Q,
            Eigen::MatrixXd P,
            size_t numberOfNN);

double triangleArea(
		Eigen::RowVector3d a,
		Eigen::RowVector3d b,
		Eigen::RowVector3d c);


Eigen::SparseMatrix<double> invertDiagSparseMatrix(
        Eigen::SparseMatrix<double> M);


void get_example_mesh(
                      std::string const meshname,
                      Eigen::MatrixXd & V,
                      Eigen::MatrixXi & F,
                      Eigen::MatrixXd & VN);


double getAngle(
                Eigen::RowVector3d a,
                Eigen::RowVector3d b,
                Eigen::RowVector3d c);


Eigen::MatrixXd getSpectralMeshes(
		Eigen::MatrixXd V,
		Eigen::MatrixXi F,
		int k);


std::string pad_number(
		int number,
		int needed_length);

void addGaussNoise(
		Eigen::MatrixXd & M,
		double scale);

void decomp(
        Eigen::MatrixXf M,
        int k,
        Eigen::MatrixXf & eigenvectors,
        Eigen::MatrixXf & evalues);

Eigen::MatrixXf downsample(
        Eigen::MatrixXd V,
        bool reduction,
        int reductionSize,
        int & nRows);

Eigen::MatrixXd upscale(
        Eigen::MatrixXd sampled_mean_shape,
        Eigen::MatrixXd full_mean_shape,
        int reductionSize,
        Eigen::MatrixXd transformed_sampled_points,
        float threshold);

Eigen::MatrixXd recompose_V_matrix(
        Eigen::MatrixXf v);

Eigen::MatrixXd findIdxOfNN(
        Eigen::MatrixXd Q,
        Eigen::MatrixXd findFrom,
        size_t numberOfNN);

ANNpointArray getANNpointsFromMatrix(
        Eigen::MatrixXd points);

void print(std::string mex);

ANNpointArray getANNpointsFromMatrix(
        Eigen::MatrixXd points);

#endif


