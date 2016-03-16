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

// I/O parameters
#define N_MESHES		"n_meshes"
#define N_FRAMES		"n_frames"

#define MESH_PREFIX				"mesh_prefix"
#define MESH_SUFFIX				"mesh_suffix"
#define MESH_FIRST_IDX			"mesh_first_idx"
#define MESH_N_DIGITS			"mesh_n_digits"
#define MESH_HAS_ALBEDO			"mesh_has_albedo"
#define MESH_SH_COEFF_FILENAME	"mesh_sh_coeff_filename"

#define SPECULAR_THRESHOLD	"specular_threshold"

#define SAVE_MESH_PREFIX			"save_mesh_prefix"
#define SAVE_MESH_SUFFIX			"save_mesh_suffix"
#define SAVE_MESH_FIRST_IDX			"save_mesh_first_idx"
#define SAVE_MESH_N_DIGITS			"save_mesh_n_digits"
#define SAVE_N_INTERFRAMES_FILENAME	"save_n_interframes_filename"

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

			// I/O parameters
			n_meshes = 2;
			n_frames = 1;

			mesh_prefix.resize(n_meshes);
			mesh_suffix.resize(n_meshes);
			mesh_first_idx.resize(n_meshes);
			mesh_n_digits.resize(n_meshes);
			mesh_has_albedo.resize(n_meshes);
			mesh_sh_coeff_filename.resize(n_meshes);

			mesh_prefix[0] = "C:/Users/Qi/Documents/GitHub/_PangaeaResults/low_res_gray/mesh_pyramid/";
			mesh_suffix[0] = ".ply";
			mesh_first_idx[0] = 180;
			mesh_n_digits[0] = 4;
			mesh_has_albedo[0] = false;
			mesh_sh_coeff_filename[0] = "";

			mesh_prefix[1] = "C:/Users/Qi/Documents/GitHub/_PangaeaResults/test20_intrinsic_color/mesh_pyramid/mesh";
			mesh_suffix[1] = "_level00.obj";
			mesh_first_idx[1] = 1;
			mesh_n_digits[1] = 4;
			mesh_has_albedo[1] = true;
			mesh_sh_coeff_filename[1] = "C:/Users/Qi/Desktop/generated/images/x_-1_z_1_a_0.1/_sh_coeff.txt";

			specular_threshold = 0.8;

			save_mesh_prefix = "";
			save_mesh_suffix = "";
			save_mesh_first_idx = 1;
			save_mesh_n_digits = 4;
			save_n_interframes_filename = "";
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

		// I/O parameters
		int n_meshes;
		int n_frames;

		vector<string> mesh_prefix;
		vector<string> mesh_suffix;
		vector<int> mesh_first_idx;
		vector<int> mesh_n_digits;
		vector<int> mesh_has_albedo;
		vector<string> mesh_sh_coeff_filename;

		float specular_threshold;

		string save_mesh_prefix;
		string save_mesh_suffix;
		int save_mesh_first_idx;
		int save_mesh_n_digits;
		string save_n_interframes_filename;

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

			if (!fs[MESH_HAS_ALBEDO].empty())
			{
				fs[MESH_HAS_ALBEDO] >> mesh_has_albedo;
			}

			if (!fs[SPECULAR_THRESHOLD].empty())
			{
				fs[SPECULAR_THRESHOLD] >> specular_threshold;
			}

			if (!fs[SAVE_MESH_PREFIX].empty())
			{
				fs[SAVE_MESH_PREFIX] >> save_mesh_prefix;
			}

			if (!fs[SAVE_MESH_SUFFIX].empty())
			{
				fs[SAVE_MESH_SUFFIX] >> save_mesh_suffix;
			}

			if (!fs[SAVE_MESH_FIRST_IDX].empty())
			{
				fs[SAVE_MESH_FIRST_IDX] >> save_mesh_first_idx;
			}

			if (!fs[SAVE_MESH_N_DIGITS].empty())
			{
				fs[SAVE_MESH_N_DIGITS] >> save_mesh_n_digits;
			}
			
			if (!fs[SAVE_N_INTERFRAMES_FILENAME].empty())
			{
				fs[SAVE_N_INTERFRAMES_FILENAME] >> save_n_interframes_filename;
			}
		}

		inline void save(const std::string &_filename)
		{

		}
	};
}

#endif  // PARAMETERS_H_
