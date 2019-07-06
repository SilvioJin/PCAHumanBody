#include "mytools.h"
#include <Eigen/SVD>
#include <random>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/Sparse>
#include <igl/eigs.h>
//#include <igl/invert_diag.h>
#include <Spectra/SymEigsSolver.h>
#include <Spectra/GenEigsSolver.h>
#include <Spectra/MatOp/SparseGenMatProd.h>
#include <Eigen/Core>
#include <Eigen/SparseCore>
//#include "igl_inline.h"
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <ANN/ANN.h>

#define PI 3.14159265

void get_axis(Eigen::MatrixXd & axis)
{
    axis = (Eigen::MatrixXd (31,3) <<
            0   ,0  ,0,
            0.01,0  ,0,
            0.02,0  ,0,
            0.03,0  ,0,
            0.04,0  ,0,
            0.05,0  ,0,
            0.06,0  ,0,
            0.07,0  ,0,
            0.08,0  ,0,
            0.09,0  ,0,
            0.1 ,0  ,0,
            0  ,0.01,0,
            0  ,0.02,0,
            0  ,0.03,0,
            0  ,0.04,0,
            0  ,0.05,0,
            0  ,0.06,0,
            0  ,0.07,0,
            0  ,0.08,0,
            0  ,0.09,0,
            0  ,0.1 ,0,
            0  ,0   ,0.01,
            0  ,0   ,0.02,
            0  ,0   ,0.03,
            0  ,0   ,0.04,
            0  ,0   ,0.05,
            0  ,0   ,0.06,
            0  ,0   ,0.07,
            0  ,0   ,0.08,
            0  ,0   ,0.09,
            0  ,0   ,0.1  ).finished() ;
}

void calculate_vertex_normal(Eigen::MatrixXd const & V, Eigen::MatrixXi const & F, Eigen::MatrixXd const & FN, Eigen::MatrixXd & out_VN)
{
    //
    // input:
    //   V: vertices
    //   F: face
    //   FN: face normals
    // output:
    //   out_VN
    //
    //   Your job is to implement vertex normal calculation
    //
    
    out_VN.resize(V.rows(), V.cols());
    out_VN.setZero();
    
    std::vector<std::vector<int> > VF; //#V list of lists of incident faces (adjacency list)
    std::vector<std::vector<int> > VFi;//#V list of lists of index of incidence within incident faces listed
    igl::vertex_triangle_adjacency(V.rows(), F, VF, VFi);
    
    //std::cout << "Face Normals " << FN << std::endl;
    
    // loop for each vertex
    for (int vert = 0; vert<V.rows(); vert++) {
        //std::cout << "Vertex " << vert << std::endl;
        
        // accumulator of the normals for the output
        Eigen::MatrixXd accumulator;
        accumulator.resize(1, FN.cols());
        accumulator.setZero();
        
        // loop for the neighbor faces of current vertex
        for (int neighbor = 0; neighbor < VF[vert].size(); neighbor++) {
            accumulator = accumulator + FN.row(VF[vert][neighbor]);
        }
        accumulator.normalize(); // normalize the normal vector of the vertex
        
        // output
        out_VN.row(vert) = accumulator;
    }
    
}

void calculate_vertex_normal_flann(Eigen::MatrixXd const & V, Eigen::MatrixXi const & F, Eigen::MatrixXd & out_VN)
{
    //
    // input:
    //   V: vertices
    //   F: face
    //   FN: face normals
    // output:
    //   out_VN
    //
    // Your job is to implement vertex normal calculation vis using flann and igl:fitplane
    //
    // igl::fit_plane(V, N, C);
    // Input:
    //   V #Vx3 matrix. The 3D point cloud, one row for each vertex.
    // Output:
    //   N 1x3 Vector. The normal of the fitted plane.
    //   C 1x3 Vector. A point that lies in the fitted plane.
    //
    
    out_VN.resize(V.rows(), V.cols());
    out_VN.setZero();

    // build tree
    nanoflann::KDTreeEigenMatrixAdaptor< Eigen::MatrixXd > mat_index( V, 50); // 50 is the max leaf
    mat_index.index->buildIndex();

    Eigen::RowVector3d v_cen = V.colwise().sum() / V.rows();
    
    // for each vertex
    for (int vert = 0; vert<V.rows(); vert++) {
        Eigen::RowVector3d rd_pts = V.row(vert);
        
        // set K nearest samples
        const size_t par_K = 10;
        
        // create a query object
        std::vector<size_t> indexes(par_K);
        std::vector<double> dists_sqr(par_K);
        
        nanoflann::KNNResultSet<double> res(par_K);
        res.init(indexes.data(), dists_sqr.data());
        
        // find KNN
        mat_index.index->findNeighbors(res, rd_pts.data(), nanoflann::SearchParams(50));
        
        Eigen::MatrixXd nn_vex(indexes.size(),3);
        // loop for all nearest neighbors
        for (size_t i=0 ; i < indexes.size() ;i++){
            nn_vex.row(i) = V.row(indexes[i]);
        }
        // fit a plane on the nearest neighbors vertices and find the normal of the plane
        Eigen::RowVector3d N;
        Eigen::RowVector3d C;
        igl::fit_plane(nn_vex,N,C);

        if(vert>0){
            if(N.dot(v_cen - rd_pts)>0){
                N = -N;
            }
        }
        //output
        out_VN.row(vert) = N;
    }
}

Eigen::MatrixXd findNN(Eigen::MatrixXd Q, Eigen::MatrixXd P, size_t numberOfNN){
    Eigen::MatrixXd NNs = Eigen::MatrixXd::Zero(Q.rows(), Q.cols());
    // build tree
    nanoflann::KDTreeEigenMatrixAdaptor< Eigen::MatrixXd > mat_index( P, 100); // 50 is the max leaf
    mat_index.index->buildIndex();
    
    // iterate for every q in Q to find its nearest neighbor
    for (int idx = 0; idx < Q.rows(); idx++) {
        // current q
        Eigen::RowVector3d q_i = Q.row(idx);
        
        // set the parameters, how many neighbors to find (num_results = 2 means : finds 1 nearest neighbor, + itself)
        const size_t num_results = numberOfNN;
        std::vector<size_t>   ret_indexes(num_results);
        std::vector<double> out_dists_sqr(num_results);
        
        // resultSet will contain the nearest neighbors
        nanoflann::KNNResultSet<double> resultSet(num_results);
        
        // find nearest neighbors
        resultSet.init(ret_indexes.data(), out_dists_sqr.data());
        mat_index.index->findNeighbors(resultSet, q_i.data() , nanoflann::SearchParams(25));
        
        
        NNs.row(idx) = P.row(ret_indexes[0]);
    }
    
    return NNs;
}

double getAngle(Eigen::RowVector3d a, Eigen::RowVector3d b, Eigen::RowVector3d c){
    /*
     calculate the angle between ab and ac
     */
    double angle;
    
    Eigen::RowVector3d ab = b - a;
    Eigen::RowVector3d ac = c - a;
    
    angle = (ab.dot(ac))/(ab.norm()*ac.norm());
    angle = acos(angle);
    // it's in radians
    //std::cout<< "angle " << angle << std::endl;
    
    return angle;
}

Eigen::SparseMatrix<double> invertDiagSparseMatrix(Eigen::SparseMatrix<double> M){
    /*
     * input: sparse matrix M
     * output: inverse of M
     * solving M * x = I
     * x = M^-1
     * */
    std::cout << "start: invertDiagSparseMatrix" << std::endl;
    /*
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    solver.compute(M);
    Eigen::SparseMatrix<double> I(M.rows(),M.cols());
    I.setIdentity();
    Eigen::SparseMatrix<double>  inverse_M = solver.solve(I);
    // std::cout << M*inv_M << std::endl;
    */
    Eigen::SparseMatrix<double>  inverse_M;
    inverse_M.resize(M.rows(),M.cols());
    inverse_M.setZero();
    for (int i = 0; i <M.rows() ; i++) {
        inverse_M.coeffRef(i,i) = 1/M.coeffRef(i,i);
    }

    std::cout << "end: invertDiagSparseMatrix" << std::endl;
    return inverse_M ;

}

double triangleArea(Eigen::RowVector3d a, Eigen::RowVector3d b, Eigen::RowVector3d c) {
    /*
     * input  : 3 vertices positions
     * output : area of the triangle formed by the 3 vertices
     * */

    Eigen::RowVector3d ab = a - b;
    Eigen::RowVector3d bc = b - c;
    
    double area = (ab.cross(bc)).norm() / 2.0;

    return area;
}
/*
Eigen::MatrixXd getSpectralMeshes(Eigen::MatrixXd V, Eigen::MatrixXi F, int k){


    std::cout<< "start: getSpectralMeshes" << std::endl;
    std::cout<< "V " << V.rows() << " " << V.cols() << std::endl;
    std::cout<< "F " << F.rows() << " " << F.cols() << std::endl;
    Eigen::SparseMatrix<double> C = cotan_LPB(V,F);
    //igl::cotmatrix(V,F, C);
    //igl::massmatrix(V, F, igl::MASSMATRIX_TYPE_VORONOI, M );

    C = (-0.5*C).eval();

    Eigen::SparseMatrix<double> M = getM(V,F);

    Eigen::MatrixXd eigenvectors;
    Eigen::VectorXd eigenvalues;

    //=====================
    std::cout <<"M(1,1) " << M.coeffRef(1,1) << std::endl;
    Eigen::SparseMatrix<double> inv_M = invertDiagSparseMatrix(M);
    //std::cout <<"inv_M(1,1) " << inv_M.coeffRef(1,1) << std::endl;
    Eigen::SparseMatrix<double> inv_M_half = inv_M.cwiseSqrt();
    //std::cout <<"inv_M_half(1,1) " << inv_M_half.coeffRef(1,1) << std::endl;
    Eigen::SparseMatrix<double> M_half = M.cwiseSqrt();
    //std::cout <<"M_half(1,1) " << M_half.coeffRef(1,1) << std::endl;
    Eigen::SparseMatrix<double> A = inv_M_half*C*inv_M_half;
    Eigen::SparseMatrix<double> I(C.rows(),C.cols());
    I.setIdentity();
    igl::eigs(A, I, k, igl::EIGS_TYPE_SM, U, D);
    eigenvectors = inv_M_half*U;
    //=====================

    //

    if(k>10){
        //Eigen::SparseMatrix<double> inv_M;
        //igl::invert_diag(M,inv_M);
        Eigen::SparseMatrix<double> inv_M = invertDiagSparseMatrix(M);
        Eigen::SparseMatrix<double> inv_M_half = inv_M.cwiseSqrt();
        C = inv_M_half*C*inv_M_half;
        Spectra::SparseGenMatProd<double> op(C);
        //Spectra::GenEigsSolver< double, Spectra::SMALLEST_MAGN, Spectra::SparseGenMatProd<double> > eigs(&op, k, 2*k);
        Spectra::SymEigsSolver< double, Spectra::SMALLEST_MAGN, Spectra::SparseGenMatProd<double> > eigs(&op, k, 2*k);
        eigs.init();
        int nconv = eigs.compute();
        Eigen::VectorXcd evalues;
        if(eigs.info() == Spectra::SUCCESSFUL){
            evalues = eigs.eigenvalues();
            eigenvectors = eigs.eigenvectors().real();

            eigenvectors = inv_M_half*eigenvectors;
            std::cout << "Eigenvalues found:\n" << evalues << std::endl;
        }

    } else {
        igl::eigs(C, M, k, igl::EIGS_TYPE_SM, eigenvectors, eigenvalues);
    }

    //
    std::cout<< eigenvectors << std::endl;

    //Eigen::VectorXd lastCol = U.col(maxCols-1).cwiseInverse();
    //std::cout << "lastCol " << lastCol << std::endl;
    //eigenvectors = U.array().colwise() * lastCol.array();

    //std::cout << "last col "<< U.col(maxCols-1) << std::endl;
    //std::cout << "eigenvectors \n"<< eigenvectors << std::endl;
    int maxCols = eigenvectors.cols();
    std::cout << "eigenvectors size "<< eigenvectors.rows() <<" "<< eigenvectors.cols() << std::endl;

    Eigen::MatrixXd output;
    output.resize(V.rows(),V.cols());
    output.setZero();
    for (int frequency = 0; frequency < k; frequency++) {
        //std::cout << "output first row "<< output.row(0) << std::endl;
        Eigen::MatrixXd a;
        a = V.transpose() *M* eigenvectors.col(maxCols-1-frequency);
        //std::cout << "a " << a.rows() << " by " << a.cols() << " " << a << std::endl;
        //std::cout <<"dot e_i, e_i " << eigenvectors.col(maxCols -1 -frequency).dot(eigenvectors.col(maxCols -1 -frequency)) << std::endl;
        //std::cout <<"dot e_i, e_j " << eigenvectors.col(maxCols -1 -frequency).dot(eigenvectors.col(0)) << std::endl;
        output.col(0) += a(0)*eigenvectors.col(maxCols-1-frequency);
        output.col(1) += a(1)*eigenvectors.col(maxCols-1-frequency);
        output.col(2) += a(2)*eigenvectors.col(maxCols-1-frequency);
    }

    //std::cout << "output " << output << std::endl;

    std::cout<< "end: getSpectralMeshes" << std::endl;
    return output;
}
*/
void addGaussNoise( Eigen::MatrixXd & M, double scale){

    std::cout<< "Adding Gaussian Noise with std "<< scale << "% of the bounding box" << std::endl;

    // find min and max point values of a mesh
    Eigen::MatrixXd max = M.colwise().maxCoeff();
    Eigen::MatrixXd min = M.colwise().minCoeff();

    // find distances
    double Xdist = max(0) - min(0);
    double Ydist = max(1) - min(1);
    double Zdist = max(2) - min(2);
    double mean = 0;
    double Xnoise ;
    double Ynoise ;
    double Znoise ;

    // create distributions
    std::default_random_engine generator;
    std::normal_distribution<double> Xdistribution(mean,Xdist*scale);
    std::normal_distribution<double> Ydistribution(mean,Ydist*scale);
    std::normal_distribution<double> Zdistribution(mean,Zdist*scale);

    Eigen::MatrixXd noise(1,3);

    // for all points add noise
    for (int i=0; i<M.rows(); i++) {
        // noise values for each dimension
        Xnoise = Xdistribution(generator);
        Ynoise = Ydistribution(generator);
        Znoise = Zdistribution(generator);
        noise << Xnoise, Ynoise, Znoise;

        M.row(i) = M.row(i) + noise;

        noise.setZero();
    }

}

std::string pad_number(int number,int needed_length){

    std::string output = std::to_string(number);

    output = std::string(needed_length - output.length(), '0') + output;

    return output;
}

void decomp(Eigen::MatrixXf M, int k, Eigen::MatrixXf & eigenvectors, Eigen::MatrixXf & evalues){

    print("Starting decomp");
/*
    Spectra::DenseSymMatProd<float> op(M);
    Spectra::SymEigsSolver< float, Spectra::LARGEST_MAGN, Spectra::DenseSymMatProd<float> > eigs(&op, k, M.rows());
    eigs.init();
    int nconv = eigs.compute();
    std::cout << "nconv  " << nconv << std::endl;
    if(eigs.info() == Spectra::SUCCESSFUL){
        evalues = eigs.eigenvalues().real();
        eigenvectors = eigs.eigenvectors().real();
        std::cout << "Eigenvalues found:\n" << evalues << std::endl;
        std::cout << "Eigenvectors found:\n" << eigenvectors << std::endl;
    }
*/
    // SVD decomposition
    Eigen::BDCSVD<Eigen::MatrixXf> C_svd(M, Eigen::ComputeFullU);// | Eigen::ComputeFullV);
    Eigen::MatrixXf U = C_svd.matrixU();
    // U is going to be of size 3n x 3n
    //Eigen::MatrixXf V = C_svd.matrixV();
    evalues = C_svd.singularValues();
    eigenvectors = U;
    std::cout << " eigenvectors matrix found, size: " << eigenvectors.rows() << " by " << eigenvectors.cols() << std::endl;

}

Eigen::MatrixXf downsample(Eigen::MatrixXd V, bool reduction, int reductionSize, int & nRows){

    // downsampling the original mesh V
    // sample rows every reductionSize rows
    // cast to float to save memory

    Eigen::MatrixXf sampledMesh;
    if (reduction){
        int nSamples = nRows / reductionSize;
        nRows = nSamples; // will change size of S
        Eigen::MatrixXf sampled(nSamples,V.cols());
        sampled.setZero();
        // sample every reductionSize rows
        int j = 0;
        for (int k = 0; k < sampled.rows(); k++) {
            sampled.row(k) = V.row(j).cast<float>();
            j = j + reductionSize;
        }
        sampledMesh = sampled.transpose();
    }
    else{ // if not subsampling, just cast to float
        sampledMesh = V.transpose().cast<float>();
    }

    return sampledMesh;
}

Eigen::MatrixXd upscale(Eigen::MatrixXd sampled_mean_shape, Eigen::MatrixXd full_mean_shape, int reductionSize, Eigen::MatrixXd transformed_sampled_points, float threshold){

    ANNpointArray PANN = getANNpointsFromMatrix(sampled_mean_shape);
    ANNpointArray queryPoints = getANNpointsFromMatrix(full_mean_shape);
    ANNkd_tree PkdTree = ANNkd_tree(PANN, sampled_mean_shape.rows(), sampled_mean_shape.cols());
    Eigen::MatrixXd upsampledMesh = Eigen::MatrixXd::Zero(full_mean_shape.rows(), full_mean_shape.cols());
    print("start upscale loop");

    for (int i = 0; i < full_mean_shape.rows(); i++) {
        if (i % reductionSize != 0) {

            ANNidxArray neighbourIndicies = new ANNidx[4];
            ANNdistArray neighbourDistances = new ANNdist[4];
            ANNpoint queryPoint = queryPoints[i];
            PkdTree.annkSearch(queryPoint, 4, neighbourIndicies, neighbourDistances, 0.0);

            // find barycenter of queryPoint based on neighbours
            Eigen::Vector3d q = full_mean_shape.row(i);

            Eigen::Vector3d p1 = sampled_mean_shape.row(neighbourIndicies[0]);
            Eigen::Vector3d p2 = sampled_mean_shape.row(neighbourIndicies[1]);
            Eigen::Vector3d p3 = sampled_mean_shape.row(neighbourIndicies[2]);
            Eigen::Vector3d p4 = sampled_mean_shape.row(neighbourIndicies[3]);
            // solve Ax=b system
            Eigen::Matrix4d A;
            A << p1.row(0),p2.row(0),p3.row(0),p4.row(0), p1.row(1),p2.row(1),p3.row(1),p4.row(1), p1.row(2),p2.row(2),p3.row(2),p4.row(2), 1,1,1,1;
            Eigen::Vector4d b;
            b << q.row(0),q.row(1),q.row(2),1;

            Eigen::Vector4d x = A.colPivHouseholderQr().solve(b);

            // get neighbours in the updated_sampled_mean_points array

            Eigen::Vector3d np1 = transformed_sampled_points.row(neighbourIndicies[0]);
            Eigen::Vector3d np2 = transformed_sampled_points.row(neighbourIndicies[1]);
            Eigen::Vector3d np3 = transformed_sampled_points.row(neighbourIndicies[2]);
            Eigen::Vector3d np4 = transformed_sampled_points.row(neighbourIndicies[3]);

            // get queryPoint position from updated_sampled_mean_points barycenter position
            Eigen::Vector3d updated_position = (np1 * x.row(0)) + (np2 * x.row(1)) + (np3 * x.row(2)) + (np4 * x.row(3));

            float distance = (np1-updated_position).norm();
            if (distance > threshold){
                updated_position = (np1+np2+np3+np4)/4;
            }

            // add this position to the upsampled mesh
            upsampledMesh.row(i) = updated_position;
            delete[] neighbourIndicies;

        } else {
            int index = i / reductionSize;
            if(index > transformed_sampled_points.rows()-1){
                index = transformed_sampled_points.rows()-1;
            }
            upsampledMesh.row(i) = transformed_sampled_points.row(index);
        }
    }

    return upsampledMesh;
}

Eigen::MatrixXd recompose_V_matrix(Eigen::MatrixXf v){

    // resize from 3n x 1 to n x 3
    Eigen::MatrixXf output = v;
    int rows = output.rows()/3;
    output.resize(3,rows);
    output.transposeInPlace();

    return output.cast<double>();

}


void print(std::string mex){
    std::cout << mex << std::endl;
}

ANNpointArray getANNpointsFromMatrix(Eigen::MatrixXd points) {
    ANNpointArray P = annAllocPts(points.rows(), points.cols());
    for (int row = 0; row < points.rows(); row++) {
        ANNpoint point = annAllocPt(points.cols());
        for (int col = 0; col < points.cols(); col++) {
            point[col] = points(row, col);
        }
        P[row] = point;
    }
    return P;
}