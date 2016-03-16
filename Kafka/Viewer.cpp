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

const int Viewer::N_COLOR_MODES = 8;
const char* Viewer::STRING_COLOR_MODES[] = {
	"Intensity", 
	"Albedo", 
	"Shading", 
	"Single color", 
	"Normal",
	"Position heatmap",
	"XY Position heatmap",
	"Orientation heatmap"
};

const int Viewer::N_LIGHT_MODES = 2;
const char* Viewer::STRING_LIGHT_MODES[] = {
	"None",
	"GL"
};
//=============================================================================
/**** VARIABLES ****/

parameters::Parameters Viewer::params;

OpenMesh::IO::Options Viewer::ropt;

/* MAIN VARIABLES */
vector<vector<Mesh>> Viewer::meshes;
vector<vector<bool>> Viewer::is_loaded;
vector<int> Viewer::last_loaded_frame;
vector<vector<boost::thread*>> Viewer::threads;
bool Viewer::continue_loading = true;
vector<Mesh::Point> Viewer::curr_gt_vertices;
vector<Mesh::Normal> Viewer::curr_gt_normals;
int Viewer::curr_frame = 1;
bool Viewer::play = false;
clock_t Viewer::last_time;

vector<vector<float>> Viewer::sh_coeff;

int Viewer::next_mesh_idx = 1;
vector<int> Viewer::n_inter_frames;
MatrixXf Viewer::prev_shape;
MatrixXf Viewer::prev_color;
OpenMesh::IO::Options Viewer::wopt;

// colors per point per frame
vector< vector<float> > Viewer::red;
vector< vector<float> > Viewer::green;
vector< vector<float> > Viewer::blue;

vector< vector<float> > Viewer::red_filtered;
vector< vector<float> > Viewer::green_filtered;
vector< vector<float> > Viewer::blue_filtered;

vector< Eigen::MatrixXf > Viewer::rgb;


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
GLUI_Listbox* Viewer::glui_light_list = nullptr;
GLUI_StaticText* Viewer::glui_loading_text = nullptr;
//=============================================================================
void Viewer::initGLUT(int *argc, char **argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA);
	glutInitWindowSize(params.window_width, params.window_height);

	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - params.window_width) / 2,
		10);

	window_id = glutCreateWindow(params.mesh_prefix[0].c_str());

	// Uncomment to enable transparencies
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);

	glutDisplayFunc(display);
	glutMotionFunc(motion);
	glutCloseFunc(destroy);

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
	GLUI_Master.set_glutSpecialFunc(specialKey);
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

	glui->add_column_to_panel(glui_panel_1, 0);

	glui_play_button = new GLUI_Button(glui_panel_1, PLAY_TEXT, -1, updatePlay);

	GLUI_Panel* glui_panel_2 = glui->add_panel_to_panel(glui_panel, "", GLUI_PANEL_NONE);

	glui_rotation = new GLUI_Rotation(glui_panel_2, "Rotate", rotation);
	glui->add_column_to_panel(glui_panel_2, 0);
	glui_trans_xy = new GLUI_Translation(glui_panel_2, "Translate", 
		GLUI_TRANSLATION_XY, translation);
	glui->add_column_to_panel(glui_panel_2, 0);
	glui_trans_z = new GLUI_Translation(glui_panel_2, "Zoom out/in", 
		GLUI_TRANSLATION_Z, &translation[2]);
	glui_trans_z->set_speed(5);

	glui->add_column_to_panel(glui_panel_2, 0);
	glui_color_list.resize(params.n_meshes);
	for (int i = 0; i < params.n_meshes; i++)
	{
		string text = i==0 ? "Ground truth color : " : "Result " + to_string(i) + " color :       ";

		glui_color_list[i] = new GLUI_Listbox(glui_panel_2, text.c_str(), &mesh_color_mode[i]);
		for (int j = 0; j < N_COLOR_MODES; j++)
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

	glui->add_column_to_panel(glui_panel_2, 0);
	glui_loading_text = new GLUI_StaticText(glui_panel_2, "");
	glui_loading_text->set_alignment(GLUI_ALIGN_CENTER);
	updateFrameText();
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

	string loading_text = "";
	for (int i = 0; i < params.n_meshes; i++)
	{
		if (last_loaded_frame[i] < params.n_frames)
		{
			loading_text += "Mesh " + to_string(i) + ": " 
				+ to_string(last_loaded_frame[i]) + "/" + to_string(params.n_frames);
			loading_text += "\n";
		}
	}
	glui_loading_text->set_text(loading_text.c_str());
}
//=============================================================================
void Viewer::updatePlay(int state)
{
	play = !play;
	if (play)
	{
		glui_play_button->set_text(PAUSE_TEXT);
		glui_play_button->set_name(PAUSE_TEXT);

		if (curr_frame == params.n_frames)
			curr_frame = 1;
	}
	else
	{
		glui_play_button->set_text(PLAY_TEXT);
		glui_play_button->set_name(PLAY_TEXT);
	}
}
//=============================================================================
void Viewer::display(void)
{
	clock_t curr_time = clock();
	double diff_time = (curr_time - last_time) / (double)(CLOCKS_PER_SEC / 1000);

	if (play && diff_time >= FRAME_FREQ && curr_frame<params.n_frames)
	{
		nextFrame(curr_time);

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

	gluLookAt(eye[0], eye[1], eye[2] + mesh_center[0][2],	/* eye */
		eye[0], eye[1], -1.f,	/* center */
		0.f, 1.f, 0.f);	/* up is in positive Y direction */
	
	glTranslatef(translation[0], translation[1], -translation[2]);

	for (int i = 0; i < params.n_meshes; i++)
	{
		if (is_loaded[i][curr_frame - 1])
		{
			glPushMatrix();
			glTranslatef(mesh_translation[i][0], mesh_translation[i][1],
				mesh_translation[i][2]);
			glMultMatrixf(rotation);
			drawModel(i, curr_frame - 1);
			glPopMatrix();
		}
	}

	updateFrameText();

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
		break;
	case LOWER_R:
		break;
	case UPPER_F:
		break;
	case LOWER_F:
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
void Viewer::specialKey(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_UP:
		break;
	case GLUT_KEY_DOWN:
		break;
	case GLUT_KEY_LEFT:
		nextFrame(clock(), false);
		break;
	case GLUT_KEY_RIGHT:
		nextFrame(clock(), true);
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
	glRotatef(180, 0, 1, 0);
	glRotatef(-90, 0, 0, 1);

	Mesh* mesh2draw = &meshes[_mesh_idx][_frame_idx];

	if (_mesh_idx == 0)
	{
		curr_gt_vertices.resize(meshes[_mesh_idx][_frame_idx].n_vertices());
		curr_gt_normals.resize(meshes[_mesh_idx][_frame_idx].n_vertices());
	}

	glBegin(GL_TRIANGLES);

	Mesh::FaceIter f_it;
	Mesh::FaceIter f_end = mesh2draw->faces_end();
	for (f_it = mesh2draw->faces_begin(); f_it != f_end; ++f_it)
	{
		Mesh::FaceVertexIter fv_it;
		for (fv_it = mesh2draw->fv_iter(*f_it); fv_it.is_valid(); ++fv_it)
		{
			int v_idx = fv_it->idx();

			Mesh::Point p = mesh2draw->point(*fv_it);

			GLfloat point[3] {p[0] - mesh_center[0][0],
				p[1] - mesh_center[0][1],
				p[2] - mesh_center[0][2]};

			Mesh::Normal n = mesh2draw->normal(*fv_it);
			GLfloat normal[3] {n[0], n[1], n[2]};

			Mesh::Color c = mesh2draw->color(*fv_it);

			if (_mesh_idx == 0)
			{
				curr_gt_vertices[v_idx][0] = p[0];
				curr_gt_vertices[v_idx][1] = p[1];
				curr_gt_vertices[v_idx][2] = p[2];

				curr_gt_normals[v_idx][0] = n[0];
				curr_gt_normals[v_idx][1] = n[1];
				curr_gt_normals[v_idx][2] = n[2];
			}

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
			case MODE_NORMALS:
				computeNormalColor(color, n);
				break;
			case MODE_POSITION_HEATMAP:
				computeHeatMapDistanceColor(color, p, curr_gt_vertices[v_idx], 
					MIN_DISTANCE, MAX_DISTANCE);
				break;
			case MODE_XY_HEATMAP:
				computeHeatMapDistanceColor(color, p, curr_gt_vertices[v_idx],
					MIN_DISTANCE, MAX_DISTANCE, true);
				break;
			case MODE_NORMAL_HEATMAP:
				computeHeatMapOrientationColor(color, n, curr_gt_normals[v_idx],
					MIN_ORIENT_DIFF, MAX_ORIENT_DIFF);
				break;
			default:
				break;
			}

			glColor3fv(color);

			glNormal3fv(normal);
			glVertex3fv(point);
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

	n_inter_frames.resize(params.n_frames, 0);

	wopt += OpenMesh::IO::Options::VertexColor;
	wopt += OpenMesh::IO::Options::VertexNormal;
	wopt += OpenMesh::IO::Options::Binary;

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
	continue_loading = false;
	for (int mesh_idx = 0; mesh_idx < params.n_meshes; mesh_idx++)
	{
		int last_frame_idx = last_loaded_frame[mesh_idx];
		if (last_frame_idx < params.n_frames)
		{
			if (threads[mesh_idx][last_frame_idx] != nullptr)
			{
				threads[mesh_idx][last_frame_idx]->join();
				delete threads[mesh_idx][last_frame_idx];
				threads[mesh_idx][last_frame_idx] = nullptr;
			}
		}
		for (int frame_idx = 0; frame_idx < last_frame_idx; frame_idx++)
		{
			delete threads[mesh_idx][frame_idx];
			threads[mesh_idx][frame_idx] = nullptr;
		}
	}
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
	is_loaded.resize(params.n_meshes);
	last_loaded_frame.resize(params.n_meshes);
	threads.resize(params.n_meshes);

	for (int i = 0; i < params.n_meshes; i++)
	{
		meshes[i].resize(params.n_frames);
		mesh_translation[i].resize(3);
		is_loaded[i].resize(params.n_frames, false);
		threads[i].resize(params.n_frames, nullptr);
	}

	for (int mesh_idx = 0; mesh_idx < params.n_meshes; mesh_idx++)
	{
		string mesh_filename = getMeshFilename(mesh_idx, 0);
		threads[mesh_idx][0] = 
			new boost::thread(&readMesh, boost::ref(meshes[mesh_idx][0]), mesh_filename, ropt);
		threads[mesh_idx][0]->join();
	}

	cvtMeshVerticesToMatrix(meshes[0][0], prev_shape, CVT_VERTEX);
	cvtMeshVerticesToMatrix(meshes[0][0], prev_color, CVT_COLOR);
	
	saveIntermidiateFrame(meshes[0][0], next_mesh_idx, wopt);

	int num_vertices = meshes[0][0].n_vertices();

	//red.resize(num_vertices);
	//green.resize(num_vertices);
	//blue.resize(num_vertices);

	//red_filtered.resize(num_vertices);
	//green_filtered.resize(num_vertices);
	//blue_filtered.resize(num_vertices);

	rgb.resize(num_vertices);

	for (int i = 0; i < num_vertices; i++)
	{
		//red[i].resize(params.n_frames);
		//blue[i].resize(params.n_frames);
		//green[i].resize(params.n_frames);

		rgb[i].resize(3, params.n_frames);
	}

	addToFinalColors(0);

	if (params.n_frames > 1)
	{
		threads[0][1] = new boost::thread(&readMeshNext, 0, 1);
	}

	for (int i = 0; i < params.n_meshes; i++)
	{

		last_loaded_frame[i] = 1;

		is_loaded[i][0] = true;

		calculateCenterPoint(i);
	}

	std::cout << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000)
		<< " ms" << endl;
}
//=============================================================================
void Viewer::loadMesh(const int _mesh_idx, const int _frame_idx)
{
	string mesh_filename = getMeshFilename(_mesh_idx, _frame_idx);
	readMesh(meshes[_mesh_idx][_frame_idx], mesh_filename, ropt);
	is_loaded[_mesh_idx][_frame_idx] = true;
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
		shading += _sh_coeff[9] * (3 * x2 - y2) * y;	// (3 * x^2 - y^2) * y 
		shading += _sh_coeff[10] * xy * z;				// x * y * z
		shading += _sh_coeff[11] * (5 * z2 - 1) * y;	// (5 * z^2 - 1) * y
		shading += _sh_coeff[12] * (5 * z2 - 3) * z;	// (5 * z^2 - 3) * z
		shading += _sh_coeff[13] * (5 * z2 - 1) * x;	// (5 * z^2 - 1) * x
		shading += _sh_coeff[14] * x2_y2 * z;			// (x^2 - y^2) * z
		shading += _sh_coeff[15] * (x2 - 3 * y2) * x;	// (x^2 - 3 * y^2) * x
	}

	if (_sh_coeff.size() > 15)
	{
		shading += _sh_coeff[16] * x2_y2 * xy;									// (x^2 - y^2) * x * y
		shading += _sh_coeff[17] * (3 * x2 - y2) * yz;							// (3 * x^2 - y^2) * yz
		shading += _sh_coeff[18] * (7 * z2 - 1) * xy;							// (7 * z^2 - 1) * x * y
		shading += _sh_coeff[19] * (7 * z2 - 3 * 1) * yz;						// (7 * z^2 - 3) * y * z
		shading += _sh_coeff[20] * (3 - 30 * z2 + 35 * z2 * z2);				// 3 - 30 * z^2 + 35 * z^4
		shading += _sh_coeff[21] * (7 * z2 - 3) * xz;							// (7 * z^2 - 3) * x * z
		shading += _sh_coeff[22] * (7 * z2 - 1) * x2_y2;						// (7 * z^2 - 1) * (x^2 - y^2)
		shading += _sh_coeff[23] * (x2 - 3 * y2) * xz;							// (x^2 - 3 * y^2) * x * z
		shading += _sh_coeff[24] * ((x2 - 3 * y2) * x2 - (3 * x2 - y2) * y2);	// (x^2 - 3 * y^2) * x^2 - (3 * x^2 - y^2) * y^2 
	}

	return shading;
}
//=============================================================================
void Viewer::computeHeatMapDistanceColor(GLfloat* _color, 
	const Mesh::Point &_vertex, const Mesh::Point &_gt_vertex, 
	const GLfloat _min, const GLfloat _max, const bool _xy)
{
	GLfloat v[3] = { _vertex[0] - _gt_vertex[0], _vertex[1] - _gt_vertex[1],
		_vertex[2] - _gt_vertex[2] };
	//GLfloat distance = log(sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) + 1e-10);
	GLfloat distance = v[0] * v[0] + v[1] * v[1];
	if (!_xy)
	{
		distance += v[2] * v[2];
	}
	distance = min(max(sqrt(distance), _min), _max);
	GLfloat ratio = 2 * (distance - _min) / (_max - _min);
	_color[0] = max(ratio - 1, 0.f);
	_color[2] = max(1 - ratio, 0.f);
	_color[1] = 1 - _color[0] - _color[2];
}
//=============================================================================
void Viewer::computeHeatMapOrientationColor(GLfloat* _color, 
	const Mesh::Point &_normal, const Mesh::Point &_gt_normal,
	const GLfloat _min, const GLfloat _max)
{
	// GLfloat cos_a = _normal[0] * _gt_normal[0] + _normal[1] * _gt_normal[1]
	// 	+ _normal[2] * _gt_normal[2];
	// cos_a = min(max(cos_a, _min), _max);
	// GLfloat ratio = 2 * (cos_a - _min) / (_max - _min);
	// _color[0] = max(1 - ratio, 0.f);
	// _color[2] = max(ratio - 1, 0.f);
	// _color[1] = 1 - _color[0] - _color[2];

	GLfloat diff_nx = _normal[0] - _gt_normal[0];
	GLfloat diff_ny = _normal[1] - _gt_normal[1];
	GLfloat diff_nz = _normal[2] - _gt_normal[2];
	GLfloat dist_n = sqrt(diff_nx * diff_nx + diff_ny * diff_ny 
		+ diff_nz * diff_nz);
	dist_n = min(max(dist_n, _min), _max);
	GLfloat ratio = 2 * (dist_n - _min) / (_max - _min);

	_color[0] = max(ratio - 1, 0.f);
	_color[2] = max(1 - ratio, 0.f);
	_color[1] = 1 - _color[0] - _color[2];
}
//=============================================================================
void Viewer::computeNormalColor(GLfloat* _color, const Mesh::Normal &_normal)
{
	//_color[0] = (_normal[0] + 1) / 2;
	//_color[1] = (_normal[1] + 1) / 2;
	//_color[2] = (_normal[2] + 1) / 2;

	_color[0] = (-_normal[1] + 1) / 2;
	_color[1] = (-_normal[0] + 1) / 2;
	_color[2] = (-_normal[2] + 1) / 2;
}
//=============================================================================
void Viewer::nextFrame(clock_t _curr_time, bool forward)
{
	curr_frame = max(min (curr_frame + (forward ? 1 : -1), params.n_frames), 1);
	glui_frame_scroll->set_int_val(curr_frame);
	updateFrameText();
	last_time = _curr_time;
}
//=============================================================================
void Viewer::readMeshNext(int _mesh_idx, int _frame_idx)
{
	string mesh_filename = getMeshFilename(_mesh_idx, _frame_idx);
	readMesh(meshes[_mesh_idx][_frame_idx], mesh_filename, ropt);
	is_loaded[_mesh_idx][_frame_idx] = true;
	last_loaded_frame[_mesh_idx]++;

	//if (_mesh_idx == 0)
	//{
	//	addToFinalColors(_frame_idx);
	//}

	saveIntermidiateFrames(meshes[_mesh_idx][_frame_idx], _frame_idx,
		prev_shape, prev_color);

	_mesh_idx++;
	if (_mesh_idx == params.n_meshes)
	{
		_frame_idx++;
		_mesh_idx = 0;
	}

	if (continue_loading && _frame_idx < params.n_frames)
	{
		threads[_mesh_idx][_frame_idx] =
			new boost::thread(&readMeshNext, _mesh_idx, _frame_idx);
	} 
	else
	{
		//saveFinalMesh();

		std::ofstream ofs(params.save_n_interframes_filename);
		std::ostream_iterator<float> output_iterator(ofs, "\n");
		std::copy(n_inter_frames.begin(), n_inter_frames.end(), output_iterator);
		ofs.close();
	}
}
//=============================================================================
void Viewer::addToFinalColors(int _frame_idx)
{
	Mesh &model = meshes[0][_frame_idx];

	Mesh::VertexIter v_it;
	Mesh::VertexIter v_end = model.vertices_end();
	for (v_it = model.vertices_begin(); v_it != v_end; ++v_it)
	{
		int v_idx = v_it->idx();
		Mesh::Color c = model.color(*v_it);

		float gray = 0.2989 * c[0] + 0.5870 * c[1] + 0.1140 * c[2];

		//if (gray < params.specular_threshold)
		//{
		//	red_filtered[v_idx].push_back(c[0]);
		//	green_filtered[v_idx].push_back(c[1]);
		//	blue_filtered[v_idx].push_back(c[2]);
		//}
		//red[v_idx][_frame_idx] = c[0];
		//green[v_idx][_frame_idx] = c[1];
		//blue[v_idx][_frame_idx] = c[2];

		rgb[v_idx].col(_frame_idx) = Vector3f(c[0], c[1], c[2]);
	}
}
//=============================================================================
void Viewer::saveFinalMesh()
{
	Mesh final_mesh = meshes[0][0];
	Mesh::VertexIter v_it;
	Mesh::VertexIter v_end = final_mesh.vertices_end();

	OpenMesh::IO::Options wopt;
	wopt += OpenMesh::IO::Options::VertexColor;
	wopt += OpenMesh::IO::Options::VertexNormal;
	wopt += OpenMesh::IO::Options::Binary;

	//// Mean color
	//for (v_it = final_mesh.vertices_begin(); v_it != v_end; ++v_it)
	//{
	//	int v_idx = v_it->idx();

	//	accumulator_set<float, stats<tag::mean> > acc_red;
	//	for_each(red[v_idx].begin(), red[v_idx].end(), 
	//		bind<void>(ref(acc_red), _1)
	//		);
	//	float r = mean(acc_red);

	//	accumulator_set<float, stats<tag::mean> > acc_green;
	//	for_each(green[v_idx].begin(), green[v_idx].end(), 
	//		bind<void>(ref(acc_green), _1));
	//	float g = mean(acc_green);

	//	accumulator_set<float, stats<tag::mean> > acc_blue;
	//	for_each(blue[v_idx].begin(), blue[v_idx].end(),
	//		bind<void>(ref(acc_blue), _1));
	//	float b = mean(acc_blue);

	//	Mesh::Color c = Mesh::Color(r ,g, b);

	//	final_mesh.set_color(*v_it, c);
	//}
	//writeMesh(final_mesh, params.mesh_prefix[0] + "mean" + params.mesh_suffix[0], wopt);

	//// Median color
	///*int n = params.n_frames / 2;*/
	//for (v_it = final_mesh.vertices_begin(); v_it != v_end; ++v_it)
	//{
	//	int v_idx = v_it->idx();

	//	int n = red_filtered[v_idx].size() / 2;

	//	float r, g, b;
	//	if (n > 0)
	//	{
	//		nth_element(red_filtered[v_idx].begin(), red_filtered[v_idx].begin() + n, red_filtered[v_idx].end());
	//		r = red_filtered[v_idx][n];

	//		nth_element(green_filtered[v_idx].begin(), green_filtered[v_idx].begin() + n, green_filtered[v_idx].end());
	//		g = green_filtered[v_idx][n];

	//		nth_element(blue_filtered[v_idx].begin(), blue_filtered[v_idx].begin() + n, blue_filtered[v_idx].end());
	//		b = blue_filtered[v_idx][n];
	//	}
	//	else
	//	{
	//		r = *min_element(red[v_idx].begin(), red[v_idx].end());
	//		g = *min_element(green[v_idx].begin(), green[v_idx].end());
	//		b = *min_element(blue[v_idx].begin(), blue[v_idx].end());
	//	}

	//	Mesh::Color c = Mesh::Color(r, g, b);

	//	final_mesh.set_color(*v_it, c);
	//}
	//writeMesh(final_mesh, params.mesh_prefix[0] + "median" + params.mesh_suffix[0], wopt);

	//// Max color
	//for (v_it = final_mesh.vertices_begin(); v_it != v_end; ++v_it)
	//{
	//	int v_idx = v_it->idx();

	//	float r, g, b;

	//	if (red_filtered[v_idx].size() > 0)
	//	{
	//		r = *max_element(red_filtered[v_idx].begin(), red_filtered[v_idx].end());
	//		g = *max_element(green_filtered[v_idx].begin(), green_filtered[v_idx].end());
	//		b = *max_element(blue_filtered[v_idx].begin(), blue_filtered[v_idx].end());
	//	}
	//	else
	//	{
	//		r = *min_element(red[v_idx].begin(), red[v_idx].end());
	//		g = *min_element(green[v_idx].begin(), green[v_idx].end());
	//		b = *min_element(blue[v_idx].begin(), blue[v_idx].end());
	//	}

	//	Mesh::Color c = Mesh::Color(r, g, b);

	//	final_mesh.set_color(*v_it, c);
	//}
	//writeMesh(final_mesh, params.mesh_prefix[0] + "max_" 
	//	+ to_string(params.specular_threshold) + params.mesh_suffix[0], wopt);

	//// Min color
	//for (v_it = final_mesh.vertices_begin(); v_it != v_end; ++v_it)
	//{
	//	int v_idx = v_it->idx();

	//	float r = *min_element(red[v_idx].begin(), red[v_idx].end());
	//	float g = *min_element(green[v_idx].begin(), green[v_idx].end());
	//	float b = *min_element(blue[v_idx].begin(), blue[v_idx].end());

	//	Mesh::Color c = Mesh::Color(r, g, b);

	//	final_mesh.set_color(*v_it, c);
	//}
	//writeMesh(final_mesh, params.mesh_prefix[0] + "min" + params.mesh_suffix[0], wopt);

	// SVD color

	string rgb_filename = params.mesh_prefix[0] + "rgb" + ".txt";
	ofstream ofs;
	ofs.open(rgb_filename, ofstream::out | ofstream::trunc);

	for (v_it = final_mesh.vertices_begin(); v_it != v_end; ++v_it)
	{
		int v_idx = v_it->idx();

		JacobiSVD<MatrixXf> svd(rgb[v_idx].transpose(), ComputeFullU | ComputeFullV);

		Vector3f v(0, 0, 0);

		VectorXf scales = rgb[v_idx].transpose() * svd.matrixV().col(0);

		float max_scale = scales.maxCoeff();
		
		if (svd.singularValues()(0) > 0.0f)
		{
			v = max_scale * svd.matrixV().col(0);
			//v = svd.matrixV().col(0) * (1 - (1 / svd.singularValues()(0)));
			//v = svd.matrixV().col(0) * svd.singularValues()(0);

			ofs << rgb[v_idx] << endl;
		}

		Mesh::Color c = Mesh::Color(v(0), v(1), v(2));

		final_mesh.set_color(*v_it, c);
	}
	writeMesh(final_mesh, params.mesh_prefix[0] + "svd"
		+ params.mesh_suffix[0], wopt);

	ofs.close();
}
//=============================================================================
void Viewer::cvtMeshVerticesToMatrix(const Mesh &_mesh, MatrixXf &_m, int _mode)
{
	_m.resize(_mesh.n_vertices(), 3);

	Mesh::ConstVertexIter v_it;
	Mesh::ConstVertexIter v_end = _mesh.vertices_end();
	for (v_it = _mesh.vertices_begin(); v_it != v_end; ++v_it)
	{
		int v_idx = v_it->idx();

		switch (_mode)
		{
		case CVT_VERTEX:
		{
			Mesh::Point p = _mesh.point(*v_it);

			_m(v_idx, 0) = p[0];
			_m(v_idx, 1) = p[1];
			_m(v_idx, 2) = p[2];
		}
		break;
		case CVT_NORMAL:
		{
			Mesh::Normal n = _mesh.normal(*v_it);

			_m(v_idx, 0) = n[0];
			_m(v_idx, 1) = n[1];
			_m(v_idx, 2) = n[2];
		}
		break;
		case CVT_COLOR:
		{
			Mesh::Color c = _mesh.color(*v_it);

			_m(v_idx, 0) = c[0];
			_m(v_idx, 1) = c[1];
			_m(v_idx, 2) = c[2];
		}
		break;
		default:
			break;
		}
	}
}
//=============================================================================
void Viewer::saveIntermidiateFrames(const Mesh &_curr_mesh, int _frame_idx, 
	MatrixXf &_prev_shape, MatrixXf &_prev_color)
{
	MatrixXf curr_shape;
	cvtMeshVerticesToMatrix(_curr_mesh, curr_shape, CVT_VERTEX);
	MatrixXf curr_color;
	cvtMeshVerticesToMatrix(_curr_mesh, curr_color, CVT_COLOR);

	MatrixXf c_diff = curr_color - _prev_color;
	MatrixXf v_diff = curr_shape - _prev_shape;
	float max_v_diff_norm = v_diff.rowwise().norm().maxCoeff();

	int curr_n_inter_frames = ceil(max_v_diff_norm) - 1;
	n_inter_frames[_frame_idx] = curr_n_inter_frames;

	Mesh new_mesh = _curr_mesh;
	for (int i = 0; i < curr_n_inter_frames; i++)
	{
		float ratio = float(i + 1) / float(curr_n_inter_frames + 1);
		Mesh::ConstVertexIter v_it;
		Mesh::ConstVertexIter v_end = new_mesh.vertices_end();
		for (v_it = new_mesh.vertices_begin(); v_it != v_end; ++v_it)
		{
			int v_idx = v_it->idx();

			Mesh::Point p = Mesh::Point(
				_prev_shape(v_idx, 0) + ratio * v_diff(v_idx, 0),
				_prev_shape(v_idx, 1) + ratio * v_diff(v_idx, 1),
				_prev_shape(v_idx, 2) + ratio * v_diff(v_idx, 2)
				);
			new_mesh.set_point(*v_it, p);

			Mesh::Color c = Mesh::Color(
				_prev_color(v_idx, 0) + ratio * c_diff(v_idx, 0),
				_prev_color(v_idx, 1) + ratio * c_diff(v_idx, 1),
				_prev_color(v_idx, 2) + ratio * c_diff(v_idx, 2)
				);
			new_mesh.set_color(*v_it, c);
		}
		new_mesh.update_normals();

		saveIntermidiateFrame(new_mesh, next_mesh_idx, wopt);
	}

	_prev_shape = curr_shape;
	_prev_color = curr_color;

	saveIntermidiateFrame(_curr_mesh, next_mesh_idx, wopt);
}
//=============================================================================
void Viewer::saveIntermidiateFrame(const Mesh &_mesh, int &_mesh_idx, 
	const OpenMesh::IO::Options _wopt)
{
	int save_idx = params.save_mesh_first_idx + _mesh_idx - 1;
	string mesh_filename = params.save_mesh_prefix
		+ cvtIntToString(save_idx, params.save_mesh_n_digits)
		+ params.save_mesh_suffix;
	writeMesh(_mesh, mesh_filename, _wopt);
	_mesh_idx++;
}
//=============================================================================
