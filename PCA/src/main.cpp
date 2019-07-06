#include <igl/readPLY.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/opengl/glfw/imgui/ImGuiHelpers.h>
#include <igl/vertex_triangle_adjacency.h>
#include <igl/writePLY.h>
#include <igl/file_exists.h>
#include <imgui/imgui.h>
#include <iostream>
#include <random>
#include "mytools.h"
#include <Eigen/SVD>

#include <algorithm>
#include <vector>
#include <ANN/ANN.h>
#include <unistd.h>


class MyContext
{
public:

	std::string sourceFile1 = "../../data/SPRING0001.obj";


	MyContext() : point_size(2), line_width(0.5), mode(0)//, model(0)
	{

        std::cout<<"eigen version.:"<<EIGEN_WORLD_VERSION<<"."<<EIGEN_MAJOR_VERSION <<"."<< EIGEN_MINOR_VERSION<<"\n";
        char *path=NULL;
        size_t size;
        path=getcwd(path,size);
        std::cout<<"\n current Path "<<path << std::endl;
        //CLION



	}
	~MyContext() {}
    // front
    int mode;
    float point_size;
    float line_width;

    bool reduction = true;
    bool fullSize = false;
    int reductionSize=4;
    int nModels = 25;
    int maxModel;
    int model = 1;
    int k_eig = 10;

    float scaling0 = 1;
    float scaling1 = 1;
    float scaling2 = 1;
    float scaling3 = 1;
    float scaling4 = 1;
    float scaling5 = 1;
    float threshold = 0.01;

    // back
	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
    Eigen::MatrixXf eigenvectors,eigenvectors_;
    Eigen::MatrixXf evalues,evalues_;
    Eigen::MatrixXf S,full_S,U,U_;
    Eigen::VectorXf s_mean,s_mean_;
    Eigen::MatrixXd sampled_Points,sampled_Points_;

    std::string path = "../../data/";
    std::string meshName = "SPRING";
    std::string suffix = ".obj";
    std::string number;
    std::string filename;
    int needed_zero_length = 4;



	void reset_display(igl::opengl::glfw::Viewer& viewer)
	{
		static std::default_random_engine generator;
		viewer.data().clear();
        viewer.data().show_lines = 0;
        viewer.data().show_overlay_depth = 1;


		if (mode == 1)
		{
            print("Start loading meshes ");
            // data folder contains SPRINGxxxx.obj files
            // some numbers are missing so we will retrieve only the first nModels models
            int column = 1;
            int i = 1;
            while(column <= nModels) {
                // create file path of the mesh model to load
                number = pad_number(i, needed_zero_length);
                filename = path + meshName + number + suffix;

                if (igl::file_exists(filename)) {
                    // read mesh
                    std::cout << filename << std::endl;
                    igl::readOBJ(filename, V, F);

                    int nRows = V.rows();
                    Eigen::MatrixXf sampledMesh;
                    sampledMesh = downsample(V,false,reductionSize,nRows);
                    sampledMesh.resize(3 * nRows, 1);
                    full_S.conservativeResize(3 * nRows, column);
                    full_S.col(column - 1) = sampledMesh;

                    if(reduction){
                        // optionally subsample only a set of points
                        sampledMesh = downsample(V, reduction, reductionSize, nRows);
                        sampledMesh.resize(3 * nRows, 1);
                        // S will be out matrix 3n x k, (n=#vertices, k=#models) containing all the models
                        S.conservativeResize(3 * nRows, column);
                        S.col(column - 1) = sampledMesh;
                    }

                    column++;
                }
                i++;
            }
            print(" mesh loaded");
            print("S matrix built , S is 3n x k , where n is # of vertices and k is # of models");

            // compute mean shape
            s_mean = S.rowwise().mean();
            U = S - s_mean * Eigen::RowVectorXf::Ones(S.cols());
            if(fullSize){

            //} else {
                s_mean_ = full_S.rowwise().mean();
                U_ = full_S - s_mean_ * Eigen::RowVectorXf::Ones(full_S.cols());
                decomp( U_ , k_eig, eigenvectors_, evalues_);
            }

            std::cout << "mean shape computed from S, size: " << s_mean.rows() << " by " << s_mean.cols() << std::endl;
            // s_mean has size 3nx1, we multiply for a row vector 1xk of ones to repeat it
            // U has same size as S

            std::cout << "U matrix (S - mean shape), size: " << U.rows() << " by " << U.cols() << std::endl;
            // decomposition to find the eigenvectors
            decomp( U , k_eig, eigenvectors, evalues);


            maxModel = nModels;

		}
		else if (mode == 2)
		{
		    print("Showing mean shape");

            Eigen::MatrixXd outputMean = recompose_V_matrix(s_mean);
            Eigen::MatrixXd full_mean_shape = recompose_V_matrix(full_S.rowwise().mean());

            if(reduction){
                viewer.data().add_points(outputMean,Eigen::RowVector3d(0, 0, 0));
                viewer.data().add_points(full_mean_shape,Eigen::RowVector3d(1, 1, 1));
                viewer.data().set_mesh(full_mean_shape, F);
                viewer.core.align_camera_center(full_mean_shape, F);
            } else {
                viewer.data().set_mesh(V, F);
                viewer.core.align_camera_center(V, F);
            }

            viewer.data().show_overlay_depth = 1;
            viewer.data().show_overlay = 1;
		}
		else if(mode == 3)
		{
		    print("Start PCA on mean mesh");

            Eigen::MatrixXf outputMesh(s_mean.rows(),s_mean.cols());
            outputMesh.setZero();
            //std::cout<< s_mean_.rows() << " " << s_mean_.cols() ;

            // reconstruct
            Eigen::RowVectorXf scalar =  Eigen::RowVectorXf::Ones(eigenvectors.cols());
            scalar(0) = scaling0; scalar(1) = scaling1; scalar(2) = scaling2;
            scalar(3) = scaling3; scalar(4) = scaling4; scalar(5) = scaling5;
            int maxCols = eigenvectors.cols();
            print("reconstruction loop");
            for (int frequency = 0; frequency < eigenvectors.cols(); frequency++) {

                Eigen::MatrixXf a;
                a = s_mean.transpose() * eigenvectors.col(frequency);
                outputMesh.col(0) += a(0)* scalar[frequency] *eigenvectors.col(frequency);

            }
            sampled_Points = recompose_V_matrix(outputMesh.col(0));
            if(fullSize){
                Eigen::MatrixXf outputMesh_(s_mean_.rows(),s_mean_.cols());
                outputMesh_.setZero();
                Eigen::RowVectorXf scalar_ =  Eigen::RowVectorXf::Ones(eigenvectors_.cols());
                scalar_(0) = scaling0; scalar_(1) = scaling0; scalar_(2) = scaling0;
                scalar_(3) = scaling0; scalar_(4) = scaling0; scalar_(5) = scaling0;

                print("fullSize loop");
                for (int frequency = 0; frequency < eigenvectors_.cols(); frequency++) {
                    Eigen::MatrixXf a_;
                    a_ = s_mean_.transpose() * eigenvectors_.col(frequency);
                    outputMesh_.col(0) += a_(0)* scalar_[frequency] *eigenvectors_.col(frequency);

                }
                sampled_Points_ = recompose_V_matrix(outputMesh_.col(0));
            }

            viewer.data().add_points(sampled_Points,Eigen::RowVector3d(0, 0, 0));
            //viewer.data().set_mesh(V, F);
            //viewer.core.align_camera_center(V, F);
            viewer.data().show_overlay_depth = 1;
            viewer.data().show_overlay = 1;

		}
        else if(mode == 4)
        {

            print("starting upsampling ");
            Eigen::MatrixXd sampled_mean_shape = recompose_V_matrix(s_mean);
            Eigen::MatrixXd full_mean_shape = recompose_V_matrix(full_S.rowwise().mean());
            std::cout <<"full_mean_shape " << full_mean_shape.rows() << " " << full_mean_shape.cols() << std::endl;
            Eigen::MatrixXd transformed_sampled_points = sampled_Points;
            std::cout <<"transformed_sampled_points " << transformed_sampled_points.rows() << " " << transformed_sampled_points.cols() << std::endl;

            Eigen::MatrixXd upsampledMesh = upscale(sampled_mean_shape, full_mean_shape, reductionSize, transformed_sampled_points, threshold);
            // visualize
            //viewer.data().add_points(transformed_sampled_points,Eigen::RowVector3d(1, 1, 1));
            //viewer.data().add_points(upsampledMesh,Eigen::RowVector3d(0, 0, 0));
            viewer.data().set_mesh(upsampledMesh,F);
            viewer.data().show_overlay_depth = 1;
            viewer.data().show_overlay = 1;
            if(fullSize){
                double ssd = (sampled_Points_-upsampledMesh).rowwise().norm().sum();
                print("ssd: "); std::cout<< ssd << std::endl;
            }

        }
        else if(mode == 5)
        {
            print(" start PCA with target model ");

            Eigen::MatrixXf targetV = S.col(model-1);

            Eigen::MatrixXf outputMesh(s_mean.rows(),s_mean.cols());
            outputMesh.setZero();
            Eigen::RowVectorXf scalar =  Eigen::RowVectorXf::Ones(eigenvectors.cols());
            scalar(0) = scaling0;
            scalar(1) = scaling1;
            scalar(2) = scaling2;
            scalar(3) = scaling3;
            scalar(4) = scaling4;
            scalar(5) = scaling5;
            int maxCols = eigenvectors.cols();
            for (int frequency = 0; frequency < eigenvectors.cols(); frequency++) {
                Eigen::MatrixXf a;
                a = (targetV.transpose() - s_mean.transpose()) * eigenvectors.col(frequency);
                outputMesh.col(0) += a(0)* scalar(frequency) *eigenvectors.col(frequency);
            }
            outputMesh.col(0) += s_mean;

            print("starting upsampling ");
            Eigen::MatrixXd sampled_mean_shape = recompose_V_matrix(s_mean);
            Eigen::MatrixXd full_mean_shape = recompose_V_matrix(full_S.rowwise().mean());
            std::cout <<"full_mean_shape " << full_mean_shape.rows() << " " << full_mean_shape.cols() << std::endl;
            Eigen::MatrixXd target_sampled_points = recompose_V_matrix(outputMesh);
            Eigen::MatrixXd transformed_sampled_points = target_sampled_points;
            std::cout <<"transformed_sampled_points " << transformed_sampled_points.rows() << " " << transformed_sampled_points.cols() << std::endl;

            Eigen::MatrixXd upsampledMesh = upscale(sampled_mean_shape, full_mean_shape, reductionSize, transformed_sampled_points, threshold);

            //V = recompose_V_matrix(upsampledMesh.col(0));

            //viewer.data().add_points(upsampledMesh,Eigen::RowVector3d(0, 0, 0));
            viewer.data().set_mesh(upsampledMesh, F);
            viewer.core.align_camera_center(upsampledMesh, F);
            viewer.data().show_overlay_depth = 1;
            viewer.data().show_overlay = 1;

        }
        else if(mode == 7)
        {
            print(" testing with a single mesh ");
            std::string filename = "../../data/SPRING0001.obj";
            igl::readPLY(filename, V, F);

            Eigen::MatrixXf testV = V.cast<float>();
            Eigen::MatrixXf testMean = testV.colwise().mean();
            std::cout << "testMean "<< testMean.rows() << " " << testMean.cols() << std::endl;
            testV = testV - Eigen::RowVectorXf::Ones(testV.rows()).transpose()*testMean;

            decomp(testV,k_eig,eigenvectors,evalues);

            Eigen::MatrixXd outputV = testV.cast<double>();

            viewer.data().set_mesh(outputV,F);
            viewer.core.align_camera_center(outputV, F);
            viewer.data().set_face_based(true);
            viewer.data().show_overlay_depth = 1;
            viewer.data().show_overlay = 1;

        }
        else if(mode == 8)
        {
            print(" testing PCA with a single mesh ");
            Eigen::MatrixXf testV = V.cast<float>();
            int maxCols = eigenvectors.cols();
            Eigen::MatrixXf output;
            output.resize(testV.rows(),testV.cols());
            output.setZero();

            Eigen::RowVectorXf scalar =  Eigen::RowVectorXf::Ones(eigenvectors.cols());
            scalar(0) = scaling0;
            scalar(1) = scaling1;
            scalar(2) = scaling2;
            scalar(3) = scaling3;
            scalar(4) = scaling4;
            scalar(5) = scaling5;

            for (int frequency = 0; frequency < eigenvectors.cols(); frequency++) {

                Eigen::MatrixXf a;
                a = testV.transpose()* eigenvectors.col(frequency);

                output.col(0) += a(0)* scalar(0) *eigenvectors.col(frequency);
                output.col(1) += a(1)* scalar(1) *eigenvectors.col(frequency);
                output.col(2) += a(2)* scalar(2) *eigenvectors.col(frequency);
            }
            Eigen::MatrixXd showMesh;
            showMesh = output.cast<double>();
            viewer.data().set_mesh(showMesh,F);

            viewer.data().show_overlay_depth = 1;
            viewer.data().show_overlay = 1;
        }
		else {
			print("");

            switch(model){
                default:
                    sourceFile1 = "../../data/SPRING0001.obj";
                    break;
                case 1:
                    sourceFile1 = "../../data/SPRING0002.obj";
                    break;
            }

			igl::readOBJ(sourceFile1, V, F);
            viewer.data().set_mesh(V, F);
			viewer.core.align_camera_center(V, F);
			viewer.data().show_overlay_depth = 1;
			viewer.data().show_overlay = 1;
		}

		viewer.data().line_width = line_width;
		viewer.data().point_size = point_size;

	}

private:

};

MyContext g_myctx;


bool key_down(igl::opengl::glfw::Viewer& viewer, unsigned char key, int modifier)
{

	std::cout << "Key: " << key << " " << (unsigned int)key << std::endl;
	if (key=='q' || key=='Q')
	{
		exit(0);
	}
	return false;
}


int main(int argc, char *argv[])
{

    std::cout<<"eigen version.:"<<EIGEN_WORLD_VERSION<<"."<<EIGEN_MAJOR_VERSION <<"."<< EIGEN_MINOR_VERSION<<"\n";

	//################################################################
	// Init the viewer
	igl::opengl::glfw::Viewer viewer;

	// Attach a menu plugin
	igl::opengl::glfw::imgui::ImGuiMenu menu;
	viewer.plugins.push_back(&menu);

	// menu variable Shared between two menus
	double doubleVariable = 0.1f; 

	// Add content to the default menu window via defining a Lambda expression with captures by reference([&])
	menu.callback_draw_viewer_menu = [&]()
	{
		// Draw parent menu content
		menu.draw_viewer_menu();

		// Add new group
		if (ImGui::CollapsingHeader("New Group", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Expose variable directly ...
			ImGui::InputDouble("double", &doubleVariable, 0, 0, "%.4f");

			// ... or using a custom callback
			static bool boolVariable = true;
			if (ImGui::Checkbox("bool", &boolVariable))
			{
				// do something
				std::cout << "boolVariable: " << std::boolalpha << boolVariable << std::endl;
			}

			// Expose an enumeration type
			enum Orientation { Up = 0, Down, Left, Right };
			static Orientation dir = Up;
			ImGui::Combo("Direction", (int *)(&dir), "Up\0Down\0Left\0Right\0\0");

			// We can also use a std::vector<std::string> defined dynamically
			static int num_choices = 3;
			static std::vector<std::string> choices;
			static int idx_choice = 0;
			if (ImGui::InputInt("Num letters", &num_choices))
			{
				num_choices = std::max(1, std::min(26, num_choices));
			}
			if (num_choices != (int)choices.size())
			{
				choices.resize(num_choices);
				for (int i = 0; i < num_choices; ++i)
					choices[i] = std::string(1, 'A' + i);
				if (idx_choice >= num_choices)
					idx_choice = num_choices - 1;
			}
			ImGui::Combo("Letter", &idx_choice, choices);

			// Add a button
			if (ImGui::Button("Print Hello", ImVec2(-1, 0)))
			{
				std::cout << "Hello\n";
			}
		}
	};

	// Add additional windows via defining a Lambda expression with captures by reference([&])
	menu.callback_draw_custom_window = [&]()
	{
		// Define next window position + size
		ImGui::SetNextWindowPos(ImVec2(180.f * menu.menu_scaling(), 10), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(300, 450), ImGuiSetCond_FirstUseEver);
		ImGui::Begin( "MyProperties", nullptr, ImGuiWindowFlags_NoSavedSettings );
		
		// point size
		// [event handle] if value changed
		if (ImGui::InputFloat("point_size", &g_myctx.point_size))
		{
			std::cout << "point_size changed\n";
			viewer.data().point_size = g_myctx.point_size;
		}

		// line width
		// [event handle] if value changed
		if(ImGui::InputFloat("line_width", &g_myctx.line_width))
		{
			std::cout << "line_width changed\n";
			viewer.data().line_width = g_myctx.line_width;
		}

		//mode
		ImGui::SliderInt("Mode", &g_myctx.mode, 0,8);
        // select model
        ImGui::SliderInt("Models to load", &g_myctx.nModels, 2,1500);
        ImGui::SliderInt("Target model", &g_myctx.model, 1, g_myctx.maxModel);

        ImGui::Checkbox("Reduction", &g_myctx.reduction);
        ImGui::Checkbox("fullSize", &g_myctx.fullSize);

        ImGui::InputInt("Reduction size", &g_myctx.reductionSize, 1,500);

        //ImGui::SliderInt("# eigenvalues", &g_myctx.k_eig, 1, 10);

        ImGui::InputFloat("threshold", &g_myctx.threshold, 0.5,0.1);

        if(ImGui::InputFloat("Scale0", &g_myctx.scaling0, -3,3)){
            g_myctx.reset_display(viewer);
        }
        if(ImGui::InputFloat("Scale1", &g_myctx.scaling1, -3,3)){
            g_myctx.reset_display(viewer);
        }
        if(ImGui::InputFloat("Scale2", &g_myctx.scaling2, -3,3)){
            g_myctx.reset_display(viewer);
        }
        if(ImGui::InputFloat("Scale3", &g_myctx.scaling3, -3,3)){
            g_myctx.reset_display(viewer);
        }
        if(ImGui::InputFloat("Scale4", &g_myctx.scaling4, -3,3)){
            g_myctx.reset_display(viewer);
        }
        if(ImGui::InputFloat("Scale5", &g_myctx.scaling5, -3,3)){
            g_myctx.reset_display(viewer);
        }



		//mode-text
		if (g_myctx.mode == 1) { 
			ImGui::Text("mode: loading meshes ");
		}
        else if (g_myctx.mode == 2) {
            ImGui::Text("mode: show average shape ");
        }
        else if (g_myctx.mode == 3) {
            ImGui::Text("mode: PCA with the mean ");
        }
        else if (g_myctx.mode == 4){
            ImGui::Text("mode: upsample to original size");
        }
        else if (g_myctx.mode == 5) {
            ImGui::Text("mode: PCA with a target mesh (# selected on Target model)");
        }
        else if (g_myctx.mode == 7) {
            ImGui::Text("mode: testing PCA on a single model ");
        }
        else if (g_myctx.mode == 8) {
            ImGui::Text("mode: testing with a single model ");
        }
        else {
            ImGui::Text("mode: ");
        }

        if (ImGui::Button("Apply")) {
        	std::cout << "Applying" << std::endl;
            g_myctx.reset_display(viewer);
        }


        ImGui::End();
	};


	// registered a event handler
	viewer.callback_key_down = &key_down;

	g_myctx.reset_display(viewer);

	// Call GUI
	viewer.launch();

}


void get_example_mesh(std::string const meshname , Eigen::MatrixXd & V, Eigen::MatrixXi & F, Eigen::MatrixXd & VN)
{
    
    
    std::vector<const char *> cands{
        "../../data/",
        "../../../data/",
        "../../../../data/",
        "../../../../../data/" };
    
    bool found = false;
    for (const auto & val : cands)
    {
        if ( igl::file_exists(val+ meshname) )
        {
            std::cout << "loading example mesh from:" << val+ meshname << "\n";
            
            //if (igl::readOFF(val+ meshname, V,F)) {
            if (igl::readOBJ(val+ meshname, V,F)) {
                igl::per_vertex_normals(V, F, VN);
                found = 1;
                break;
            }
            else {
                std::cout << "file loading failed " << cands[0] + meshname << "\n";
            }
        }
    }
    
    if (!found) {
        std::cout << "cannot locate "<<cands[0]+ meshname <<"\n";
        exit(1);
    }
    
}
