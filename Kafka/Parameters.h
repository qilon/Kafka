#ifndef PARAMETERS_H_
#define PARAMETERS_H_

// View parameters
#define FRUSTUM_LEFT		"frustum_left"
#define FRUSTUM_RIGHT		"frustum_right"
#define FRUSTUM_BOTTOM		"frustum_bottom"
#define FRUSTUM_TOP			"frustum_top"
#define FRUSTUM_NEAR		"frustum_near"
#define FRUSTUM_FAR			"frustum_far"
#define EYE					"eye"

// Light parameters
#define USE_GL_LIGHT		"use_gl_light"
#define USE_SH_LIGHT		"use_sh_light"
#define LIGHT_AMBIENT		"light_ambient"
#define LIGHT_POSITION		"light_position"
#define LIGHT_INTENSITY		"light_intensity"

// Colours
#define BACKGROUND_COLOUR	"background_colour"
#define MESH_COLOUR			"mesh_colour"

#define HEAT_DIST_RANGE		"heat_dist_range"
#define HEAT_ORIENT_RANGE	"heat_orient_range"
#define HEAT_PROJ_RANGE		"heat_proj_range"

#define MESH_ROTATION		"mesh_rotation"

#define REVERSE_NORMAL		"reverse_normal"

// I/O parameters
#define N_MESHES		"n_meshes"
#define N_FRAMES		"n_frames"

#define MESH_PREFIX				"mesh_prefix"
#define MESH_SUFFIX				"mesh_suffix"
#define MESH_FIRST_IDX			"mesh_first_idx"
#define MESH_N_DIGITS			"mesh_n_digits"
#define MESH_HAS_ALBEDO			"mesh_has_albedo"
#define MESH_SH_COEFF_FILENAME	"mesh_sh_coeff_filename"
#define MESH_TYPE				"mesh_type"
#define MESH_PER_FRAME_VALUES	"mesh_per_frame_values"

#define INTRINSICS_FILENAME		"intrinsics_filename"

#define SAVE_IMAGES "save_images"
#define SAVE_IMAGE_PREFIX "save_image_prefix"
#define SAVE_IMAGE_SUFFIX "save_image_suffix"

#define MESH_GT			0
#define MESH_COMP		1
#define MESH_INTRINSIC	2

#include <cmath>
#include <string>
#include <vector>
#include <opencv2/core/core.hpp>

using namespace std;

namespace parameters {
	struct Parameters {

		// Default constructor that sets up a generic sparse problem.
		Parameters() {
			// Window parameters
			window_title = "Kafka";
			window_width = 1600;
			window_height = 900;

			// View parameters
			frustum_left = -80.f;
			frustum_right = 80.f;
			frustum_bottom = -45.f;
			frustum_top = 45.f;
			frustum_near = 1000.f;
			frustum_far = 3000.f;
			eye = { 0.f, 0.f, 0.f };

			// Light parameters
			light_ambient = { 0.1f, 0.1f, 0.1f, 1.f };
			light_position = { -1.f, 0.f, 1.f, 0.f };
			light_intensity = { 1.f, 1.f, 1.f, 1.f };

			// Colours
			background_colour = { 0.46f, 0.46f, 0.54f, 1.f };
			mesh_colour = { 0.46f, 0.46f, 0.54f, 1.f };

			heat_dist_range = { 0, 5 };
			heat_orient_range = { 0, 0.517638090205041f };
			heat_proj_range = { 0, 5 };

			reverse_normal = false;

			// Mesh initial rotation
			mesh_rotation = { 90.0f, 180.0f, 0.0f };

			// I/O parameters
			n_meshes = 2;
			n_frames = 1;

			mesh_prefix.resize(n_meshes);
			mesh_suffix.resize(n_meshes);
			mesh_first_idx.resize(n_meshes);
			mesh_n_digits.resize(n_meshes);
			mesh_has_albedo.resize(n_meshes);
			mesh_sh_coeff_filename.resize(n_meshes);
			mesh_type.resize(n_meshes);
			mesh_per_frame_values.resize(n_meshes);

			mesh_prefix[0] = "C:/Users/Qi/Documents/GitHub/_PangaeaResults/low_res_gray/mesh_pyramid/";
			mesh_suffix[0] = ".ply";
			mesh_first_idx[0] = 180;
			mesh_n_digits[0] = 4;
			mesh_has_albedo[0] = false;
			mesh_sh_coeff_filename[0] = "";
			mesh_type[0] = MESH_GT;
			mesh_per_frame_values[0] = false;

			mesh_prefix[1] = "C:/Users/Qi/Documents/GitHub/_PangaeaResults/test20_intrinsic_color/mesh_pyramid/mesh";
			mesh_suffix[1] = "_level00.obj";
			mesh_first_idx[1] = 1;
			mesh_n_digits[1] = 4;
			mesh_has_albedo[1] = true;
			mesh_sh_coeff_filename[1] = "C:/Users/Qi/Desktop/generated/images/x_-1_z_1_a_0.1/_sh_coeff.txt";
			mesh_type[1] = MESH_INTRINSIC;

			intrinsics_filename = "C:/Users/Qi/Documents/GitHub/data/levi/images/sh_2_50k/intrinsics.txt";

			save_images = false;
			save_image_prefix = "";
			save_image_suffix = "";
		}

		// Window parameters
		string window_title;
		int window_width;
		int window_height;

		// View parameters
		float frustum_left;
		float frustum_right;
		float frustum_bottom;
		float frustum_top;
		float frustum_near;
		float frustum_far;
		vector<float> eye;

		// Light parameters
		vector<float> light_ambient;
		vector<float> light_position;
		vector<float> light_intensity;

		// Colours
		vector<float> background_colour;
		vector<float> mesh_colour;

		vector<float> heat_dist_range;
		vector<float> heat_orient_range;
		vector<float> heat_proj_range;

		bool reverse_normal;
		
		// Mesh initial rotation
		vector<float> mesh_rotation;

		// I/O parameters
		int n_meshes;
		int n_frames;

		vector<string> mesh_prefix;
		vector<string> mesh_suffix;
		vector<int> mesh_first_idx;
		vector<int> mesh_n_digits;
		vector<int> mesh_has_albedo;
		vector<int> mesh_type;
		vector<string> mesh_sh_coeff_filename;
		vector<int> mesh_per_frame_values;

		string intrinsics_filename;

		bool save_images;
		string save_image_prefix;
		string save_image_suffix;


		// Load parameters from an ini file
		inline void load(const std::string &_filename)
		{
			cv::FileStorage fs(_filename, cv::FileStorage::READ);

			// View parameters
			if (!fs[FRUSTUM_LEFT].empty())
			{
				fs[FRUSTUM_LEFT] >> frustum_left;
			}

			if (!fs[FRUSTUM_RIGHT].empty())
			{
				fs[FRUSTUM_RIGHT] >> frustum_right;
			}

			if (!fs[FRUSTUM_BOTTOM].empty())
			{
				fs[FRUSTUM_BOTTOM] >> frustum_bottom;
			}

			if (!fs[FRUSTUM_TOP].empty())
			{
				fs[FRUSTUM_TOP] >> frustum_top;
			}

			if (!fs[FRUSTUM_NEAR].empty())
			{
				fs[FRUSTUM_NEAR] >> frustum_near;
			}

			if (!fs[FRUSTUM_FAR].empty())
			{
				fs[FRUSTUM_FAR] >> frustum_far;
			}

			if (!fs[EYE].empty())
			{
				fs[EYE] >> eye;
			}


			// Light parameters
			if (!fs[LIGHT_AMBIENT].empty())
			{
				fs[LIGHT_AMBIENT] >> light_ambient;
			}

			if (!fs[LIGHT_POSITION].empty())
			{
				fs[LIGHT_POSITION] >> light_position;
			}

			if (!fs[LIGHT_INTENSITY].empty())
			{
				fs[LIGHT_INTENSITY] >> light_intensity;
			}


			// Colours
			if (!fs[BACKGROUND_COLOUR].empty())
			{
				fs[BACKGROUND_COLOUR] >> background_colour;
			}

			if (!fs[MESH_COLOUR].empty())
			{
				fs[MESH_COLOUR] >> mesh_colour;
			}

			if (!fs[HEAT_DIST_RANGE].empty())
			{
				fs[HEAT_DIST_RANGE] >> heat_dist_range;
			}

			if (!fs[HEAT_ORIENT_RANGE].empty())
			{
				fs[HEAT_ORIENT_RANGE] >> heat_orient_range;
			}

			if (!fs[HEAT_PROJ_RANGE].empty())
			{
				fs[HEAT_PROJ_RANGE] >> heat_proj_range;
			}

			if (!fs[REVERSE_NORMAL].empty())
			{
				fs[REVERSE_NORMAL] >> reverse_normal;
			}

			// Mesh initial rotation
			if (!fs[MESH_ROTATION].empty())
			{
				fs[MESH_ROTATION] >> mesh_rotation;
			}

			// I/O parameters
			if (!fs[N_MESHES].empty())
			{
				fs[N_MESHES] >> n_meshes;
			}

			if (!fs[N_FRAMES].empty())
			{
				fs[N_FRAMES] >> n_frames;
			}

			mesh_prefix.resize(n_meshes);
			mesh_suffix.resize(n_meshes);
			mesh_sh_coeff_filename.resize(n_meshes);

			for (int i = 0; i < n_meshes; i++)
			{
				string separator = "_";
				string prefix = MESH_PREFIX + separator + to_string(i);
				string suffix = MESH_SUFFIX + separator + to_string(i);
				string sh_coeff_filename = MESH_SH_COEFF_FILENAME + separator + to_string(i);

				if (!fs[prefix].empty())
				{
					fs[prefix] >> mesh_prefix[i];
				}

				if (!fs[suffix].empty())
				{
					fs[suffix] >> mesh_suffix[i];
				}

				if (!fs[sh_coeff_filename].empty())
				{
					fs[sh_coeff_filename] >> mesh_sh_coeff_filename[i];
				}
			}

			if (!fs[MESH_FIRST_IDX].empty())
			{
				fs[MESH_FIRST_IDX] >> mesh_first_idx;
			}

			if (!fs[MESH_N_DIGITS].empty())
			{
				fs[MESH_N_DIGITS] >> mesh_n_digits;
			}

			mesh_has_albedo.resize(n_meshes, 0);
			if (!fs[MESH_HAS_ALBEDO].empty())
			{
				fs[MESH_HAS_ALBEDO] >> mesh_has_albedo;
			}

			if (!fs[MESH_TYPE].empty())
			{
				fs[MESH_TYPE] >> mesh_type;
			}

			if (!fs[MESH_PER_FRAME_VALUES].empty())
			{
				fs[MESH_PER_FRAME_VALUES] >> mesh_per_frame_values;
			}

			if (!fs[INTRINSICS_FILENAME].empty())
			{
				fs[INTRINSICS_FILENAME] >> intrinsics_filename;
			}

			if (!fs[SAVE_IMAGES].empty())
			{
				fs[SAVE_IMAGES] >> save_images;
			}

			if (!fs[SAVE_IMAGE_PREFIX].empty())
			{
				fs[SAVE_IMAGE_PREFIX] >> save_image_prefix;
			}

			if (!fs[SAVE_IMAGE_SUFFIX].empty())
			{
				fs[SAVE_IMAGE_SUFFIX] >> save_image_suffix;
			}
		}

		inline void save(const std::string &_filename)
		{

		}
	};
}

#endif  // PARAMETERS_H_
