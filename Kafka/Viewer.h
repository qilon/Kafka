//*****************************************************************************
// Qi Liu Yin
// Recovering High Frequency Surface Details for Dynamic 4D Capture
// UCL - MSc Computer Graphics, Vision and Imaging
//*****************************************************************************
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glui.h>

#include <boost/thread/thread.hpp>

#include "Parameters.h"
#include "Mesh.h"
//=============================================================================
/**** KEYBOARD KEYS ****/
#define UPPER_A 65
#define LOWER_A 97
#define UPPER_C 67
#define LOWER_C 99
#define UPPER_D 68
#define LOWER_D 100
#define UPPER_E 69
#define LOWER_E 101
#define UPPER_F 70
#define LOWER_F 102
#define UPPER_I 73
#define LOWER_I 105
#define UPPER_K 75
#define LOWER_K 107
#define UPPER_L 76
#define LOWER_L 108
#define UPPER_M 77
#define LOWER_M 109
#define UPPER_O 79
#define LOWER_O 111
#define UPPER_Q 81
#define LOWER_Q 113
#define UPPER_R 82
#define LOWER_R 114
#define UPPER_S 83
#define LOWER_S 115
#define UPPER_T 84
#define LOWER_T 116
#define UPPER_W 87
#define LOWER_W 119

#define MODE_INTENSITY	0
#define MODE_ALBEDO		1
#define MODE_SHADING	2
#define MODE_UNICOLOR	3
#define MODE_HEATMAP	4

#define MODE_NO_LIGHT	0
#define MODE_GL_LIGHT	1
#define MODE_SH_LIGHT	2

#define MIN_DISTANCE	0
#define MAX_DISTANCE	3

//=============================================================================
using namespace std;
using namespace Eigen;
//=============================================================================
class Viewer
{
private:
	/**** CONSTANTS ****/

	/* PERSPECTIVE PROPERTIES */
	const static GLfloat ZOOM_INCR;	/* zoom increment on */

	/* GLUI CONTROL PARAMETERS */
	const static float TRANSLATION_SPEED;
	const static float ZOOM_SPEED;
	const static float ROTATION_SPIN_FACTOR;

	const static char* FRAME_TEXT;
	const static char* PLAY_TEXT;
	const static char* PAUSE_TEXT;

	const static double FRAME_FREQ;

	const static int PLAY_BUTTON_WIDTH;

	const static int N_COLOR_MODES;
	const static char* STRING_COLOR_MODES[];

	const static int N_LIGHT_MODES;
	const static char* STRING_LIGHT_MODES[];

	//=========================================================================

	/**** VARIABLES ****/

	static parameters::Parameters params;

	static OpenMesh::IO::Options ropt;

	/* MAIN VARIABLES */
	static vector<vector<Mesh>> meshes;
	static vector<Mesh::Point> curr_gt_vertices;
	static int curr_frame;
	static bool play;
	static clock_t last_time;

	static vector<vector<float>> sh_coeff;

	static VectorXf sh_coeff_eigen;
	static MatrixXf* sh_functions;
	static MatrixXf albedo;
	static VectorXf* shading;
	
	//static VectorXi mode;

	/* VIEW VARIABLES */
	static GLfloat eye[3]; /* eye position*/
	static GLfloat aspectRatio; /* view aspect ratio*/
	static GLfloat frustum_right;

	/* ROTATION AND TRANSLATION MATRIXES*/
	static GLfloat translation[];
	static GLfloat rotation[];

	static vector<vector<GLfloat>> mesh_center;
	static vector<vector<GLfloat>> mesh_translation;
	static vector<int> mesh_color_mode;
	static int light_mode;

	/* MOUSE CONTROL VARIABLES */
	static int moving;
	static int beginx, beginy;
	static GLfloat anglex, angley;

	/* GLUI COMPONENTS */
	static int window_id;
	static GLUI* glui;
	static GLUI_Scrollbar* glui_frame_scroll;
	static GLUI_StaticText* glui_frame_text;
	static GLUI_Button* glui_play_button;
	static GLUI_Rotation* glui_rotation;
	static GLUI_Translation* glui_trans_xy;
	static GLUI_Translation* glui_trans_z;
	static vector<GLUI_Listbox*> glui_color_list;
	static GLUI_Listbox* glui_light_list;

	//=========================================================================

	/**** PRIVATE FUNCTIONS ****/

	/* INITIALIZATION METHODS */
	static void initGLUT(int *argc, char **argv);
	static void initGLUI(void);
	static void initGLUIComponents(void);

	/* GLUT AND GLUI FUNCTIONS */
	static void display(void);
	static void reshape(int x, int y);
	static void mouse(int button, int state, int x, int y);
	static void motion(int x, int y);
	static void key(unsigned char key, int x, int y);
	static void idle(void);
	static void updateFrame(int state);
	static void updateFrameText();
	static void updatePlay(int state);

	/* DRAWING FUNCTIONS */
	static void drawModel(int _mesh_idx, int _frame_idx);
	static void drawSphere();

	/* OTHER FUNCTIONS */
	static void zoom(GLfloat distance); /* increase or decrease eye depth */
	static void calculateCenterPoint(int _mesh_idx); /* calculates rotation point based on mesh size */
	static string cvtIntToString(int _n, int _no_digits);
	static int numDigits(int _number);
	static MatrixXf normals2colour(const MatrixXf &_normals);
	static MatrixXf normals2SHfunctions(const MatrixXf &_normals, int _order);

	static void loadMeshes();
	static string getMeshFilename(int _mesh_idx, int _frame_idx);
	static void readSHCoeff(vector<float> &_sh_coeff, const string _sh_coeff_filename);
	static GLfloat getShading(float* _normal, vector<float> &_sh_coeff);
	static void computeHeatMapDistanceColor(GLfloat* _color, const Mesh::Point &_vertex,
		const Mesh::Point &_gt_vertex, const GLfloat _min, const GLfloat _max);

public:
	static void initialize(int *argcp, char **argv);
	static void run();
	static void destroy();
};
//=============================================================================
