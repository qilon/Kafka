//*****************************************************************************
// Qi Liu Yin
// Recovering High Frequency Surface Details for Dynamic 4D Capture
// UCL - MSc Computer Graphics, Vision and Imaging
//*****************************************************************************
#include "Viewer.h"
//=============================================================================
/**** CONSTANTS****/

/* PERSPECTIVE PROPERTIES */
const GLfloat Viewer::ZOOM_INCR = 5.f;

/* GLUI CONTROL PARAMETERS */
const float Viewer::TRANSLATION_SPEED = .5f;
const float Viewer::ZOOM_SPEED = .5f;
const float Viewer::ROTATION_SPIN_FACTOR = .2f;

const char* Viewer::FRAME_TEXT = "Frame: ";
const char* Viewer::PLAY_TEXT = "Play";
const char* Viewer::PAUSE_TEXT = "Pause";

const double Viewer::FRAME_FREQ = 1000.f / 30.f;

const int Viewer::PLAY_BUTTON_WIDTH = 300;

const int Viewer::N_COLOR_MODES = 5;
const char* Viewer::STRING_COLOR_MODES[] = {
	"Intensity", 
	"Albedo", 
	"Shading", 
	"Single color", 
	"Heat map"
};

const int Viewer::N_LIGHT_MODES = 3;
const char* Viewer::STRING_LIGHT_MODES[] = {
	"None",
	"GL",
	"SH",
};
//=============================================================================
/**** VARIABLES ****/

parameters::Parameters Viewer::params;

OpenMesh::IO::Options Viewer::ropt;

/* MAIN VARIABLES */
vector<vector<Mesh>> Viewer::meshes;
vector<Mesh::Point> Viewer::curr_gt_vertices;
int Viewer::curr_frame = 1;
bool Viewer::play = false;
clock_t Viewer::last_time;

vector<vector<float>> Viewer::sh_coeff;

VectorXf Viewer::sh_coeff_eigen;
MatrixXf* Viewer::sh_functions;
MatrixXf Viewer::albedo;
VectorXf* Viewer::shading;

//MatrixXf** Viewer::intensities = nullptr;
//VectorXi Viewer::mode;

/* VIEW VARIABLES */
GLfloat Viewer::eye[3] = { 0.f, 0.f, params.eye[2] };
GLfloat Viewer::aspectRatio = 1.f;
GLfloat Viewer::frustum_right;

/* ROTATION AND TRANSLATION MATRIXES*/
GLfloat Viewer::translation[3] = { 0.f, 0.f, 0.f };
GLfloat Viewer::rotation[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
vector<vector<GLfloat>> Viewer::mesh_center;
vector<vector<GLfloat>> Viewer::mesh_translation;
vector<int> Viewer::mesh_color_mode;
int Viewer::light_mode = MODE_NO_LIGHT;

/* MOUSE CONTROL VARIABLES */
int Viewer::moving = 0;
int Viewer::beginx = 0;
int Viewer::beginy = 0;
float Viewer::anglex = 0.f;
float Viewer::angley = 0.f;

/* GLUI COMPONENTS */
GLUI* Viewer::glui = nullptr;
int Viewer::window_id;
GLUI_Scrollbar* Viewer::glui_frame_scroll = nullptr;
GLUI_StaticText* Viewer::glui_frame_text = nullptr;
GLUI_Button* Viewer::glui_play_button = nullptr;
GLUI_Rotation* Viewer::glui_rotation = nullptr;
GLUI_Translation* Viewer::glui_trans_xy = nullptr;
GLUI_Translation* Viewer::glui_trans_z = nullptr;
vector<GLUI_Listbox*> Viewer::glui_color_list;
GLUI_Listbox* Viewer::glui_light_list;
//=============================================================================
void Viewer::initGLUT(int *argc, char **argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowSize(params.window_width, params.window_height);

	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - params.window_width) / 2,
		10);

	window_id = glutCreateWindow(params.window_title.c_str());

	// Uncomment to enable transparencies
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);

	glutDisplayFunc(display);
	glutMotionFunc(motion);

	glewInit();

	glClearColor(params.background_colour[0], params.background_colour[1],
		params.background_colour[2], params.background_colour[3]);

	/* Use depth buffering for hidden surface elimination. */
	glEnable(GL_DEPTH_TEST);

	/* Anti-aliasing*/
	glEnable(GL_MULTISAMPLE);

	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}
//=============================================================================
void Viewer::initGLUI(void)
{
	glui = GLUI_Master.create_glui_subwindow(window_id,
		GLUI_SUBWINDOW_BOTTOM);
	glui->set_main_gfx_window(window_id);

	GLUI_Master.set_glutReshapeFunc(reshape);
	GLUI_Master.set_glutKeyboardFunc(key);
	GLUI_Master.set_glutSpecialFunc(NULL);
	GLUI_Master.set_glutMouseFunc(mouse);
	GLUI_Master.set_glutIdleFunc(idle);

	initGLUIComponents();
}
//=============================================================================
/*
 * Function that initialize the GLUI components
 * Adds the following elements:
 * - list of feature spinners from the info in pca
 * - translation tool
 * - zoom tool
 * - rotation tool
 * - check for showing or hiding guidance circles
 */
void Viewer::initGLUIComponents(void)
{
	GLUI_Panel* glui_panel = glui->add_panel("", GLUI_PANEL_NONE);

	GLUI_Panel* glui_panel_1 = glui->add_panel_to_panel(glui_panel, "", GLUI_PANEL_NONE);

	glui_frame_scroll = new GLUI_Scrollbar(glui_panel_1, "Scrollbar",
		GLUI_SCROLL_HORIZONTAL, GLUI_SCROLL_INT, -1, updateFrame);
	glui_frame_scroll->set_int_limits(1, params.n_frames, 1);
	glui_frame_scroll->set_w(glutGet(GLUT_WINDOW_WIDTH) - PLAY_BUTTON_WIDTH);
	glui_frame_scroll->set_int_limits(1, params.n_frames);

	glui->add_column_to_panel(glui_panel_1, 0);

	glui_frame_text = new GLUI_StaticText(glui_panel_1, "");
	glui_frame_text->set_alignment(GLUI_ALIGN_CENTER);
	updateFrameText();

	glui->add_column_to_panel(glui_panel_1, 0);

	glui_play_button = new GLUI_Button(glui_panel_1, PLAY_TEXT, -1, updatePlay);

	GLUI_Panel* glui_panel_2 = glui->add_panel_to_panel(glui_panel, "", GLUI_PANEL_NONE);

	glui_rotation = new GLUI_Rotation(glui_panel_2, "Rotate", rotation);
	glui->add_column_to_panel(glui_panel_2, 0);
	glui_trans_xy = new GLUI_Translation(glui_panel_2, "Translate", 
		GLUI_TRANSLATION_XY, translation);
	glui->add_column_to_panel(glui_panel_2, 0);
	glui_trans_z = new GLUI_Translation(glui_panel_2, "Zoom in/out", 
		GLUI_TRANSLATION_Z, &translation[2]);
	glui_trans_z->set_speed(5);

	glui->add_column_to_panel(glui_panel_2, 0);
	glui_color_list.resize(params.n_meshes);
	for (int i = 0; i < params.n_meshes; i++)
	{
		string text = i==0 ? "Ground truth color : " : "Result " + to_string(i) + " color :       ";

		glui_color_list[i] = new GLUI_Listbox(glui_panel_2, text.c_str(), &mesh_color_mode[i]);
		int n_modes = N_COLOR_MODES - (i > 0 ? 0 : 1);
		for (int j = 0; j < n_modes; j++)
		{
			glui_color_list[i]->add_item(j, STRING_COLOR_MODES[j]);
		}
	}

	glui->add_column_to_panel(glui_panel_2, 0);
	glui_light_list = new GLUI_Listbox(glui_panel_2, "Light : ", &light_mode);
	for (int i = 0; i < N_LIGHT_MODES; i++)
	{
		glui_light_list->add_item(i, STRING_LIGHT_MODES[i]);
	}
}
//=============================================================================
void Viewer::updateFrame(int state)
{
	curr_frame = glui_frame_scroll->get_int_val();
	updateFrameText();
}
//=============================================================================
void Viewer::updateFrameText()
{
	string text = FRAME_TEXT + to_string(curr_frame);
	glui_frame_text->set_text(text.c_str());
}
//=============================================================================
void Viewer::updatePlay(int state)
{
	play = !play;
	if (play)
	{
		glui_play_button->set_text(PAUSE_TEXT);
		glui_play_button->set_name(PAUSE_TEXT);
	}
	else
	{
		glui_play_button->set_text(PLAY_TEXT);
		glui_play_button->set_name(PLAY_TEXT);

		if (curr_frame == params.n_frames)
			curr_frame = 1;
	}
}
//=============================================================================
void Viewer::display(void)
{
	clock_t curr_time = clock();
	double diff_time = (curr_time - last_time) / (double)(CLOCKS_PER_SEC / 1000);

	if (play && diff_time >= FRAME_FREQ && curr_frame<params.n_frames)
	{
		curr_frame++;
		glui_frame_scroll->set_int_val(curr_frame);
		updateFrameText();
		last_time = curr_time;

		if (curr_frame == params.n_frames)
		{
			updatePlay(0);
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	switch (light_mode)
	{
	case MODE_GL_LIGHT:
		/* Enable a single OpenGL light. */
		glLightfv(GL_LIGHT0, GL_DIFFUSE, &params.light_intensity[0]);
		glLightfv(GL_LIGHT0, GL_POSITION, &params.light_position[0]);
		glEnable(GL_COLOR_MATERIAL);
		glLightfv(GL_LIGHT0, GL_AMBIENT, &params.light_ambient[0]);
		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHTING);
		break;
	default:
		glDisable(GL_LIGHTING);
		break;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(-frustum_right, frustum_right,
		-params.frustum_top, params.frustum_top,
		params.frustum_near, params.frustum_far);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	gluLookAt(eye[0], eye[1], eye[2] - mesh_center[0][2],	/* eye */
		eye[0], eye[1], 1.f,	/* center */
		0.f, -1.f, 0.f);	/* up is in positive Y direction */
	
	glTranslatef(translation[0], -translation[1], -translation[2]);

	for (int i = 0; i < params.n_meshes; i++)
	{
		glPushMatrix();
		glTranslatef(mesh_translation[i][0], mesh_translation[i][1], 
			mesh_translation[i][2]);
		glMultMatrixf(rotation);
		drawModel(i, curr_frame - 1);
		glPopMatrix();
	}

	glutSwapBuffers();
}
//=============================================================================
void Viewer::reshape(int x, int y)
{
	int tx, ty, tw, th;
	GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
	glViewport(tx, ty, tw, th);

	aspectRatio = (float)tw / (float)th;

	glui_frame_scroll->set_w(glutGet(GLUT_WINDOW_WIDTH) - PLAY_BUTTON_WIDTH);

	frustum_right = aspectRatio * params.frustum_top;

	GLfloat max_right = mesh_center[0][2] * frustum_right / params.frustum_near;
	GLfloat offset_x = max_right / params.n_meshes;
	for (int i = 0; i < params.n_meshes; i++)
	{
		mesh_translation[i][0] = -max_right + (2 * i + 1) * offset_x;
	}

	glutPostRedisplay();
}
//=============================================================================
void Viewer::mouse(int button, int state, int x, int y)
{
	// Left button pressed
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		moving = 1;
		beginx = x;
		beginy = y;
	}
	// Left button released
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		moving = 0;
	}
	// Wheel scroll down
	if (button == 3 && state == GLUT_DOWN) {
		zoom(-ZOOM_INCR);
	}
	// Wheel scroll up
	if (button == 4 && state == GLUT_DOWN) {
		zoom(ZOOM_INCR);
	}
}
//=============================================================================
void Viewer::motion(int x, int y)
{
	if (moving) {

		anglex = anglex + (x - beginx);
		angley = angley + (y - beginy);

		beginx = x;
		beginy = y;

		glutPostRedisplay();
	}
}
//=============================================================================
/*
* Controls:
* R/F -> Zoom in/out
* C   -> Show/hide guidance circles
*/
void Viewer::key(unsigned char key, int x, int y) {
	switch (key) {
	case UPPER_R:
		zoom(ZOOM_INCR);
		break;
	case LOWER_R:
		zoom(ZOOM_INCR);
		break;
	case UPPER_F:
		zoom(-ZOOM_INCR);
		break;
	case LOWER_F:
		zoom(-ZOOM_INCR);
		break;
	case UPPER_C:
		break;
	case LOWER_C:
		break;
	default:
		break;
	}
	glutPostRedisplay();
}
//=============================================================================
void Viewer::idle(void)
{
	/* According to the GLUT specification, the current window is
	undefined during an idle callback.  So we need to explicitly change
	it if necessary */
	if (glutGetWindow() != window_id)
		glutSetWindow(window_id);

	glutPostRedisplay();
}
//=============================================================================
void Viewer::drawModel(int _mesh_idx, int _frame_idx)
{
	glRotatef(90, 0, 0, 1);

	Mesh* mesh2draw = &meshes[_mesh_idx][_frame_idx];

	if (_mesh_idx == 0)
	{
		curr_gt_vertices.resize(meshes[_mesh_idx][_frame_idx].n_vertices());
	}

	glBegin(GL_TRIANGLES);

	Mesh::FaceIter f_it;
	Mesh::FaceIter f_end = mesh2draw->faces_end();
	for (f_it = mesh2draw->faces_begin(); f_it != f_end; ++f_it)
	{
		Mesh::FaceVertexIter fv_it;
		for (fv_it = mesh2draw->fv_iter(*f_it); fv_it.is_valid(); ++fv_it)
		{
			int v_idx = fv_it.handle().idx();

			Mesh::Point p = mesh2draw->point(*fv_it);

			GLfloat point[3] {p[0] - mesh_center[0][0],
				p[1] - mesh_center[0][1],
				p[2] - mesh_center[0][2]};

			Mesh::Normal n = mesh2draw->normal(*fv_it);
			GLfloat normal[3] {n[0], n[1], n[2]};

			Mesh::Color c = mesh2draw->color(*fv_it);

			GLfloat color[3];
			GLfloat shading;
			switch (mesh_color_mode[_mesh_idx])
			{
			case MODE_INTENSITY:
				if (params.mesh_has_albedo[_mesh_idx])
				{
					shading = getShading(normal, sh_coeff[_mesh_idx]);
					color[0] = c[0] * shading;
					color[1] = c[1] * shading;
					color[2] = c[2] * shading;
				}
				else
				{
					color[0] = c[0];
					color[1] = c[1];
					color[2] = c[2];
				}
				break;
			case MODE_ALBEDO:
				color[0] = c[0];
				color[1] = c[1];
				color[2] = c[2];
				break;
			case MODE_SHADING:
				if (params.mesh_has_albedo[_mesh_idx])
				{
					shading = getShading(normal, sh_coeff[_mesh_idx]);
					color[0] = shading;
					color[1] = shading;
					color[2] = shading;
				}
				else
				{
					color[0] = c[0];
					color[1] = c[1];
					color[2] = c[2];
				}
				break;
			case MODE_UNICOLOR:
				color[0] = params.mesh_colour[0];
				color[1] = params.mesh_colour[1];
				color[2] = params.mesh_colour[2];
				break;
			case MODE_HEATMAP:
				computeHeatMapDistanceColor(color, p, curr_gt_vertices[v_idx], 
					MIN_DISTANCE, MAX_DISTANCE);
				break;
			default:
				break;
			}

			glColor3fv(color);

			glNormal3fv(normal);
			glVertex3fv(point);

			if (_mesh_idx == 0)
			{
				curr_gt_vertices[v_idx][0] = p[0];
				curr_gt_vertices[v_idx][1] = p[1];
				curr_gt_vertices[v_idx][2] = p[2];
			}
		}
	}
	glEnd();
}
//=============================================================================
void Viewer::zoom(GLfloat distance)
{
	translation[2] += distance;
	glui_trans_z->set_float_val(translation[2]);
}
//=============================================================================
void Viewer::calculateCenterPoint(int _mesh_idx)
{
	Mesh* curr_mesh = &meshes[_mesh_idx][0];

	GLfloat max_x = numeric_limits<float>::min(),
		max_y = numeric_limits<float>::min(),
		max_z = numeric_limits<float>::min();
	GLfloat min_x = numeric_limits<float>::max(),
		min_y = numeric_limits<float>::max(),
		min_z = numeric_limits<float>::max();
	Mesh::VertexIter v_it, v_end(curr_mesh->vertices_end());
	int i_v = 0;
	for (v_it = curr_mesh->vertices_begin(); v_it != v_end; v_it++)
	{
		Mesh::Point p = curr_mesh->point(*v_it);
		max_x = max(max_x, p[0]);
		max_y = max(max_y, p[1]);
		max_z = max(max_z, p[2]);
		min_x = min(min_x, p[0]);
		min_y = min(min_y, p[1]);
		min_z = min(min_z, p[2]);
	}

	mesh_center[_mesh_idx].resize(3);
	mesh_center[_mesh_idx][0] = min_x + (max_x - min_x) / 2;
	mesh_center[_mesh_idx][1] = min_y + (max_y - min_y) / 2;
	mesh_center[_mesh_idx][2] = min_z + (max_z - min_z) / 2;
}
//=============================================================================
void Viewer::initialize(int *argc, char **argv)
{
	string config_filename = argv[1];
	params.load(config_filename);

	sh_coeff.resize(params.n_meshes);
	mesh_color_mode.resize(params.n_meshes);
	for (int i = 0; i < params.n_meshes; i++)
	{
		if (params.mesh_has_albedo[i])
		{
			readSHCoeff(sh_coeff[i], params.mesh_sh_coeff_filename[i]);
		}
		mesh_color_mode[i] = MODE_INTENSITY;
	}

	loadMeshes();

	initGLUT(argc, argv);
	initGLUI();
}
//=============================================================================
void Viewer::run()
{
	last_time = clock();

	glutMainLoop();
}
//=============================================================================
void Viewer::destroy()
{

}
//=============================================================================
void Viewer::loadMeshes()
{
	std::cout << "Loading meshes... ";

	ropt += OpenMesh::IO::Options::VertexNormal;
	ropt += OpenMesh::IO::Options::VertexColor;

	clock_t start;
	start = clock();

	meshes.resize(params.n_meshes);
	mesh_center.resize(params.n_meshes);
	mesh_translation.resize(params.n_meshes);
	mesh_color_mode.resize(params.n_meshes);

	vector<boost::thread*> threads;
	for (int i = 0; i < params.n_meshes; i++)
	{
		meshes[i].resize(params.n_frames);
		mesh_translation[i].resize(3);
		for (int j = 0; j < params.n_frames; j++)
		{
			string mesh_filename = getMeshFilename(i, j);
			//threads.push_back(
			//	new boost::thread(
			//	&readMesh, boost::ref(meshes[i][j]), mesh_filename, ropt)
			//	);
			//threads[i]->join();
			readMesh(meshes[i][j], mesh_filename, ropt);
		}
	}

	//for (int i = 0; i < params.n_meshes * params.n_frames; i++)
	//{
	//	threads[i]->join();
	//	delete threads[i];
	//}

	for (int i = 0; i < params.n_meshes; i++)
	{
		calculateCenterPoint(i);
	}

	std::cout << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000)
		<< " ms" << endl;
}
//=============================================================================
string Viewer::cvtIntToString(int _n, int _no_digits)
{
	int no_zeros = _no_digits - numDigits(_n);

	return string(no_zeros, '0').append(to_string(_n));
}
//=============================================================================
int Viewer::numDigits(int _number)
{
	int digits = 0;
	while (_number) {
		_number /= 10;
		digits++;
	}

	return digits;
}
//=============================================================================
MatrixXf Viewer::normals2colour(const MatrixXf &_normals)
{
	MatrixXf colours = _normals;
	colours += MatrixXf::Ones(colours.rows(), colours.cols());
	colours /= 2;

	return colours;
}
//=============================================================================
MatrixXf Viewer::normals2SHfunctions(const MatrixXf &_normals, int _order)
{
	MatrixXf h;
	int no_points = _normals.rows();

	VectorXf x = _normals.col(0);				// x
	VectorXf y = _normals.col(1);				// y 
	VectorXf z = _normals.col(2);				// z
	VectorXf x2 = x.cwiseProduct(x);			// x^2
	VectorXf y2 = y.cwiseProduct(y);			// y^2
	VectorXf z2 = z.cwiseProduct(z);			// z^2
	VectorXf xy = x.cwiseProduct(y);			// x * y
	VectorXf xz = x.cwiseProduct(z);			// x * z
	VectorXf yz = y.cwiseProduct(z);			// y * z
	VectorXf x2_y2 = x2 - y2;					// x^2 - y^2
	VectorXf ones = VectorXf::Ones(no_points);	// 1

	h = MatrixXf::Ones(no_points, (_order + 1) * (_order + 1));

	if (_order > 0)
		h.block(0, 1, no_points, 3) = _normals;	// x, y, z

	if (_order > 1)
	{
		h.col(4) = xy;				// x * y
		h.col(5) = xz;				// x * z
		h.col(6) = yz;				// y * z
		h.col(7) = x2_y2;			// x^2 - y^2
		h.col(8) = 3 * z2 - ones;	// 3 * z^2 - 1
	}

	if (_order > 2)
	{
		h.col(9) = (3 * x2 - y2).cwiseProduct(y);			// (3 * x^2 - y^2) * y 
		h.col(10) = xy.cwiseProduct(z);						// x * y * z
		h.col(11) = (5 * z2 - ones).cwiseProduct(y);		// (5 * z^2 - 1) * y
		h.col(12) = (5 * z2 - 3 * ones).cwiseProduct(z);	// (5 * z^2 - 3) * z
		h.col(13) = (5 * z2 - ones).cwiseProduct(x);		// (5 * z^2 - 1) * x
		h.col(14) = x2_y2.cwiseProduct(z);					// (x^2 - y^2) * z
		h.col(15) = (x2 - 3 * y2).cwiseProduct(x);			// (x^2 - 3 * y^2) * x
	}

	if (_order > 3)
	{
		h.col(16) = x2_y2.cwiseProduct(xy);							// (x^2 - y^2) * x * y
		h.col(17) = (3 * x2 - y2).cwiseProduct(yz);					// (3 * x^2 - y^2) * yz
		h.col(18) = (7 * z2 - ones).cwiseProduct(xy);				// (7 * z^2 - 1) * x * y
		h.col(19) = (7 * z2 - 3 * ones).cwiseProduct(yz);			// (7 * z^2 - 3) * y * z
		h.col(20) = 3 * ones - 30 * z2 + 35 * z2.cwiseProduct(z2);	// 3 - 30 * z^2 + 35 * z^4
		h.col(21) = (7 * z2 - 3 * ones).cwiseProduct(xz);			// (7 * z^2 - 3) * x * z
		h.col(22) = (7 * z2 - ones).cwiseProduct(x2_y2);			// (7 * z^2 - 1) * (x^2 - y^2)
		h.col(23) = (x2 - 3 * y2).cwiseProduct(xz);					// (x^2 - 3 * y^2) * x * z
		h.col(24) = (x2 - 3 * y2).cwiseProduct(x2)					// (x^2 - 3 * y^2) * x^2 - (3 * x^2 - y^2) * y^2 
			- (3 * x2 - y2).cwiseProduct(y2);
	}

	return h;
}
//=============================================================================
string Viewer::getMeshFilename(int _mesh_idx, int _frame_idx)
{
	string filename = params.mesh_prefix[_mesh_idx];
	int idx = _frame_idx + params.mesh_first_idx[_mesh_idx];
	filename.append(cvtIntToString(idx, params.mesh_n_digits[_mesh_idx]));
	filename.append(params.mesh_suffix[_mesh_idx]);

	return filename;
}
//=============================================================================
void Viewer::readSHCoeff(vector<float> &_sh_coeff, const string _sh_coeff_filename)
{
	ifstream ifs(_sh_coeff_filename);

	if (!ifs.is_open())
	{
		cerr << "could not open file " << _sh_coeff_filename << endl;
		return;
	}

	float value;
	while (ifs >> value) {
		_sh_coeff.push_back(value);
	}

	ifs.close();

	int sh_order = std::sqrt(_sh_coeff.size()) - 1;

	if ((sh_order + 1) * (sh_order + 1) != _sh_coeff.size())
	{
		cerr << "The number of SH coefficients is not correct (" << _sh_coeff_filename
			<< "). It should be equal to (n + 1)^2 where n is the SH order." << endl;
		return;
	}
}
//=============================================================================
GLfloat Viewer::getShading(float* _normal, vector<float> &_sh_coeff)
{
	GLfloat shading = 0.0f;

	GLfloat x = _normal[0];		// x
	GLfloat y = _normal[1];		// y
	GLfloat z = _normal[2];		// z
	GLfloat x2 = x * x;			// x^2
	GLfloat y2 = y * y;			// y^2
	GLfloat z2 = z * z;			// z^2
	GLfloat xy = x * y;			// x * y
	GLfloat xz = x * z;			// x * z
	GLfloat yz = y * z;			// y * z
	GLfloat x2_y2 = x2 - y2;	// x^2 - y^2

	shading += _sh_coeff[0];

	if (_sh_coeff.size() > 1)
	{
		shading += _sh_coeff[1] * x;	// x
		shading += _sh_coeff[2] * y;	// y
		shading += _sh_coeff[3] * z;	// z
	}

	if (_sh_coeff.size() > 4)
	{
		shading += _sh_coeff[4] * xy;			// x * y
		shading += _sh_coeff[5] * xz;			// x * z
		shading += _sh_coeff[6] * yz;			// y * z
		shading += _sh_coeff[7] * x2_y2;		// x^2 - y^2
		shading += _sh_coeff[8] * (3 * z2 - 1);	// 3 * z^2 - 1
	}

	if (_sh_coeff.size() > 9)
	{
		shading += _sh_coeff[9] * (3 * x2 - y2) * y;		// (3 * x^2 - y^2) * y 
		shading += _sh_coeff[10] * xy * z;					// x * y * z
		shading += _sh_coeff[11] * (5 * z2 - 1) * y;		// (5 * z^2 - 1) * y
		shading += _sh_coeff[12] * (5 * z2 - 3 * 1) * z;	// (5 * z^2 - 3) * z
		shading += _sh_coeff[13] * (5 * z2 - 1) * x;		// (5 * z^2 - 1) * x
		shading += _sh_coeff[14] * x2_y2 * z;				// (x^2 - y^2) * z
		shading += _sh_coeff[15] * (x2 - 3 * y2) * x;		// (x^2 - 3 * y^2) * x
	}

	if (_sh_coeff.size() > 15)
	{
		shading += _sh_coeff[16] * x2_y2 * xy;								// (x^2 - y^2) * x * y
		shading += _sh_coeff[17] * (3 * x2 - y2) * yz;						// (3 * x^2 - y^2) * yz
		shading += _sh_coeff[18] * (7 * z2 - 1) * xy;						// (7 * z^2 - 1) * x * y
		shading += _sh_coeff[19] * (7 * z2 - 3 * 1) * yz;					// (7 * z^2 - 3) * y * z
		shading += _sh_coeff[20] * (3 - 30 * z2 + 35 * z2 * z2);				// 3 - 30 * z^2 + 35 * z^4
		shading += _sh_coeff[21] * (7 * z2 - 3) * xz;						// (7 * z^2 - 3) * x * z
		shading += _sh_coeff[22] * (7 * z2 - 1) * x2_y2;					// (7 * z^2 - 1) * (x^2 - y^2)
		shading += _sh_coeff[23] * (x2 - 3 * y2) * xz;						// (x^2 - 3 * y^2) * x * z
		shading += _sh_coeff[24] * ((x2 - 3 * y2) * x2 - (3 * x2 - y2) * y2);	// (x^2 - 3 * y^2) * x^2 - (3 * x^2 - y^2) * y^2 
	}

	return shading;
}
//=============================================================================
void Viewer::computeHeatMapDistanceColor(GLfloat* _color, 
	const Mesh::Point &_vertex, const Mesh::Point &_gt_vertex, 
	const GLfloat _min, const GLfloat _max)
{
	GLfloat v[3] = { _vertex[0] - _gt_vertex[0], _vertex[1] - _gt_vertex[1],
		_vertex[2] - _gt_vertex[2] };
	//GLfloat distance = log(sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) + 1e-10);
	GLfloat distance = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	GLfloat ratio = 2 * (distance - _min) / (_max - _min);
	_color[0] = min(max(ratio - 1, 0.f), 1.f);
	_color[1] = min(max(1 - ratio, 0.f), 1.f);
	_color[2] = 1 - _color[0] - _color[1];
}
//=============================================================================